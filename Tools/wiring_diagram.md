# CANSim – Wiring & Hardware Notes

## Blue Pill (STM32F103C8T6) Pin Assignment

```
Blue Pill                     Function
─────────────────────────────────────────────────────
PA11   USB D-  ──────────────► USB connector (D-)
PA12   USB D+  ──────────────► USB connector (D+)

PB8    CAN_RX  ──────────────► CAN transceiver CRXD / RXD
PB9    CAN_TX  ──────────────► CAN transceiver CTXD / TXD

PC13   LED     ──────────────► Onboard LED (active-low)

3.3 V  VCC     ──────────────► CAN transceiver VCC (if 3.3 V type)
GND    GND     ──────────────► CAN transceiver GND
```

> **Why PB8/PB9?**  
> The default CAN pins (PA11 / PA12) are the same physical pads as USB D− / D+.
> `__HAL_AFIO_REMAP_CAN1_2()` switches CAN to the alternate mapping on PB8/PB9,
> so both USB and CAN can be active at the same time.

---

## Recommended CAN Transceiver

| Part        | Supply | Notes                                   |
|-------------|--------|-----------------------------------------|
| SN65HVD230  | 3.3 V  | Preferred – directly 3.3 V compatible  |
| MCP2551     | 5 V    | Requires 5 V; add level-shifter or use the 5 V-tolerant I/O trick |
| TJA1050     | 5 V    | Same caution as MCP2551                 |

### SN65HVD230 Wiring (recommended)

```
Blue Pill 3.3 V ──── VCC (pin 3)
Blue Pill GND   ──── GND (pin 2)
PB9  (CAN_TX)   ──── D / TXD (pin 1)
PB8  (CAN_RX)   ──── R / RXD (pin 4)

CAN bus:
  CANH ──── CAN bus HIGH wire
  CANL ──── CAN bus LOW  wire

Bus termination (required at each physical end of the bus):
  120 Ω between CANH and CANL
```

### MCP2551 Wiring (5 V, with caution)

```
Blue Pill 5 V   ──── VDD (pin 3)
Blue Pill GND   ──── VSS (pin 4) + RS (pin 8) to GND
PB9  (CAN_TX)   ──── TXD (pin 1)   ← 5 V-tolerant I/O on F103, OK
PB8  (CAN_RX)   ──── RXD (pin 4)   ← output is 5 V; add 10 kΩ pull-down
                                        or use a voltage divider to stay ≤ 3.6 V
CANH            ──── CAN bus HIGH
CANL            ──── CAN bus LOW
```

---

## USB Pull-up

The Blue Pill board has a **fixed 1.5 kΩ pull-up** resistor on PA12 (USB D+),
which signals Full-Speed USB to the host. No external pull-up is needed.  
Some clone boards omit this resistor – add 1.5 kΩ between PA12 and 3.3 V if
the device is not detected.

---

## Loopback Testing (no transceiver needed)

Set `CAN_MODE_LOOPBACK` in `MX_CAN_Init()` to test the firmware without
connecting any bus hardware:

```c
hcan.Init.Mode = CAN_MODE_LOOPBACK;   /* replace CAN_MODE_NORMAL */
```

Transmitted frames will be received back by the same peripheral; enable
`LISTEN ON` in the CLI to see them.

---

## Block Diagram

```
 ┌──────────────────────────────────────────────────┐
 │              STM32F103C8T6 (Blue Pill)           │
 │                                                  │
 │  PA11 ◄─── USB D─ ────────────────┐             │
 │  PA12 ◄─── USB D+ ──┬─1.5kΩ─3V3  │             │
 │              (USB FS CDC VCP)      │             │
 │                                    ▼             │
 │                               [USB Mini-B]       │
 │                                    │             │
 │                            Host PC / laptop      │
 │                                                  │
 │  PB8  ──► CAN_RX ──┐                            │
 │  PB9  ◄── CAN_TX ──┤  [SN65HVD230]             │
 │                     └──── CANH ────┬──── bus     │
 │                          CANL ─────┘             │
 │                           120Ω terminator        │
 └──────────────────────────────────────────────────┘
```

---

## IRQ Priority Map

| Priority | Peripheral       | IRQ vector                |
|----------|-----------------|---------------------------|
| 3        | USB low-priority | `USB_LP_CAN1_RX0_IRQn`   |
| 5        | CAN FIFO1 RX    | `CAN1_RX1_IRQn`           |
| 15       | SysTick (HAL)   | `SysTick_IRQn`            |

`USB_LP_CAN1_RX0_IRQn` is shared between USB_LP events and CAN FIFO0.
By routing all CAN RX to FIFO1, the `CAN1_RX1_IRQn` vector (exclusive to CAN)
is used instead, eliminating any shared-IRQ conflict.
