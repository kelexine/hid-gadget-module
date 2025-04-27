# HID Browser Search Tool Documentation

## Overview

The HID Browser Search Tool is a shell script designed to automate browser-based searches across multiple operating systems. Using HID (Human Interface Device) keyboard and mouse emulation, this tool can open a browser and perform searches on Windows, macOS, Linux, Android, and iOS devices.

## Features

- Cross-platform support for major operating systems
- Automatic OS detection with manual override option
- Support for multiple browsers (Chrome, Firefox, Safari, Edge)
- Configurable timing between operations
- URL encoding for search queries
- Command-line interface with various options

## Requirements

- Magisk HID Gadget Module (for Android devices)
- HID keyboard/mouse control utilities (`hid-keyboard`, `hid-mouse`)
- Shell environment (`/system/bin/sh`)

## Installation

1. Download the script to your device
2. Make it executable: `chmod +x sample-script.sh`
3. Ensure HID utilities are in your PATH

## Basic Usage

```bash
./sample-script.sh "your search query"
```

## Command Line Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-h` | `--help` | Display help message |
| `-w N` | `--wait N` | Wait N seconds between operations (default: 2) |
| `-o TYPE` | `--os TYPE` | Specify OS type (windows, macos, linux, android, ios) |
| `-b NAME` | `--browser NAME` | Specify browser to use (chrome, firefox, safari, edge) |

## OS Detection

When no OS is specified, the script attempts to detect the OS by sending test keyboard combinations:
- Windows: Tests with Ctrl+Esc
- macOS: Tests with Command+Space

If detection is inconclusive, it defaults to Windows keyboard shortcuts.

## Examples

### Basic Search

```bash
./sample-script.sh "weather forecast"
```

This will:
1. Attempt to detect the OS
2. Open the default browser
3. Navigate to Google
4. Search for "weather forecast"

### Specify OS and Browser

```bash
./sample-script.sh -o macos -b safari "programming tutorials"
```

This will:
1. Use macOS-specific keyboard shortcuts
2. Open Safari browser
3. Navigate to Google
4. Search for "programming tutorials"

### Custom Wait Time

```bash
./sample-script.sh -w 5 -o linux -b firefox "Linux kernel compilation"
```

This will:
1. Use Linux-specific keyboard shortcuts
2. Open Firefox browser
3. Wait 5 seconds between operations
4. Search for "Linux kernel compilation"

### Android Example

```bash
./sample-script.sh -o android -b chrome "best android apps 2025"
```

This will:
1. Press the home button on Android
2. Open the app drawer
3. Launch Chrome
4. Search for "best android apps 2025"

## Advanced Features

### Custom URL Encoding

The script includes basic URL encoding that replaces spaces with plus signs. This ensures search queries with spaces work properly.

### Key Combination Functions

The script provides several utility functions for keyboard interaction:
- `send_key_combo`: Send keyboard combinations like CTRL-ALT-t
- `type_text`: Type text strings
- `press_enter`: Press the Enter key

### OS-Specific Implementations

Each supported OS has a dedicated function for optimized browser interaction:
- `open_browser_windows`: For Windows systems
- `open_browser_macos`: For macOS systems
- `open_browser_linux`: For Linux systems
- `open_browser_android`: For Android devices
- `open_browser_ios`: For iOS devices

## Troubleshooting

### Common Issues

1. **Script fails to detect OS**
   - Manually specify OS with `-o` option
   - Example: `./sample-script.sh -o windows "search query"`

2. **Browser doesn't open**
   - Increase wait time: `./sample-script.sh -w 4 "search query"`
   - Specify browser explicitly: `./sample-script.sh -b chrome "search query"`

3. **Search query doesn't work with special characters**
   - Use quotes around complex queries: `./sample-script.sh "how to fix error & warning"`

### Debugging

The script outputs status messages to help with debugging:
- OS detection process
- Browser opening confirmation
- Operation completion status

## Limitations

- OS detection is limited and may require manual specification
- Different browser versions may respond differently to keyboard shortcuts
- Some mobile device interactions might need refinement
- No support for advanced mouse gestures

## Usage Notes

- For Android devices, this script requires Magisk HID Gadget Module
- iOS support is experimental and uses basic gestures
- The script uses Google as the default search engine
- Wait times may need adjustment based on device performance

## Security Considerations

This tool simulates keyboard and mouse input, which could potentially:
- Enter data into unexpected fields if timing is off
- Execute commands if focus changes unexpectedly
- Interact with unintended applications

Always use caution when running automated input scripts, especially on systems with sensitive information.
