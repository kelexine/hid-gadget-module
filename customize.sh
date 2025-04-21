#!/system/bin/sh
# Magisk Module Installation Script for HID Gadget Module
# Adapted for improved robustness, logging, and architecture handling

set -e # Exit immediately if a command exits with a non-zero status.

# Inherit environment from update-binary ($MODPATH, $ZIPFILE, $OUTFD, $ARCH, $API, and sourced util_functions.sh)

ui_print "**************************************************"
ui_print " Running custom installation script..."
ui_print " Device Arch: $ARCH  |  Android API: $API" # Show detected Arch/API
ui_print "**************************************************"

# Ensure MODPATH is set
if [ -z "$MODPATH" ]; then
  ui_print "**************************************************"
  ui_print " Error: MODPATH variable is NOT set!"
  ui_print " Cannot determine installation path. Aborting."
  ui_print "**************************************************"
  exit 1
fi
ui_print "- Installation path (MODPATH) is: $MODPATH"
ui_print ""

# --- Display Module Info Banner ---
# (Keep the existing module info reading and display logic here)
ui_print "- Reading module info from module.prop at $MODPATH/module.prop..."
MODULE_ID=$(grep "^id=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_NAME=$(grep "^name=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_VERSION=$(grep "^version=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_VERSION_CODE=$(grep "^versionCode=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)
MODULE_AUTHOR=$(grep "^author=" "$MODPATH/module.prop" 2>/dev/null | cut -d= -f2)

if [ -z "$MODULE_ID" ] || [ -z "$MODULE_NAME" ]; then
  ui_print "**************************************************"
  ui_print " Warning: Could not read critical info from module.prop!"
  ui_print "**************************************************"
fi

ui_print "**************************************************"
ui_print "      Module Details"
ui_print "**************************************************"
ui_print " ID:            ${MODULE_ID:-N/A}"
ui_print " Name:          ${MODULE_NAME:-N/A}"
ui_print " Version:       ${MODULE_VERSION:-N/A} (${MODULE_VERSION_CODE:-N/A})"
ui_print " Author:        ${MODULE_AUTHOR:-N/A}"
ui_print "**************************************************"
ui_print ""

ui_print "- Starting custom installation tasks..."

# --- Architecture-Specific Binary Handling ---
ui_print "  Detecting device architecture: $ARCH"
# Define the path to the pre-compiled binary for the detected architecture
ARCH_BINARY_SRC="$MODPATH/blobs/$ARCH/hid-gadget"
# Define the destination path for the binary
BINARY_DEST="$MODPATH/system/bin/hid-gadget"

ui_print "  Checking for pre-compiled binary at: $ARCH_BINARY_SRC"
if [ -f "$ARCH_BINARY_SRC" ]; then
  ui_print "  Found binary for $ARCH architecture."
  ui_print "  Copying binary to $BINARY_DEST..."
  # Ensure the destination directory exists
  mkdir -p "$MODPATH/system/bin" || {
    ui_print "**************************************************"
    ui_print " Error: Failed to create $MODPATH/system/bin directory!"
    ui_print " Installation aborted."
    ui_print "**************************************************"
    exit 1
  }
  # Copy the binary
  cp "$ARCH_BINARY_SRC" "$BINARY_DEST" || {
    ui_print "**************************************************"
    ui_print " Error: Failed to copy binary from $ARCH_BINARY_SRC to $BINARY_DEST!"
    ui_print " Installation aborted."
    ui_print "**************************************************"
    exit 1
  }
  ui_print "  Binary copied successfully."

  # Clean up the blobs directory now that we've copied the needed binary
  ui_print "  Removing temporary blobs directory..."
  rm -rf "$MODPATH/blobs" || ui_print "  Warning: Failed to remove $MODPATH/blobs directory."
  ui_print "  Blobs directory removed."

else
  ui_print "**************************************************"
  ui_print " Error: Pre-compiled binary for architecture '$ARCH' not found!"
  ui_print " Looked in: $ARCH_BINARY_SRC"
  ui_print " This module may not support your device architecture."
  ui_print " Installation aborted."
  ui_print "**************************************************"
  exit 1
fi
# --- End Architecture Handling ---


# --- Installation Steps ---

# Remove META-INF
ui_print "  Checking for and removing META-INF directory..."
if [ -d "$MODPATH/META-INF" ]; then
  rm -rf "$MODPATH/META-INF" || ui_print "  Warning: Failed to remove META-INF directory!"
  ui_print "  META-INF directory removed."
else
  ui_print "  META-INF directory not found, skipping removal."
fi

# Set permissions
ui_print "  Setting file permissions recursively for $MODPATH..."
# Set default permissions (owner=root, group=root, dirs=0755, files=0644)
set_perm_recursive "$MODPATH" 0 0 0755 0644 || {
    ui_print "**************************************************"
    ui_print " Error: Failed to set recursive permissions on $MODPATH!"
    ui_print " Installation aborted."
    ui_print "**************************************************"
    exit 1
}
ui_print "  Recursive permissions set (0755 dirs / 0644 files)."

ui_print "  Setting specific executable permissions..."
# Set executable permissions for the main binary (owner=root, group=shell, perms=0755)
set_perm "$MODPATH/system/bin/hid-gadget" 0 2000 0755 || { ui_print "Error setting perms for hid-gadget! Aborting."; exit 1; }
# Set executable permissions for the wrapper scripts (owner=root, group=shell, perms=0755)
set_perm "$MODPATH/system/bin/hid-keyboard" 0 2000 0755 || { ui_print "Error setting perms for hid-keyboard! Aborting."; exit 1; }
set_perm "$MODPATH/system/bin/hid-mouse" 0 2000 0755 || { ui_print "Error setting perms for hid-mouse! Aborting."; exit 1; }
set_perm "$MODPATH/system/bin/hid-consumer" 0 2000 0755 || { ui_print "Error setting perms for hid-consumer! Aborting."; exit 1; }
set_perm "$MODPATH/system/bin/hid-setup" 0 2000 0755 || { ui_print "Error setting perms for hid-setup! Aborting."; exit 1; }
ui_print "  Specific executable permissions set (0755 owner:root, group:shell)."

# (Keep any other custom installation steps here)

ui_print ""
ui_print "- Custom installation tasks finished."
ui_print "**************************************************"
ui_print " Module Installation Successful!"
ui_print " Please REBOOT your device for changes to take effect."
ui_print "**************************************************"

exit 0
