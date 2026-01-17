# USB HID Gadget Module v1.23.8

This Magisk module provides USB HID (Human Interface Device) gadget functionality for Android devices. It allows your device to act as a USB keyboard, mouse, or consumer control device.

> [!IMPORTANT]
> **Universal Static Binaries**: All binaries are compiled with `musl` and zero dependencies, ensuring compatibility across all Android versions (Bionic) and Linux distributions (glibc).
> 
> **Auto-Recovery**: As of v1.23.8, all wrappers include automatic failure detection and recovery using `hid-setup`.

## C-based TUI Keyboard
The module features a powerful **dependency-free terminal keyboard** (`hid-tui`).

To launch it:
```bash
hid-tui
```
or in portable mode:
```bash
./portable/hid-tui
```

**Features:**
- Laptop-style 60-75% layout.
- Mouse support (clickable keys).
- Physical keyboard passthrough.
- Sticky modifiers (CTRL, ALT, SHIFT).
- Smart WIN key logic (Double-press for standalone WIN).

---

## Supported Keys

### Modifiers
| Key | Aliases |
| :--- | :--- |
| **CTRL** | `CONTROL` |
| **SHIFT** | - |
| **ALT** | - |
| **GUI** | `WIN`, `META`, `SUPER` |
| **RGUI** | `RWIN`, `RMETA`, `RSUPER` |

### Special & Function Keys
`F1` through `F12`, `INSERT`, `HOME`, `PAGEUP`, `DELETE`, `END`, `PAGEDOWN`, `RIGHT`, `LEFT`, `DOWN`, `UP`, `NUMLOCK`, `ESC`, `TAB`, `CAPSLOCK`, `PRINTSCREEN`, `SCROLLLOCK`, `PAUSE`, `BACKSPACE`, `RETURN`, `ENTER`, `SPACE`.

### Consumer / Media Keys
`PLAY`, `PAUSE`, `RECORD`, `FORWARD`, `REWIND`, `NEXT`, `PREVIOUS`, `STOP`, `EJECT`, `MUTE`, `VOL+`, `VOL-`, `BRIGHTNESS+`, `BRIGHTNESS-`.

---

## Build Instructions (Developers)

The module uses **Zig CC** for cross-compilation.

### Prerequisites
- [Zig](https://ziglang.org/download/) (v0.11.0 or newer)
- `make`
- `zip`

### Building the Module
1. **Compile Static Binaries**:
   ```bash
   make static
   ```
2. **Repack Magisk ZIP**:
   ```bash
   zip -r hid-gadget-module.zip . -x ".*" "build/*" "portable/*"
   ```

---

## Usage

### 1. Flashable Magisk Module
Install `hid-gadget-module.zip` via the Magisk app and reboot.

### 2. Portable Mode (No Install)
Run the architecture-aware scripts directly from the `portable/` directory:
```bash
./portable/hid-keyboard CTRL-C
./portable/hid-mouse move 10 -5
./portable/hid-consumer VOL+
./portable/hid-tui
```

---

## License
MIT License - see [LICENSE](LICENSE) for details.
