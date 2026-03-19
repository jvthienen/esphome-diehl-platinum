# Diehl Platinum Solar Inverter

The **Diehl Platinum** component allows you to read real-time data from a Diehl Platinum series solar inverter over its RS232 serial interface using an ESP32 running ESP-IDF via ESPHome.

This component communicates using the proprietary Diehl RS232 protocol, querying individual parameters (power, voltage, current, energy, temperatures, errors, etc.) and publishing them as ESPHome sensors.

## Supported Hardware

- **Diehl Platinum** series solar inverters (single-phase and three-phase models)
- **ESP32** (any variant; ESP-IDF framework required)
- **RS232-to-TTL converter** (e.g., MAX3232 module)

## Wiring / Hardware Connection

The Diehl Platinum inverter exposes an **RS232 serial port** (typically a DB9 or RJ45 connector depending on the model). The ESP32 uses **3.3V TTL-level UART**, so you **must** use an RS232-to-TTL level converter.

### Required Components

| Component | Description |
|---|---|
| ESP32 Dev Board | Any ESP32 board (e.g., ESP32-DevKitC, ESP32-WROOM-32) |
| MAX3232 Module | RS232 ↔ 3.3V TTL level converter |
| DB9 Cable / RJ45 Cable | To connect to the inverter's RS232 port |
| Jumper Wires | For connections between ESP32 and MAX3232 |

### Wiring Diagram

```
Diehl Platinum Inverter           MAX3232 Module              ESP32
┌─────────────────┐          ┌──────────────────┐       ┌──────────────┐
│  RS232 Port     │          │                  │       │              │
│  Pin 2 (RX) ◄───┼──────────┤ T1OUT    T1IN  ◄┼───────┤ GPIO17 (TX)  │
│  Pin 3 (TX) ────┼──────────► R1IN     R1OUT ──┼───────► GPIO16 (RX)  │
│  Pin 5 (GND) ───┼──────────┤ GND        GND ─┼───────┤ GND          │
│                 │          │            VCC ──┼───────┤ 3.3V         │
└─────────────────┘          └──────────────────┘       └──────────────┘
```

### Pin Reference

| Signal | Inverter DB9 Pin | MAX3232 Side | ESP32 Pin |
|---|---|---|---|
| TX (Inverter → ESP) | Pin 3 (TXD) | R1IN → R1OUT | GPIO16 (RX) |
| RX (ESP → Inverter) | Pin 2 (RXD) | T1OUT ← T1IN | GPIO17 (TX) |
| Ground | Pin 5 (GND) | GND | GND |
| Power | — | VCC | 3.3V |

> **⚠️ Important:** Never connect the ESP32 UART pins directly to the RS232 port! RS232 uses ±12V signaling which will damage the ESP32. Always use a MAX3232 or similar level converter.

> **💡 Tip:** If your inverter uses an RJ45 connector for RS232, consult the inverter manual for the pinout. The signals are the same (TX, RX, GND) but on different physical pins.

### Serial Parameters

| Parameter | Value |
|---|---|
| Baud Rate | 19200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None |

## ESPHome Configuration

### Basic Configuration

```yaml
esphome:
  name: diehl-inverter
  friendly_name: Diehl Platinum Inverter
  platformio_options:
    board_build.flash_mode: dio

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
  level: INFO  # Set to DEBUG for protocol troubleshooting

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: !secret api_encryption_key

ota:
  - platform: esphome
    password: !secret ota_password

external_components:
  - source: github://Roeland54/esphome-diehl-platinum@main
    components: [ diehl ]

uart:
  - id: uart_bus
    baud_rate: 19200
    tx_pin: GPIO17
    rx_pin: GPIO16

diehl_platinum:
  id: my_inverter
  uart_id: uart_bus
  update_interval: 10s

sensor:
  - platform: diehl
    diehl_platinum_id: my_inverter
    ac_power:
      name: "AC Power"
    dc_power:
      name: "DC Power"
    ac_voltage_phase1:
      name: "AC Voltage Phase 1"
    ac_current_phase1:
      name: "AC Current Phase 1"
    dc_voltage:
      name: "DC Voltage"
    dc_current:
      name: "DC Current"
    ac_frequency:
      name: "AC Frequency"
    energy_today:
      name: "Energy Today"
    energy_total:
      name: "Energy Total"
    hours_total:
      name: "Total Operating Hours"
    hours_today:
      name: "Operating Hours Today"
    temperature_1:
      name: "Inverter Temperature 1"
    temperature_2:
      name: "Inverter Temperature 2"
    temperature_3:
      name: "Inverter Temperature 3"
    insulation_resistance:
      name: "Insulation Resistance"

text_sensor:
  - platform: diehl
    diehl_platinum_id: my_inverter
    operating_state:
      name: "Operating State"
    serial_number:
      name: "Serial Number"
    error_status_1:
      name: "Error Status 1"
    error_status_2:
      name: "Error Status 2"
    error_source:
      name: "Error Source"

binary_sensor:
  - platform: diehl
    diehl_platinum_id: my_inverter
    connection_status:
      name: "Inverter Connection"
```

