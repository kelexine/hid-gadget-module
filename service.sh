#!/system/bin/sh
# Magisk Module service script for HID Gadget
# Ensures HID gadget is initialized on boot and monitors for changes.

# Wait for the system to settle down after boot
sleep 30

# Force initial setup to prevent "no device" errors on first run
setprop sys.usb.config hid
/system/bin/hid-setup

# Variable to store the previous configuration for monitoring
prev_config=$(getprop sys.usb.config)

while true; do
    # Get the current USB configuration
    current_config=$(getprop sys.usb.config)

    # Check if the configuration has changed or if HID has been dropped
    if [ "$current_config" != "$prev_config" ]; then
        case "$current_config" in
            *hid*)
                # Configuration confirmed as HID, ensure setup is applied
                /system/bin/hid-setup
                ;;
            *)
                # HID dropped? You might want to auto-restore it here
                # setprop sys.usb.config hid
                ;;
        esac
        prev_config="$current_config"
    fi

    # Wait before checking again
    sleep 5
done
