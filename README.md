# esphome-pipsolar

![GitHub actions](https://github.com/syssi/esphome-pipsolar/actions/workflows/ci.yaml/badge.svg)
![GitHub stars](https://img.shields.io/github/stars/syssi/esphome-pipsolar)
![GitHub forks](https://img.shields.io/github/forks/syssi/esphome-pipsolar)
![GitHub watchers](https://img.shields.io/github/watchers/syssi/esphome-pipsolar)
[!["Buy Me A Coffee"](https://img.shields.io/badge/buy%20me%20a%20coffee-donate-yellow.svg)](https://www.buymeacoffee.com/syssi)

ESPHome example configuration to monitor and control a pipsolar inverter via RS232

Kudos to [@andreashergert1984](https://github.com/andreashergert1984) for the great work!

## Supported devices

The project provides three components for different protocol variants:

### pip8048 (Q-command protocol)

* PIP4048 compatible PV Inverter
* Axpert King II 6.2KW TWIN (reported by [@voronin10](https://github.com/syssi/esphome-pipsolar/issues/196))
* Powmr 4.2KW (reported by [@Martyn911](https://github.com/syssi/esphome-pipsolar/issues/231))

### pip2424mse1 (Q-command protocol, extended)

* PIP2424MSE1 and compatible inverters

### pi18 (PI18 protocol, `^P`/`^D` framing)

* MPP Solar LV5048 Hybrid V2
* SunGoldPower 6048
* Voltronic InfiniSolar V 4 (3.6 kW / 5.6 kW / 6 kW)
* AXIOMA 5 kW
* MppSolar compatible units responding to `^P005GS`

The `number` platform sets the battery and charging limits of a pi18 inverter:
`battery_under_voltage` (cut-off), `battery_bulk_voltage`, `battery_float_voltage`,
`battery_recharge_voltage` (back to utility), `battery_redischarge_voltage` (back to
battery; 0 means "full") and the two charging currents. The voltage numbers need
`battery_rated_voltage: 24` or `48` to pick their default ranges; `min_value`,
`max_value` and `step` override them. A number shows what the inverter last
reported, so it keeps the old value until the next `^P007PIRI` reply confirms a write.
The inverter's allowed current lists are read from `^P009MCHGCR` and `^P010MUCHGCR`,
and a current outside them is rejected.

Warning: some inverters accept changes to the bulk and float voltages only while the
battery type is "User", and answer `^0` (refused) otherwise; the number then returns to
the inverter's value. Changing the battery type may also reset the battery voltages, so
set the type first and check the voltages afterwards.

```yaml
number:
  - platform: pi18
    pipsolar_id: inverter0
    battery_rated_voltage: 24
    battery_recharge_voltage:
      name: "back to utility voltage"
    battery_redischarge_voltage:
      name: "back to battery voltage"
```

A Home Assistant automation action that makes a generator charge the battery by
returning to the utility at 24.0 V:

```yaml
- action: number.set_value
  target: {entity_id: number.mppsolar_back_to_utility_voltage}
  data: {value: 24.0}
```

The `select` platform has one type with built-in options: `battery_type` (AGM,
Flooded, User). Give it only a `name`; an `optionsmap` and `statusmap` replace the
built-in ones. Choosing
the option the inverter already reports sends nothing. Changing `battery_type` may
reset the battery voltages, and the bulk and float voltages may only be writable while
the type is "User" (see the `number` warning above).

```yaml
select:
  - platform: pi18
    pipsolar_id: inverter0
    battery_type:
      name: "battery type"
```

The `datetime` platform has four time entities for the AC charge window and the
AC output window: `ac_charge_start_time`, `ac_charge_end_time`, `ac_output_start_time`
and `ac_output_end_time`. They are read every minute. Setting one end sends both
(`^S016ACCT` or `^S016ACLT`), with the other end as last read, so a write is refused
until the first reading arrives. Seconds are ignored. The inverter only accepts whole
hours, so a time with non-zero minutes is rejected with the warning "The inverter only
accepts whole hours" and the entity shows its reading again (an LV2424 refuses such
windows with `^0` anyway). Like the numbers, the entity shows the inverter's reading, not
the time you set, until the next reading confirms it.

```yaml
datetime:
  - platform: pi18
    pipsolar_id: inverter0
    ac_charge_start_time:
      name: "AC charge start time"
    ac_charge_end_time:
      name: "AC charge end time"
```

The `ac_output` switch (`switch` platform) turns the inverter's 110 V output on and
off. The LV2424 refuses the vendor's own output-off command (`LON0`), so the switch uses
the AC output ON/Off timer (`ACLT`) instead: the output is on only inside the timer's
window, and a window of `00:00`-`00:00` disables the timer, which leaves the output on.
It is verified on the LV2424 only; other models may behave differently. It has the usual
switch options (`inverted` isn't allowed).

```yaml
switch:
  - platform: pi18
    pipsolar_id: inverter0
    ac_output:
      name: "ac output"
```

- **Off** writes the window for the hour that just ended, `[h-1:00, h:00)` by the
  inverter's clock. On the LV2424 the output went off within about 1 s, and that cuts
  the grid pass-through too, not only the inverter's own output.
- **On** writes `00:00`-`00:00`, which turns the output on within about 3 s. If the
  timer already has the output on, nothing is written.
- **Whole hours only.** The inverter accepts only whole hours, so the windows are whole
  hours too. It has no "always off" window: an equal start and end is stored as
  `00:00`-`00:00`.
- **Staying off.** While the switch is Off it moves the window about once a day, an hour
  before it would open, so the output stays off. That is about one write a day, because
  the inverter already reports an "EEPROM fail" warning. If the ESP32 is down when the
  window opens, the output comes back on by itself within 23 hours.
- **The switch owns the timer while Off.** A schedule you set with the
  `ac_output_start_time` and `ac_output_end_time` entities is overwritten at the next
  re-arm, or at once if it puts the current hour inside the window. While the switch is
  On it leaves the timer alone, so a schedule you set by hand keeps working, and the
  switch shows whether that schedule has the output on. "Keep off" is saved on the ESP32
  and survives reboots; `restore_mode` defaults to `DISABLED`, and an On after a reboot
  writes nothing.
- **State.** The switch shows what the timer gives, from the inverter's last readings of
  the window and the clock, so it updates after the next window reading and not at the
  moment of the write.
- **Low-battery guard.** On is refused, with a warning in the log, while the inverter
  reports the battery-under-shutdown warning, or before the first fault and warning
  reply after boot. This is because switching the output on during a low-battery cut-off
  made the inverter alarm and drop the output again after 26 s. The guard has a limit: the
  inverter turns the output on by itself when a window starts, even during such a
  cut-off, and the switch can't prevent that.
- **Clock.** Everything is checked against the inverter's own clock, which is not kept
  right by the inverter (an LV2424 was 30 minutes off). Off is refused, with a warning,
  until the clock and the window have been read, or when the command queue is full. Press
  `sync_clock` daily (see below).
- **Polls.** The switch adds the `^P004T` clock and `^P007ACLT` window polls (once a
  minute) and the `^P006FWS` poll (every update), even without the clock sensor or the
  window entities.

The inverter clock has two entities. The `inverter_time` text sensor
(`text_sensor` platform) shows the inverter's date and time as `YYYY-MM-DD HH:MM:SS`,
read once a minute. The `sync_clock` button (`button` platform) sets the inverter
clock to the local time of an ESPHome `time` component, given as `time_id`, and then
reads the clock again. It does nothing but log a warning while the time source isn't
valid yet. The inverter never changes its clock by itself, so a Home Assistant
automation that presses the button keeps it right, for example every night:

```yaml
time:
  - platform: homeassistant
    id: ha_time

button:
  - platform: pi18
    pipsolar_id: inverter0
    sync_clock:
      name: "sync clock"
      time_id: ha_time
```

```yaml
- trigger: {trigger: time, at: "03:00:00"}
  action: {action: button.press, target: {entity_id: button.mppsolar_sync_clock}}
```

## Requirements

* [ESPHome 2026.3.0 or higher](https://github.com/esphome/esphome/releases).
* One half of an ethernet cable with RJ45 connector
* RS232-to-TTL module (`MAX3232CSE` f.e.)
* Generic ESP32 or ESP8266 board

## Schematics

<a href="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/001.jpg" target="_blank">
  <img src="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/001.jpg" height="172">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/002.jpg" target="_blank">
  <img src="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/002.jpg" height="172">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/004.jpg" target="_blank">
  <img src="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/004.jpg" height="172">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/005.jpg" target="_blank">
  <img src="https://raw.githubusercontent.com/syssi/esphome-pipsolar/main/images/005.jpg" height="172">
</a>

```
               RS232                     UART-TTL
┌──────────┐              ┌──────────┐                ┌─────────┐
│          │              │          │<----- RX ----->│         │
│          │<---- TX ---->│  RS232   │<----- TX ----->│ ESP32/  │
│   PIP    │<---- RX ---->│  to TTL  │<----- GND ---->│ ESP8266 │
│          │<---- GND --->│  module  │<-- 3.3V VCC -->│         │<--- VCC
│          │              │          │                │         │<--- GND
└──────────┘              └──────────┘                └─────────┘
```

### RJ45 connector

| Pin     | Purpose      | MAX3232 pin       | Color T-568B |
| :-----: | :----------- | :---------------- | :------------|
|    1    | TX           | P13 (RIN1)        | White-Orange |
|    2    | RX           | P14 (DOUT1)       | Orange       |
|    3    |              |                   |              |
|    4    | VCC 12V      | -                 | Blue         |
|    5    |              |                   |              |
|    6    |              |                   |              |
|    7    |              |                   |              |
|    8    | GND          | P15 (GND)         | Brown        |

Please be aware of the different RJ45 pinout colors ([T-568A vs. T-568B](images/rj45-colors-t568a-vs-t568.png)).

The inverter provides +12V on pin 4 or 7 depending on the model. You can use a cheap DC-DC converter to power the ESP with 3.3V.

The [source for the pinout is here](docs/HS_MS_MSX%20RS232%20Protocol.pdf).

### MAX3232

| Pin          | Label        | ESPHome     | ESP8266 example  | ESP32 example |
| :----------- | :----------- | :---------- | :--------------- | :------------ |
| P11 (DIN1)   | TXD          | `tx_pin`    | `GPIO4`          | `GPIO16`      |
| P12 (ROUT1)  | RXD          | `rx_pin`    | `GPIO5`          | `GPIO17`      |
| P16 (VCC)    | VCC          |             |                  |               |
| P15 (GND)    | GND          |             |                  |               |

## Installation

Use the `esp32-example.yaml` / `esp8266-example.yaml` as proof of concept:

```bash
# Install esphome
pip3 install esphome

# Clone this external component
git clone https://github.com/syssi/esphome-pipsolar.git
cd esphome-pipsolar

# Create a secret.yaml containing some setup specific secrets
cat > secrets.yaml <<EOF
wifi_ssid: MY_WIFI_SSID
wifi_password: MY_WIFI_PASSWORD

mqtt_host: MY_MQTT_HOST
mqtt_username: MY_MQTT_USERNAME
mqtt_password: MY_MQTT_PASSWORD
EOF

# Validate the configuration, create a binary, upload it, and start logs
# If you use a esp8266 run the esp8266-examle.yaml
esphome run esp32-example.yaml

```

Take a look at the [official documentation of the component](https://esphome.io/components/pipsolar.html) for additional details.

## Known issues

1. If you configure a lot of the possible sensors etc. it could be that you run out of memory (on esp32). If you configure nearly all sensors etc. you run in a stack-size issue. In this case you have to increase stack size: https://github.com/esphome/issues/issues/855

## Debugging

If this component doesn't work out of the box for your device please update your configuration to enable the debug output of the UART component and increase the log level to the see outgoing and incoming serial traffic:

```
logger:
  level: DEBUG
  # Don't write log messages to UART0 (GPIO1/GPIO3) if the inverter is connected to GPIO1/GPIO3
  baud_rate: 0

uart:
  id: uart_0
  baud_rate: 2400
  tx_pin: ${tx_pin}
  rx_pin: ${rx_pin}
  debug:
    direction: BOTH
    dummy_receiver: false
    after:
      delimiter: "\r"
    sequence:
      - lambda: UARTDebug::log_string(direction, bytes);
```

If you don't know the protocol of your inverter please use [this configuration](tests/esp8266-test-protocols.yaml) and try to identify to which request (`>>>`) your inverter responds (`<<<`). `NAK` is a negative response f.e. if the command isn't supported. If you have trouble to interpret the log please create an issue and provide your ESPHome log.

## References

* https://github.com/esphome/esphome/pull/1664
* https://github.com/esphome/esphome-docs/pull/1084/files
* https://github.com/andreashergert1984/esphome/tree/feature_pipsolar_anh
* https://github.com/jblance/mpp-solar/tree/master/docs/protocols
