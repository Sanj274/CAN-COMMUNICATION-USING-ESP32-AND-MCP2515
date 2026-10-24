# CAN Communication using ESP32 and MCP2515

Two ESP32 boards talk to each other over a **CAN bus**, each using an **MCP2515 CAN controller module**. One board plays the role of a Battery Management System (BMS) and sends dummy battery data (voltage, current, charge, temperature, cycles). The other board receives the data over CAN, decodes it and prints a battery status report on the Serial Monitor.

```
 ESP32 #1 (Transmitter)                                   ESP32 #2 (Receiver)
 dummy BMS data                                           decodes + displays data
      |  SPI                                                   ^  SPI
      v                                                        |
 MCP2515 module  ======= CANH / CANL twisted pair =======  MCP2515 module
```

---

## Table of contents

1. [What you need](#what-you-need)
2. [Connections](#connections)
3. [Software setup](#software-setup)
4. [How it works](#how-it-works)
5. [CAN frame format](#can-frame-format)
6. [Upload and run](#upload-and-run)
7. [Expected output](#expected-output)
8. [Troubleshooting](#troubleshooting)
9. [Known issues and ideas](#known-issues-and-ideas)

---

## What you need

| Item | Quantity | Notes |
|------|----------|-------|
| ESP32 development board | 2 | e.g. ESP32 DevKit (30 or 38 pin) |
| MCP2515 CAN module (with TJA1050 transceiver) | 2 | The common blue module, 5 V, 8 MHz crystal |
| Jumper wires | about 16 | 7 per ESP32-to-module link, plus CAN lines |
| USB cables | 2 | One per ESP32 (power + Serial Monitor) |

---

## Connections

The code calls `SPI.begin()` with no arguments, so the ESP32 uses its **default SPI pins (VSPI)**. The chip-select pin is set in the code as `MCP2515 mcp2515(5);`, i.e. **GPIO 5**.

> Do the wiring in **section A on both boards**, then connect the two CAN modules together as in **section B**.

### A. ESP32 to MCP2515 module (repeat for both boards)

| MCP2515 module pin | ESP32 pin | Purpose |
|--------------------|-----------|---------|
| **VCC** | **5V** (VIN) | Power for the module (needs 5 V for the TJA1050 transceiver) |
| **GND** | **GND** | Ground |
| **CS** | **GPIO 5** | SPI chip select (set in code: `MCP2515 mcp2515(5)`) |
| **SCK** | **GPIO 18** | SPI clock |
| **SI** (MOSI) | **GPIO 23** | Data from ESP32 to MCP2515 |
| **SO** (MISO) | **GPIO 19** | Data from MCP2515 to ESP32 |
| **INT** | not connected | Interrupt pin, not used (the code polls with `readMessage()`) |

### B. CAN bus between the two modules

| Module on ESP32 #1 | Module on ESP32 #2 |
|--------------------|--------------------|
| **CANH** | **CANH** |
| **CANL** | **CANL** |
| **GND** | **GND** (common ground) |

Connect **CANH to CANH** and **CANL to CANL**. Do not cross them. Using a twisted pair for CANH/CANL is best, but for short desk wiring plain jumper wires work.

### Wiring diagram

```
   ESP32 #1                MCP2515 #1                    MCP2515 #2                ESP32 #2
 +-----------+          +-------------+               +-------------+          +-----------+
 | 5V        |--------->| VCC         |               |         VCC |<---------| 5V        |
 | GND       |--------->| GND         |               |         GND |<---------| GND       |
 | GPIO 5    |--------->| CS          |               |          CS |<---------| GPIO 5    |
 | GPIO 18   |--------->| SCK         |               |         SCK |<---------| GPIO 18   |
 | GPIO 23   |--------->| SI          |               |          SI |<---------| GPIO 23   |
 | GPIO 19   |<---------| SO          |               |          SO |--------->| GPIO 19   |
 +-----------+          |             |               |             |          +-----------+
                        | CANH -------+---------------+------- CANH |
                        | CANL -------+---------------+------- CANL |
                        | GND  -------+---------------+-------  GND |   (common ground)
                        +-------------+               +-------------+
```

Both ESP32 and module pairs are wired identically (table A). Only the three CAN lines in the middle join the two sides (table B).

### Quick pin summary

```
ESP32 GPIO 5   -> CS        ESP32 GPIO 18 -> SCK
ESP32 GPIO 23  -> SI        ESP32 GPIO 19 <- SO
ESP32 5V       -> VCC       ESP32 GND     -> GND
Module A CANH <-> Module B CANH      Module A CANL <-> Module B CANL
Module A GND  <-> Module B GND
```

### Wiring notes

- **Common ground is essential.** Both ESP32 boards and both modules must share GND, otherwise the CAN signals have no reference and communication fails.
- **Termination:** a CAN bus needs a **120 ohm resistor at each end**. Most MCP2515 modules have one built in, often enabled by a jumper (check for a jumper next to the CANH/CANL terminal). Keep it **enabled on both modules**.
- **Power:** supply each module with **5 V**. The ESP32's 3V3 pin is not enough for the TJA1050 transceiver.
- **Logic levels:** the module runs at 5 V while the ESP32 pins are 3.3 V. Many people connect them directly and it works, but the 5 V signal on **SO** goes into an ESP32 input. For a safer setup, put a level shifter (or a resistor divider) on the **SO** line.
- Keep SPI wires short to avoid random resets and unreliable data.
- **INT** is left unconnected because this code reads messages by polling, not by interrupt.

---

## Software setup

1. Install the **Arduino IDE** and add the **ESP32 board package** (Boards Manager).
2. Select the board **ESP32 Dev Module** and the correct COM port.
3. Install the CAN library: **`mcp2515`** by *autowp* (Library Manager, or from GitHub: `autowp/arduino-mcp2515`).
4. Set the Serial Monitor to **9600 baud**.

### Project structure

```
.
├── README.md
└── can-bms/
    ├── bms_transmitter/bms_transmitter.ino   # ESP32 #1 (Tx)
    └── bms_receiver/bms_receiver.ino         # ESP32 #2 (Rx)
```

Each `.ino` must be inside a folder with the same name (Arduino IDE requirement).

---

## How it works

**Transmitter (ESP32 #1)**
- Starts SPI and resets the MCP2515, then sets the CAN bit rate to **500 kbit/s** for an **8 MHz** crystal (falls back to 16 MHz if that call returns an error) and enters normal mode.
- Every **2 seconds** it slightly changes the dummy battery values, packs them into one 8-byte frame with CAN ID `0x100`, and sends it.
- Prints `SENT` or `FAILED` on the Serial Monitor for every frame.

**Receiver (ESP32 #2)**
- Uses the same CAN settings (500 kbit/s, 8 MHz crystal).
- Polls the MCP2515 with `readMessage()`. When a frame with ID `0x100` arrives it decodes the bytes back into voltage, current, charge, temperature and cycles.
- Calculates `power = voltage x current` and a status label from the charge level, then prints a status report.
- If no new frame arrives for 5 seconds (after at least one was received) it prints a warning.

Both boards must use the **same bit rate and crystal setting**, and the CAN ID must match (`BMS_DATA_ID 0x100`).

### Dummy values produced by the transmitter

| Parameter | Start value | Range | Unit |
|-----------|-------------|-------|------|
| Voltage | 12.50 | 11.0 - 13.5 | V |
| Current | 2.50 | 0.5 - 5.0 | A |
| Charge | 85 | 0 - 100 | % |
| Temperature | 25.0 | 20.0 - 35.0 | deg C |
| Cycles | 240 | 240 - 500 (wraps to 240) | count |
| Power | 31.25 | voltage x current | W |

### Battery status label (from charge)

| Charge | Status |
|--------|--------|
| above 80 % | Excellent |
| above 50 % | Good |
| above 20 % | Low |
| 20 % or below | Critical |

---

## CAN frame format

| Property | Value |
|----------|-------|
| CAN ID | `0x100` (`BMS_DATA_ID`) |
| Data length (DLC) | 8 bytes |
| Bit rate | 500 kbit/s |
| Send interval | every 2 seconds |
| `0x101` (`ACK_ID`) | defined in the code but not used yet |

| Byte | Content | Encoding | Example |
|------|---------|----------|---------|
| 0 | Voltage high byte | `voltage x 100`, 16-bit big-endian | 12.50 V -> 1250 = `0x04E2` |
| 1 | Voltage low byte | | |
| 2 | Current high byte | `current x 100`, 16-bit big-endian | 2.50 A -> 250 = `0x00FA` |
| 3 | Current low byte | | |
| 4 | Charge | percent, 0 - 100 | 85 -> `0x55` |
| 5 | Temperature | `temperature x 10` (see [known issues](#known-issues-and-ideas)) | 25.0 -> 250 = `0xFA` |
| 6 | Cycles high byte | 16-bit big-endian | 240 -> `0x00F0` |
| 7 | Cycles low byte | | |

Power and status are **not** sent. The receiver calculates them.

---

## Upload and run

1. Wire everything as described in [Connections](#connections).
2. Open `bms_transmitter.ino`, select the port of the first ESP32 and upload.
3. Open `bms_receiver.ino`, select the port of the second ESP32 and upload.
4. Open a Serial Monitor for **each** board (9600 baud). Two Arduino IDE windows, or two terminal programs, are the easiest way to watch both.
5. Within a couple of seconds, the transmitter should print `SENT` and the receiver should print the battery status.

---

## Expected output

**Transmitter**

```
===========================================
   BMS TRANSMITTER
===========================================
✓ CAN initialized
✓ Sending BMS data...
===========================================

Transmitting: BAT:12.50,2.50,85,25.0,240 → SENT ✓
```

**Receiver**

```
===========================================
   BMS RECEIVER
===========================================
✓ CAN initialized
✓ Waiting for BMS data...
===========================================

=== BATTERY STATUS ===
Voltage:    12.50 V
Current:    2.50 A
Charge:     85 %
Temp:       25.0 °C
Cycles:     240
Power:      31.25 W
Status:     Excellent
======================
```

The numbers change every 2 seconds because the transmitter drifts them randomly. The values above are only an example.

---

## Troubleshooting

| Symptom | What to check |
|---------|---------------|
| Transmitter prints `FAILED` every time | CANH/CANL connected (not swapped), common GND, termination enabled, and the receiver module powered. A CAN transmitter with no listener on the bus gets no acknowledgement and reports a failure |
| Receiver prints nothing | Same CAN bit rate and crystal setting on both boards, CS wired to GPIO 5, and the transmitter is running |
| Nothing works on either board | Check the SPI wires (CS, SCK, SI, SO) and the 5 V supply on the module |
| Works only with the wrong crystal setting | Look at the metal can on the MCP2515 board: it says `8.000` or `16.000`. Set `MCP_8MHZ` or `MCP_16MHZ` in `setBitrate()` to match, on **both** boards |
| Random resets or corrupted data | Shorten the SPI wires, use a good USB cable and a stable supply |
| `mcp2515.h: No such file` when compiling | Install the **mcp2515** library by autowp |
| Garbled text in Serial Monitor | Set the baud rate to **9600** |
| `✓`, `→`, `°C` show as odd symbols | Cosmetic: your terminal is not using UTF-8. The data is still correct |

---

## Known issues and ideas

Things noticed while documenting the code:

- **Temperature byte overflow.** Temperature is sent as `temperature x 10` in a single byte, which only holds 0 - 255 (that is 0 - 25.5 deg C). Above 25.5 deg C the value wraps around and the receiver shows a wrong number. Fix: use two bytes (like voltage) or send the whole-degree value.
- **"Auto-detect crystal" does not really detect.** The fallback to 16 MHz only runs if the 8 MHz call returns an error, which normally does not happen, so the crystal setting must match your module.
- **`ACK_ID` (`0x101`) is unused.** A natural next step is for the receiver to send an acknowledgement and for the transmitter to report when none arrives.
- `status` and `power` are recomputed on the receiver, so they are not part of the CAN frame.

Ideas to extend the project:

- Add the acknowledgement frame on `0x101`.
- Split the data over several CAN IDs (for example voltage/current, charge/temperature, cycles).
- Add a CAN filter/mask so the receiver only accepts the BMS ID.
- Add over-temperature or low-charge alarms.
- Show the received values on an OLED or web page.

---
