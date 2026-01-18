# Usage: ./build_release.sh [version_arg] [repo_slug]
# If version_arg is "auto" (default), it increments the PATCH version.
# If version_arg is a specific string (e.g., v1.35.0), it sets that version.
# If repo_slug is provided (e.g., "rexackermann/hid-gadget-module"), it updates module.prop/update.json URLs.

set -e

MODULE_PROP="module.prop"
TUI_SOURCE="tui.c"

# Function to get current version
get_current_version() {
    grep "^version=" "$MODULE_PROP" | cut -d'=' -f2
}

# Function to get current version code
get_current_code() {
    grep "^versionCode=" "$MODULE_PROP" | cut -d'=' -f2
}

# 1. Determine Target Version
TARGET_VER="$1"
REPO_SLUG="$2" # Optional: e.g. "rexackermann/hid-gadget-module"
CURRENT_VER=$(get_current_version)
CURRENT_CODE=$(get_current_code)

if [ -z "$TARGET_VER" ] || [ "$TARGET_VER" = "auto" ]; then
    # Auto-bump patch
    # Remove 'v' prefix
    ver_clean=${CURRENT_VER#v}
    MAJOR=$(echo $ver_clean | cut -d. -f1)
    MINOR=$(echo $ver_clean | cut -d. -f2)
    PATCH=$(echo $ver_clean | cut -d. -f3)
    NEW_PATCH=$((PATCH + 1))
    TARGET_VER="v$MAJOR.$MINOR.$NEW_PATCH"
    # Auto-bump code (e.g., 13200 -> 13201)
    # Simple logic: remove dots, verify length? 
    # Better: just increment existing integer
    TARGET_CODE=$((CURRENT_CODE + 100)) # Simple strategy: +100 for minor/patch signifier?
    # Or just +1
    TARGET_CODE=$((CURRENT_CODE + 1))
else
    # Explicit version set (e.g., v1.35.0)
    # Validate format vX.Y.Z
    if ! echo "$TARGET_VER" | grep -qE "^v[0-9]+\.[0-9]+\.[0-9]+$"; then
        echo "Error: Invalid version format. Use vX.Y.Z"
        exit 1
    fi
    
    # Calculate Version Code from Version String
    # v1.35.0 -> 13500
    ver_clean=${TARGET_VER#v}
    MAJOR=$(echo $ver_clean | cut -d. -f1)
    MINOR=$(echo $ver_clean | cut -d. -f2)
    PATCH=$(echo $ver_clean | cut -d. -f3)
    
    # Formula: Major * 10000 + Minor * 100 + Patch
    TARGET_CODE=$((MAJOR * 10000 + MINOR * 100 + PATCH))
fi

echo ">> Bumping version: $CURRENT_VER -> $TARGET_VER (Code: $CURRENT_CODE -> $TARGET_CODE)"

# 2. Update module.prop
sed -i "s/^version=.*/version=$TARGET_VER/" "$MODULE_PROP"
sed -i "s/^versionCode=.*/versionCode=$TARGET_CODE/" "$MODULE_PROP"

if [ -n "$REPO_SLUG" ]; then
    echo ">> Updating Repository URLs to: $REPO_SLUG"
    # module.prop: updateJson=https://raw.githubusercontent.com/<SLUG>/main/update.json
    sed -i "s|^updateJson=.*|updateJson=https://raw.githubusercontent.com/$REPO_SLUG/main/update.json|" "$MODULE_PROP"
    
    # update.json: "changelog": "https://github.com/<SLUG>/blob/main/CHANGELOG.md"
    sed -i "s|\"changelog\": \".*\"|\"changelog\": \"https://github.com/$REPO_SLUG/blob/main/CHANGELOG.md\"|g" update.json
    
    # update.json: zipUrl will be updated below with filename
    # We first set the base, then append filename
    # Actually, simpler to just replace the logic below to use REPO_SLUG
fi

# 3. Update update.json
sed -i "s/\"version\": \".*\"/\"version\": \"$TARGET_VER\"/" update.json
sed -i "s/\"versionCode\": [0-9]*/\"versionCode\": $TARGET_CODE/" update.json

# Sync zipUrl name
# If REPO_SLUG is set, construct full URL. Else just filename/previous base
if [ -n "$REPO_SLUG" ]; then
    NEW_URL="https://github.com/$REPO_SLUG/releases/download/$TARGET_VER/hid-gadget-module-$TARGET_VER.zip"
    sed -i "s|\"zipUrl\": \".*\"|\"zipUrl\": \"$NEW_URL\"|" update.json
else
    # Fallback: Just update filename in existing URL
    sed -i "s/hid-gadget-module.*.zip/hid-gadget-module-$TARGET_VER.zip/" update.json
fi

# 4. Update README badge
# Pattern: ![Version](https://img.shields.io/badge/version-vX.Y.Z-blue...
sed -i "s/version-v[0-9]\+\.[0-9]\+\.[0-9]\+/version-$TARGET_VER/" README.md

# 5. Update C Sources
# tui.c: "HID INDUSTRIAL vX.Y.Z"
sed -i "s/HID INDUSTRIAL v[0-9]\+\.[0-9]\+\.[0-9]\+/HID INDUSTRIAL $TARGET_VER/" "$TUI_SOURCE"
# hid-gadget.c: "HID Gadget Controller vX.Y.Z"
sed -i "s/HID Gadget Controller v[0-9]\+\.[0-9]\+\.[0-9]\+/HID Gadget Controller $TARGET_VER/" hid-gadget.c

# 6. Build
echo ">> Building static binaries with Make..."
make clean
make static

# 7. Zip
echo ">> Zipping module..."
ZIP_NAME="hid-gadget-module-$TARGET_VER.zip"
zip -r "$ZIP_NAME" . -x ".*" "build/*" "*.zip" "build_release.sh"

echo ">> Done! Created $ZIP_NAME"
ls -lh "$ZIP_NAME"
