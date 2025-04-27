# Sample-Script Documentation

This document explains how to use the `sample-script.sh` utility to perform automated web searches via USB HID gadget emulation.

## 1. Overview

`sample-script.sh` leverages the Magisk HID gadget module to emulate keyboard input on a host machine. Its primary use case is on Android devices with root access, using Termux (or any compatible terminal app), and an OTG/USB cable connection to a client computer or target device.

Key features:
- Emulates Run/Spotlight dialogs across multiple OSes
- Builds and types browser-search URLs for Google and DuckDuckGo
- Designed for Android hosts but supports Windows, macOS, Linux, and iOS emulation

## 2. Prerequisites

1. **Android Device** with:
   - **Root access** (Magisk v20.4+)
   - **HID Gadget Magisk module** installed and active
   - **Termux** or another Android terminal app
   
2. **OTG/USB Cable** connecting:
   - Android device (acting as USB gadget)
   - Host computer or target device (Windows/macOS/Linux/iOS)
   
3. **sample-script.sh** placed in a writable directory and marked executable

## 3. Installation

1. **Download or clone your repository** containing `sample-script.sh`

2. Open Termux (or your preferred terminal) and navigate to the directory:
   ```sh
   cd /path/to/your/scripts
   ```

3. Make the script executable:
   ```sh
   chmod +x sample-script.sh
   ```

4. Verify that hid-keyboard is available:
   ```sh
   command -v hid-keyboard || echo "Error: hid-keyboard not found"
   ```

## 4. Usage

```sh
./sample-script.sh [ -o windows|mac|linux|android|ios ] [ -e google|duckduckgo ] [ -d delay_seconds ] keywords...
```

Parameters:
- `-o`: Target OS (default: android)
- `-e`: Search engine (google or duckduckgo, default: google)
- `-d`: Delay between keystrokes in seconds (default: 0.5)
- `keywords...`: One or more terms to search (use quotes for multi-word queries)

### Typical Android Invocation

**On the Android host itself (using am intents):**
```sh
./sample-script.sh -o android -e google "custom ROM kernels"
```
This will launch the default browser on the Android device (client) with the search results.

### HID Emulation on Connected Host

**When targeting a connected computer via OTG:**
```sh
./sample-script.sh -o linux -e duckduckgo "android development"
```

The script will:
1. Send the OS-specific shortcut (e.g., `Alt+F2` on Linux)
2. Type the constructed URL
3. Press Enter to navigate

## 5. Examples

1. **Search Wikipedia on macOS:**
   ```sh
   ./sample-script.sh -o mac -e google "Wikipedia shell scripts"
   ```

2. **Quick DuckDuckGo search on Windows:**
   ```sh
   ./sample-script.sh -o windows -e duckduckgo "termux automation"
   ```

3. **Android native search:**
   ```sh
   ./sample-script.sh -o android -e google "HID gadget Magisk"
   ```

## 6. Troubleshooting

1. **hid-keyboard not found:** Ensure the Magisk HID gadget module is installed and PATH includes `/system/bin`

2. **No Run dialog appears:** Verify OTG cable connection and that the host recognizes the Android gadget

3. **Intent launch fails on Android:** Confirm Termux has the am binary (pkg install termux-tools)

4. **Slow typing or missed keys:** Increase the `-d` delay value (e.g., `-d 1`)

## 7. Future Enhancements

- **Macro Profiles:** Load JSON-based macros for complex multi-step tasks
- **Configurable Keymaps:** Allow custom OS shortcuts via a config file
- **Self-Diagnostics:** Add a `--status` flag to verify gadget readiness

---

You're now ready to automate browser searches directly from your rooted Android device!
