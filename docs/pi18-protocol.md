# PI18 protocol as MPP Solar's SolarPower uses it

This describes the PI18 serial protocol the way MPP Solar's own monitoring app,
SolarPower 1.23SP3, speaks it, with notes for the LV2424. It was recovered by
decompiling the app's Java classes (static analysis only; nothing from the
package was run). It shows what the vendor sends and expects. The inverter
firmware can still refuse a command: an LV2424 answers `^0` to `LON0`, which
the app offers.

Field numbers count from 0 after the `^Dnnn` header. "÷10" means the field is
sent in tenths.

## Framing

| | Format |
|---|---|
| Serial | 2400 baud, 8N1 (the app has a 1200-baud fallback) |
| Query | `^P` + length + payload + CRC + CR |
| Set command | `^S` + length + payload + CRC + CR |
| Data reply | `^D` + length + data + CRC + CR |
| Set reply | `^1` accepted, `^0` refused, each + CRC + CR |
| Length | 3 digits: payload length + 3 (two CRC bytes and CR) |
| CRC | CRC-16/XMODEM over everything before it; a CRC byte equal to `0x28`, `0x0D` or `0x0A` is incremented by one |

Examples from `docs/pdus/pi18/lv5048.txt`: the `GS` query is `^P005GS` +
`58 14` + CR, and a `MOD` reply is `^D00503` + `B9 59` + CR (mode 03, battery).
The app waits up to 3 s for CR and retries up to 3 times on an empty reply or
NAK.

## Model classes

The app picks a class per inverter:

| Class | Match |
|---|---|
| `P181TO2K` ("P1812K") | `PI` starts with `18`, `ID` returns a serial, PIRI field 5 (rated VA) < 3000. The LV2424 (2400 VA) lands here. |
| `P183TO4K` | same, PIRI field 5 from 3000 to 4999 |
| `P18` | `GMN` in 0, 2, 4, 6–9, 12, 13, `LST` empty or NAK, PIRI field 5 ≥ 5000 |

For `P1812K` each poll sends `GS`, then `FWS`, `GMN` and `MOD`. It never sends
the parallel queries (`PRI`, `PGS`, `PS`) or `LOADPR`. Settings are refreshed
with `FLAG`, `PIRI`, `MCHGCR`, `LST`, `MUCHGCR`, `ACCT`, `ACLT` and `LED`;
device information with `VFW` and `PIRI`; factory defaults with `DI`.

## Queries

### `GS` general status

| # | Meaning | Unit |
|---|---|---|
| 0 | Grid voltage | ÷10 V |
| 1 | Grid frequency | ÷10 Hz |
| 2 | AC output voltage | ÷10 V |
| 3 | AC output frequency | ÷10 Hz |
| 4 | AC output apparent power | VA |
| 5 | AC output active power | W |
| 6 | Output load | % |
| 7 | Battery voltage | ÷10 V |
| 8 | Battery voltage from SCC1 | ÷10 V |
| 9 | Battery voltage from SCC2 | ÷10 V |
| 10 | Battery discharge current | A |
| 11 | Battery charge current | A |
| 12 | Battery capacity | % |
| 13 | Heat sink temperature | °C |
| 14 | MPPT1 charger temperature | °C |
| 15 | MPPT2 charger temperature | °C |
| 16 | PV1 input power | W |
| 17 | PV2 input power | W |
| 18 | PV1 input voltage | ÷10 V |
| 19 | PV2 input voltage | ÷10 V |
| 20 | Not read by the app (this component publishes it as `setting_value_configuration_state`) | |
| 21 | MPPT1 work status (raw code) | |
| 22 | MPPT2 work status (raw code) | |
| 23 | Load connected (≠ 0) | |
| 24 | Battery status (raw code) | |
| 25 | DC/AC direction; the app shows 0 unless PV1 ≥ 90 V or PV2 > 90 V | |
| 26 | Line direction | |
| 27 | Parallel ID | |

When FWS field 17 is 1 (battery open), the app shows battery voltage, capacity
and status as 0.

### `PIRI` rated information (26 fields)

