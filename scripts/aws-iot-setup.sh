#!/usr/bin/env bash
# =============================================================================
# aws-iot-setup.sh — One-shot AWS IoT Core + Timestream provisioning
#
# Creates: IoT Thing, X.509 certs, IoT Policy, Timestream DB+table, IoT Rule.
# Downloads certs to docker/certs/ for the cloud-connector service.
#
# Prerequisites:
#   - AWS CLI v2 configured with admin credentials
#   - Region set (defaults to eu-central-1)
#
# Usage:
#   ./scripts/aws-iot-setup.sh [--region eu-central-1] [--thing-name taktflow-pi-001]
# =============================================================================

set -euo pipefail

# --- Defaults ---
REGION="${AWS_DEFAULT_REGION:-eu-central-1}"
THING_NAME="taktflow-pi-001"
CERT_DIR="docker/certs"
TS_DB="TaktflowTelemetry"
TS_TABLE="vehicle_data"
IOT_RULE_NAME="TaktflowToTimestream"
POLICY_NAME="TaktflowDevicePolicy"

# --- Parse args ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        --region) REGION="$2"; shift 2 ;;
        --thing-name) THING_NAME="$2"; shift 2 ;;
        *) echo "Unknown arg: $1"; exit 1 ;;
    esac
done

echo "=== AWS IoT Core Setup ==="
echo "Region:     $REGION"
echo "Thing:      $THING_NAME"
echo "Cert dir:   $CERT_DIR"
echo ""

# --- 1. Create IoT Thing ---
echo "[1/9] Creating IoT Thing: $THING_NAME"
aws iot create-thing \
    --thing-name "$THING_NAME" \
    --region "$REGION" 2>/dev/null || echo "  (already exists)"

# --- 2. Create and download X.509 certificates ---
echo "[2/9] Creating X.509 certificates"
mkdir -p "$CERT_DIR"

CERT_RESPONSE=$(aws iot create-keys-and-certificate \
    --set-as-active \
    --region "$REGION" \
    --output json)

CERT_ARN=$(echo "$CERT_RESPONSE" | python3 -c "import sys,json; print(json.load(sys.stdin)['certificateArn'])")
CERT_ID=$(echo "$CERT_RESPONSE" | python3 -c "import sys,json; print(json.load(sys.stdin)['certificateId'])")

echo "$CERT_RESPONSE" | python3 -c "import sys,json; print(json.load(sys.stdin)['certificatePem'])" > "$CERT_DIR/certificate.pem.crt"
echo "$CERT_RESPONSE" | python3 -c "import sys,json; print(json.load(sys.stdin)['keyPair']['PrivateKey'])" > "$CERT_DIR/private.pem.key"
echo "$CERT_RESPONSE" | python3 -c "import sys,json; print(json.load(sys.stdin)['keyPair']['PublicKey'])" > "$CERT_DIR/public.pem.key"

chmod 600 "$CERT_DIR/private.pem.key"
echo "  Certificate ID: $CERT_ID"
echo "  Certificate ARN: $CERT_ARN"

# --- 3. Download Amazon Root CA ---
echo "[3/9] Downloading Amazon Root CA"
curl -s -o "$CERT_DIR/AmazonRootCA1.pem" \
    "https://www.amazontrust.com/repository/AmazonRootCA1.pem"

# --- 4. Create IoT Policy ---
echo "[4/9] Creating IoT Policy: $POLICY_NAME"
POLICY_DOC=$(cat <<'POLICY'
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": "iot:Connect",
            "Resource": "*"
        },
        {
            "Effect": "Allow",
            "Action": "iot:Publish",
            "Resource": "arn:aws:iot:*:*:topic/vehicle/*"
        }
    ]
}
POLICY
)

aws iot create-policy \
    --policy-name "$POLICY_NAME" \
    --policy-document "$POLICY_DOC" \
    --region "$REGION" 2>/dev/null || echo "  (already exists)"

# --- 5. Attach policy and thing to certificate ---
echo "[5/9] Attaching policy and thing to certificate"
aws iot attach-policy \
    --policy-name "$POLICY_NAME" \
    --target "$CERT_ARN" \
    --region "$REGION"

