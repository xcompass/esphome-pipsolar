# PI18 protocol as MPP Solar's SolarPower uses it

This describes the PI18 serial protocol the way MPP Solar's own monitoring app,
SolarPower 1.23SP3, speaks it, with notes for the LV2424. It was recovered by
decompiling the app with JADX and Ghidra (static analysis only; nothing from
the package was run). It shows what the vendor sends and expects. The inverter
firmware can still refuse a command: an LV2424 answers `^0` to `LON0`, which
the app offers.

Field numbers count from 0 after the `^Dnnn` header. "÷10" means the field is
sent in tenths.

## Framing

| | Format |
|---|---|
| Serial | 2400 baud, 8N1. Device discovery retries at 9600 baud if nothing answers. |
| Query | `^P` + length + payload + CRC + CR |
| Set command | `^S` + length + payload + CRC + CR |
| Data reply | `^D` + length + data + CRC + CR |
| Set reply | `^1` accepted, `^0` refused, each + CRC + CR |
| Length | 3 digits: payload length + 3 (two CRC bytes and CR) |
| CRC | CRC-16/XMODEM over everything before it; a CRC byte equal to `0x28`, `0x0D` or `0x0A` is incremented by one |

Examples from `docs/pdus/pi18/lv5048.txt`: the `GS` query is `^P005GS` +
`58 14` + CR, and a `MOD` reply is `^D00503` + `B9 59` + CR (mode 03, battery).

The fault and warning query is `^P006FWS` (`FWS` is 3 characters). This
component sent `^P005FWS` until November 2026. The LV2424 answers that with a
reply that carries no data, so the fault code and every FWS warning stayed unknown.

The app waits up to 3 s for CR and sends each command at most 3 times in
total. It sends again when the reply is empty, fails the CRC, or is a NAK
(`^0`); that includes set commands the inverter refused.

This component logs a refused query (`^0`) as a warning and handles it like a
timeout, so the readings it feeds become unknown. Like the app, it tries a
refused query at most 3 times per update interval.

## Model classes

The app probes PI18 classes in this order and uses the first that matches:

| Class (protocol ID) | Match |
|---|---|
| `P18` ("P18") | `GMN` in 0, 2, 4, 6–9, 12, 13 (a NAK parses as 0), `LST` empty or NAK, `PI` starts with `18`, `ID` returns a serial, PIRI field 5 (rated VA) ≥ 5000 |
| `P18VII6k` ("P18") | `PI`, `ID`, PIRI field 5 < 3000, except exactly 2000 VA with PIRI field 19 = 0 |
| `P183TO4K` ("P1834K") | `PI`, `GMN`, `ID`, PIRI field 5 from 3000 to 4999 |
| `P181TO2K` ("P1812K") | `PI`, `ID`, PIRI field 5 < 3000 (in practice only 2000 VA units with PIRI field 19 = 0 get here) |

**The LV2424:** a unit monitored with this component reports PIRI field 5 =
3000 VA, field 19 = 9 and field 22 = 0. It therefore matches `P183TO4K`
("P1834K"), which uses the generic P18 processor and the
`Events_P181TO2K.xml` fault text.

The app's model number gates many features. `P183TO4K` reads it from `GMN`.
The two small classes never send `GMN`, so they keep the default model number
14. The app then treats them as a "V4 WP Plus" model: it polls `HFS00`–`HFS08`
with every status poll, sends a long list of grid-tie and second-output
queries, divides GS fields 14 and 15 by 10, and labels flag I "Grid connection
enabled". This is an app quirk, not inverter behaviour.

## What the app sends

| When | Queries |
|---|---|
| Connect | `T`, `VFW`, `PIRI`, then `PIRI`, `MCHGCR`, `LST`, `MUCHGCR`, `ACCT`, `ACLT`, `LED`, then `FLAG`, then `DI`. It also sets the inverter clock to the PC time with `DAT` when AutoTime is on (the default). |
| Every poll (back to back with a 1 s pause while the device is shown, every 60 s otherwise) | `GS`, `FWS`, `GMN`, `MOD`, `LOADPR`, `VFW`, `PIRI`. If PIRI field 22 (output mode) isn't 0, `PRI`n, `PGS`n and `PS`n for each parallel unit replace `GS`, `MOD` and `LOADPR`. |
| About every 2.5 s | `FWS` |
| Every 10 s | Energy history: a background thread refreshes it from a local database, which it fills with `ED`, `EM`, `EY` and `ET` queries |
| Parameters dialog | `FLAG` and the settings queries again after it closes |