| # | Meaning | Values |
|---|---|---|
| 0 | Grid rating voltage | ÷10 V |
| 1 | Grid rating current | ÷10 A |
| 2 | AC output rating voltage | ÷10 V |
| 3 | AC output rating frequency | ÷10 Hz |
| 4 | AC output rating current | ÷10 A |
| 5 | AC output rating apparent power (picks the model class) | VA |
| 6 | AC output rating active power | W |
| 7 | Battery rating voltage | ÷10 V |
| 8 | Battery recharge (back to utility) voltage | ÷10 V |
| 9 | Battery redischarge (back to battery) voltage | ÷10 V |
| 10 | Battery under voltage | ÷10 V |
| 11 | Battery bulk voltage | ÷10 V |
| 12 | Battery float voltage | ÷10 V |
| 13 | Battery type | 0 AGM, 1 Flooded, 2 User, 3 PYLON, 5 WECO, 6 Soltaro, 8 Lib, 9 Lic |
| 14 | Max AC charging current | A |
| 15 | Max charging current | A |
| 16 | Input voltage range | 0 Appliance, 1 UPS |
| 17 | Output source priority | 0 Solar-Utility-Battery, 1 Solar-Battery-Utility |
| 18 | Charger source priority | 0 Solar first, 1 Solar and Utility, 2 Solar only |
| 19 | Max parallel units | |
| 20 | Machine type (flag I) | 0 Off-grid, 1 Hybrid |
| 21 | Topology | 0 Transformerless, 1 Transformer |
| 22 | Output mode | 0 Single, 1 Parallel, 2–4 three-phase R/S/T, 5–6 split-phase 1/2 |
| 23 | Solar power priority | 0 Battery-Load-Utility, 1 Load-Battery-Utility |
| 24 | MPPT track count | |
| 25 | Regulation standard | 00 India, 01 Germany, 02 South America, 03 NRS097, 04 IEC62116 |

### `FWS` fault and warning status

| # | Meaning |
|---|---|
| 0 | Fault code (see the table below) |
| 1–16 | Warnings: line fail, output circuit short, inverter over temperature, fan lock, battery voltage high, battery low, battery under, overload, EEPROM fail, power limit, PV voltage high, PV2 voltage high, MPPT overload, MPPT2 overload, battery too low to charge for SCC, for SCC2 |
| 17 | Battery connection is open (app code 1056) |

### `FLAG` enable flags

