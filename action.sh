#!/system/bin/sh
# action.sh - launches the hid-gadget WebUI from the manager's Action button.
# Author: kelexine <https://github.com/kelexine>

MODID="hid-gadget"

# Native MySU Manager
pm path dev.kelexine.mysu > /dev/null 2>&1 && {
    echo "- Launching WebUI in MySU Manager..."
    am start -n "dev.kelexine.mysu/.ui.webui.WebUIActivity" -e id "$MODID"
    exit 0
}

# KSUWebUIStandalone
pm path io.github.a13e300.ksuwebui > /dev/null 2>&1 && {
    echo "- Launching WebUI in KSUWebUIStandalone..."
    am start -n "io.github.a13e300.ksuwebui/.WebUIActivity" -e id "$MODID"
    exit 0
}

# WebUI X (MMRL)
pm path com.dergoogler.mmrl.wx > /dev/null 2>&1 && {
    echo "- Launching WebUI in WebUI X..."
    am start -n "com.dergoogler.mmrl.wx/.ui.activity.webui.WebUIActivity" -e MOD_ID "$MODID"
    exit 0
}

# No WebUI-capable manager found: fall back to the interactive TUI.
echo "- No WebUI host found, falling back to TUI..."
exec /system/bin/hid-tui
