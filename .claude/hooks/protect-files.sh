#!/bin/bash
# Pre-edit hook: Block edits to protected files
# Returns JSON with permissionDecision: deny to block the edit

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty')

if [ -z "$FILE_PATH" ]; then
  exit 0
fi

# Protected file patterns
PROTECTED_PATTERNS=(
  ".env"
  ".pem"
  ".key"
  "secrets/"
  "credentials"
  ".git/"
)

for pattern in "${PROTECTED_PATTERNS[@]}"; do
  if [[ "$FILE_PATH" == *"$pattern"* ]]; then
    echo "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"permissionDecision\":\"deny\",\"permissionDecisionReason\":\"Protected file: $FILE_PATH — cannot edit secrets, keys, or credentials directly.\"}}"
    exit 0
  fi
done

exit 0