| # | Flag | Letter for `PE`/`PD` |
|---|---|---|
| 0 | Buzzer on (the app's "Mute buzzer alarm" is this flag inverted) | A |
| 1 | Overload bypass when utility is available | B |
| 2 | LCD escape to default page after 1 min | C |
| 3 | Overload auto restart | D |
| 4 | Over temperature auto restart | E |
| 5 | Backlight on | F |
| 6 | Beeps while primary source interrupt | G |
| 7 | Fault code record | H |
| 8 | Not read by the app; meaning unconfirmed | |
| 9 | BMS connected (longer replies only) | |
| 12 | Generator charging enable | M |
| 14, 15 | Dry contact enable, battery feed into grid | N, O |

Flag I is the machine type (PIRI field 20), not a FLAG field.

### `MOD` working mode

`00` Power on, `01` Standby, `02` Bypass, `03` Battery, `04` Fault, `05` Hybrid,
`06` Charge. An LV2424 also reports `07` just before it powers off.

### Other queries

| Query | Reply |
|---|---|
| `PI` | Protocol ID, `18` |
| `ID` | Two-digit length n, then the serial number (n characters) |
| `GMN` | Model number |
| `VFW` | Main CPU firmware, slave 1 firmware, slave 2 firmware |
| `DI` | Factory defaults: output V ÷10, output Hz ÷10, input range, under V ÷10, float V ÷10, bulk V ÷10, recharge V ÷10, redischarge V ÷10, max charging A, max AC charging A, battery type, output source, charger source, solar priority, machine type, output mode, then flags A, D, E, F, G, H, B, C. Float comes before bulk and charging before AC charging, the reverse of PIRI. |
| `MCHGCR` | Every allowed value for the max charging current |
| `MUCHGCR` | Every allowed value for the max AC charging current |
| `ET` | Total generated energy (Wh on an LV2424) |
| `EY`yyyy, `EM`yyyyMM, `ED`yyyyMMdd | Energy for that year, month or day |
| `T` | Inverter clock, `yyyyMMddHHmmss` |
| `LST` | LCD sleep time |
| `ACCT`, `ACLT` | AC charge start and end time; AC output start and end time |

## Set commands

Payloads follow `^Snnn`. Values marked ×10 are sent in tenths.

| Payload | Effect |
|---|---|
| `LON1`, `LON0` | AC output on, off (an LV2424 refuses `LON0`) |
| `PE`x, `PD`x | Enable, disable flag x (A–H as in FLAG; I is machine type Hybrid/Off-grid; S is the second AC output) |
| `POP`n | Output source priority, 0 or 1 |
| `PSP`n | Solar power priority, 0 or 1 |
| `PCP`k,n | Charger source priority; k is the parallel index (0 for one unit) |
| `MCHGC`k,nnn | Max charging current, A |
| `MUCHGC`k,nnn | Max AC charging current, A |
| `MCHGV`bbb,fff | Bulk and float voltage ×10, always both |
| `BUCD`rrr,ddd | Back-to-utility and back-to-battery voltage ×10 |
| `PSDV`nnn | Battery cut-off voltage ×10 |
| `PBT`n | Battery type: 0 AGM, 1 Flooded, 2 User |
| `PGR`n | Input range: 0 Appliance, 1 UPS |
| `V`nnnn | Output voltage ×10 |
| `F`nn | Output frequency, 50 or 60 |
| `RS`n | Regulation standard |
| `DAT`yyMMddHHmmss | Set the inverter clock |
| `LST`nn | LCD sleep time |
| `PF` | Restore factory defaults |
| `CLE` | Clear stored energy data |
| `BST` | Activate a Li-Fe battery now |

For the 1–2 kW class the app's settings page offers `RS`, `PCP`, `POP`, `PSP`,
`MCHGC`, `MUCHGC`, `PGR`, `PBT`, `V`, `F` and `BUCD`. For a 24 V battery bank it
offers back-to-utility 22.0–25.5 V and back-to-battery FULL or 24.0–29.0 V, in
0.5 V steps. The app knows about 70 more set commands (generator, second
output, dry contact, LED ring, grid-tie and EN 50549 limits) that it doesn't
offer to this class.

## Fault text for the 1–2 kW class

| Code | Fault | Code | Fault |
|---|---|---|---|
| 1 | Fan is locked | 51 | Over current inverter |
| 2 | Over temperature | 52 | Bus soft start failed |
| 3 | Battery voltage is too high | 53 | Inverter soft start failed |
| 4 | Battery voltage is too low | 54 | Self-test failed |
| 5 | Output short circuited or over temperature | 55 | Over DC voltage on output of inverter |
| 6 | Output voltage is too high | 56 | Battery connection is open |
| 7 | Over load time out | 57 | Current sensor failed |
| 8 | Bus voltage is too high | 58 | Output voltage is too low |
| 9 | Bus soft start failed | 60 | Inverter negative power |
| 10 | PV current over | 71 | Parallel version different |
| 11 | PV voltage over | 72 | Output circuit failed |
| 12 | Charge current over | 80–86 | Parallel faults |

Codes 10–12 differ from the app's generic P18 table, where 11 is "Main relay
failed".

## Not implemented by this component

`ED`/`EM`/`EY`, `VFW`, `DI`, `MCHGCR`/`MUCHGCR`, `GMN`, `T`/`DAT`, PIRI field 25,
fault code text, and the set commands from `BUCD` down in the table above.

## Source and limits

- **Source:** `SolarPower_MAC_ARM.zip` from mppsolar.com (SolarPower 1.23SP3,
  sha256 `16820aabff29dc290af7de6eb952e244ae9b2f258958b85f362d2389582699eb`).
  The protocol code is in `lib/SolarPower.jar`, under
  `cn.com.voltronic.solar.{protocol,comusbprocessor,control,communicate}`.
  The fault text is in `config/Events_P181TO2K.xml`.
- **Method:** the classes were decompiled with Ghidra 12.2 (headless, JVM loader).
- **Not decompiled:** the shared base processor, the model helper class and the
  scheduler. Poll intervals, the energy-value scaling and some model gates are
  therefore inferred.
- **Missing labels:** the app has no names for GS fields 21, 22, 24 and 25.
