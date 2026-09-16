#!/bin/sh
# test_module_path_resolution.sh - unit test for hid-modpath.sh's
# resolve_module_dir(), exercised against a fake filesystem tree so it
# runs without root or a real device.
# Author: kelexine <https://github.com/kelexine>
set -eu

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(cd -- "$SCRIPT_DIR/.." && pwd)
RESOLVER="$ROOT_DIR/system/bin/hid-modpath.sh"

FAILURES=0

assert_eq() {
    label="$1"; expected="$2"; actual="$3"
    if [ "$expected" != "$actual" ]; then
        echo "[FAIL] $label: expected '$expected', got '$actual'"
        FAILURES=$((FAILURES + 1))
    else
        echo "[PASS] $label"
    fi
}

run_case() {
    # Runs resolve_module_dir() in a clean subshell with ROOT_PREFIX
    # pointed at $1, so state never leaks between test cases.
    # shellcheck disable=SC2034,SC1090  # consumed by the sourced resolver
    ( ROOT_PREFIX="$1"; . "$RESOLVER"; resolve_module_dir 2>/dev/null || echo "__NONE__" )
}

# Case 1: MySU path present, legacy absent -> MySU path wins.
T1=$(mktemp -d)
mkdir -p "$T1/data/adb/mysu/modules/hid-gadget"
assert_eq "prefers MySU path when present" \
    "$T1/data/adb/mysu/modules/hid-gadget" "$(run_case "$T1")"
rm -rf "$T1"

# Case 2: only legacy path present -> falls back to legacy.
T2=$(mktemp -d)
mkdir -p "$T2/data/adb/modules/hid-gadget"
assert_eq "falls back to legacy path" \
    "$T2/data/adb/modules/hid-gadget" "$(run_case "$T2")"
rm -rf "$T2"

# Case 3: both present -> MySU path still takes priority.
T3=$(mktemp -d)
mkdir -p "$T3/data/adb/mysu/modules/hid-gadget" "$T3/data/adb/modules/hid-gadget"
assert_eq "MySU path takes priority over legacy" \
    "$T3/data/adb/mysu/modules/hid-gadget" "$(run_case "$T3")"
rm -rf "$T3"

# Case 4: neither present -> resolver reports failure, no output.
T4=$(mktemp -d)
assert_eq "reports failure when neither path exists" \
    "__NONE__" "$(run_case "$T4")"
rm -rf "$T4"

if [ "$FAILURES" -ne 0 ]; then
    echo "$FAILURES test(s) failed."
    exit 1
fi
echo "All module-path resolution tests passed."
