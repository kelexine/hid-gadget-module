#!/system/bin/sh
# Magisk Module Installation Script for HID Gadget Module

# Inherit environment from update-binary ($MODPATH, $ZIPFILE, $OUTFD, and sourced util_functions.sh)

# Set default permissions for extracted files
umask 022

# Ensure MODPATH is set (should be set by update-binary, but defensive check)
[ -z "$MODPATH" ] && exit 1

# --- Display Module Info Banner ---

# Read module info from module.prop
MODULE_ID=$(grep "^id=" $MODPATH/module.prop 2>/dev/null | cut -d= -f2)
MODULE_NAME=$(grep "^name=" $MODPATH/module.prop 2>/dev/null | cut -d= -f2)
MODULE_VERSION=$(grep "^version=" $MODPATH/module.prop 2>/dev/null | cut -d= -f2)
MODULE_VERSION_CODE=$(grep "^versionCode=" $MODPATH/module.prop 2>/dev/null | cut -d= -f2)
MODULE_AUTHOR=$(grep "^author=" $MODPATH/module.prop 2>/dev/null | cut -d= -f2)

ui_print "**************************************************"
ui_print "      Magisk Module Installation"
ui_print "**************************************************"
ui_print " Module:        ${MODULE_NAME:-N/A}"
ui_print " ID:            ${MODULE_ID:-N/A}"
ui_print " Version:       ${MODULE_VERSION:-N/A} (${MODULE_VERSION_CODE:-N/A})"
ui_print " Author:        ${MODULE_AUTHOR:-N/A}"
ui_print "**************************************************"
ui_print "" # Add an empty line for spacing

# --- Installation Steps ---

ui_print "- Performing custom installation tasks"

# Remove the META-INF directory from the installed module path ($MODPATH)
# This is standard practice as META-INF is only needed inside the zip
if [ -d "$MODPATH/META-INF" ]; then
  ui_print "  Removing META-INF directory..."
  rm -rf $MODPATH/META-INF
fi

# Set permissions for the installed files
ui_print "  Setting file permissions..."

# Set default permissions for the entire module directory recursively
# owner=root, group=root, dirs=0755, files=0644
# set_perm_recursive is provided by util_functions.sh
set_perm_recursive $MODPATH 0 0 0755 0644

# Set specific permissions for executable scripts in system/bin
# owner=root, group=shell (2000), permissions=0755
# set_perm is provided by util_functions.sh
set_perm $MODPATH/system/bin/hid-gadget 0 2000 0755
set_perm $MODPATH/system/bin/hid-keyboard 0 2000 0755
set_perm $MODPATH/system/bin/hid-mouse 0 2000 0755
set_perm $MODPATH/system/bin/hid-consumer 0 2000 0755
set_perm $MODPATH/system/bin/hid-setup 0 2000 0755

# You can add other installation steps here if needed

ui_print "" # Add an empty line for spacing
ui_print "- Custom tasks complete."
ui_print "- Installation finished."

# Exit with 0 for success (install_module handles overall success based on customize.sh exit code)
exit 0
