# 📱 USB HID Gadget Module & Terminal Controller

<div align="center">

![Version](https://img.shields.io/badge/version-v1.38.1-blue?style=for-the-badge&logo=android)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-Android%20%7C%20Linux-orange?style=for-the-badge)
![Architecture](https://img.shields.io/badge/arch-ARM64%20%7C%20ARM%20%7C%20x86__64%20%7C%20x86-purple?style=for-the-badge)
![DuckyScript](https://img.shields.io/badge/DuckyScript-3.0%20Compliant-yellow?style=for-the-badge&logo=badusb)

**Turn your Android device into a professional USB Human Interface Device.**

This Magisk module enables your rooted Android device to function as a **Keyboard**, **Mouse**, and **Media Remote** for any connected computer. It features a powerful, dependency-free **Text-Based User Interface (TUI)** that runs right in your terminal.

</div>

---

## ✨ Key Features

### � **Universal & Robust**
- **Zero Dependencies**: Built with `musl` libc and statically linked. No external libraries required.
- **Cross-Platform**: Runs on **any** Android version (Magisk/KSU) and standard Linux distros.
- **Auto-Recovery**: Intelligently detects HID failures and re-initializes the gadget driver automatically via `su`.

### 🖥️ **HID TUI (Terminal Graphical Interface)**
The star of the show (`hid-tui`). A fully-featured GUI running inside your terminal:
- **⌨️ Laptop-Style Keyboard**: Full 75% layout with clickable keys and visual press feedback.
- **🖱️ "Radar" Analog Mouse**: Virtual analog stick with aspect-ratio corrected movement and "Velocity Tap" precision. Magnitude = Speed.
- **🤏 Drag & Drop**: Dedicated `[HL]`, `[HM]`, `[HR]` buttons to latch clicks for dragging. (Turns Magenta when locked).
- **🎮 Media Deck**: Full 14-key remote control (`PLAY`, `VOL`, `MUTE`, `BRIGHTNESS`...).
- **🌈 Visual Feedback**: Keys flash and buttons light up when pressed.

### 🛠️ **Power User Tools**
- **Shorthand CLI**: Convenience wrappers (`hid-keyboard`, `hid-mouse`) for automation scripting.
- **Sticky Modifiers**: Smart handling of `CTRL`, `ALT`, `SHIFT`, `WIN` for complex shortcuts.
- **DuckyScript 3.0**: Fully compliant automation engine for sophisticated HID injection payloads.

---

## 📥 Installation (Android)

1.  Download the latest `hid-gadget-module-v1.38.1.zip`.
2.  Open **Magisk** or **KernelSU** > **Modules** > **Install from Storage**.
3.  Select the zip file and reboot.

---

## 🎮 Usage Guide

### 1. The TUI Controller (`hid-tui`)
Launch it from any root terminal (Termux or adb shell):
```bash
su -c hid-tui
```

#### **Interface Layout**
- **Top Bar**: Media Controls (`PLAY`, `MUTE`, `VOL+`, etc.). Tap to control playback.
- **Middle**: The **Radar Zone** `[+]`.
    - Tap **Center** (`+`) or perform a quick tap for a **Left Click**.
    - **Move**: Drag from center. Distance from center determines move speed (Velocity-Sensitive).
- **Controls**:
    - `[ L ]` `[ M ]` `[ R ]`: Standard clicks.
    - `[ HL ]` `[ HM ]` `[ HR ]`: **Hold** toggles. Tap once to "lock" the button down. Ideal for dragging windows!
    - `SENS`: Adjust mouse sensitivity (1x to 20x).
- **Bottom**: Full QWERTY keyboard with sticky modifiers.

### 2. Initial Setup (Manual)
If the gadget doesn't auto-start, run the setup script:
```bash
su -c hid-setup
```

### 3. Command Line Interface (Automation)
Automate key presses and mouse movements from scripts.

**Keyboard**:
```bash
hid-keyboard "Hello World"          # Type text
hid-keyboard CTRL-ALT-DEL           # Send combo
hid-keyboard --hold SHIFT "hello"   # Type with held modifier
hid-keyboard --release              # Reset all states
```

**Mouse**:
```bash
hid-mouse move 100 -50              # Move X=100, Y=-50
hid-mouse click left                # Click left button
hid-mouse down right                # Latch right button
```

**Consumer (Media)**:
```bash
hid-consumer VOL+                   # Volume Up
hid-consumer PLAY                   # Play/Pause
hid-consumer BRIGHTNESS+            # Screen Brightness
```

---

## ⚠️ Known Limitations

### 🦆 **DuckyScript 3.0**
While this module implements the **100% Core Specification**, there are some platform-specific limitations:
- **No `STORAGE` Command**: Unlike a physical USB Rubber Ducky, this module cannot mount a local SD card as a Mass Storage device via DuckyScript. You must use Android's native MTP/Storage features.
- **`HID_SYNC` / `WAIT_FOR_BUTTON`**: These commands are currently stubs. Synchronizing with target LED states (e.g., waiting for CapsLock) depends heavily on the Android UDC driver support, which is inconsistent across devices.
- **Extensions**: Custom Hak5 vendor extensions (non-core) are not supported.

### 📱 **Hardware & Kernel**
- **UDC Drivers**: Not all Android kernels support ConfigFS HID gadgets. If `hid-setup` fails, your kernel may lack `CONFIG_USB_CONFIGFS_F_HID`.
- **USB Conflicts**: Enabling the HID gadget may temporarily disconnect other USB functions like ADB or MTP on some devices, depending on how your specific kernel handles USB compositions.
- **OTG Requirement**: You must use a high-quality USB-C to USB-A (or equivalent) cable. Some "charging-only" cables lack the necessary data lines for HID communication.

---

## 🏗️ Building From Source

We provide an automated build script that handles versioning, cross-compilation, and packaging.

**Prerequisites**: `zig` (v0.11+), `make`, `zip`.

```bash
# Clone the repo
git clone https://github.com/kelexine/hid-gadget-module
cd hid-gadget-module

# Build artifacts and create zip
./scripts/build_release.sh auto
```
This will create `hid-gadget-module-v1.38.1.zip` for all 4 architectures (`arm64`, `arm`, `x86_64`, `x86`).

---

## 👥 Authors
This project is a collaborative effort to bring the best HID experience to Android.

- **[kelexine](https://github.com/kelexine)**: Original Author & Core Architecture.
- **[rexackermann](https://github.com/rexackermann)**: Contributor, TUI Rewrite (v1.30+), & DuckyScript 3.0 Implementation.

**License**: [MIT](LICENSE)