### Full Configuration (All Sensors)

```yaml
sensor:
  - platform: diehl
    diehl_platinum_id: my_inverter

    # Power
    ac_power:
      name: "AC Power"
    dc_power:
      name: "DC Power"

    # AC Voltage (all 3 phases)
    ac_voltage_phase1:
      name: "AC Voltage Phase 1"
    ac_voltage_phase2:
      name: "AC Voltage Phase 2"
    ac_voltage_phase3:
      name: "AC Voltage Phase 3"

    # AC Current (all 3 phases)
    ac_current_phase1:
      name: "AC Current Phase 1"
    ac_current_phase2:
      name: "AC Current Phase 2"
    ac_current_phase3:
      name: "AC Current Phase 3"

    # DC
    dc_voltage:
      name: "DC Voltage"
    dc_current:
      name: "DC Current"

    # Grid Frequency
    ac_frequency:
      name: "AC Frequency"

    # Energy
    energy_today:
      name: "Energy Today"
    energy_total:
      name: "Energy Total"

    # Operating Hours
    hours_total:
      name: "Total Operating Hours"
    hours_today:
      name: "Operating Hours Today"

    # Temperatures (up to 6 sensors)
    temperature_1:
      name: "Inverter Temperature 1"
    temperature_2:
      name: "Inverter Temperature 2"
    temperature_3:
      name: "Inverter Temperature 3"
    temperature_4:
      name: "Inverter Temperature 4"
    temperature_5:
      name: "Inverter Temperature 5"
    temperature_6:
      name: "Inverter Temperature 6"

    # Insulation
    insulation_resistance:
      name: "Insulation Resistance"

    # Power Reduction
    power_reduction_absolute:
      name: "Power Reduction Absolute"
    power_reduction_relative:
      name: "Power Reduction Relative"
    power_reduction_duration:
      name: "Power Reduction Duration"

text_sensor:
  - platform: diehl
    diehl_platinum_id: my_inverter
    operating_state:
      name: "Operating State"
    serial_number:
      name: "Serial Number"
    error_status_1:
      name: "Error Status 1"
    error_status_2:
      name: "Error Status 2"
    error_source:
      name: "Error Source"
    power_reduction_type:
      name: "Power Reduction Type"

binary_sensor:
  - platform: diehl
    diehl_platinum_id: my_inverter
    connection_status:
      name: "Inverter Connection"
```

## Configuration Variables

### Platform: `diehl_platinum`