`GS` is dropped when the `FWS` sent with it gets no reply. After three failed
transactions in a row the app probes with `GS` and eventually closes the
device.

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
| 14 | MPPT1 charger temperature | °C (the app divides by 10 for "V4 WP" models) |
| 15 | MPPT2 charger temperature | °C (same) |
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
| 27 | Parallel ID; the app uses it as k in `PCP`, `MCHGC` and `MUCHGC` | |

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
| 23 | Solar power priority | 0 Battery-Load-Utility, 1 Load-Battery-Utility (2 Load-Utility-Battery on some models) |
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

Flag I is the machine type (PIRI field 20), not a FLAG field. The app labels
it "Model type: Hybrid / Off-grid", or "Grid connection enabled" on V4 WP
models.

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
| `EY`yyyy, `EM`yyyyMM, `ED`yyyyMMdd | Energy for that year, month or day, in the same unit as `ET` (the app doesn't scale them) |
| `T` | Inverter clock, `yyyyMMddHHmmss` |
| `LST` | LCD sleep time: `00` none, `01` 30 s, `02` 60 s, `10` 300 s, `20` 600 s |
| `ACCT`, `ACLT` | AC charge start and end time; AC output start and end time, each `HHMM` |
| `LOADPR` | Load 1 VA, load 1 W, load 2 VA, load 2 W (for models with a second output) |

## Queries probed on the LV2424

On 2026-11-20 a read-only probe sent each of these queries to an LV2424 three
times, 5 s apart. Every reply passed its CRC, and the three replies to a query
matched (apart from the clock).

| Query | Result | Reply data |
|---|---|---|
| `^P004T` | Verified | `^D017` + `yyyyMMddHHmmss`, e.g. `^D01720261120130618` |
| `^P006LST` | Refused | `^0` all three times |
| `^P007ACCT` | Verified | `^D012` + `hhmm,hhmm`; read `0000,0000` |
| `^P007ACLT` | Verified | `^D012` + `hhmm,hhmm`; read `0000,0000` |
| `^P009MCHGCR` | Verified | `^D058` + 14 values: `010,020,030,040,050,060,070,080,090,100,110,120,130,140` |
| `^P010MUCHGCR` | Verified | `^D030` + 7 values: `002,010,020,030,040,050,060` |

- **Allowed currents:** max charging 10 to 140 A in steps of 10; max AC
  charging 2 A, then 10 to 60 A in steps of 10.
- **`ACCT`/`ACLT`:** both read `0000,0000`, which is the inverter's factory
  value. What an equal start and end means (always on, or never) isn't
  verified. The component publishes 00:00 for both ends.
- **`LST`:** the LV2424 refuses the query, so the LCD sleep time can't be read
  back.
- **Clock:** the inverter clock read 13:06:18 when the ESP32's clock read
  13:36:50, so it was about 30 minutes behind. That is why the component has
  the `sync_clock` button.
- **`PIRI`:** the probe didn't capture the boot-time `^P007PIRI` reply.

## Set commands

Payloads follow `^Snnn`. Values marked ×10 are sent in tenths.

| Payload | Effect |
|---|---|
| `LON1`, `LON0` | AC output on, off. The app never reads the output state back, and an LV2424 refuses `LON0`. |
| `PE`x, `PD`x | Enable, disable flag x (A–H as in FLAG; I is machine type Hybrid/Off-grid; S is the second AC output) |
| `POP`n | Output source priority, 0 or 1 |
| `PSP`n | Solar power priority, 0 or 1 (2 on some models) |
| `PCP`k,n | Charger source priority; k is GS field 27 (0 for a single unit) |
| `MCHGC`k,nnn | Max charging current, A |
| `MUCHGC`k,nnn | Max AC charging current, A |
| `MCHGV`bbb,fff | Bulk and float voltage ×10, always both |
| `BUCD`rrr,ddd | Back-to-utility and back-to-battery voltage ×10; the app sends "FULL" as `000` |
| `PSDV`nnn | Battery cut-off voltage ×10 |
| `PBT`n | Battery type: 0 AGM, 1 Flooded, 2 User |
| `PGR`n | Input range: 0 Appliance, 1 UPS |
| `V`nnnn | Output voltage ×10 |
| `F`nn | Output frequency, 50 or 60 |
| `RS`nn | Regulation standard: `00` India, `01` Germany, `02` South America, `03` NRS097, `04` IEC62116 |
| `DAT`yyMMddHHmmss | Set the inverter clock |
| `LST`nn | LCD sleep time, codes as in the `LST` query |
| `ACCT`hhmm,hhmm, `ACLT`hhmm,hhmm | AC charge window, AC output window |
| `SBT`n | Li-Fe battery activation enable (1) or disable (0) |
| `PF` | Restore factory defaults |
| `CLE` | Clear stored energy data |
| `BST` | Activate a Li-Fe battery now |

For the LV2424's class (`P1834K`) the settings page offers these dropdowns:
`RS`, `PCP`, `POP`, `PSP`, `MCHGC`, `MUCHGC`, `POPM` (output mode), `PGR`,
`PBT`, `F`, `V` and `BUCD`. For a 24 V bank, `BUCD` offers back-to-utility
22.0–25.5 V and back-to-battery FULL or 24.0–29.0 V, in 0.5 V steps.

The same page also offers:
- the `PE`/`PD` flags;
- `SBT`;
- `MCHGV` and `PSDV` spinners (for a 24 V bank: bulk and float 24.0–29.2 V,
  cut-off 20.4–24.0 V);
- the `ACCT` and `ACLT` time windows;
- the clock.

The app knows about 70 more set commands (generator, second output, dry
contact, LED ring, grid-tie and EN 50549 limits) for other models.

## Fault text for the 1–4 kW classes

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

`P1812K` and `P1834K` both use this table (`Events_P181TO2K.xml`). Codes
10–12 differ from the generic P18 table, where 11 is "Main relay failed".
The app expects a two-digit code, and never raises 52 although it has text.

The `fault_description` text sensor uses this table and gives both meanings
for 11 ("PV voltage over or main relay failed"), so it also fits larger units.
It shows "Unknown fault" and the number for codes neither table names.

## Not implemented by this component

`ED`/`EM`/`EY`, `VFW`, `DI`, `GMN`, PIRI field 25, and the set commands `PGR`,
`V`, `F`, `RS`, `SBT`, `PF`, `CLE` and `BST` from the table above.

Implemented set commands:
- The `number` platform covers `PSDV`, `MCHGV`, `BUCD`, `MCHGC` and `MUCHGC`, and
  reads `MCHGCR`/`MUCHGCR`.
- The `select` platform sets `PBT` and `LST`, and reads `LST` with `^P006LST`
  every 60 s.
- The `datetime` platform sets the AC charge and output windows with `ACCT` and
  `ACLT`, and reads them with `^P007ACCT` and `^P007ACLT` every 60 s.
- The `inverter_time` text sensor reads `T` with `^P004T` every 60 s, and the
  `sync_clock` button sets the clock with `DAT`.

## Source and limits

- **Source:** `SolarPower_MAC_ARM.zip` from mppsolar.com (SolarPower 1.23SP3,
  sha256 `16820aabff29dc290af7de6eb952e244ae9b2f258958b85f362d2389582699eb`).
  The protocol code is in `lib/SolarPower.jar`, under
  `cn.com.voltronic.solar.{protocol,comusbprocessor,control,communicate}`.
  The fault text is in `config/Events_P181TO2K.xml`.
- **Method:** the classes were decompiled with JADX 1.5.6, after a first pass with Ghidra 12.2.
- **App quirks:** the app's handling of the small classes has bugs (the
  model-14 default above), so it isn't a perfect record of what it would put on
  the wire. Treat it as the vendor's intent.
- **Unverified on an LV2424:** the model number `GMN` returns, so whether the
  "V4 WP" gates apply.
- **Missing labels:** the app has no names for GS fields 21, 22, 24 and 25.
