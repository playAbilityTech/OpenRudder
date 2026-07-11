# MoveStick — Open-Source Motion Controller

_This project was previously named **OpenRudder**._

**MoveStick** turns any movement into game controls.

Attach it to anything — your hand, a cap, a balance board, a wheelchair joystick — and tilt to control a **virtual joystick**, **mouse**, or **keyboard/gamepad** buttons. No drivers, no proprietary hardware.

Built on the excellent [**HID Remapper by jfedor**](https://github.com/jfedor2/hid-remapper), with custom gyroscope support for the **Seeed XIAO BLE Sense**.

A collaboration between [**PlayAbility Adaptive Software**](https://www.playability.gg/) and [**HitClic**](https://hitclic.shop/).

> 💡 **Don't want to build it yourself?** [Buy a ready-made MoveStick from HitClic](https://hitclic.shop/en/products/joystick-movestick-a-detection-de-mouvement)

---

## ✨ What You Can Do

| Attach to... | Control with... | Use case |
|--------------|-----------------|----------|
| **Hand** (velcro strap) | Hand tilt | Joystick replacement |
| **Cap / Headband** | Head movement | Hands-free aiming/steering |
| **Balance board** | Feet tilt | Racing games, fitness |
| **Wheelchair joystick** | Existing movements | Smartphone/PC control |
| **Any surface** | Your imagination | Adaptive gaming setups |

### Features
- 🎮 **Virtual joystick** — map Roll/Pitch to any gamepad stick
- 🖱️ **Motion mouse** — tilt to move cursor, tap/shake to click  
- ⌨️ **Keyboard/buttons** — trigger A/B/X/Y, space, WASD from tilt angles
- 🔗 **Bluetooth hub** — connect multiple BT devices and merge into one controller
- ♿ **Adaptive ready** — works with Xbox Adaptive Controller, Hori Flex, PS Access

---

## 🔗 Quick Links

| Resource | Link |
|----------|------|
| **Firmware (.uf2)** | [Download latest release](https://github.com/playAbilityTech/MoveStick/releases) |
| **Web Configuration** | [playabilitytech.github.io/MoveStick](https://playabilitytech.github.io/MoveStick/) |
| **Buy ready-made** | [HitClic Shop](https://hitclic.shop/en/products/joystick-movestick-a-detection-de-mouvement) |

> Works on **Windows**, **macOS**, and **Linux** — enumerates as standard USB HID.

---

## 🛠️ Hardware (DIY)

Build your own MoveStick for under $25:

| Part | Price | Link |
|------|-------|------|
| **Seeed XIAO BLE Sense** (nRF52840 + IMU) | ~$15-20 | [Seeed Studio](https://www.seeedstudio.com/Seeed-XIAO-BLE-Sense-nRF52840-p-5253.html) / [Gotronic](https://www.gotronic.fr/art-carte-xiao-ble-nrf52840-sense-34719.htm) |
| **USB-C cable** | ~$5 | Any USB-C data cable |

**Optional accessories:**
- Balance board → [Amazon](https://amzn.to/47KK38L)
- Velcro strap (for hand/cap mounting)
- 3D printed case → see `/enclosure` folder

The procedure to update the firmware is similar on all variants. When you go to the configuration website and try to connect to your device when it doesn't have the latest firmware, you will get a message and a link to a version of the configuration interface that is compatible with your current (old) firmware. Click that link, connect to your HID Remapper by clicking "Open device" as usual, then go to the "Actions" tab and click "Flash firmware". This will put your device in firmware flashing mode. A drive should appear on your computer. For all the RP2040-based variants, the drive will be named "RPI-RP2". For the Bluetooth variants, it will be called something else, depending on what board you're using. Download the correct firmware file for your variant (see table below) and copy it to that drive. On custom boards v1, v2, v5, v6 and v7 (dual RP2040 boards), after flashing the firmware you have to disconnect and reconnect your HID Remapper. That's it, you can go back to the regular version of the configuration interface and carry on.

_(Please note that previously a manual "Flash B side" step was required on custom boards v1, v2, v5, v6 and v7. That is no longer necessary.)_

## ⚡ Flash the Firmware

1. **Enter bootloader mode**  
   - Hold **BOOT** + tap **RESET** (or double-tap RESET quickly)
   - A USB drive named `XIAO-SENSE` appears

variant | firmware file(s) | notes
------- | ---------------- | -----------------------
single Pico | [remapper.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper.uf2) |
dual Pico | [remapper\_dual\_a.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_dual_a.uf2)<br>[remapper\_dual\_b.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_dual_b.uf2) | each Pico needs to be flashed separately
Feather RP2040 with USB Host | [remapper\_feather.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_feather.uf2) |
custom board v1 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v2 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v3 | [remapper\_feather.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_feather.uf2) |
custom board v4 | [remapper\_feather.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_feather.uf2) |
custom board v5 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v6 | [remapper\_board.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board.uf2) | disconnect and reconnect after flashing
custom board v7 | [remapper\_board\_v7.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board_v7.uf2) | disconnect and reconnect after flashing
custom board v8 | [remapper\_board\_v8.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_board_v8.uf2) |
Feather nRF52840 Express | [remapper_adafruit_feather_nrf52840.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_adafruit_feather_nrf52840.uf2) |
Xiao nRF52840 | [remapper_seeed_xiao_nrf52840.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_seeed_xiao_nrf52840.uf2) |
Xiao nRF52840 Sense | [remapper_seeed_xiao_nrf52840_sense.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_seeed_xiao_nrf52840_sense.uf2) |
serial | [remapper_serial.uf2](https://github.com/jfedor2/hid-remapper/releases/latest/download/remapper_serial.uf2) |

For boards not listed above, use the same file name you used when flashing it for the first time.

3. **Done!** The board reboots as a USB HID device

> 💡 If the drive doesn't appear, try a different USB cable (must support data, not charge-only)

---

## ⚙️ Configuration

Open the **Web Config Tool**: [playabilitytech.github.io/MoveStick](https://playabilitytech.github.io/MoveStick/)

### Basic Setup

1. **Connect** your MoveStick via USB
2. Click **"Open Device"** in the web tool
3. Go to **Settings → IMU support → ON**
4. Load an example profile or create your own mapping

### Tuning Options

| Setting | What it does |
|---------|--------------|
| **Angle limit** | Max tilt angle before output saturates |
| **Buffer size** | Smoothing (higher = smoother but more latency) |
| **Axis inversion** | Flip X/Y directions |
| **Shake threshold** | Sensitivity for tap/shake detection |

### Mapping

Map motion axes to any output:
- **Roll (X)** → mouse X, left stick X, arrow keys...
- **Pitch (Y)** → mouse Y, left stick Y, WASD...
- **Shake** → click, button A, space, etc.

---

## 📦 Ready-Made Profiles

### Mouse Control
```
Roll → Cursor X
Pitch → Cursor Y  
Shake → Left click
```

### Gamepad (XInput/Switch)
```
Roll → Left stick X
Pitch → Left stick Y
Shake → Button A
```

### Racing (Steering only)
```
Roll → Left stick X (steering)
```

Load these from **Examples** in the web config tool.

---

## 📡 Bluetooth Mode

MoveStick can also act as a **Bluetooth hub**, connecting multiple devices wirelessly and merging all inputs into one controller.

Supported devices:
- DualSense / Xbox controllers
- Xbox Adaptive Controller
- PS Access controller
- Bluetooth keyboards/mice
- BLE wheelchair joysticks

See [BLUETOOTH.md](./BLUETOOTH.md) for setup instructions.

---

## 🤝 Credits

- **[HID Remapper](https://github.com/jfedor2/hid-remapper)** by jfedor — the foundation this project builds on
- **[PlayAbility](https://www.playability.gg/)** — gyroscope/IMU features and integration
- **[HitClic](https://hitclic.shop/)** — hardware design and distribution

---

## 📄 License

MIT License — see [LICENSE](./LICENSE)

---

## 🔗 Related Projects

- [PlayAbility Adaptive Software](https://www.playability.gg/) — Face/voice/head control for gaming
- [HitClic Handigamer](https://hitclic.shop/) — Adaptive gaming hardware
- [Xbox Adaptive Controller](https://www.xbox.com/accessories/controllers/xbox-adaptive-controller) — Microsoft's adaptive gaming platform

---

**Questions?** Open an issue or reach out on [Discord](https://discord.playability.gg)
