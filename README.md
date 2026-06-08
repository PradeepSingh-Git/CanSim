# 🚌 CANSim

CAN Bus Simulator for **STM32F103C8T6** (Blue Pill).  
Transmits periodic CAN frames, prints received frames over a **USB CDC virtual COM port**, and pairs with a browser-based Web UI for live DBC-decoded traffic display and message transmission — no UART adapter, no native drivers.

---

## ✨ Features

| Feature | Detail |
|---------|--------|
| 🚗 **CAN bitrate** | 500 kbps (configurable in `MX_CAN_Init`) |
| 📦 **Default TX frame** | ID `0x123`, DLC 8, payload `DE AD BE EF 00 00 00 00`, 10 Hz |
| 🔌 **Host interface** | USB CDC virtual COM port (any baud setting is ignored) |
| ⌨️ **CLI commands** | `HELP STATUS CAN SET LISTEN STATS` |
| 👁️ **RX display** | Received frames printed when `LISTEN ON` |
| 🔀 **Pin conflict workaround** | CAN remapped to PB8/PB9 via AFIO; RX routed to FIFO1 to avoid shared USB/CAN IRQ |
| 🌐 **Web UI** | Single-file HTML app — loads DBC, decodes live traffic, transmits frames via Web Serial |
| 📋 **Demo DBC** | Automotive demo file (Engine, ABS, TCU, BMS, UDS) matching real frame formats |
| 🔌 **Arduino DUT** | Arduino Nano + MCP2515 sketch simulating a full drive-cycle for bench testing |

---

## 🔧 Hardware

| Part | Notes |
|------|-------|
| 🟦 STM32F103C8T6 Blue Pill | Main simulator board |
| 📡 TJA1051T CAN transceiver | 5 V bus; **tie S pin to GND** for normal mode; PB8 is 5 V-tolerant |
| 🔌 USB Mini-B cable | CDC virtual COM port to host |
| 🔩 120 Ω termination resistors | One at each physical end of the CAN bus |

> See [Tools/wiring_diagram.md](Tools/wiring_diagram.md) for full pin-out, block diagram, and TJA1051 connection details.

---

## 🚀 Quick Start

### 1 – Clone and import into STM32CubeIDE

```bash
git clone <repo-url>
```

In STM32CubeIDE: **File → New → STM32 Project → From an Existing .ioc File**  
Browse to `CANSim.ioc` → Finish → click **Generate Code** (Alt+G).

> The `Drivers/`, `Middlewares/`, and `USB_DEVICE/` folders are included in the repo so the project builds immediately after import. The `Debug/` build output folder is intentionally excluded.

### 2 – Build & Flash

| Action | How |
|--------|-----|
| 🔨 Build | Ctrl+B or Project → Build All |
| ⚡ Flash via ST-Link | Run → Debug (OpenOCD) |
| ⚡ Flash via DFU | Hold BOOT0 high on power-up, use STM32CubeProgrammer |

### 3 – Connect

Open the USB CDC port in any terminal at any baud (e.g. 115200 8N1).  
Type `HELP` to see available commands, or open [Tools/webui.html](Tools/webui.html) in Chrome/Edge for the graphical interface.

---

## 💻 CLI Reference

```
HELP
  Show available commands

STATUS
  Print current configuration and state

CAN ON / CAN OFF
  Start or stop periodic CAN frame transmission

SET ID <hex>          e.g.  SET ID 0x456
  Set the 11-bit standard frame ID

SET DLC <1-8>         e.g.  SET DLC 4
  Set data length

SET DATA <hex bytes>  e.g.  SET DATA 01 02 03 04
  Set payload (also updates DLC to the byte count given)

SET RATE <1-100>      e.g.  SET RATE 20
  Set TX rate in Hz

LISTEN ON / LISTEN OFF
  Enable or disable printing of received CAN frames

STATS
  Show TX frame count, RX frame count, TX error count

STATS RESET
  Reset all counters
```

