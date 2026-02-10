#!/usr/bin/env bash
set -euo pipefail

# Fails if repo text contains conversational / direct reader references.
#
# Scope: repo-local sources and docs. Excludes MCUX workspaces and .git.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# Keep patterns conservative to avoid false positives (e.g. loop variable i).
PATTERN='\\b(you|your|yours|we|our|ours|I|me|my|mine|ChatGPT|Codex|assistant|AI model|language model)\\b'

echo "[lint] Checking for conversational phrasing / direct reader references..."

if rg -n --hidden --follow -S "$PATTERN" \
  -g'!.git/**' \
  -g'!mcuxsdk_ws/**' \
  -g'!mcuxsdk_ws_test/**' \
  -g'*.md' \
  -g'*.sh' \
  -g'*.py' \
  -g'*.c' \
  -g'*.h' \
  -g'*.cpp' \
  -g'*.hpp' \
  docs README.md STATUS.md src sdk_example tools .; then
  echo >&2
  echo "[lint] FAIL: found banned phrasing. Rewrite to neutral, repo-centric wording." >&2
  echo "[lint] See: docs/STYLE_RULES.md" >&2
  exit 1
fi

# Failsafe pointer format:
# - exactly 2 non-empty lines
# - line 1: a single artifact file path (the only artifact reference in the file)
# - line 2: one-line state summary (no additional artifact filenames)
FAILSAFE_DOC="$ROOT_DIR/docs/failsafe.md"
if [[ -f "$FAILSAFE_DOC" ]]; then
  mapfile -t _failsafe_lines < <(sed 's/\r$//' "$FAILSAFE_DOC" | awk 'NF{print}')
  if [[ "${#_failsafe_lines[@]}" -ne 2 ]]; then
    echo "[lint] FAIL: docs/failsafe.md must contain exactly 2 non-empty lines." >&2
    exit 1
  fi

  _failsafe_path="${_failsafe_lines[0]}"
  if [[ "$_failsafe_path" != failsafe/* ]]; then
    echo "[lint] FAIL: docs/failsafe.md line 1 must start with failsafe/." >&2
    exit 1
  fi

  if [[ ! -f "$ROOT_DIR/$_failsafe_path" ]]; then
    echo "[lint] FAIL: failsafe artifact missing: $_failsafe_path" >&2
    exit 1
  fi

  _artifact_refs="$( (rg -o -S '[A-Za-z0-9_./-]+[.](elf|bin|hex|srec|s19|s28|s37|axf|zip|tar|tgz|zst)' "$FAILSAFE_DOC" || true) | wc -l | tr -d ' ' )"
  if [[ "$_artifact_refs" -ne 1 ]]; then
    echo "[lint] FAIL: docs/failsafe.md must reference exactly one artifact filename." >&2
    exit 1
  fi
fi

echo "[lint] OK"
