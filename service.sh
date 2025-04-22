#!/system/bin/sh
# Magisk Module service script for HID Gadget
# Monitors sys.usb.config and runs hid-setup when 'hid' is detected.

# Wait a bit for the system to settle down after boot
sleep 30

# Log file (optional, useful for debugging)
# LOGFILE="/data/local/tmp/hid_service.log"
# echo "$(date): HID service started." >> $LOGFILE

# Variable to store the previous configuration
prev_config=""

while true; do
    # Get the current USB configuration
    current_config=$(getprop sys.usb.config)

    # Check if the configuration has changed since the last check
    if [ "$current_config" != "$prev_config" ]; then
        # echo "$(date): USB config changed from '$prev_config' to '$current_config'." >> $LOGFILE

        # Check if the new configuration contains 'hid'
        # Using case statement for robust substring check
        case "$current_config" in
            *hid*)
                # echo "$(date): 'hid' detected in config. Running hid-setup..." >> $LOGFILE
                # Execute the setup script
                /system/bin/hid-setup
                if [ $? -eq 0 ]; then
                    # echo "$(date): hid-setup completed successfully." >> $LOGFILE
                    : # Placeholder for success action if needed
                else
                    # echo "$(date): hid-setup failed with exit code $?." >> $LOGFILE
                    : # Placeholder for failure action if needed
                fi
                ;;
            *)
                # 'hid' is not present in the current configuration
                # echo "$(date): 'hid' not detected in config." >> $LOGFILE
                # Optional: Add cleanup logic here if needed when HID is disabled
                ;;
        esac

        # Update the previous configuration state
        prev_config="$current_config"
    fi

    # Wait before checking again
    sleep 5
done
