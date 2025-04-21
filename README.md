# HID Gadget Module for Magisk

This Magisk module provides USB HID (Human Interface Device) gadget functionality for Android devices. It allows your device to act as a USB keyboard, mouse, or consumer control device when connected to a computer.

## Features

### Keyboard Emulation
- Full keyboard support with standard layout, function keys (F1-F12), and modifiers (Ctrl, Alt, Shift, GUI/Meta)
- Special keys: Enter, Escape, Tab, media keys, etc.
- Key combination support

### Mouse Emulation
- Relative motion control
- Button clicks (left, middle, right)
- Double-click functionality
- Press-and-hold actions
- Vertical and horizontal scrolling
- Smooth movements with variable speeds

### Consumer Control
- Media playback controls (Play, Pause, Stop)
- Volume controls (Up, Down, Mute)
- Media navigation (Next, Previous, Forward, Rewind)
- Brightness controls

## Installation

1. Download the latest release ZIP from the [Releases page](https://github.com/kelexine/hid-gadget-module/releases)
2. Install through Magisk Manager:
   - Open Magisk Manager
   - Tap on Modules
   - Tap "Install from storage"
   - Select the downloaded ZIP file
   - Reboot your device after installation

### Requirements

- Magisk v20.4 or newer
- A device with USB OTG support and kernel configfs support
- Root access

## Usage

### Keyboard Commands

The `hid-keyboard` tool lets you send keyboard events:

```bash
# Type text
hid-keyboard "Hello World"

# Press special keys
hid-keyboard F5
hid-keyboard ENTER
hid-keyboard ESC

# Use modifiers
hid-keyboard CTRL-ALT-DELETE
hid-keyboard CTRL-C
hid-keyboard ALT-TAB

# Press and hold keys
hid-keyboard --hold ALT
hid-keyboard TAB
hid-keyboard --release

# Complex combinations
hid-keyboard CTRL-ALT-F4
```

### Mouse Commands

The `hid-mouse` tool provides mouse control:

```bash
# Move the cursor (relative coordinates)
hid-mouse move 10 0    # Move right
hid-mouse move 0 -10   # Move up
hid-mouse move -5 5    # Move left and down

# Click buttons
hid-mouse click        # Left click (default)
hid-mouse click right  # Right click
hid-mouse click middle # Middle click

# Double-click
hid-mouse doubleclick

# Press and hold
hid-mouse down         # Press and hold left button
hid-mouse move 20 20   # Drag while holding
hid-mouse up           # Release button

# Scrolling
hid-mouse scroll 0 5   # Scroll down 5 units
hid-mouse scroll 0 -5  # Scroll up 5 units
hid-mouse scroll 5 0   # Scroll right 5 units
```

### Consumer Control Commands

The `hid-consumer` tool controls media and related functions:

```bash
# Media playback
hid-consumer PLAY
hid-consumer PAUSE
hid-consumer STOP
hid-consumer NEXT
hid-consumer PREVIOUS

# Volume control
hid-consumer VOL+
hid-consumer VOL-
hid-consumer MUTE

# Other controls
hid-consumer BRIGHTNESS+
hid-consumer BRIGHTNESS-
```

## Troubleshooting

### Device Not Detected as HID

1. Make sure USB debugging is disabled
2. Try different USB cables
3. Check that your kernel supports USB configfs
4. Reboot your device
5. Check logs with `logcat | grep hidg`

### Permission Issues

If you encounter permission errors when using the commands:

```bash
# Fix permissions manually (use the correct device nodes)
su -c chmod 666 /dev/hidg1 /dev/hidg2 /dev/hidg3
```

### Commands Not Working

1. Make sure your device is connected to a host computer
2. Verify the module is properly installed and enabled in Magisk Manager
3. Some functions may not be supported by all host computers

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements

- The Magisk development team
- Linux kernel USB gadget documentation
- All contributors and testers
