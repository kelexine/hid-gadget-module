# 📱 USB HID Gadget Module & Terminal Controller

![Version](https://img.shields.io/badge/version-v1.35.6-blue?style=for-the-badge&logo=android)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-Android%20%7C%20Linux-orange?style=for-the-badge)
![Architecture](https://img.shields.io/badge/arch-ARM64%20%7C%20ARM%20%7C%20x86__64%20%7C%20x86-purple?style=for-the-badge)

**Turn your Android device into a professional USB Human Interface Device.**

This Magisk module enables your rooted Android device to function as a **Keyboard**, **Mouse**, and **Media Remote** for any connected computer. It features a powerful, dependency-free **Text-Based User Interface (TUI)** that runs right in your terminal.

---

## ✨ Key Features

### 🚀 **Universal & Robust**
- **Zero Dependencies**: Built with `musl` libc and statically linked. No external libraries required.
- **Cross-Platform**: Runs on **any** Android version (Magisk) and standard Linux distros.
- **Auto-Recovery**: Intelligently detects HID failures and re-initializes the gadget driver automatically.

### 🖥️ **HID TUI (Terminal Interface)**
The star of the show (`hid-tui`). A fully-featured GUI running inside your terminal:
- **⌨️ Laptop-Style Keyboard**: Full 75% layout with clickable keys.
- **🖱️ "Radar" Analog Mouse**: Virtual analog stick with aspect-ratio corrected movement and "Velocity Tap" precision.
- **🤏 Drag & Drop**: Dedicated `[HL]`/`[HM]`/`[HR]` buttons to latch clicks for dragging.
- **🎮 Media Deck**: Full 14-key remote control (`PLAY`, `VOL`, `MUTE`, `BRIGHTNESS`...).
- **🌈 Visual Feedback**: Keys flash and buttons light up when pressed.

### 🛠️ **Power User Tools**
- **Scriptable**: CLI tools (`hid-keyboard`, `hid-mouse`) for automation scripting.
- **Sticky Modifiers**: Smart handling of `CTRL`, `ALT`, `SHIFT`, `WIN` for complex shortcuts.

---

## 📥 Installation

1.  Download the latest `hid-gadget-module-vX.Y.Z.zip`.
2.  Open **Magisk** > **Modules** > **Install from Storage**.
3.  Select the zip file and reboot.

---

## 🎮 Usage Guide

### 1. The TUI Controller (`hid-tui`)
Launch it from a terminal (Termux or adb shell):
```bash
su -c hid-tui
```

#### **Interface Layout**
- **Top Bar**: Media Controls (`PLAY`, `MUTE`, `VOL+`, etc.). Tap to control playback.
- **Middle**: The **Radar Zone** `[+]`.
    - Tap **Center** (`+`) to Left Click.
    - Tap **Around Center** to move the mouse. Distance = Speed.
- **Controls**:
    - `[ L ]` `[ M ]` `[ R ]`: Standard clicks.
    - `[ HL ]` `[ HM ]` `[ HR ]`: **Hold** toggles. Tap once to "lock" the button down (turns Magenta). Tap again to release. Ideal for dragging windows!
    - `SENS`: Adjust mouse sensitivity.
- **Bottom**: Full QWERTY keyboard.

### 2. Command Line Interface
Automate key presses and mouse movements from scripts.

**Keyboard**:
```bash
hid-keyboard "Hello World"          # Type text
hid-keyboard --hold "CTRL-ALT-DEL"  # Send combo
hid-keyboard --release "CTRL-ALT-DEL"
```

**Mouse**:
```bash
hid-mouse move 100 -50              # Move X=100, Y=-50
hid-mouse click left                # Click left button
hid-mouse press right               # Hold right button
```

**Consumer (Media)**:
```bash
hid-consumer VOL+                   # Volume Up
hid-consumer PLAY                   # Play/Pause
hid-consumer BRIGHTNESS+            # Screen Brightness
```

---

## 🏗️ Building From Source

We provide an automated build script (`build_release.sh`) that handles versioning, compilation, and packaging.

**Prerequisites**: `zig` (v0.11+), `make`, `zip`.

```bash
# Clone the repo
git clone https://github.com/kelexine/hid-gadget-module
cd hid-gadget-module

# Build artifacts and create zip
./build_release.sh auto
```
This will create `hid-gadget-module-vX.Y.Z.zip` for all 4 architectures (`arm64`, `arm`, `x86_64`, `x86`).

---

## 👥 Credits & Authors

This project is a collaborative effort to bring the best HID experience to Android.

- **[kelexine](https://github.com/kelexine)**: Original Author & Core Architecture.
- **[rexackermann](https://github.com/rexackermann)**: Contributor, TUI Rewrite (v1.30+), & Universal Binary Port.

**License**: [MIT](LICENSE)
