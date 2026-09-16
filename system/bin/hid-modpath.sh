#!/system/bin/sh
# hid-modpath.sh - resolves the installed module directory across root
# managers (MySU-native path first, legacy Magisk/KernelSU path second).
# Sourced by hid-setup at runtime; also unit-tested standalone via
# ROOT_PREFIX, which lets a test harness point this at a fake tree
# instead of the real /data/adb.
# Author: kelexine <https://github.com/kelexine>

: "${ROOT_PREFIX:=}"

resolve_module_dir() {
    for _cand in \
        "${ROOT_PREFIX}/data/adb/mysu/modules/hid-gadget" \
        "${ROOT_PREFIX}/data/adb/modules/hid-gadget"
    do
        if [ -d "$_cand" ]; then
            echo "$_cand"
            return 0
        fi
    done
    return 1
}
