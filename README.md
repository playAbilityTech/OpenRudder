# OpenRudder — Open-source Balance Board that Shows Up as a Joystick (or a Mouse, or Keys)

**OpenRudder** turns a simple balance board into a USB **motion controller**.  
Tilt with your feet to steer a **virtual joystick**, move the **mouse**, or press **keyboard/gamepad** buttons—no drivers, no proprietary boxes.

Built on the excellent **HID Remapper** project, with gyroscope support for the **Seeed XIAO BLE Sense**.  
A collaboration between **[PlayAbility](https://www.playability.gg/)** and **HitClic.shop**.

---

## Quick Links

- **Latest firmware (.uf2)** → [Download](https://github.com/squirelo/hid-remapper/actions/runs/16246750706/artifacts/3520810322)  
- **Web configuration** → <https://squirelo.github.io/hid-remapper/>

> Enumerates as standard **USB HID** (mouse, keyboard, gamepad) on Windows, macOS, and Linux.

---

## What You Can Do

- **Feet-controlled joystick** — map **Roll**/**Pitch** to a gamepad stick (e.g., left stick X/Y)  
- **Motion mouse** — tilt to move the pointer, **tap** to click (shake threshold)  
- **Hands-free buttons** — use tilt ranges or shakes to trigger **A/B/X/Y**, **L/R**, **space**, **clicks**, etc.  
- **Adaptive setups** — pair with the Xbox Adaptive Controller, wheelchair joysticks, or other assistive gear

---

## How It Works (1-Minute Version)

1. A **Seeed XIAO BLE Sense** reads **roll/pitch** (and detects **shake**) via its IMU.  
2. **HID Remapper** firmware exposes those signals as **HID inputs** (mouse, keyboard, or gamepad).  
3. You tailor mappings in the **Web Config** and tune **angle limit**, **smoothing**, and **inversion**.

---

## Hardware

Build OpenRudder with common parts:

- **Microcontroller**: Seeed **XIAO BLE Sense** (nRF52840, with IMU)  
- **Deck**: Small **wobble/balance board** or a **DIY footplate** (plywood/3D print)  
- **Pivot**: Half-sphere, foam dome, or printed rounded puck under the center  
- **Mounting**: Secure the XIAO near the **geometric center**; USB-C with strain relief  
- **Optional**: Rubber feet, springs/foam for return-to-center, small enclosure for the XIAO

> Tip: Place the IMU close to the **rotation center** for the smoothest feel.

---

## Flash the Firmware (.uf2)

1. **Enter bootloader** on the XIAO BLE Sense  
   Hold **BOOT**, tap **RESET** (or double-tap **RESET**). A USB drive appears.  
2. **Drag & drop** the downloaded `.uf2` onto that drive.  
3. The board reboots and enumerates as a **USB HID** device.

If the drive doesn’t appear, repeat the BOOT/RESET step or try another cable/port.

---

## Configure & Map

Open the **Web Config** → <https://squirelo.github.io/hid-remapper/>

### Enable IMU
- **Settings → IMU support** → toggle **ON**

### Tune Feel
- **Angle limit** — caps max tilt so you don’t saturate a stick axis  
- **Buffer size (smoothing)** — more = smoother, but slightly higher latency  
- **Axis inversion** — flip X/Y without editing expressions  
- **Shake threshold** — adjust tap sensitivity for clicks/buttons

### Map Signals → Outputs
- **Roll (X)** / **Pitch (Y)** → **mouse X/Y**, **left stick X/Y**, or **keys**  
- **Shake** → **left click**, **A**, **space**, etc.

Screenshots:

<img width="1472" height="387" alt="IMU Inputs" src="https://github.com/user-attachments/assets/db1512d2-5e78-44f0-89c4-ee5db99b7a04" />
<img width="1919" height="678" alt="IMU Settings" src="https://github.com/user-attachments/assets/dbcfec3e-d6dc-47d2-bd06-bd1fb74eba64" />

---

## Ready-Made Profiles

- **IMU → Mouse**  
  - Roll → Cursor **X**  
  - Pitch → Cursor **Y**  
  - Shake → **Left click**

- **IMU → Gamepad (Switch/XInput-style)**  
  - Roll → **Left stick X**  
  - Pitch → **Left stick Y**  
  - Shake → **Button A**

> Use these to sanity-check your build, then customize.

---

## DIY Build Guide (Short)

1. **Make the deck**: 25–35 cm square/round plate (wood or 3D print).  
2. **Add the pivot**: Centered dome/hemisphere underneath (2–5 cm height works well).  
3. **Mount the XIAO**: On the top, near the geometric center. Align its **X/Y** with deck forward/right.  
4. **Cable & relief**: Route USB-C so it won’t snag underfoot.  
5. **First tests**: Start **seated**; heels down, balls of feet on the board.

> Non-slip surfaces help. Early tests on a mat/carpet are recommended.

---

## Ergonomics & Safety

- **Zero/neutral**: Let the board settle before use.  
- **Start small**: Low **angle limit**, moderate **smoothing**; increase gradually.  
- **Invert if needed**: Many prefer forward tilt = **up**.  
- **Shake**: If accidental clicks happen, raise the threshold or disable during tuning.  
- **Take breaks**: Foot/ankle muscles fatigue—rest often.  
- **Secure area**: Keep the space clear; consider seated use while dialing settings.

---

## Troubleshooting

- **No motion** → Enable **IMU support**; reflash `.uf2`; reconnect USB.  
- **Too twitchy** → Increase **Buffer size** and/or reduce **Angle limit**.  
- **Not reaching full in-game range** → Raise **Angle limit** or increase in-game sensitivity.  
- **Wrong direction** → Use **Axis inversion**.  
- **Random presses** → Increase **Shake threshold** or disable shake mapping while tuning.

---

## Compatibility

- Standard **USB HID** (mouse, keyboard, gamepad).  
- Works on **Windows / macOS / Linux**.  
- Compatible with most games that accept standard controllers or mouse/keyboard input.

---

## Roadmap

- Per-axis **deadzone**, **curve/gain**, and **calibration wizard** in UI  
- **BLE HID** mode (wireless)*  
- More **example profiles** (racing/flight, camera pan/zoom, accessibility presets)  
- Printable **mounting templates** and kits with **HitClic.shop**

\* subject to hardware/firmware support

---

## Contributing

Issues and PRs welcome!  
Please include OS, board, firmware version, and repro steps for bugs.  
For features, screenshots or short clips help a ton.

---

## Credits & License

- Built on **HID Remapper** — huge thanks to the upstream authors & community.  
- **OpenRudder** by **PlayAbility × HitClic.shop**.  
- Code & hardware are **open-source**. See [`LICENSE`](./LICENSE).  
  Upstream components retain their original licenses.
