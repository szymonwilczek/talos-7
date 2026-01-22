# ⚡ Talos 7
### The Ultimate Open-Source SudoPad 

<div align="center">
  <img src="web-interface/public/talos_logo.png" alt="Talos 7 Logo" width="180"/>
  <br/>
  
  [![Hephaestus Forge](https://github.com/szymonwilczek/talos-7/actions/workflows/bump-firmware.yml/badge.svg)](https://github.com/szymonwilczek/talos-7/actions)
  [![Firmware](https://img.shields.io/github/v/release/szymonwilczek/talos-7?label=firmware&color=00599C)](https://github.com/szymonwilczek/talos-7/releases)
  ![License](https://img.shields.io/badge/license-MIT-green)
  ![Platform](https://img.shields.io/badge/platform-RP2040-C51A4A)

  <p>
    <b>Professional. Driverless. Infinite Possibilities.</b><br/>
    Designed for Streamers, Developers, and Creatives. <br/>
    Forged to automate.
  </p>
</div>

---

## 📸 Preview

<table>
  <tr>
    <td width="50%" align="center">
      <img src="assets/talos_front.JPEG" alt="Talos Front" width="100%">
    </td>
    <td width="50%" align="center">
      <img src="assets/talos_atari.JPEG" alt="Talos Back" width="100%">
    </td>
  </tr>
  <tr>
    <td colspan="2" align="center">
      <img src="assets/talos_back.JPEG" alt="Atari Game" width="100%">
    </td>
  </tr>
</table>

<div align="center">
  <video src="https://github.com/user-attachments/assets/fb88e3e2-818f-48c2-9b6d-56c365594454" controls></video>
</div>

---

## 📖 Overview

**Talos 7** is not just a keypad; it's a hardware productivity platform. I've name this functionality as SudoPad. Unlike traditional macro pads that rely on heavy background software (like Synapse or iCUE), **Talos 7 runs everything on the device itself**.

Once configured via the web interface, you can plug it into **any computer** (Windows, macOS, Linux), and it will work instantly. No drivers. No background agents. Just pure hardware emulation.

### Why Talos?
* **Hybrid Device:** Simultaneously acts as a Keyboard, Mouse, MIDI Controller, and Serial Device.
* **OLED Dashboard:** A 128x64 pixel display gives you real-time feedback on layers and actions.
* **Web Configurator:** Modify your layout directly in Chrome/Edge using Web Serial API.
* **Hephaestus CI/CD:** Automated cloud builds ensure you always have the latest firmware.

---

## 🛠️ Hardware Architecture

Talos 7 is built on the accessible and powerful **Raspberry Pi Pico (RP2040)**.

### Bill of Materials (BOM)
* 1x **Raspberry Pi Pico**
* 7x **Mechanical Switches** (MX standard)
* 7x **Keycaps** (MX standard)
* 1x **SSD1306 OLED Display** (0.96", SPI Interface)
* 7x **LEDs** (0805, red)
* 10x **Resistors** (220R or 47R if you prefer dimmer light)
* 1x **RGB LED** (LTST-G563ZEGBW or FYLS-5050NRGBC)
* 1x **Micro-USB Data Cable**

### Wiring Diagram (Pinout)

See [Talos Schema](assets/board/talos_schema.pdf) for more information.

## 🚀 Setup & Installation

### 1. Flash the Firmware
You don't need to compile anything manually.
1.  Go to [**Releases**](https://github.com/szymonwilczek/talos-7/releases).
2.  Download the latest `talos7.uf2`.
3.  Hold the **BOOTSEL** button on your Pico while plugging it into USB.
4.  A drive named `RPI-RP2` will appear. Drag and drop the `.uf2` file there.
5.  The device will reboot. You should see the Talos Logo on the OLED.

### 2. Configure via Web
1.  Open the **[Talos Configurator](https://talos-7.vercel.app)** in a Chromium-based browser (Chrome, Edge, Opera).
2.  Click **Connect Device** and select *Talos 7*.
3.  **Customize** your layers and buttons.
4.  Click **Save Changes** to flash your config to the onboard memory.

---

## 🎛️ Macro Dictionary

Talos 7 supports **8 distinct action types** (for now, if you have something in mind, be welcome to propose). Here is a deep dive into each capabilities.

### 1. ⌨️ Key Press
Simulates a standard keyboard press.
* **Parameters:** Keycode (A-Z, F1-F12, etc.).
* **Use Case:** Muting Discord (`F13`), Screenshot (`PrintScreen`), Game abilities (`Q`, `W`, `E`).

### 2. ⛓️ Key Sequence
Executes a precise combination of up to 3 keys pressed simultaneously or sequentially.
* **Parameters:** List of keys + Modifiers (Ctrl, Alt, Shift, GUI).
* **Use Case:** `Ctrl` + `Shift` + `Esc` (Task Manager), `Win` + `Shift` + `S` (Snipping Tool).

### 3. 🔡 Text String
Types a text snippet automatically.
* **Parameters:** Text content (up to 32 chars).
* **Behavior:** Works like a super-fast typist. Supports basic Unicode.
* **Use Case:** Email signatures, Git commands (`git push origin main`), etc.

### 4. 🔄 Layer Toggle
Switches the active layer of the device.
* **Parameters:** Target Layer (Automatic Cycle).
* **Behavior:** Pressing this creates a loop: `Layer 1` → `Layer 2` → `Layer 3` → `Layer 4` → `Layer 1`.
* **Visuals:** The OLED instantly updates to show the new layer's name and icon.

### 5. 🖱️ Mouse Control
Takes hardware control of the mouse cursor.
* **Sub-types:**
    * **Move:** X/Y coordinates (relative). Great for AFK scripts.
    * **Click:** Left, Right, or Middle button (with Hold support).
    * **Wheel:** Scroll Up/Down.
* **Use Case:** "Cookie Clicker" macros, in-game AFK preventions, preventing screen lock.

### 6. 📜 Script Engine (Power User)
The most powerful feature. Talos acts as a "BadUSB" device to inject and execute complex scripts on the host machine.
* **Platform Awareness:** You can define separate scripts for Windows, Linux, and macOS on the same button.
* **Workflow:**
    1.  Talos opens a terminal using a shortcut (e.g., `Win+R` or `Ctrl+Alt+T` - those are just default, you can provide your own sequence if you have it).
    2.  It types a temporary script file to `/tmp` or `%TEMP%`.
    3.  It executes the file and deletes it immediately.
* **Use Case:** `docker-compose up -d`, SSH into a server, batch rename files, organize desktop.

### 7. 🎹 MIDI Note (Studio Mode)
Sends a MIDI Note On/Off signal via USB MIDI class.
* **Parameters:** Note (0-127), Velocity (0-127), Channel (1-16).
* **Why MIDI?** Unlike keyboard shortcuts, MIDI messages **never conflict** with typing and work even when the target app is in the background.
* **Integration:**
    * **OBS:** Use `obs-midi` plugin to switch scenes or toggle sources.
    * **DAWs:** Map to drum pads in FL Studio / Ableton.

### 8. 🎚️ MIDI Control Change (CC)
Sends a generic MIDI value (Potentiometer/Fader simulation).
* **Parameters:** CC Number (0-119), Value (0-127), Channel (1-16).
* **Use Case:**
    * **Lightroom:** Set Exposure to +1.0.
    * **Premiere Pro:** Set Timeline Zoom level.
    * **Windows Volume:** Use middleware (e.g., MidiKey2Key) to map CC to system volume.

### 9. 🕹️ Atari Breakout Game
A pleasant *Break* during work

---

## 🤖 DevOps: Hephaestus

This repository is managed by an automated github workflow named **Hephaestus**.

### Automated Versioning
I use **Semantic Versioning**. Hephaestus parses every commit message:
* `fix: ...` ➜ Bumps **Patch** (1.0.0 ➜ 1.0.1)
* `feat: ...` ➜ Bumps **Minor** (1.0.1 ➜ 1.1.0)
* `feat!: ...` ➜ Bumps **Major** (1.1.0 ➜ 2.0.0)

### Continuous Delivery
On every valid push to `main`:
1.  Hephaestus spins up a cloud environment with the **ARM GCC Toolchain**.
2.  It checks out the official **Pico SDK**.
3.  It compiles the firmware (`talos7.uf2`).
4.  It creates a **GitHub Release** with the new version and changelog.

---

## 🔧 Troubleshooting

### Device not connecting in browser?
* **Windows:** Ensure you are not blocking USB devices via Group Policy. No drivers needed.
* **Linux:** If you are unable to connect to the device via the web configurator on Linux (receiving `NetworkError: Failed to execute open on SerialPort`), you need to configure `udev` rules to allow non-root access to the Raspberry Pi Pico.

1. **Create a new udev rule file:**
   ```bash
   sudo nano /etc/udev/rules.d/99-talos.rules
   ```

2. **Add the following content**: This grants r/w permission to the device (Vendor ID 0x2e8a is RPi Pico):

```text
SUBSYSTEMS=="usb", ATTRS{idVendor}=="2e8a", MODE="0666"
KERNEL=="ttyACM*", ATTRS{idVendor}=="2e8a", MODE="0666"
```

3. Reload the rules and reconnect:

```bash
sudo udevadm control --reload-rules # reload
sudo udevadm trigger # apply changes
```

**Note for Fedora/RHEL users**: If connection issues persist, ModemManager might be interfering with the serial port. You can temporarily stop it with: `sudo systemctl stop ModemManager`



* **Browser:** Use Chrome, Edge, or Opera. Firefox/Safari do not support Web Serial yet.

### Firmware update failed?
If you "bricked" the configuration or the device is unresponsive:
1.  Unplug the device.
2.  Hold the physical **BOOTSEL** button on the Pico.
3.  Plug it in.
4.  Flash the `.uf2` again. This factory resets the core logic.

---

## 📄 License & Credits

* **Core Logic:** Szymon Wilczek
* **3D Case**: Kacper Kil, thanks a lot again!
* **Project Link:** [github.com/szymonwilczek/talos-7](https://github.com/szymonwilczek/talos-7)
* **License:** [MIT License](./LICENSE.md)

Thanks for interest in my project!

Built with ❤️ using **Raspberry Pi Pico SDK** and **TinyUSB**.