### 📋 Example session

```
> STATUS
  TX:       OFF
  Listen:   OFF
  Bitrate:  500 kbps
  ID:       0x123
  DLC:      8
  Rate:     10 Hz
  Data:     DE AD BE EF 00 00 00 00

> SET ID 0x7DF
ID set to 0x7DF

> SET DATA 02 10 03 AA BB CC DD EE
Data set: 02 10 03 AA BB CC DD EE

> LISTEN ON
Listen ON

> CAN ON
CAN TX started

RX  ID:0x7DF  DLC:8  DATA: 02 10 03 AA BB CC DD EE

> STATS
  TX frames: 12
  RX frames: 12
  TX errors: 0
```

---

## 🌐 Web UI

Open **[Tools/webui.html](Tools/webui.html)** directly in Chrome or Edge (no server needed).

| Feature | Detail |
|---------|--------|
| 📂 **Load DBC** | Click Load DBC → select a `.dbc` file; signals appear in the sidebar |
| 🔗 **Connect** | Click Connect → pick the Blue Pill CDC port via the Web Serial picker |
| 📊 **Traffic view** | Last-Value mode (one row per ID, updated live) or Log mode (append every frame) |
| 🔍 **Signal decode** | Intel/Motorola byte order, factor/offset, unit; click any row to expand all signals |
| 📡 **Transmit** | Manual hex bytes or DBC-guided (select message, fill physical values, auto-encode) |
| 🖥️ **Console** | Raw serial I/O; "Show CAN frames" checkbox to opt-in to frame echo |

> Uses the **Web Serial API** — Chrome 89+ / Edge 89+ required. Not supported in Firefox or Safari.

---

## 📋 Demo DBC

**[Tools/demo.dbc](Tools/demo.dbc)** defines six automotive messages that the Arduino DUT transmits:

| ID | Name | Rate | Key Signals |
|----|------|------|-------------|
| `0x100` | EngineData | 10 ms | RPM (×0.25), ThrottlePos, CoolantTemp, EngineLoad |
| `0x200` | VehicleSpeed | 10 ms | VehicleSpeed (km/h ×0.01), WheelSpeed FL/FR/RL/RR |
| `0x300` | TransmissionData | 20 ms | GearPosition, SelectedGear, TransmissionTemp, Mode |
| `0x400` | BatteryStatus | 100 ms | BattVoltage, BattCurrent (signed), BattSOC, CellVolt Max/Min |
| `0x123` | CANSimHeartbeat | 100 ms | Counter, TestValue, Flags, Temperature, Voltage |
| `0x7DF` | DiagRequest | event | UDS ServiceID, SubFunction, DataIdentifier, RequestData |

---

## 🔌 Arduino DUT (Device Under Test)

**[Tools/CANSim_DUT/CANSim_DUT.ino](Tools/CANSim_DUT/CANSim_DUT.ino)** — Arduino Nano + MCP2515 sketch for bench testing.

### Hardware

| Pin | Signal |
|-----|--------|
| D10 | MCP2515 CS |
| D2 | MCP2515 INT |
| D11/D12/D13 | SPI (MOSI/MISO/SCK) |

### Library

Install **autowp/arduino-mcp2515** via the Arduino Library Manager (search `mcp2515`).  
Set `MCP_8MHZ` or `MCP_16MHZ` to match the crystal on your module.

### Behaviour

- Transmits `0x100 / 0x200 / 0x300 / 0x400` at configured rates with a simulated 80-second drive cycle (Idle → Accelerate → Cruise → Decelerate)
- Responds to UDS requests on `0x7DF`: Session Control (0x10), Tester Present (0x3E), Read Data By ID (0x22)
- Prints a status summary to Serial every 2 seconds

---

## 📁 Project Structure

