#!/bin/bash
# Pre-edit hook: Enforce test-driven development (TDD)
# Block writes to firmware source files if no corresponding test file exists.
# The test file must be written FIRST — this is a hard gate.
#
# ISO 26262 Part 6, Table 9: "Requirements-based testing" is highly
# recommended at ASIL D. Writing tests first ensures every function
# traces to a requirement before implementation begins.

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty')

if [ -z "$FILE_PATH" ]; then
  exit 0
fi

# Normalize path separators (Windows -> Unix)
FILE_PATH=$(echo "$FILE_PATH" | sed 's|\\|/|g')

# Only check .c source files (not headers, not test files, not configs)
if [[ ! "$FILE_PATH" =~ \.c$ ]]; then
  exit 0
fi

# Skip test files themselves — those are always allowed
if [[ "$FILE_PATH" =~ /test/ ]] || [[ "$FILE_PATH" =~ test_ ]]; then
  exit 0
fi

# Skip config files
if [[ "$FILE_PATH" =~ /cfg/ ]]; then
  exit 0
fi

# Skip non-firmware files
if [[ ! "$FILE_PATH" =~ firmware/ ]]; then
  exit 0
fi

# Extract module basename
BASENAME=$(basename "$FILE_PATH" .c)
TEST_FILE=""

# Pattern 1: ECU source — firmware/<ecu>/src/<module>.c
#   Test at: firmware/<ecu>/test/test_<module>.c
if [[ "$FILE_PATH" =~ firmware/([^/]+)/src/(.+)\.c$ ]]; then
  ECU="${BASH_REMATCH[1]}"
  MODULE="${BASH_REMATCH[2]}"
  TEST_FILE="${CLAUDE_PROJECT_DIR}/firmware/${ECU}/test/test_${MODULE}.c"
fi

# Pattern 2: BSW module — firmware/shared/bsw/<layer>/<sublayer>/<module>.c
#   Test at: firmware/shared/bsw/test/test_<module>.c
if [[ "$FILE_PATH" =~ firmware/shared/bsw/.*/([^/]+)\.c$ ]]; then
  MODULE="${BASH_REMATCH[1]}"
  TEST_FILE="${CLAUDE_PROJECT_DIR}/firmware/shared/bsw/test/test_${MODULE}.c"
fi

# If no pattern matched, allow (not a governed path)
if [ -z "$TEST_FILE" ]; then
  exit 0
fi

# THE GATE: Does the test file exist?
if [ ! -f "$TEST_FILE" ]; then
  # Derive the relative path for the error message
  REL_TEST=$(echo "$TEST_FILE" | sed "s|${CLAUDE_PROJECT_DIR}/||")
  echo "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"permissionDecision\":\"deny\",\"permissionDecisionReason\":\"TEST-FIRST VIOLATION: Cannot write ${BASENAME}.c — no test file found at ${REL_TEST}. Write the test file FIRST with @verifies tags linking to requirement IDs, then implement the source. This enforces TDD per ISO 26262 Part 6 Table 9.\"}}"
  exit 0
fi

exit 0
