#!/system/bin/sh
# Magisk Module Installation Script for HID Gadget Module
# Enhanced for better visual feedback and robustness

# Strict mode: Exit on any error
set -e

# --- Initialization ---
# Inherit environment from update-binary ($MODPATH, $ZIPFILE, $OUTFD, $ARCH, $API, and sourced util_functions.sh)

# Function for printing separators
print_separator() {
  ui_print "##################################################"
}

# Function for printing step headers
print_step() {
  ui_print " " # Add spacing before step
  ui_print "--- $1 ---"
}

# Function for printing success messages
print_success() {
  ui_print "[+] $1"
}

# Function for printing error messages and exiting
print_error_and_exit() {
  ui_print "[!] ERROR: $1"
  ui_print "[!] Installation failed."
  print_separator
  exit 1
}

# --- Start Installation ---
print_separator
ui_print " Running HID Gadget Module Installation Script"
ui_print " Device Arch: $ARCH  |  Android API: $API"
print_separator
ui_print " "

# --- Check Environment ---
print_step "Checking Environment"
if [ -z "$MODPATH" ]; then
  print_error_and_exit "MODPATH variable is not set. Cannot determine installation path."
fi
ui_print "  - Installation path (MODPATH): $MODPATH"
# Check if util_functions.sh was sourced (check for a known function)
if ! command -v set_perm &> /dev/null; then
    print_error_and_exit "Magisk utility functions (util_functions.sh) not loaded."
fi
print_success "Environment checks passed."

# --- Display Module Info ---
print_step "Reading Module Information"
# Read module info safely, providing defaults
MODULE_ID=$(grep_prop id "$MODPATH/module.prop")
MODULE_NAME=$(grep_prop name "$MODPATH/module.prop")
MODULE_VERSION=$(grep_prop version "$MODPATH/module.prop")
MODULE_VERSION_CODE=$(grep_prop versionCode "$MODPATH/module.prop")
MODULE_AUTHOR=$(grep_prop author "$MODPATH/module.prop")

if [ -z "$MODULE_ID" ] || [ -z "$MODULE_NAME" ]; then
  ui_print "  [!] Warning: Could not read critical info from module.prop!"
fi

ui_print " "
ui_print "    Module ID:        ${MODULE_ID:-N/A}"
ui_print "    Module Name:      ${MODULE_NAME:-N/A}"
ui_print "    Version:          ${MODULE_VERSION:-N/A} (Code: ${MODULE_VERSION_CODE:-N/A})"
ui_print "    Author:           ${MODULE_AUTHOR:-N/A}"
ui_print " "
print_success "Module information displayed."

# --- Architecture-Specific Binary Handling ---
print_step "Handling Architecture-Specific Binary"
ui_print "  - Detected device architecture: $ARCH"

ARCH_BINARY_SRC="$MODPATH/blobs/$ARCH/hid-gadget"
BINARY_DEST_DIR="$MODPATH/system/bin"
BINARY_DEST_FILE="$BINARY_DEST_DIR/hid-gadget"

ui_print "  - Checking for pre-compiled binary: $ARCH_BINARY_SRC"
if [ ! -f "$ARCH_BINARY_SRC" ]; then
  print_error_and_exit "Binary for architecture '$ARCH' not found at $ARCH_BINARY_SRC. Module may not support your device."
fi
print_success "Found binary for $ARCH architecture."

ui_print "  - Preparing destination directory: $BINARY_DEST_DIR"
mkdir -p "$BINARY_DEST_DIR" || print_error_and_exit "Failed to create destination directory $BINARY_DEST_DIR."
print_success "Destination directory ensured."

ui_print "  - Copying binary to $BINARY_DEST_FILE"
cp "$ARCH_BINARY_SRC" "$BINARY_DEST_FILE" || print_error_and_exit "Failed to copy binary from $ARCH_BINARY_SRC to $BINARY_DEST_FILE."
print_success "Binary copied successfully."

# Clean up the blobs directory
print_step "Cleaning Up Temporary Files"
ui_print "  - Removing temporary blobs directory: $MODPATH/blobs"
rm -rf "$MODPATH/blobs" || ui_print "  [!] Warning: Failed to remove $MODPATH/blobs directory."
print_success "Temporary files cleaned up."

# --- Set Permissions ---
print_step "Setting File Permissions"

# Remove META-INF (often done by Magisk, but good to ensure)
ui_print "  - Removing META-INF directory..."
if [ -d "$MODPATH/META-INF" ]; then
  rm -rf "$MODPATH/META-INF" || ui_print "  [!] Warning: Failed to remove META-INF directory!"
  print_success "META-INF directory removed."
else
  ui_print "  - META-INF directory not found, skipping removal."
fi

ui_print "  - Setting default recursive permissions (root:root, 0755 dirs, 0644 files)..."
set_perm_recursive "$MODPATH" 0 0 0755 0644 || print_error_and_exit "Failed to set recursive permissions on $MODPATH."
print_success "Default permissions set."

ui_print "  - Setting specific executable permissions (root:shell, 0755)..."
# Set executable permissions for the main binary and wrappers
set_perm "$MODPATH/system/bin/hid-gadget"   0 2000 0755 || print_error_and_exit "Failed setting permissions for hid-gadget."
set_perm "$MODPATH/system/bin/hid-keyboard" 0 2000 0755 || print_error_and_exit "Failed setting permissions for hid-keyboard."
set_perm "$MODPATH/system/bin/hid-mouse"    0 2000 0755 || print_error_and_exit "Failed setting permissions for hid-mouse."
set_perm "$MODPATH/system/bin/hid-consumer" 0 2000 0755 || print_error_and_exit "Failed setting permissions for hid-consumer."
set_perm "$MODPATH/system/bin/hid-setup"    0 2000 0755 || print_error_and_exit "Failed setting permissions for hid-setup."
# Set permissions for the init script
set_perm "$MODPATH/system/etc/init/init.hidgadget.rc" 0 0 0644 || print_error_and_exit "Failed setting permissions for init.hidgadget.rc."
# Set permissions for descriptor files
if [ -d "$MODPATH/system/etc/hid" ]; then
  set_perm_recursive "$MODPATH/system/etc/hid" 0 0 0755 0644 || print_error_and_exit "Failed setting permissions for /system/etc/hid."
fi
print_success "Executable and specific permissions set."

# --- Finalization ---
ui_print " "
print_separator
ui_print " Module Installation Successful!"
ui_print " Please REBOOT your device for changes to take effect."
print_separator
ui_print " "

exit 0