```
CANSim/
├── CANSim.ioc                   CubeMX project — open in STM32CubeIDE
├── STM32F103C8TX_FLASH.ld       Linker script
├── Core/
│   ├── Inc/
│   │   ├── main.h               LED macros, Error_Handler declaration
│   │   ├── can_handler.h        CAN API
│   │   ├── cli.h                CLI API
│   │   └── usb_cdc_handler.h    USB CDC ring-buffer API
│   └── Src/
│       ├── main.c               Clock, GPIO, CAN init + main loop
│       ├── stm32f1xx_hal_msp.c  CAN GPIO / AFIO remap / NVIC setup
│       ├── stm32f1xx_it.c       USB & CAN interrupt vectors
│       ├── can_handler.c        CAN TX/RX logic, FIFO1 callback
│       ├── cli.c                USB CDC command parser
│       └── usb_cdc_handler.c    Ring buffer + CDC_Transmit_FS wrapper
├── Drivers/                     STM32F1 HAL + CMSIS (included, ready to build)
├── Middlewares/                 STM32 USB Device Library
├── USB_DEVICE/                  CDC class instance
└── Tools/
    ├── webui.html               Single-file Web UI (no dependencies)
    ├── demo.dbc                 Automotive demo DBC
    ├── pyserialScript.py        Python host CLI script
    ├── wiring_diagram.md        Pin-out and transceiver wiring
    └── CANSim_DUT/
        └── CANSim_DUT.ino       Arduino Nano + MCP2515 DUT sketch
```

---

## 🧠 Key Design Decisions

### 🔀 PA11/PA12 pin conflict: USB vs CAN

On STM32F103C8T6, PA11 = USB D− and PA12 = USB D+ — these are also the **default** CAN pins.  
`__HAL_AFIO_REMAP_CAN1_2()` in `stm32f1xx_hal_msp.c` remaps CAN to **PB8 (RX) / PB9 (TX)**.  
PB8 is 5 V-tolerant, so the TJA1051 TX line connects directly.

### ⚡ Shared IRQ: USB_LP and CAN FIFO0

`USB_LP_CAN1_RX0_IRQn` (vector 20) is shared between USB low-priority events and CAN FIFO0.  
CAN RX filters route all messages to **FIFO1**; `CAN1_RX1_IRQn` (vector 21) is exclusive to FIFO1 — no conflict.

### 🛡️ USB CDC reliability

Two failure modes discovered on Blue Pill hardware and mitigated:

| Symptom | Root Cause | Fix |
|---------|-----------|-----|
| CDC stops after USB reconnect | `TxState` stuck after USB glitch | `tx_dead` flag + 100 ms TX timeout in `CDC_Handler_Write`; cleared on `SET_CONTROL_LINE_STATE` |
| No RX data after reconnect | `USBD_CDC_ReceivePacket` fails to re-arm OUT endpoint | Retry once in `CDC_Receive_FS` |

Python script uses `time.sleep(2)` before asserting DTR to allow the CDC enumeration to complete on macOS.

### ⏱️ Bitrate (500 kbps)

```
APB1  = 36 MHz   (SYSCLK 72 MHz ÷ 2)
TQ    = 9 ÷ 36 MHz = 250 ns
Bit   = (1 + BS1=5 + BS2=2) × 250 ns = 2 µs → 500 kbps
SP    = 75 %
```

Adjust `Prescaler`, `BS1`, `BS2` in `MX_CAN_Init()` to change bitrate.

---

## 📋 Requirements

| Tool | Version |
|------|---------|
| 🛠️ STM32CubeIDE | ≥ 1.13 (includes CubeMX 6.x) |
| ⚙️ arm-none-eabi-gcc | Bundled with CubeIDE |
| 🔗 ST-Link v2 | Or compatible debugger/programmer |
| 🌐 Chrome / Edge | ≥ 89 for Web Serial API in the Web UI |
| 🐍 Python | ≥ 3.8 + `pyserial` for `pyserialScript.py` |
| 🔌 Arduino IDE | ≥ 1.8 + autowp/arduino-mcp2515 library for the DUT sketch |
