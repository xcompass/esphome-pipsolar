# Running SolarPower over the network

These tools let MPP Solar's SolarPower app (1.23SP3, macOS) talk to a PI18 inverter through the
ESP32 that normally runs the `pi18` component, without moving any cables:

```text
SolarPower ── virtual serial port (Mac) ── sp_bridge.py ── ESPHome native API (encrypted)
           ── ESP32 serial_proxy ── RS-232 ── inverter
```

Use it to see exactly what the vendor app sends, and how the inverter answers. Every byte is
logged in both directions.

**Tested on:** an MPP Solar LV2424 on 2026-11-20, with ESPHome 2026.11.0, SolarPower 1.23SP3 and
macOS. ESPHome's `serial_proxy` is still marked experimental.

## What's here

| File | What it does |
|---|---|
| `make_bridge_config.py` | Makes a bridge firmware config from your device config. It drops the `pi18` hub and every `pi18` entity, so nothing else talks on the serial line, and adds `serial_proxy` on the inverter's UART. Wi-Fi, API key, OTA and other components stay. |
| `sp_bridge.py` | Mac side. It opens a pseudo-terminal and relays it to the ESP32's `serial_proxy`. It reads the API key from stdin and never prints it. `--trace` logs every chunk with a timestamp and direction (`>` to the inverter, `<` from it). |
| `jserialcomm/build.sh`, `getCommPorts.patch` | Builds a patched jSerialComm 2.7.0 class (see below). It downloads the official source, checks its hash, patches it and compiles it. |
| `run_solarpower.sh` | Starts your installed SolarPower with the patched class and the right time zone. |

SolarPower itself isn't included. Download it from MPP Solar.

## Why the patch

SolarPower only finds ports through jSerialComm's `getCommPorts()`. On macOS that lists only
IOKit serial devices, which includes Bluetooth ports but never a pseudo-terminal. The patched
`getCommPorts()` returns just the ports named in `-Dsolarpower.onlyPorts`. That also stops
SolarPower from opening every other serial device on the Mac and sending inverter queries to it.

## One-time setup (Mac)

```sh
cd tools/solarpower-bridge
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
jserialcomm/build.sh /Applications/SolarPower/lib/jSerialComm2.7.jar   # needs JDK 11+ (javac)
```

## A session

1. **Make and compile the bridge firmware** (inside the ESPHome add-on container).
   Don't put it in `/config/esphome`, or the dashboard shows it as another device:
   ```sh
   python3 make_bridge_config.py /config/esphome/<device>.yaml /tmp/bridge
   esphome compile /tmp/bridge/<device>-bridge.yaml
   ```
   Then flash it with `esphome upload /tmp/bridge/<device>-bridge.yaml --device <device-ip>`.
   While it runs, Home Assistant has no `pi18` readings or controls. The inverter keeps running
   on its own.
2. **Start the bridge on the Mac.** It pipes the API key from Home Assistant's `secrets.yaml`
   straight in:
   ```sh
   ssh root@<ha-host> 'docker exec <esphome-container> python3 -c "import yaml; print(yaml.safe_load(open(\"/config/esphome/secrets.yaml\"))[\"<api-key-secret>\"])"' \
     | .venv/bin/python sp_bridge.py --host <device-ip> --link ./tty.inverter --trace traffic.log
   ```
3. **Quit any running SolarPower** (tray icon → Exit). Its single-instance check stops a second
   copy from starting.
4. **Start SolarPower:** `BRIDGE_PORT=$PWD/tty.inverter ./run_solarpower.sh`. To control or
   change settings, log in. The manual's default administrator password is `administrator`;
   as guest you can only view.
5. **Afterwards:** quit SolarPower, stop the bridge (Ctrl-C), and flash your normal firmware back
   with `esphome run /config/esphome/<device>.yaml --device <device-ip>`.

## Things to know

- **SolarPower sets the inverter clock (`DAT`) every time it connects** while AutoTime is on, which
  is the default.
  - Its bundled Java 11.0.10 has 2021 time zone rules. On a Mac set to America/Vancouver
    (UTC−7 since 2026), it sent a time one hour behind.
  - `run_solarpower.sh` passes the system's current UTC offset to avoid this. Override it with
    `SOLARPOWER_TZ` if you need to.
  - The installed app, started normally, still has the bug.
- **Only one program may talk on the serial line.** That's why the bridge firmware drops `pi18`.
- **On the LV2424**, SolarPower's "Real-time control → AC output control: Off" sends `^S007LON0`
  and the inverter refuses it (`^0`), so the app reports "setting false". The only working way to
  switch that model's output is the "AC Output ON/Off Timer" (`ACLT`); see
  `docs/pi18-protocol.md`.
