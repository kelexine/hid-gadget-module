#!/system/bin/sh
# Magisk Module Installation Script for HID Gadget Module
# Adapted for improved robustness and logging

set -e # Exit immediately if a command exits with a non-zero status. This is a primary robustness measure.

# Inherit environment from update-binary ($MODPATH, $ZIPFILE, $OUTFD, and sourced util_functions.sh)
# The zip has already been extracted to $MODPATH by install_module before this script is executed.

ui_print "**************************************************"
ui_print " Running custom installation script..."
ui_print "**************************************************"

# Ensure MODPATH is set (should be set by update-binary, but this is a critical defensive check)
if [ -z "$MODPATH" ]; then
  ui_print "**************************************************"
  ui_print " Error: MODPATH variable is NOT set!"
  ui_print " Cannot determine installation path. Aborting."
  ui_print "**************************************************"
  exit 1 # Exit with error code
fi
ui_print "- Installation path (MODPATH) is: $MODPATH"
ui_print "" # Add an empty line for spacing


# --- Display Module Info Banner ---

ui_print "- Reading module info from module.prop at $MODPATH/module.prop..."
# Read module info using grep and cut. Redirect stderr to /dev/null for silent errors if file/fields are missing.
MODULE_ID=$(grep "^id=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_NAME=$(grep "^name=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_VERSION=$(grep "^version=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_VERSION_CODE=$(grep "^versionCode=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_AUTHOR=$(grep "^author=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)

# Check if critical info was successfully read. Print warning if not.
if [ -z "$MODULE_ID" ] || [ -z "$MODULE_NAME" ]; then
  ui_print "**************************************************"
  ui_print " Warning: Could not read critical info from module.prop!"
  ui_print " Module might not be listed correctly in Magisk Manager."
  ui_print " Ensure $MODPATH/module.prop exists and is valid."
  ui_print "**************************************************"
  # We don't exit here, allowing installation to proceed if possible, but warning the user.
fi

# Display the module information read
ui_print "**************************************************"
ui_print "      Module Details"
ui_print "**************************************************"
# Use :-N/A for variables in case reading module.prop failed
ui_print " ID:            ${MODULE_ID:-N/A}"
ui_print " Name:          ${MODULE_NAME:-N/A}"
ui_print " Version:       ${MODULE_VERSION:-N/A} (${MODULE_VERSION_CODE:-N/A})"
ui_print " Author:        ${MODULE_AUTHOR:-N/A}"
ui_print "**************************************************"
ui_print "" # Add an empty line for spacing

ui_print "- Starting custom installation tasks..."

# --- Installation Steps ---

# Remove the META-INF directory from the installed module path ($MODPATH)
# This is standard practice as META-INF is only needed inside the zip for installation metadata.
ui_print "  Checking for and removing META-INF directory..."
if [ -d "$MODPATH/META-INF" ]; then
  # Use rm -rf for robustness in case permissions are weird or contents cause issues
  rm -rf "$MODPATH/META-INF" || ui_print "  Warning: Failed to remove META-INF directory! (Installation may still proceed)"
  ui_print "  META-INF directory removed."
else
  ui_print "  META-INF directory not found in $MODPATH, skipping removal."
fi

# Set permissions for the installed files and directories
ui_print "  Setting file permissions recursively for $MODPATH..."

# Set default permissions for the entire module directory recursively.
# This includes the module directory itself ($MODPATH).
# owner=root (0), group=root (0), directories=0755 (drwxr-xr-x), files=0644 (-rw-r--r--)
# set_perm_recursive is provided by Magisk's util_functions.sh. It handles standard SELinux contexts.
set_perm_recursive "$MODPATH" 0 0 0755 0644 || {
    ui_print "**************************************************"
    ui_print " Error: Failed to set recursive permissions on $MODPATH!"
    ui_print " Installation aborted. Check logs."
    ui_print "**************************************************"
    exit 1 # Exit if recursive permissions fail
}
ui_print "  Recursive permissions set (0755 dirs / 0644 files, owner:root, group:root)."


ui_print "  Setting specific executable permissions for scripts in $MODPATH/system/bin..."
# Set specific permissions for executable scripts/binaries in system/bin.
# owner=root (0), group=shell (2000), permissions=0755 (-rwxr-xr-x)
# The group 'shell' (2000) is common for user-executable system binaries.
# set_perm is provided by Magisk's util_functions.sh.
set_perm "$MODPATH/system/bin/hid-gadget" 0 2000 0755 || { ui_print "Error setting perms for hid-gadget! Aborting."; exit 1; }
set_perm "$MODPATH/system/bin/hid-keyboard" 0 2000 0755 || { ui_print "Error setting perms for hid-keyboard! Aborting."; exit 1; }
set_perm "$MODPATH/system/bin/hid-mouse" 0 2000 0755 || { ui_print "Error setting perms for hid-mouse! Aborting."; exit 1; }
set_perm "$MODPATH/system/bin/hid-consumer" 0 2000 0755 || { ui_print "Error setting perms for hid-consumer! Aborting."; exit 1; }
set_perm "$MODPATH/system/bin/hid-setup" 0 2000 0755 || { ui_print "Error setting perms for hid-setup! Aborting."; exit 1; }
ui_print "  Specific executable permissions set (0755 owner:root, group:shell)."

# You can add other custom installation steps here if needed, e.g.,
# - Adjusting config files based on device properties
# - Displaying configuration options (more advanced UI)
# - Performing device-specific patches


ui_print "" # Add an empty line for spacing
ui_print "- Custom installation tasks finished."
ui_print "**************************************************"
ui_print " Module Installation Successful!"
ui_print " Please REBOOT your device for changes to take effect."
ui_print "**************************************************"


# Exit with 0 to indicate success to install_module.
# install_module will finalize the installation if this script exits with 0.
exit 0
