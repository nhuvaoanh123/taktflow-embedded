#!/bin/bash
# Post-edit hook: Check for banned patterns in firmware code
# Exit 2 = blocking error (shown to Claude), exit 0 = pass

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty')

if [ -z "$FILE_PATH" ]; then
  exit 0
fi

# Only check C/C++ firmware files
if [[ ! "$FILE_PATH" =~ \.(c|h|cpp|hpp)$ ]]; then
  exit 0
fi

# Only check if file exists
if [ ! -f "$FILE_PATH" ]; then
  exit 0
fi

ERRORS=""

# Check for banned unsafe functions
if grep -n 'gets(' "$FILE_PATH" | grep -v '//.*gets(' | grep -v 'fgets(' > /dev/null 2>&1; then
  ERRORS="${ERRORS}BANNED: gets() found — use fgets() with buffer size\n"
fi

if grep -nP '(?<!\w)strcpy\(' "$FILE_PATH" | grep -v '//.*strcpy(' > /dev/null 2>&1; then
  ERRORS="${ERRORS}BANNED: strcpy() found — use strncpy() or strlcpy()\n"
fi

if grep -nP '(?<!\w)sprintf\(' "$FILE_PATH" | grep -v '//.*sprintf(' > /dev/null 2>&1; then
  ERRORS="${ERRORS}BANNED: sprintf() found — use snprintf()\n"
fi

if grep -nP '(?<!\w)strcat\(' "$FILE_PATH" | grep -v '//.*strcat(' > /dev/null 2>&1; then
  ERRORS="${ERRORS}BANNED: strcat() found — use strncat()\n"
fi

if grep -nP '(?<!\w)atoi\(' "$FILE_PATH" | grep -v '//.*atoi(' > /dev/null 2>&1; then
  ERRORS="${ERRORS}WARNING: atoi() found — use strtol() with error checking\n"
fi

# Check for hardcoded credentials
if grep -niE '(password|secret|api_key|token)\s*=\s*"[^"]+' "$FILE_PATH" | grep -v '//.*password' > /dev/null 2>&1; then
  ERRORS="${ERRORS}SECURITY: Possible hardcoded credential found\n"
fi

# Check for system() calls
if grep -nP '(?<!\w)system\(' "$FILE_PATH" | grep -v '//.*system(' > /dev/null 2>&1; then
  ERRORS="${ERRORS}BANNED: system() call found — never use in production firmware\n"
fi

if [ -n "$ERRORS" ]; then
  echo -e "Lint violations in $FILE_PATH:\n$ERRORS" >&2
  exit 2
fi

exit 0
