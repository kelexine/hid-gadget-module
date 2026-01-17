# HID Gadget Module for Magisk (v1.23.6)

This Magisk module provides USB HID (Human Interface Device) gadget functionality for Android devices. It allows your device to act as a USB keyboard, mouse, or consumer control device.

> [!IMPORTANT]
> This version (v1.23.3) features **Universal Static Binaries** compiled with `musl`, ensuring compatibility across all Android versions (Bionic) and Linux distributions (glibc) without any external dependencies.

## Key Features

- **Robust Modifier Parsing**: Correctly handles single modifiers (CTRL, ALT, etc.) and composite strings like `CTRL-C`.
- **Universal Static Linking**: Compiled with Zig CC and `musl` for zero-dependency execution.
- **Standalone Portable Mode**: Test and run HID logic on any system without installing the module.
- **Architecture Aware**: Automatic selection of ARM64, ARM, x86_64, or x86 binaries.

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

The module uses **Zig CC** for cross-compilation to provide statically linked `musl` binaries. This is the recommended way to build to ensure the binary runs on the Android environment.

### Prerequisites
- [Zig](https://ziglang.org/download/) (v0.11.0 or newer)
- `make`
- `zip`

### Building the Module
1. **Compile Static Binaries**:
   ```bash
   make static
   ```
   *This uses `zig cc --target=aarch64-linux-musl -static` to target different architectures.*

2. **Repack Magisk ZIP**:
   ```bash
   # Build the ARM64 production binary
   zig cc --target=aarch64-linux-gnu -O2 -o blobs/arm64/hid-gadget hid-gadget.c
   # ... repeat for other architectures into blobs/ ...
   # Pack the ZIP (ensure system/ and META-INF/ are included)
   zip -r hid-gadget-module.zip . -x ".*" "build/*" "portable/*"
   ```

3. **Cleanup**:
   ```bash
   make clean
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
```

---

## License
MIT License - see [LICENSE](LICENSE) for details.
