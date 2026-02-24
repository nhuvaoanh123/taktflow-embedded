#!/usr/bin/env bash
set -euo pipefail

# trace-slice.sh
# Small, deterministic trace extraction to avoid large-context AI runs.
#
# Usage:
#   bash scripts/trace-slice.sh --layer stk-sys
#   bash scripts/trace-slice.sh --layer stk-sys --from STK-011 --to STK-020
#   bash scripts/trace-slice.sh --layer sys-swr --from SYS-037 --to SYS-045
#   bash scripts/trace-slice.sh --layer stk-sys --out docs/tmp/stk_sys.csv
#
# Output CSV columns:
#   layer=stk-sys -> STK_ID,SYS_ID
#   layer=sys-swr -> SYS_ID,SWR_ID

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SYSREQ="$ROOT_DIR/docs/aspice/system/system-requirements.md"
OUT=""
LAYER=""
FROM_ID=""
TO_ID=""

usage() {
  cat <<'EOF'
Usage:
  bash scripts/trace-slice.sh --layer <stk-sys|sys-swr> [--from ID] [--to ID] [--out PATH]

Examples:
  bash scripts/trace-slice.sh --layer stk-sys
  bash scripts/trace-slice.sh --layer stk-sys --from STK-011 --to STK-020
  bash scripts/trace-slice.sh --layer sys-swr --from SYS-037 --to SYS-045
  bash scripts/trace-slice.sh --layer stk-sys --out docs/tmp/stk_sys.csv
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --layer)
      LAYER="${2:-}"
      shift 2
      ;;
    --from)
      FROM_ID="${2:-}"
      shift 2
      ;;
    --to)
      TO_ID="${2:-}"
      shift 2
      ;;
    --out)
      OUT="${2:-}"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [ -z "$LAYER" ]; then
  echo "--layer is required" >&2
  usage
  exit 2
fi

if [ ! -f "$SYSREQ" ]; then
  echo "Missing file: $SYSREQ" >&2
  exit 1
fi

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

in_range() {
  # Compare by numeric suffix only (e.g., STK-011 -> 11)
  local id="$1" from="$2" to="$3"
  local n fn tn
  n="$(echo "$id" | sed -E 's/.*-([0-9]+)$/\1/')"
  if [ -z "$from" ] && [ -z "$to" ]; then
    return 0
  fi
  if [ -n "$from" ]; then
    fn="$(echo "$from" | sed -E 's/.*-([0-9]+)$/\1/')"
    if [ "$n" -lt "$fn" ]; then
      return 1
    fi
  fi
  if [ -n "$to" ]; then
    tn="$(echo "$to" | sed -E 's/.*-([0-9]+)$/\1/')"
    if [ "$n" -gt "$tn" ]; then
      return 1
    fi
  fi
  return 0
}

extract_stk_sys() {
  awk '
    /^### SYS-[0-9]+:/ {
      match($0, /SYS-[0-9]+/, m)
      cur_sys = m[0]
      next
    }
    /- \*\*Traces up\*\*:/ {
      if (cur_sys == "") next
      line = $0
      while (match(line, /STK-[0-9]+/, m)) {
        print m[0] "," cur_sys
        line = substr(line, RSTART + RLENGTH)
      }
    }
  ' "$SYSREQ" | sort -u
}

extract_sys_swr() {
  awk '
    /^### SYS-[0-9]+:/ {
      match($0, /SYS-[0-9]+/, m)
      cur_sys = m[0]
      next
    }
    /- \*\*Traces down\*\*:/ {
      if (cur_sys == "") next
      line = $0
      while (match(line, /SWR-[A-Z]+-[0-9]+/, m)) {
        print cur_sys "," m[0]
        line = substr(line, RSTART + RLENGTH)
      }
    }
  ' "$SYSREQ" | sort -u
}

case "$LAYER" in
  stk-sys)
    {
      echo "STK_ID,SYS_ID"
      extract_stk_sys | while IFS=, read -r left right; do
        if in_range "$left" "$FROM_ID" "$TO_ID"; then
          echo "$left,$right"
        fi
      done
    } > "$tmp"
    ;;
  sys-swr)
    {
      echo "SYS_ID,SWR_ID"
      extract_sys_swr | while IFS=, read -r left right; do
        if in_range "$left" "$FROM_ID" "$TO_ID"; then
          echo "$left,$right"
        fi
      done
    } > "$tmp"
    ;;
  *)
    echo "Invalid --layer: $LAYER (expected stk-sys or sys-swr)" >&2
    exit 2
    ;;
esac

if [ -n "$OUT" ]; then
  out_abs="$ROOT_DIR/$OUT"
  mkdir -p "$(dirname "$out_abs")"
  cp "$tmp" "$out_abs"
  echo "Wrote: $out_abs"
else
  cat "$tmp"
fi