aws iot attach-thing-principal \
    --thing-name "$THING_NAME" \
    --principal "$CERT_ARN" \
    --region "$REGION"

# --- 6. Create Timestream database and table ---
echo "[6/9] Creating Timestream database: $TS_DB"
aws timestream-write create-database \
    --database-name "$TS_DB" \
    --region "$REGION" 2>/dev/null || echo "  (already exists)"

echo "[7/9] Creating Timestream table: $TS_TABLE"
aws timestream-write create-table \
    --database-name "$TS_DB" \
    --table-name "$TS_TABLE" \
    --retention-properties \
        MemoryStoreRetentionPeriodInHours=24,MagneticStoreRetentionPeriodInDays=365 \
    --region "$REGION" 2>/dev/null || echo "  (already exists)"

# --- 7. Create IAM role for IoT Rule → Timestream ---
echo "[8/9] Creating IAM role for IoT → Timestream"
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)

ASSUME_ROLE_DOC=$(cat <<'ROLE'
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Principal": {"Service": "iot.amazonaws.com"},
            "Action": "sts:AssumeRole"
        }
    ]
}
ROLE
)

ROLE_ARN=$(aws iam create-role \
    --role-name TaktflowIoTTimestreamRole \
    --assume-role-policy-document "$ASSUME_ROLE_DOC" \
    --query 'Role.Arn' --output text 2>/dev/null || \
    aws iam get-role \
        --role-name TaktflowIoTTimestreamRole \
        --query 'Role.Arn' --output text)

# Inline policy for Timestream write
TS_POLICY=$(cat <<TSPOLICY
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "timestream:WriteRecords",
                "timestream:DescribeEndpoints"
            ],
            "Resource": "arn:aws:timestream:${REGION}:${ACCOUNT_ID}:database/${TS_DB}/table/${TS_TABLE}"
        },
        {
            "Effect": "Allow",
            "Action": "timestream:DescribeEndpoints",
            "Resource": "*"
        }
    ]
}
TSPOLICY
)

aws iam put-role-policy \
    --role-name TaktflowIoTTimestreamRole \
    --policy-name TaktflowTimestreamWrite \
    --policy-document "$TS_POLICY"

# --- 8. Create IoT Rule ---
echo "[9/9] Creating IoT Rule: $IOT_RULE_NAME"

# Wait for IAM role propagation
sleep 5

RULE_PAYLOAD=$(cat <<RULE
{
    "sql": "SELECT * FROM 'vehicle/#'",
    "actions": [
        {
            "timestream": {
                "roleArn": "${ROLE_ARN}",
                "databaseName": "${TS_DB}",
                "tableName": "${TS_TABLE}",
                "dimensions": [
                    {"name": "device_id", "value": "\${topic(1)}"},
                    {"name": "metric_type", "value": "\${topic(2)}"}
                ]
            }
        }
    ]
}
RULE
)

aws iot create-topic-rule \
    --rule-name "$IOT_RULE_NAME" \
    --topic-rule-payload "$RULE_PAYLOAD" \
    --region "$REGION" 2>/dev/null || echo "  (already exists, updating...)" && \
aws iot replace-topic-rule \
    --rule-name "$IOT_RULE_NAME" \
    --topic-rule-payload "$RULE_PAYLOAD" \
    --region "$REGION" 2>/dev/null || true

# --- Done ---
ENDPOINT=$(aws iot describe-endpoint \
    --endpoint-type iot:Data-ATS \
    --query 'endpointAddress' --output text \
    --region "$REGION")

echo ""
echo "=== Setup Complete ==="
echo ""
echo "IoT Endpoint: $ENDPOINT"
echo "Certificates: $CERT_DIR/"
echo ""
echo "Add to your .env file:"
echo "  AWS_IOT_ENDPOINT=$ENDPOINT"
echo ""
echo "Test connection:"
echo "  mosquitto_pub -h $ENDPOINT -p 8883 \\"
echo "    --cafile $CERT_DIR/AmazonRootCA1.pem \\"
echo "    --cert $CERT_DIR/certificate.pem.crt \\"
echo "    --key $CERT_DIR/private.pem.key \\"
echo "    -t vehicle/test -m '{\"hello\":\"cloud\"}'"
