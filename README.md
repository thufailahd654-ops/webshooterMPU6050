# 🕸️ Web Shooter – DIY Air-Mouse Spider-Man Game

Wave your hand to aim, tap to shoot a web, and catch bugs crawling across a brick wall. A wrist-mounted **ESP32-C3** with a gyro acts as a Bluetooth air mouse, and the game runs in any browser.

**▶ Play online:** ` https://thufailahd654-ops.github.io/webshooterMPU6050/`
*(The game also works with a normal mouse, trackpad or touchscreen, so anyone can try it.)*

## How it works

- An **MPU6050 / MPU6500** gyro on your hand measures how you rotate it.
- The **ESP32-C3 Super Mini** turns that into mouse movement and sends it to your computer over **Bluetooth (BLE HID)**.
- A **TTP223 touch sensor** is your trigger. Touch it and the ESP32 sends a left click.
- The game is one HTML file. It sees a normal mouse, so the cursor is your crosshair and each click shoots a web.

## Game

- Ants, beetles, spiders and rare golden bugs crawl across the wall.
- 60-second rounds that get faster over time.
- Points: ant 10, beetle 20, spider 30, golden 100.
- Combo multiplier up to x5 for consecutive hits. A miss costs 2 points and resets the combo.
- Best score is saved in your browser.

## Hardware

| Part | Notes |
|---|---|
| ESP32-C3 Super Mini | Any ESP32-C3 board with Bluetooth works |
| MPU6050 (or MPU6500 clone) | Many "MPU6050" modules are actually MPU6500. This code supports both |
| TTP223 touch sensor module | Used as the shoot trigger |
| Push button (optional) | "Clutch": freeze the cursor, like lifting a mouse |
| Small LiPo + charger module (optional) | For wireless use |

### Wiring

| From | To |
|---|---|
| MPU VCC | 3V3 |
| MPU GND | GND |
| MPU SDA | GPIO6 |
| MPU SCL | GPIO7 |
| TTP223 VCC | 3V3 |
| TTP223 GND | GND |
| TTP223 OUT | GPIO1 |
| Clutch button (optional) | GPIO2 and GND |

Leave the MPU's AD0 and INT pins unconnected. Mount the MPU flat on the back of your hand with its Z axis pointing up.

## Firmware setup

1. Install the **ESP32 board package** in Arduino IDE. **Use core version 3.2.0 or older**, since newer versions can break the BLE mouse library.
2. Install a BLE mouse library that provides `BleMouse.h` (for example the ESP32-BLE-Mouse NimBLE fork by wakwak-koba, with NimBLE-Arduino).
3. Open `firmware/webshooter_c3_raw.ino`.
4. Select the board **ESP32C3 Dev Module** and set **USB CDC On Boot: Enabled**.
5. Upload.
6. Pair **WebShooter** in your computer's Bluetooth settings.
7. After it connects, keep your hand still for a few seconds while the gyro calibrates.

No MPU library is needed. The sketch reads the gyro directly over I2C.

## Tuning

These values are at the top of the sketch:

| Setting | What it does |
|---|---|
| `SPEED` | Cursor speed. Raise for faster |
| `DEADZONE` | Ignores tiny motion. Raise it if the cursor creeps at rest |
| `SMOOTH` | Higher is smoother but laggier |
| `HIT_COOLDOWN` | Minimum milliseconds between shots |
| `TOUCH_ACTIVE_HIGH` | Set to `0` if your touch module's output is inverted |

If the cursor moves the wrong way, flip the minus signs on the `vx` and `vy` lines. If left/right and up/down are swapped, swap which gyro axis feeds each one.

**Clutch button:** hold it to freeze the cursor. Hold it for 2 seconds to recalibrate.

## Troubleshooting

- **Windows says "Try connecting your device again":** remove every old "WebShooter" / "Unknown device" entry, enable *Tools → Erase All Flash Before Sketch Upload* for one upload, power cycle the board, and pair again. Renaming the device in the sketch also forces a fresh pairing.
- **"Failed to find MPU6050 chip" with the Adafruit library:** your module is probably an MPU6500 (WHO_AM_I = 0x70). Use this sketch, which doesn't check the chip ID.
- **Cursor drifts at rest:** keep still during calibration, raise `DEADZONE`, or hold the clutch for 2 seconds to recalibrate.
- **Phantom clicks:** make sure the touch sensor's OUT pin is actually connected. A floating pin can trigger clicks.
- **Stops responding after touching the laptop touchpad:** the sketch re-sleeps the MPU on disconnect and reboots if the link stays down for 15 seconds. Re-pair if it still happens.

## Repo layout

```
index.html              the game (open it in any browser)
firmware/
  webshooter_c3_raw.ino ESP32-C3 BLE air-mouse firmware
README.md
```

## Ideas to extend it

- Add a wrist-mounted battery and case.
- Add vibration feedback when you hit a bug.
- Multiplayer or online leaderboard.
- Sound effects.

## License

MIT. Build it, remix it, and tag me if you make one. 🕷️