- **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types#config-id)): Manually specify the component ID.
- **uart_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types#config-id)): The UART bus to use. Required if multiple UART buses are configured.
- **update_interval** (*Optional*, [Time](https://esphome.io/guides/configuration-types#config-time)): How often to poll the inverter. Defaults to `10s`.

### Sensor Platform

All sensors are optional. Only configured sensors will be queried from the inverter, minimizing bus traffic.

| Sensor Key | Description | Unit | Device Class |
|---|---|---|---|
| `ac_power` | Active AC output power | W | power |
| `dc_power` | DC input power from panels | W | power |
| `ac_voltage_phase1` | AC voltage on phase 1 | V | voltage |
| `ac_voltage_phase2` | AC voltage on phase 2 | V | voltage |
| `ac_voltage_phase3` | AC voltage on phase 3 | V | voltage |
| `ac_current_phase1` | AC current on phase 1 | A | current |
| `ac_current_phase2` | AC current on phase 2 | A | current |
| `ac_current_phase3` | AC current on phase 3 | A | current |
| `dc_voltage` | DC input voltage | V | voltage |
| `dc_current` | DC input current | A | current |
| `ac_frequency` | Grid frequency | Hz | frequency |
| `energy_today` | Energy produced today | Wh | energy |
| `energy_total` | Total energy produced (lifetime) | kWh | energy |
| `hours_total` | Total operating hours (lifetime) | h | duration |
| `hours_today` | Operating hours today | h | duration |
| `temperature_1` | Inverter temperature sensor 1 | °C | temperature |
| `temperature_2` | Inverter temperature sensor 2 | °C | temperature |
| `temperature_3` | Inverter temperature sensor 3 | °C | temperature |
| `temperature_4` | Inverter temperature sensor 4 | °C | temperature |
| `temperature_5` | Inverter temperature sensor 5 | °C | temperature |
| `temperature_6` | Inverter temperature sensor 6 | °C | temperature |
| `insulation_resistance` | DC insulation resistance | Ω | — |
| `power_reduction_absolute` | Absolute power reduction | W | power |
| `power_reduction_relative` | Relative power reduction | % | — |
| `power_reduction_duration` | Active power reduction duration | h | duration |

### Text Sensor Platform

| Sensor Key | Description | Icon |
|---|---|---|
| `operating_state` | Current operating state of the inverter (e.g., "Feeding In", "Night Mode") | mdi:state-machine |
| `serial_number` | Inverter serial number | mdi:identifier |
| `error_status_1` | Error register 1 | mdi:alert-circle-outline |
| `error_status_2` | Error register 2 | mdi:alert-circle-outline |
| `error_source` | Source module of last error | mdi:alert-decagram-outline |
| `power_reduction_type` | Type of active power reduction | mdi:arrow-down-bold-circle-outline |

### Binary Sensor Platform

| Sensor Key | Description | Device Class |
|---|---|---|
| `connection_status` | Whether communication with the inverter is active | connectivity |

The connection status turns **ON** when valid responses are received and **OFF** after 5 consecutive communication errors (timeouts, CRC failures, or error responses).

## Operating States

The inverter reports the following operating states:

| Code | State | Description |
|---|---|---|
| 0 | Initializing | Inverter is booting up |
| 1 | Waiting | Waiting for sufficient DC power |
| 10 | Checking DC | Verifying DC input parameters |
| 11 | Checking AC | Verifying grid parameters |
| 31 | Feeding In | Normal operation, feeding power to the grid |
| 32 | Reduced Power | Operating at reduced power (power limitation active) |
| 40 | Cooling Down | Shutting down, waiting for temperature to drop |
| 50 | Night Mode | No solar production, inverter is in standby |
| 60 | Error | An error has occurred |
| 70 | Derating | Power output is being derated (temperature/grid) |

## Debugging

Set the ESPHome logger to `DEBUG` to see all protocol traffic:

```yaml
logger:
  level: DEBUG
```

At **INFO** level, the component is silent during normal operation — no log output is produced unless an error occurs.

At **DEBUG** level, you will see:
- Every TX frame sent to the inverter (hex bytes)
- Every RX frame received (hex bytes)
- CRC validation results (pass/fail with values)
- Parsed values for each sensor
- Connection state changes
- Query cycle progress

Example debug output:
```
[D][diehl:178]: Starting update cycle, querying 15 values
[D][diehl:189]: Querying value type 0x05 (index 1/15)
[D][diehl:430]: TX: 35 13 01 05 A2 B7
[D][diehl:224]: RX [0x05]: 35 13 02 03 E8 xx xx (7 bytes)
[D][diehl:453]: CRC OK: 0xA2B7
[D][diehl:284]: AC Power: 100.0 W
```

## Protocol Reference

This section documents the Diehl Platinum RS232 communication protocol in full detail.

### Physical Layer

| Parameter | Value |
|---|---|
| Interface | RS232 (DB9 or RJ45) |
| Baud Rate | 19200 bps |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None |
| Voltage Levels | ±12V (RS232 standard) |

### Protocol Overview

The protocol is a **master-slave, request-response** protocol. The ESP32 (master) sends a request frame to the inverter (slave), and the inverter responds with a data frame. Communication is **half-duplex**: the master sends a request and then waits for the response before sending the next request.

All multi-byte integer values are encoded in **big-endian** (network byte order).

### Frame Structure

Every frame (both request and response) follows this general structure:

```
┌──────────┬─────────┬────────────┬───────────────┬──────────┐
│ Header   │ Address │ Length     │ Payload       │ CRC-16   │
│ (1 byte) │ (1 byte)│ (1 byte)  │ (N bytes)     │ (2 bytes)│
└──────────┴─────────┴────────────┴───────────────┴──────────┘
```

| Field | Size | Description |
|---|---|---|
| Header | 1 byte | Command/response type identifier |
| Address | 1 byte | Always `0x13` (19) — inverter address |
| Length | 1 byte | Number of payload bytes that follow |
| Payload | N bytes | Command parameters or response data |
| CRC-16 | 2 bytes | CRC-16 checksum over all preceding bytes (big-endian) |

### CRC-16 Checksum

The checksum is a **CRC-16/XMODEM** variant with polynomial `0x1021` and initial value `0xFFFF`.

**Algorithm (pseudocode):**

```
function calc_crc16(data, length):
    crc = 0xFFFF
    for i = 0 to length - 1:
        index = data[i] XOR (crc >> 8)
        crc = CRC16_TABLE[index] XOR (crc << 8)
        crc = crc AND 0xFFFF
    return crc
```

The CRC is computed over **all bytes preceding it** in the frame (header + address + length + payload). The result is appended as 2 bytes in **big-endian** order (high byte first).

**Validation:** On reception, compute the CRC over all bytes except the last two and compare with the received CRC.

### Request Types

#### getValue Request (Single Parameter Query)

Used to request a single real-time value from the inverter.

**Request frame:**

```
┌──────┬──────┬──────┬───────────┬──────────┐
│ 0x35 │ 0x13 │ 0x01 │ ValueType │ CRC (2B) │
└──────┴──────┴──────┴───────────┴──────────┘
  53     19      1     (see table)
```

| Byte | Value | Description |
|---|---|---|
| 0 | `0x35` (53) | getValue command header |
| 1 | `0x13` (19) | Inverter address |
| 2 | `0x01` (1) | Payload length (1 byte follows) |
| 3 | ValueType | The parameter to query (see Value Type table) |
| 4-5 | CRC-16 | Checksum of bytes 0–3 |

**Total frame size: 6 bytes**

#### getDataIntvData Request (Interval/Detail Data)

Used to request historical interval data for a specific day and index.

**Request frame:**

```
┌──────┬──────┬──────┬──────┬──────┬───────┬───────┬──────┬──────┬──────────┐
│ 0x2D │ 0x13 │ 0x06 │ 0x12 │ Year │ Month │ Day   │ IdxH │ IdxL │ CRC (2B) │
└──────┴──────┴──────┴──────┴──────┴───────┴───────┴──────┴──────┴──────────┘
  45     19      6      18
```

| Byte | Value | Description |
|---|---|---|
| 0 | `0x2D` (45) | Data request command header |
| 1 | `0x13` (19) | Inverter address |
| 2 | `0x06` (6) | Payload length |
| 3 | `0x12` (18) | Sub-command: interval data |
| 4 | Year | Year minus 2000 (e.g., 24 for 2024) |
| 5 | Month | Month (1–12) |
| 6 | Day | Day of month (1–31) |
| 7 | Index High | Day index high byte |
| 8 | Index Low | Day index low byte |
| 9-10 | CRC-16 | Checksum of bytes 0–8 |

**Total frame size: 11 bytes**

**Interval data response payload:**

| Offset | Size | Description | Encoding |
|---|---|---|---|
| 4 | 1 | Hour | Integer |
| 5 | 1 | Minute | Integer |
| 6 | 1 | Second | Integer |
| 7 | 1 | Status code | See Operating States |
| 8 | 1 | Event code | Integer |
| 9–10 | 2 | DC Voltage | Big-endian, integer V |
| 11 | 1 | DC Current | ÷ 10 → A |
| 12–13 | 2 | DC Power | ÷ 10 → W |
| 14–15 | 2 | AC Voltage | Big-endian, integer V |
| 16 | 1 | AC Current | ÷ 10 → A |
| 17–18 | 2 | AC Power | ÷ 10 → W |
| 19–22 | 4 | Energy Day | Big-endian, integer Wh |
| 23 | 1 | Temperature 1 | Integer °C |
| 24 | 1 | Temperature 2 | Integer °C |
| 25 | 1 | Temperature 3 | Integer °C |
| 26–27 | 2 | Unknown | Big-endian |

#### getDataDayData Request (Daily Summary)

Used to request the summary data for a specific day.

**Request frame:**

```
┌──────┬──────┬──────┬──────┬──────┬───────┬──────┬──────────┐
│ 0x2D │ 0x13 │ 0x04 │ 0x13 │ Year │ Month │ Day  │ CRC (2B) │
└──────┴──────┴──────┴──────┴──────┴───────┴──────┴──────────┘
  45     19      4      19
```

| Byte | Value | Description |
|---|---|---|
| 0 | `0x2D` (45) | Data request command header |
| 1 | `0x13` (19) | Inverter address |
| 2 | `0x04` (4) | Payload length |
| 3 | `0x13` (19) | Sub-command: day summary |
| 4 | Year | Year minus 2000 |
| 5 | Month | Month (1–12) |
| 6 | Day | Day of month (1–31) |
| 7-8 | CRC-16 | Checksum of bytes 0–6 |

**Total frame size: 9 bytes**

**Day summary response payload:**

| Offset | Size | Description | Encoding |
|---|---|---|---|
| 4 | 1 | Year (YY) | + 2000 |
| 5 | 1 | Month | Integer |
| 6 | 1 | Day | Integer |
| 7–8 | 2 | DC Voltage Peak | Big-endian, integer V |
| 9–10 | 2 | DC Voltage Mean | Big-endian, integer V |
| 11 | 1 | DC Current Peak | ÷ 10 → A |
| 12 | 1 | DC Current Mean | ÷ 10 → A |
| 13–14 | 2 | DC Power Peak | ÷ 10 → W |
| 15–16 | 2 | DC Power Mean | ÷ 10 → W |
| 17–18 | 2 | AC Voltage Peak | Big-endian, integer V |
| 19–20 | 2 | AC Voltage Mean | Big-endian, integer V |
| 21 | 1 | AC Current Peak | ÷ 10 → A |
| 22 | 1 | AC Current Mean | ÷ 10 → A |
| 23–24 | 2 | AC Power Peak | ÷ 10 → W |
| 25–26 | 2 | AC Power Mean | ÷ 10 → W |
| 29–30 | 2 | Energy Day | Big-endian, Wh |
| 31 | 1 | Unknown 1 | |
| 32 | 1 | Unknown 2 | |
| 33 | 1 | Unknown 3 | |
| 34 | 1 | Unknown 4 | |
| 35 | 1 | Unknown 5 | |
| 36 | 1 | Unknown 6 | |
| 37–40 | 4 | Energy Total | Big-endian, Wh |
| 41–44 | 4 | Temperature Peak | Big-endian |

### Value Type Codes

These codes are used in the `getValue` request (byte 3) to select which parameter to read:

| Code (Hex) | Code (Dec) | Short Name | Description | Response Encoding |
|---|---|---|---|---|
| `0x00` | 0 | STATE | Operating state | 1 byte, state code |
| `0x01` | 1 | h_TOTAL | Total operating hours | 2–4 bytes, integer hours |
| `0x02` | 2 | h_ON | Active hours today | 2 bytes, ÷10 → hours |
| `0x03` | 3 | E_TOTAL | Total energy (lifetime) | 4 bytes, Wh (÷1000 → kWh) |
| `0x04` | 4 | E_DAY | Energy today | 2–4 bytes, Wh |
| `0x05` | 5 | P_AC | AC active power | 2 bytes, ÷10 → W |
| `0x06` | 6 | P_DC | DC power | 2 bytes, ÷10 → W |
| `0x07` | 7 | F_AC | Grid frequency | 2 bytes, ÷10 → Hz |
| `0x08` | 8 | U_AC_1 | AC voltage phase 1 | 2 bytes, integer V |
| `0x09` | 9 | U_AC_2 | AC voltage phase 2 | 2 bytes, integer V |
| `0x0A` | 10 | U_AC_3 | AC voltage phase 3 | 2 bytes, integer V |
| `0x0B` | 11 | I_AC_1 | AC current phase 1 | 1–2 bytes, ÷10 → A |
| `0x0C` | 12 | I_AC_2 | AC current phase 2 | 1–2 bytes, ÷10 → A |
| `0x0D` | 13 | I_AC_3 | AC current phase 3 | 1–2 bytes, ÷10 → A |
| `0x0E` | 14 | U_DC | DC voltage | 2 bytes, integer V |
| `0x0F` | 15 | I_DC | DC current | 1–2 bytes, ÷10 → A |
| `0x10` | 16 | Red_absolut | Absolute power reduction | 2 bytes, integer W |
| `0x11` | 17 | Red_activ | Power reduction duration | 2 bytes, ÷10 → hours |
| `0x12` | 18 | Red_relativ | Relative power reduction | 2 bytes, ÷10 → % |
| `0x13` | 19 | Red_Type | Power reduction type | 1–2 bytes, code |
| `0x14` | 20 | T_WR_1 | Temperature sensor 1 | 1 byte, integer °C |
| `0x15` | 21 | T_WR_2 | Temperature sensor 2 | 1 byte, integer °C |
| `0x16` | 22 | T_WR_3 | Temperature sensor 3 | 1 byte, integer °C |
| `0x17` | 23 | T_WR_4 | Temperature sensor 4 | 1 byte, integer °C |
| `0x18` | 24 | T_WR_5 | Temperature sensor 5 | 1 byte, integer °C |
| `0x19` | 25 | T_WR_6 | Temperature sensor 6 | 1 byte, integer °C |
| `0x1A` | 26 | R_ISO | Insulation resistance | 2 bytes, integer Ω |
| `0x1B` | 27 | E1 | Error status register 1 | 1–2 bytes, code |
| `0x1C` | 28 | E2 | Error status register 2 | 1–2 bytes, code |
| `0x1D` | 29 | E_S | Error source | 1–2 bytes, code |
| `0x1E` | 30 | Sn | Serial number | N bytes, ASCII or binary |

### Response Structure

**Successful response:**

```
┌──────────┬─────────┬────────────┬───────────────┬──────────┐
│ Header   │ Address │ Length     │ Payload       │ CRC-16   │
│ (echo)   │ 0x13    │ N          │ (N bytes)     ��� (2 bytes)│
└──────────┴─────────┴────────────┴───────────────┴──────────┘
```

The header byte in the response echoes the request header. The address is always `0x13`. The length byte indicates how many payload bytes follow. The CRC is computed over all bytes before it.

**Error/End-of-list response:**

When a request fails or data is unavailable, the inverter responds with a single-byte header of `0x84` (132):

```
┌──────┬──────────┐
│ 0x84 │ ...      │
└──────┴──────────┘
```

This response indicates:
- The requested value type is not supported
- The requested data index is beyond the available range
- A communication error occurred in the inverter

### Communication Flow Example

**Requesting AC Power (P_AC):**

1. **Master sends** (6 bytes):
   ```
   35 13 01 05 [CRC_H] [CRC_L]
   ```
   - `0x35` = getValue command
   - `0x13` = address
   - `0x01` = 1 byte payload
   - `0x05` = P_AC value type

2. **Slave responds** (7 bytes):
   ```
   35 13 02 03 E8 [CRC_H] [CRC_L]
   ```
   - `0x35` = echoed header
   - `0x13` = address
   - `0x02` = 2 bytes payload
   - `0x03 0xE8` = 1000 → 1000 ÷ 10 = **100.0 W**

### Timing

- **Inter-query delay:** The component waits for a response (up to 500ms timeout) before sending the next request.
- **Response time:** The inverter typically responds within 50–200ms.
- **Update interval:** Configurable (default 10s). All configured sensors are queried sequentially during each update cycle.

## Troubleshooting

| Symptom | Possible Cause | Solution |
|---|---|---|
| No data / connection always OFF | Wiring incorrect | Check TX/RX are not swapped; verify GND connection |
| No data / connection always OFF | Missing level converter | RS232 requires MAX3232; do not connect directly |
| CRC errors in debug log | Noise on the line | Use shielded cable; keep cable short |
| CRC errors in debug log | Baud rate mismatch | Verify `baud_rate: 19200` |
| Some sensors show NaN | Inverter doesn't support that parameter | Not all models support all value types; remove unsupported sensors |
| Values seem wrong | Single-phase inverter with 3-phase config | Only use `phase1` sensors for single-phase inverters |
| Connection drops intermittently | Update interval too fast | Increase `update_interval` to 15s or 30s |
| 0x84 responses for all queries | Inverter in night mode | Normal; inverter may not respond when sleeping |

## See Also

- [UART Bus](https://esphome.io/components/uart)
- [Sensor Component](https://esphome.io/components/sensor/)
- [Text Sensor Component](https://esphome.io/components/text_sensor/)
- [Binary Sensor Component](https://esphome.io/components/binary_sensor/)
