<div align="center">

```
  █████╗ ██████╗      ██╗██╗   ██╗███╗   ██╗ █████╗
 ██╔══██╗██╔══██╗     ██║██║   ██║████╗  ██║██╔══██╗
 ███████║██████╔╝     ██║██║   ██║██╔██╗ ██║███████║
 ██╔══██║██╔══██╗██   ██║██║   ██║██║╚██╗██║██╔══██║
 ██║  ██║██║  ██║╚█████╔╝╚██████╔╝██║ ╚████║██║  ██║
 ╚═╝  ╚═╝╚═╝  ╚═╝ ╚════╝  ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝
```

### **Autonomous Resilient Jamming-resistant Unified Network Architecture**

*Infrastructure-free · AES-256 Encrypted · LoRa Mesh · No Phone Required*

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-blue.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![LoRa: SX1262](https://img.shields.io/badge/LoRa-SX1262-orange.svg)](https://www.semtech.com/products/wireless-rf/lora-connect/sx1262)
[![Encryption: AES-128](https://img.shields.io/badge/Encryption-AES--128--CTR-red.svg)](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard)
[![Encryption: AES-256](https://img.shields.io/badge/Encryption-AES--256--CTR-red.svg)](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-teal.svg)](https://www.arduino.cc/)
[![Build: PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)
[![Status: Working](https://img.shields.io/badge/Hardware-Field--Tested-brightgreen.svg)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-Welcome-brightgreen.svg)](CONTRIBUTING.md)

</div>

---

> **ARJUNA** is a fully standalone, military-grade encrypted mesh radio communicator. Two units, no phones, no towers, no internet — just pick it up and communicate. Every message is encrypted with AES-256-CTR + BLAKE2s-4 authentication before it leaves the antenna. Inspired by the Meshtastic ecosystem, designed for field conditions where phones die and towers don't exist.

---

## 📸 Hardware Preview

```
┌─────────────────────────┐     ┌─────────────────────────┐
│      ╔══════════╗  ○ ⏻  │     │  [SMA Antenna]          │
│      ║  OLED    ║  ● ⚡  │     │  ARJUNA v3              │
│      ║ 128×64   ║  (∘∘∘)│     │  Node: UX4E             │
│      ╚══════════╝       │     │  TUE 01/04/2026 21:42   │
│                         │     │                         │
│   [1]  [2]  [3]         │     │  INBOX(1) SENT(3) OUT   │
│ .,?!  ABC  DEF          │     │                         │
│   [4]  [5]  [6]         │     │  1=IN 3=SN 9=OUT        │
│  GHI  JKL  MNO          │     │  7=MSG *=EMRG 5=NDS     │
│   [7]  [8]  [9]         │     └─────────────────────────┘
│  PQRS TUV  WXYZ         │       Two working units, April 2026
│   [*]  [0]  [#]         │       Olive green military enclosure
│ QA/SOS SPC SEND         │       Field-tested in mesh config
└─────────────────────────┘
```

---

## 📑 Table of Contents

- [Why ARJUNA?](#-why-arjuna)
- [Features](#-features)
- [How It Compares](#-how-it-compares)
- [Hardware Requirements](#-hardware-requirements)
- [Pin Mapping](#-pin-mapping)
- [Firmware Architecture](#-firmware-architecture)
- [Repository Structure](#-repository-structure)
- [Quick Start](#-quick-start)
  - [Method A: PlatformIO](#method-a--platformio-recommended)
  - [Method B: Arduino IDE](#method-b--arduino-ide)
  - [Method C: esptool](#method-c--esptool-command-line)
  - [Method D: Web Flasher](#method-d--web-flasher-zero-install)
- [Configuration](#️-configuration--per-device-setup)
- [Flashing Multiple Devices](#-flashing-multiple-devices)
- [Keypad Controls](#-keypad-controls--t9-reference)
- [Screen Guide](#-screen-guide)
- [Encryption Details](#-encryption--security-architecture)
- [Mesh Protocol](#-mesh-protocol)
- [RSSI Proximity](#-rssi-based-proximity-detection-gps-free)
- [Emergency Shortcuts](#️-emergency-shortcut-system)
- [Case / Enclosure](#-case--enclosure)
- [Bill of Materials](#-bill-of-materials-bom)
- [Known Issues & Troubleshooting](#-known-issues--troubleshooting)
- [Roadmap](#️-roadmap)
- [Contributing](#-contributing)
- [License](#-license)
- [Acknowledgements](#-acknowledgements)

---

## 💡 Why ARJUNA?

Modern communication fails when infrastructure fails. Cellular towers go down in earthquakes. WiFi doesn't exist in forests. Satellites are expensive. Phones need charging. **ARJUNA solves this** with a device that:

- Creates its own mesh network the moment you turn it on
- Works node-to-node up to **5 km open terrain** per hop (SF9, 14 dBm)
- Multi-hops through relay nodes to extend range indefinitely
- Encrypts every single packet before it leaves the chip
- Requires **no phone, no app, no internet, no account**
- Runs for **12–24 hours** on a 1000mAh Li-ion battery

Named after Arjuna — *"the one who always hits the target"* — from the Mahabharata.

---

## ✨ Features

### 🔒 Security
- **AES-256-CTR** encryption on every packet (stronger than Meshtastic's AES-128)
- **BLAKE2s-4** authentication tag — prevents packet forgery and replay attacks
- **Hardware TRNG** (ESP32-S3 true random number generator) generates a fresh 64-bit nonce per packet
- **Pre-shared key** network — all nodes share the same key, changed per deployment
- `ENC ERROR` displayed on OLED if decryption/authentication fails — silent dropping of bad packets

### 📡 Mesh Networking
- **Zero infrastructure** — no gateway, no server, no router
- **Flooding relay** with TTL=5 — every node transparently relays packets
- **16-entry duplicate filter** prevents relay loops
- **Auto-discovery** — nodes appear in PEERS list automatically
- **Unicast** (to specific node) or **Broadcast** (to all nodes)
- **Beacons** every 15 seconds for neighbour presence and RSSI mapping
- **Up to 8 peers** tracked simultaneously

### ⌨️ Input
- **4×3 matrix keypad** — works with gloves, in dark, in rain
- **Nokia T9 multi-tap** text input (800ms lock timer)
- **Emergency shortcut menu** — 17 coded military/rescue phrases, one key away
- **PRG button** (built-in GPIO0) — screen navigation, send, and 5-second MAYDAY

### 📺 Display
- **4-screen state machine**: HOME → COMPOSE → HISTORY → PEERS
- **Mailbox UI**: INBOX / SENT / OUTBOX with scrollable message history (5 per folder)
- **IST timestamps** on every message (NTP-synced at boot via WiFi, optional)
- **Battery level** with voltage bar and percentage
- **RSSI signal strength** display and proximity label

### 📍 GPS-Free Proximity
- Estimates distance using RSSI rolling average (4-sample)
- Labels: **NEAR** (<50m) · **MID** (<500m) · **FAR** (<3km) · **DIST** (>3km)
- Calibrated for LoRa SF9, 125kHz, 14dBm

### 🔔 Buzzer Feedback
- Short beep — every keypress (tactile confirmation)
- Double pulse — incoming message
- Ascending chirp — boot/ready
- SOS Morse (`... --- ...`) — MAYDAY transmitted or received
- Continuous rapid pulse — MAYDAY active (silenced by any key)

---

## 📊 How It Compares

| Feature | **ARJUNA** | Meshtastic | Thomas LoRa Mesh | Commercial IP Mesh Radio |
|---|:---:|:---:|:---:|:---:|
| Phone required | ❌ None | ✅ Required | ✅ Required | ❌ None |
| Physical keypad | ✅ 4×3 T9 | ❌ | ❌ | ✅ |
| Encryption | AES-256-CTR | AES-128 | RSA-256 | AES-128/256 |
| Packet MAC | BLAKE2s-4 | None | SHA-256 | Varies |
| Per-packet nonce | ✅ TRNG | ❌ Static IV | ❌ | ❌ |
| GPS-free proximity | ✅ RSSI | ✅ (with GPS) | ❌ | ❌ |
| Standalone OLED UI | ✅ 4 screens | ❌ | ❌ | ✅ |
| Emergency shortcuts | ✅ 17 phrases | ❌ | ❌ | Limited |
| Open source | ✅ MIT | ✅ GPL | ✅ | ❌ |
| Hardware cost | ~₹2,500 | ~₹2,000+ | ~₹8,000+ | ₹1,50,000+ |
| Range (open air) | 2–5 km/hop | 2–5 km/hop | 2–5 km/hop | 5–30 km |

---

## 🔧 Hardware Requirements

### Core Board
| Component | Specification | Notes |
|---|---|---|
| **Microcontroller** | Heltec WiFi LoRa 32 V3 | **White board only** — ESP32-S3FN8 + SX1262 |
| MCU | ESP32-S3FN8 | Dual-core LX7 240MHz, 8MB SiP Flash, 512KB SRAM |
| LoRa Radio | SX1262 | 863–928 MHz, max 21 dBm |
| Display | SSD1306 OLED 0.96" | 128×64 pixels, I2C, onboard |
| USB | Type-C | CP2102 serial, integrated charging |

### External Components (wire to board)
| Component | Specification | Qty | GPIO |
|---|---|---|---|
| Matrix keypad | 4×3 membrane, 4.7kΩ pull-up | 1 | Rows: 33,34,47,3 / Cols: 4,5,6 |
| Active buzzer | 5V, 12mm diameter | 1 | GPIO 2 |
| Li-ion battery | 3.7V 1000mAh, JST 1.25-2P | 1 | JST connector |
| LoRa antenna | 868MHz, SMA, 2–3 dBi | 1 | SMA connector |

### Optional
| Component | Purpose | Notes |
|---|---|---|
| GPS NEO-6M | Location-tagged messages | UART2 on GPIO 43/44 |
| DHT22 | Temperature in SITREP | Any free GPIO |
| Solar panel 6V/1W | Extended field life | Via TP4056 to JST |

---

## 📌 Pin Mapping

> ⚠️ **Critical — Read Before Wiring**
>
> The Heltec V3 has several GPIO traps that will silently break your build if ignored. All corrections below are verified against the official datasheet (HTIT-WB32LA V3 Rev 1.1, Sep 2022) and confirmed by cross-reference with Meshtastic source.

### LoRa SX1262 — Internal (no wiring needed)
| Signal | GPIO | Notes |
|---|---|---|
| SCK | 9 | Internal SPI |
| MISO | 11 | Internal SPI |
| MOSI | 10 | Internal SPI |
| NSS/CS | 8 | Internal SPI |
| BUSY | 13 | Internal |
| RST | 12 | Internal |
| DIO1 | 14 | Interrupt — used by RadioLib |

### OLED SSD1306 — Internal (no wiring needed)
| Signal | GPIO | Notes |
|---|---|---|
| SDA | **17** | Internal board trace — NOT on header |
| SCL | **18** | Internal board trace — NOT on header |
| RST | 21 | Header J2 pin 16 |

> 🚨 **GPIO36 (Vext Ctrl)** must be driven `HIGH` in `setup()` before calling `Wire.begin()`.
> Without this the SSD1306 gets no power regardless of what you write to I2C.
> ```cpp
> pinMode(36, OUTPUT);
> digitalWrite(36, HIGH);
> delay(50);
> Wire.begin(17, 18);
> ```

### Keypad (external wiring required)
| Signal | GPIO | Header | Notes |
|---|---|---|---|
| Row 1 | **33** | J2 pin 12 | Changed from GPIO1 (VBAT conflict) |
| Row 2 | **34** | J2 pin 11 | |
| Row 3 | **47** | J3 | Changed from GPIO35 (LED Ctrl conflict) |
| Row 4 | **3** | J3 pin 14 | |
| Col 1 | **4** | J3 pin 15 | Has internal pull-up |
| Col 2 | **5** | J3 pin 16 | Has internal pull-up |
| Col 3 | **6** | J3 pin 17 | Has internal pull-up |

### Other
| Component | GPIO | Notes |
|---|---|---|
| Buzzer (+) | **2** | Changed from GPIO45 (conflict) |
| VBAT ADC | **1** | Read-only. Formula: `VBAT = (raw/4095) × 3.3 × 4.9` |
| PRG Button | **0** | Built-in, active LOW, internal pull-up |

### Wiring Diagram (ASCII)
```
Heltec V3 Header J3                    4×3 Keypad
──────────────────                     ──────────
GPIO 33  ────────────────────────────► Row 1
GPIO 34  ────────────────────────────► Row 2
GPIO 47  ────────────────────────────► Row 3
GPIO  3  ────────────────────────────► Row 4
GPIO  4  ◄───────────── [4.7kΩ ──3.3V] Col 1
GPIO  5  ◄───────────── [4.7kΩ ──3.3V] Col 2
GPIO  6  ◄───────────── [4.7kΩ ──3.3V] Col 3

GPIO  2  ────────── Buzzer (+) ── GND
GPIO 36  ── Vext Ctrl (drive HIGH)
```

---

## 🏗️ Firmware Architecture

```
arjuna/
├── include/
│   ├── config.h          ← All GPIO, RF, and timing constants + DeviceID generator
│   ├── crypto.h          ← AES-256-CTR encrypt/decrypt + BLAKE2s-4 MAC interface
│   ├── mesh.h            ← RadioLib SX1262 mesh interface (send/receive/beacon/peers)
│   ├── display.h         ← OLED screen API (home/compose/history/peers/notify)
│   ├── storage.h         ← NVS ring-buffer mailbox (INBOX/SENT/OUTBOX)
│   ├── emergency.h       ← Military shortcut phrase table (17 entries)
│   ├── keypad_driver.h   ← T9 multi-tap engine + event types
│   └── buzzer.h          ← LEDC PWM buzzer tones (inline)
│
├── src/
│   ├── main.cpp          ← Setup + main state machine loop
│   ├── mesh.cpp          ← RadioLib SX1262 flooding mesh implementation
│   ├── crypto.cpp        ← AES-256-CTR + BLAKE2s-4 implementation
│   ├── display.cpp       ← SSD1306 128×64 OLED rendering (all screens)
│   ├── keypad.cpp        ← Keypad polling + T9 character engine
│   └── storage.cpp       ← ESP32 NVS persistent message store
│
├── device_config.h       ← ⭐ CHANGE THIS PER DEVICE (ID + key + WiFi for NTP)
├── platformio.ini        ← PlatformIO build configuration
└── FLASH_GUIDE.md        ← Step-by-step flashing instructions
```

### Module Dependency Graph
```
main.cpp
  ├── mesh.cpp     ── RadioLib (SX1262) ── config.h ── crypto.cpp
  ├── display.cpp  ── Adafruit SSD1306  ── emergency.h
  ├── keypad.cpp   ── Keypad library    ── config.h
  ├── storage.cpp  ── ESP32 NVS         ── config.h
  └── buzzer.h     ── ESP32 LEDC        ── config.h
```

### State Machine (UI)
```
                    ┌─────────────────────────────────┐
                    ▼                                 │
  BOOT → SPLASH → HOME ──1──► INBOX                  │
                    │   ──3──► SENT     (folder list) │
                    │   ──9──► OUTBOX                 │
                    │                    │            │
                    │         # = open   │            │
                    │         msg view   ▼            │
                    │   ──7──► COMPOSE ──#──► SEND    │
                    │   ──*──► EMERG. MENU ──#──► TX  │
                    │   ──5──► PEERS ──#──► set target│
                    │                                 │
                    └─────────────────────────────────┘
                    PRG single click = cycle screens
                    PRG hold 5s = MAYDAY broadcast
```

---

## 📂 Repository Structure

```
arjuna/
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── CHANGELOG.md
├── platformio.ini
├── device_config.h           ← Change before flashing each device
│
├── include/                  ← Header files
├── src/                      ← Source files
│
├── docs/
│   ├── FLASH_GUIDE.md        ← All flashing methods (Arduino IDE, esptool, web)
│   ├── WIRING_GUIDE.md       ← GPIO reference + wiring diagrams
│   ├── ENCRYPTION.md         ← Detailed crypto architecture
│   ├── MESH_PROTOCOL.md      ← Packet format + routing specification
│   └── CASE_DESIGN.md        ← 3D printable enclosure + AI prompts
│
├── case/
│   ├── ARJUNA_case_design.md ← Component dimensions + print settings
│   └── openscad/             ← Parametric OpenSCAD source (community contribution)
│
└── tools/
    └── key_generator.py      ← Python script to generate a new random 32-byte PSK
```

---

## 🚀 Quick Start

### Prerequisites (all methods)

1. **One or more** Heltec WiFi LoRa 32 V3 boards
2. **4×3 matrix keypad** wired per the pin map above
3. **Active buzzer** on GPIO 2
4. **LoRa 868MHz antenna** connected to SMA

---

### Method A — PlatformIO (Recommended)

**Step 1:** Install [VS Code](https://code.visualstudio.com/) + [PlatformIO extension](https://platformio.org/install/ide?install=vscode)

**Step 2:** Clone the repo
```bash
git clone https://github.com/YOUR_USERNAME/arjuna.git
cd arjuna
```

**Step 3:** Set your device ID — open `device_config.h` and change:
```cpp
#define DEVICE_ID  "CMD1"   // ← unique 4-char ID for this node
```

**Step 4:** Build and flash
```bash
pio run --target upload
pio device monitor   # 115200 baud — watch boot log
```

**Tip:** If upload fails, hold the **BOOT button** on the board while clicking upload, release after "Connecting..." appears.

---

### Method B — Arduino IDE

**Step 1:** Download [Arduino IDE 2.x](https://www.arduino.cc/en/software)

**Step 2:** Add Heltec board support
- Go to `File → Preferences → Additional Boards Manager URLs`
- Add: `https://resource.heltec.cn/download/package_heltec_esp32_index.json`

**Step 3:** Install Heltec board package
- `Tools → Board → Boards Manager` → search `Heltec` → Install

**Step 4:** Install libraries (`Tools → Manage Libraries`)

| Library | Search Term | Version |
|---|---|---|
| RadioLib | `RadioLib` | ≥ 6.6.0 |
| ArduinoCrypto | `Crypto` | ≥ 0.4.0 |
| Adafruit SSD1306 | `SSD1306` | ≥ 2.5.7 |
| Adafruit GFX Library | `Adafruit GFX` | ≥ 1.11.9 |
| Keypad | `Keypad` | ≥ 3.1.1 |

**Step 5:** Select board
- `Tools → Board → Heltec ESP32 Series → Heltec WiFi LoRa 32(V3)`

**Step 6:** Merge source files
- Copy contents of `src/*.cpp` into the Arduino sketch folder
- Rename `main.cpp` to `arjuna.ino`
- Remove `#include <Arduino.h>` from the top (Arduino IDE adds it)
- Copy all `include/*.h` files into the same folder

**Step 7:** Select port and click **Upload** (→ arrow)

---

### Method C — esptool (Command Line)

Build in Arduino IDE or PlatformIO first to generate a `.bin` file, then:

```bash
# Install esptool
pip install esptool

# Flash (replace /dev/ttyUSB0 with your port, COM3 on Windows)
esptool.py \
  --chip esp32s3 \
  --port /dev/ttyUSB0 \
  --baud 921600 \
  --before default_reset \
  --after hard_reset \
  write_flash -z 0x0 arjuna.ino.bin
```

---

### Method D — Web Flasher (Zero Install)

1. Build in Arduino IDE → `Sketch → Export Compiled Binary` → get `.bin` file
2. Open Chrome → go to [https://espressif.github.io/esptool-js/](https://espressif.github.io/esptool-js/)
3. Connect Heltec V3 via USB-C → Click **Connect** → Flash the `.bin`

---

## ⚙️ Configuration — Per-Device Setup

The **only file you change** between devices is `device_config.h`:

```cpp
// ╔════════════════════════════════════════════════╗
// ║  CHANGE THIS BEFORE FLASHING EACH DEVICE      ║
// ╚════════════════════════════════════════════════╝

// Unique callsign — max 4 chars, UPPERCASE, no spaces
// Suggested: "CMD1", "ALF1", "BRV1", "MED1", "SCT1"
#define DEVICE_ID         "CMD1"

// LoRa frequency — must match on ALL nodes
// 868.0 = India/EU  |  915.0 = USA/Australia
#define LORA_FREQ_HZ      868.0E6

// AES-256 Pre-Shared Key — 32 bytes, SAME on ALL nodes
// Generate a new key: python tools/key_generator.py
#define CRYPTO_KEY_BYTES { \
  0xA3,0x7F,0x2B,0xE1,0x94,0xC0,0xD8,0x5A, \
  0x11,0x3E,0x6D,0xF2,0x88,0x47,0xBC,0x99, \
  0x0C,0x51,0xDA,0x72,0x3F,0xAE,0x60,0x19, \
  0x7C,0xB4,0x85,0x2D,0xF0,0x36,0x9E,0xC7  \
}

// Optional: WiFi for NTP clock sync at boot
// Leave as "" to skip (time shows --:--)
// WiFi is disconnected immediately after NTP sync
#define NTP_WIFI_SSID     ""
#define NTP_WIFI_PASS     ""
```

### Generating a New Network Key

```bash
python tools/key_generator.py

# Output:
# New 32-byte AES-256 key:
# 0xF3,0x9A,0x1C,0xE7,0x84,0x56,0xB2,0x0D,
# 0x72,0xAF,0x33,0xC9,0x1E,0x8B,0x47,0xF0,
# 0x6D,0x25,0x9E,0xB4,0xA1,0x73,0xC8,0x3F,
# 0x0B,0xE6,0x58,0xD4,0x92,0x7A,0x15,0xC3
# 
# Mnemonic fingerprint: KITE-MOON-FLUX-ZERO
# Copy the above into device_config.h CRYPTO_KEY_BYTES
# All nodes must use the same key to communicate.
```

---

## 📟 Flashing Multiple Devices

Each device needs its own unique `DEVICE_ID`. Recommended callsign scheme:

| Role | Device ID | Suggested Use |
|---|---|---|
| Command Post | `CMD1` | Team leader node |
| Alpha Squad 1 | `ALF1` | Alpha team member 1 |
| Alpha Squad 2 | `ALF2` | Alpha team member 2 |
| Bravo Squad 1 | `BRV1` | Bravo team member 1 |
| Medical Unit | `MED1` | Medic node |
| Logistics | `LOG1` | Support/supply |
| Scout/Recon | `SCT1` | Forward observer |
| Relay Node | `REL1` | Fixed relay, no user |

All nodes use the **same** `CRYPTO_KEY_BYTES` and `LORA_FREQ_HZ`.

**Flash sequence:**
1. Edit `device_config.h` → set unique `DEVICE_ID`
2. Flash the device
3. Label the device with its callsign
4. Repeat for each unit

The Device ID is also auto-generated from the WiFi MAC address as a fallback (FNV-1a hash → Base-32), stored in NVS flash on first boot.

---

## ⌨️ Keypad Controls & T9 Reference

### Keypad Layout
```
┌─────┬─────┬─────┐
│  1  │  2  │  3  │
│.,?! │ ABC │ DEF │
├─────┼─────┼─────┤
│  4  │  5  │  6  │
│ GHI │ JKL │ MNO │
├─────┼─────┼─────┤
│  7  │  8  │  9  │
│PQRS │ TUV │WXYZ │
├─────┼─────┼─────┤
│  *  │  0  │  #  │
│QA/  │     │SEND/│
│SOS  │ SPC │CONF │
└─────┴─────┴─────┘
```

### T9 Multi-Tap
- Press a key **repeatedly** within 800ms to cycle through its characters
- Wait 800ms **or press a different key** to lock the current character
- The pending character is shown with an **underscore** cursor on the OLED
- Key `1` = backspace (delete last character or cancel pending)
- Key `0` = space (always immediate, no cycling)

**Example:** Type "HI"
- Press `4` once → shows `G_`
- Press `4` again within 800ms → shows `H_`
- Press `4` again → shows `I_`
- Wait 800ms → `I` is locked
- Press `4` twice → shows `IH_` ... (oops, press `1` to backspace)
- Correct approach: Wait for lock between different letters of same key

### Navigation Controls (all screens)

| Key | HOME | COMPOSE | INBOX/SENT | PEERS |
|---|---|---|---|---|
| `1` | — | Backspace | — | — |
| `2` | SENT | T9 'ABC' | Scroll up | Scroll up |
| `3` | OUTBOX | T9 'DEF' | — | — |
| `4` | — | T9 'GHI' | Scroll up | Scroll up |
| `5` | PEERS | T9 'JKL' | Open msg | Select peer |
| `6` | — | T9 'MNO' | Scroll down | Scroll down |
| `7` | COMPOSE | T9 'PQRS' | Back | Back |
| `8` | — | T9 'TUV' | Scroll down | Scroll down |
| `9` | OUTBOX | T9 'WXYZ' | — | — |
| `*` | EMERG. | QA menu | Scroll up | Scroll up |
| `0` | — | Space | — | — |
| `#` | MAYDAY¹ | SEND | Open/Confirm | Select target |
| PRG click | Next screen | Next screen | Next screen | Next screen |
| PRG hold 2s | — | Send msg | — | — |
| PRG hold 5s | 🚨 MAYDAY | 🚨 MAYDAY | 🚨 MAYDAY | 🚨 MAYDAY |

¹ `#` on HOME screen sends MAYDAY. PRG hold 5s also sends MAYDAY from any screen.

---

## 🖥️ Screen Guide

### Screen 1 — HOME
```
┌──────────────────────────────────┐
│ ARJUNA:CMD1           BAT:3.8v  │  ← Node ID + battery
│ TUE 01/04/2026        IST        │  ← Date in IST
│         21:42                    │  ← Time in IST (large)
├──────────────────────────────────┤
│ INBOX(2)  SENT(3)  OUTBOX(0)    │  ← Folder counts
│ RX[ALF1]: SITREP OK             │  ← Last received msg
│ 1=IN 3=SN 9=OUT 7=MSG *=EMRG   │  ← Controls
└──────────────────────────────────┘
```

### Screen 2 — COMPOSE
```
┌──────────────────────────────────┐
│ COMPOSE              TO:ALF1    │  ← Target node
│ CONTACT ETA 0600 AM_  [22/96]  │  ← Text + cursor + count
│                                  │
├──────────────────────────────────┤
│ *=QA  1=DEL  #=SEND             │
│ PRGhold2s=SND  PRG=nav          │
└──────────────────────────────────┘
```

### Screen 3 — HISTORY (INBOX/SENT/OUTBOX)
```
┌──────────────────────────────────┐
│ INBOX                     [2/5] │  ← Folder name + count
├──────────────────────────────────┤
│ << ALF1  14:22 01/04            │  ← Received from ALF1
│    SITREP OK                    │  ← Message preview
├──────────────────────────────────┤
│ >> ME    14:30 01/04            │  ← Sent by us
│    CONTACT ALPHA GRID           │
└──────────────────────────────────┘
│ 2/8=scroll  #=open  0=back      │
```

**Message detail view:**
```
┌──────────────────────────────────┐
│ FR:ALF1  14:22 01/04            │
│ CONTACT REPORTED AT             │
│ GRID ALPHA 0600                 │
│                                  │
├──────────────────────────────────┤
│ →DEC:A3F2C8B1  [AES-256 🔒]    │  ← Crypto indicator
│ AES-256-CTR   0=bk  #=reply    │
└──────────────────────────────────┘
```
The `→DEC:` line shows the first 8 hex chars of the packet nonce — a fingerprint confirming this message was successfully decrypted.

### Screen 4 — PEERS
```
┌──────────────────────────────────┐
│ PEER NODES                  [3] │
├──────────────────────────────────┤
│ > BROADCAST (ALL)               │  ← Currently selected target
│   ALF1  14:22  ████░░  -72dB  NEAR│
│   BRV1  13:55  ██░░░░  -88dB  MID │
├──────────────────────────────────┤
│ *=prev  #=next  PRG=select      │
└──────────────────────────────────┘
```

---

## 🔐 Encryption & Security Architecture

### Packet Structure
```
ArjPacket (150 bytes, packed)
┌────────┬──────┬──────┬────────┬──────┬──────┬─────┬─────┬────────────────┐
│ magic  │ src  │ dst  │ nonce  │ mac4 │ type │ seq │ ttl │ paylen+payload  │
│ "ARJN" │ [5]  │ [5]  │  [8]   │ [4]  │ [1]  │ [1] │ [1] │  [1] + [0-120] │
└────────┴──────┴──────┴────────┴──────┴──────┴─────┴─────┴────────────────┘
  4 bytes  5      5       8        4      1      1     1      1 + N bytes
```

### Encrypt Flow (TX)
```
plaintext  ──►  Hardware TRNG  ──►  nonce[8]
                                        │
plaintext + AES_KEY + nonce  ──►  AES-256-CTR  ──►  ciphertext
                                                          │
[src+dst+nonce+type+seq+paylen+ciphertext] + AES_KEY  ──►  BLAKE2s  ──►  mac4[4]
                                                                               │
[magic + src + dst + nonce + mac4 + type + seq + ttl + paylen + ciphertext]  ──►  LoRa TX
```

### Decrypt Flow (RX)
```
LoRa RX  ──►  magic check  ──►  BLAKE2s verify (mac4)
                                      │
                               ┌──────┴──────┐
                             PASS           FAIL
                               │              │
                    AES-256-CTR decrypt    drop silently
                               │           display "ENC ERROR"
                    ASCII sanity check
                               │
                    deliver to INBOX
```

### Why per-packet nonces matter
Standard AES-CTR reuse of the same nonce+key leaks plaintext via XOR. ARJUNA generates a fresh 64-bit nonce from the ESP32-S3 hardware TRNG for every single packet. Even if an attacker captures 1000 packets from the same key, they cannot recover any plaintext. This makes ARJUNA cryptographically superior to most commercial LoRa devices.

### Changing the Network Key
Generate a new key with `tools/key_generator.py`, update `device_config.h`, and reflash all nodes. For field deployments: change the key before every operation.

> 🔑 **Do not commit your production key to a public repository.** The default key in this repo is a demo key.

---

## 📡 Mesh Protocol

### Why flooding instead of routing?
For small tactical networks (2–20 nodes), flooding with duplicate filtering is:
- **More reliable** — no routing tables to corrupt or desync
- **Self-healing** — if a relay node dies, others automatically relay
- **Lower latency** — no route discovery phase
- **Simpler** — less code, fewer bugs, easier to audit
- **Proven** — this is also how Meshtastic works

### Relay Logic
```
RX packet
    │
    ├─► magic check (fail = drop)
    ├─► BLAKE2s MAC verify (fail = drop silently)
    ├─► duplicate filter: seen (src, seq) before? (yes = drop)
    ├─► update neighbour RSSI table
    │
    ├─► TTL > 1? ──YES──► decrement TTL, re-transmit (relay)
    │
    └─► dest == myID or dest == "BCAST"?
            │
           YES ──► decrypt ──► ASCII sanity ──► deliver to app
```

### Beacon Format
Every 15 seconds each node broadcasts a PKT_BEACON packet (no payload, just header). This allows:
- All nodes to discover each other without sending messages
- RSSI measurement for proximity estimation
- "Last seen" time tracking in the PEERS screen

---

## 📍 RSSI-Based Proximity Detection (GPS-Free)

ARJUNA estimates inter-node distance using LoRa receive signal strength. No GPS hardware is needed.

### How it works
1. Every received packet (message or beacon) records the RSSI
2. A **4-sample rolling average** smooths out transient variations
3. The average is mapped to a proximity category using calibrated thresholds

### Calibration (SF9, 125kHz BW, 14dBm TX, open terrain)
| Label | RSSI Range | Estimated Distance |
|---|---|---|
| **NEAR** | > -70 dBm | < 50 m |
| **MID** | -70 to -90 dBm | 50 m – 500 m |
| **FAR** | -90 to -110 dBm | 500 m – 3 km |
| **DIST** | < -110 dBm | > 3 km (marginal link) |

### Limitations
- RSSI is affected by obstacles, multipath, and antenna orientation
- These are estimates, not GPS coordinates
- For precise location, add a NEO-6M GPS module (see docs/GPS_ADDON.md)

---

## 🆘 Emergency Shortcut System

Press `*` at any time to open the emergency menu. Use `*` to cycle, `#` to send.

### Full Phrase List
| # | Code | Full Phrase | Category |
|---|---|---|---|
| 1 | MAYDAY | MAYDAY MAYDAY | 🔴 Emergency |
| 2 | SOS | SOS NEED HELP | 🔴 Emergency |
| 3 | MEDIC | MEDIC URGENT | 🔴 Emergency |
| 4 | SITREP | SITREP OK | 🟡 Status |
| 5 | CONTACT | CONTACT/ENEMY | 🔴 Tactical |
| 6 | HOLD | HOLD POSITION | 🟡 Tactical |
| 7 | ADV | ADVANCE NOW | 🟡 Tactical |
| 8 | FALLBK | FALL BACK | 🔴 Tactical |
| 9 | COVER | COVER ME | 🟡 Tactical |
| 10 | CLEAR | AREA CLEAR | 🟢 Tactical |
| 11 | EVAC | EVAC REQUEST | 🔴 Logistics |
| 12 | AMMO | AMMO LOW | 🟡 Logistics |
| 13 | RGRP | REGROUP RALLY | 🟡 Tactical |
| 14 | SECR | SECURE PERIMETER | 🟡 Tactical |
| 15 | EXTR | EXTRACTION REQ | 🔴 Logistics |
| 16 | RSUP | RESUPPLY NEEDED | 🟡 Logistics |
| 17 | CAS | CASUALTY RPT | 🔴 Medical |

### MAYDAY Behaviour
When MAYDAY is sent or received:
- Continuous rapid buzzer pulsing (180ms on/off)
- Full-screen inverted MAYDAY alert on OLED
- Buzzer silenced by pressing **any key**
- MAYDAY can be triggered from any screen by **holding PRG for 5 seconds**

---

## 🧱 Case / Enclosure

The olive-green military enclosure shown in the photos was designed to fit the exact component dimensions measured from the hardware:

| Component | W × H | Fits in Zone |
|---|---|---|
| 4×3 keypad | 40 × 55 mm | Bottom face |
| Battery (LiPo 104050) | 38 × 54 mm | Bottom-left interior |
| Buzzer | ∅12 mm | Top-right interior |
| Heltec V3 | 52 × 26 mm | Top-left interior |
| OLED window | 28 × 17 mm | Top-left face cutout |

**External case dimensions:** 90 × 100 × 28 mm

### 3D Printing
See [docs/CASE_DESIGN.md](docs/CASE_DESIGN.md) for:
- Full parametric OpenSCAD source
- AI generation prompts for Meshy / TripoSG
- Tinkercad step-by-step instructions
- Print settings (0.2mm layer, 25% gyroid, 4 walls, PETG recommended)

**Existing compatible case:** [TextMesh for Heltec V3 on Printables](https://www.printables.com/model/811577-textmesh-heltec-v3-edition) — uses the same Heltec V3 PCB. Modify the front panel for the 44×59mm 4×3 keypad opening.

---

## 📦 Bill of Materials (BOM)

| Component | Spec | Qty | Approx Cost (INR) |
|---|---|---|---|
| Heltec WiFi LoRa 32 V3 | ESP32-S3 + SX1262 + OLED | 1 | ₹1,200 |
| 4×3 Matrix Keypad | Membrane, 40×55mm | 1 | ₹80 |
| Active Buzzer | 5V, 12mm diameter | 1 | ₹30 |
| Li-ion Battery | 3.7V 1000mAh, JST 1.25-2P | 1 | ₹350 |
| LoRa Antenna | 868MHz, SMA, 2dBi | 1 | ₹150 |
| Jumper wires | 20cm DuPont female-female | 1 pack | ₹60 |
| PLA/PETG filament | ~50g for case | 50g | ₹100 |
| M2.5 screws | 8mm countersunk, stainless | 4 | ₹40 |
| M2 standoffs | 5mm F-F brass | 4 | ₹60 |
| **Total (1 unit)** | | | **~₹2,070** |

---

## 🐛 Known Issues & Troubleshooting

### OLED is blank after flashing
✅ **Fix:** Make sure `setup()` drives GPIO36 HIGH before `Wire.begin()`.
```cpp
pinMode(36, OUTPUT);
digitalWrite(36, HIGH);
delay(50);
Wire.begin(17, 18);
```
This enables the Vext power rail. Without it, the SSD1306 receives no power.

### Upload fails / stuck at "Connecting..."
✅ **Fix:** Hold the **BOOT button** on the Heltec board while clicking Upload. Release after "Connecting..." appears in the terminal. This forces the ESP32-S3 into download mode.

### Battery shows wrong voltage (~2.5V instead of ~3.7V)
✅ **Fix:** v2.x firmware used the wrong ADC multiplier (`×2.0`). v3.0 uses the correct formula from the datasheet:
```cpp
VBAT = (analogRead(1) / 4095.0) * 3.3 * 4.9
```
The Heltec V3 uses a 100kΩ/390kΩ voltage divider on GPIO1 (ADC1_CH0).

### LoRa radio fails to init / no mesh packets
✅ **Check:**
1. SPI pins 8,9,10,11 must not be reassigned
2. `radio.setDio2AsRfSwitch(true)` must be called after `radio.begin()`
3. Antenna must be connected — transmitting without antenna can damage SX1262
4. Both nodes must use same `LORA_FREQ_HZ` and `LORA_SYNC_WORD` (0x12)

### `gpio_install_isr_service` error or keypad not responding
✅ **Fix:** Both RadioLib and the Keypad library need the GPIO ISR service. Call `gpio_install_isr_service(0)` once before either library initializes:
```cpp
gpio_install_isr_service(0);  // safe to call twice — RadioLib won't panic
```

### PSRAM boot error (`PSRAM ID read error: 0x00ffffff`)
✅ **Fix:** ESP32-S3**FN8** (Heltec V3) has NO external PSRAM. Ensure `platformio.ini` has:
```ini
board_build.arduino.memory_type = qio_qspi
build_flags = -DBOARD_HAS_PSRAM=0 -DCONFIG_SPIRAM_SUPPORT=0
```

### Keypad keys stuck or wrong character
✅ **Check:**
1. Column GPIOs (4,5,6) must have pull-up resistors (4.7kΩ to 3.3V) if not using internal pull-ups
2. Row GPIOs (33,34,47,3) are outputs — don't share with other drivers
3. Do not use GPIO 35 as Row 3 — it is LED_WRITE_CTRL and affects the onboard LED

### Messages not decrypting (shows "ENC ERROR")
✅ **Fix:** Both nodes must have **identical** `CRYPTO_KEY_BYTES` in `device_config.h`. Even one wrong byte fails the BLAKE2s-4 MAC check.

---

## 🗺️ Roadmap

### v3.1 (Next)
- [ ] GPS NEO-6M integration — location-tagged messages `@lat,lon`
- [ ] BLE companion mode — phone as keyboard (no LoRa on phone)
- [ ] SPIFFS persistent message log (survives power cycles)
- [ ] Delivery acknowledgement (PKT_ACK with optional retry)
- [ ] Channel hopping — multiple frequency channels configurable

### v3.2
- [ ] Voice annotation — PTT short audio clips over LoRa (compressed)
- [ ] Group management — named groups with separate keys
- [ ] Relay-only mode — dedicated relay node with low power sleep
- [ ] OTA firmware update via mesh broadcast

### v4.0 (Future)
- [ ] Elliptic Curve Diffie-Hellman key exchange (per-session keys)
- [ ] Post-quantum cryptography hooks (CRYSTALS-Kyber)
- [ ] Custom PCB design with all components integrated
- [ ] IP67 waterproofing
- [ ] GPS-based blue force tracking overlay

---

## 🤝 Contributing

Contributions are welcome and appreciated. This project exists because of open-source.

### Ways to Contribute
- 🐛 **Bug reports** — open an issue with steps to reproduce
- 💡 **Feature requests** — open an issue with use case description
- 🔧 **Pull requests** — see guidelines below
- 📖 **Documentation** — fix typos, add examples, translate
- 🖨️ **Case designs** — share your 3D printed enclosure designs
- 🗺️ **Field reports** — share real-world range and performance results

### Pull Request Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Make your changes — keep commits focused and descriptive
4. Test on real hardware (Heltec V3 is the reference platform)
5. Update documentation if the change affects behaviour
6. Submit a PR with a clear description of what and why

### Code Style
- C++14, Arduino framework
- Tabs for indentation (matching existing code)
- Comment non-obvious logic; no need to comment obvious code
- Keep `.cpp` files focused on one module
- No dynamic memory allocation in hot paths (`new`/`delete` in `loop()`)
- No `String` objects — use `char[]` for all text handling
- No blocking `delay()` in `loop()` — use `millis()` timers

### Reporting Security Vulnerabilities
Please **do not** open a public issue for security vulnerabilities. Email the maintainer directly. We aim to respond within 72 hours.

---

## 📄 License

```
MIT License

Copyright (c) 2026 ARJUNA Project Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
```

See [LICENSE](LICENSE) for the full text.

### Notice on Encryption

This software implements AES-256 encryption. Before deploying in your jurisdiction, verify that the export, import, and use of this encryption strength is legally permitted. In India, the use of encryption up to 40-bit key length is freely permitted; above that (including AES-256), check the latest IT Act guidelines and obtain necessary clearances if deploying in a government or defense context.

---

## 🙏 Acknowledgements

ARJUNA stands on the shoulders of many excellent open-source projects:

| Project | Contribution to ARJUNA |
|---|---|
| [Meshtastic](https://github.com/meshtastic/firmware) | Inspiration for standalone LoRa mesh; confirmed RadioLib as the right library for SX1262; UI concept reference |
| [RadioLib](https://github.com/jgromes/RadioLib) by Jan Gromeš | Core SX1262 driver — stable, well-maintained, exactly what was needed when LoRaMesher's API broke |
| [Arduino Crypto](https://github.com/rweather/arduinolibs) by Rhys Weatherley | AES-256-CTR and BLAKE2s implementations for constrained MCUs |
| [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) | OLED display driver |
| [Keypad](https://github.com/Chris--A/Keypad) by Chris--A | Matrix keypad scanning library |
| [Heltec Automation](https://heltec.org) | Excellent hardware platform — WiFi LoRa 32 V3 is a joy to build with |
| [TextMesh](https://www.printables.com/model/811577) | 3D printable case design reference for Heltec V3 form factor |
| Espressif Systems | ESP32-S3 platform, ESP-IDF, Arduino framework |
| The amateur radio community | LoRa propagation knowledge, field testing insights |

---

## 📬 Contact & Community

- **Issues & Bug Reports:** [GitHub Issues](https://github.com/YOUR_USERNAME/arjuna/issues)
- **Discussions:** [GitHub Discussions](https://github.com/YOUR_USERNAME/arjuna/discussions)
- **Project Wiki:** [GitHub Wiki](https://github.com/YOUR_USERNAME/arjuna/wiki)

---

<div align="center">

**Built for the field. Encrypted by default. Open to all.**

*"The one who always hits the target."*

⭐ If ARJUNA helped your project, give it a star — it helps others find it.

</div>
