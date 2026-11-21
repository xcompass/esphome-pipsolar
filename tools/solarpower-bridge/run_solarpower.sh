#!/bin/sh
# Run SolarPower (1.23SP3, macOS) with the patched jSerialComm, so its only serial port is the
# bridge's virtual port. No Bluetooth or other ports are listed or probed.
# Start sp_bridge.py first, and quit any running SolarPower (tray icon -> Exit) before this.
#
# Environment:
#   BRIDGE_PORT     the --link path sp_bridge.py created (required)
#   SOLARPOWER_APP  SolarPower install directory (default /Applications/SolarPower)
#   PATCH_CLASSES   output of jserialcomm/build.sh (default jserialcomm/classes next to this script)
#   SOLARPOWER_TZ   Java time zone. SolarPower sets the inverter clock on every connect, and its
#                   bundled Java 11 has old time zone rules, so by default this passes the
#                   system's current UTC offset (e.g. GMT-07:00).
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
PORT=${BRIDGE_PORT:?set BRIDGE_PORT to the virtual port sp_bridge.py created}
APP=${SOLARPOWER_APP:-/Applications/SolarPower}
CLASSES=${PATCH_CLASSES:-$HERE/jserialcomm/classes}
OFFSET=$(date +%z)
TZ_ARG=${SOLARPOWER_TZ:-GMT$(echo "$OFFSET" | cut -c1-3):$(echo "$OFFSET" | cut -c4-5)}
JAVA=$APP/.install4j/jre.bundle/Contents/Home/bin/java

[ -e "$PORT" ] || { echo "virtual port $PORT missing: start sp_bridge.py first" >&2; exit 1; }
[ -f "$CLASSES/com/fazecast/jSerialComm/SerialPort.class" ] || { echo "run jserialcomm/build.sh first" >&2; exit 1; }
[ -x "$JAVA" ] || { echo "no bundled Java at $JAVA; set SOLARPOWER_APP" >&2; exit 1; }

cd "$APP"
exec "$JAVA" -Dsolarpower.onlyPorts="$PORT" -Duser.timezone="$TZ_ARG" \
  -Dinstall4j.launcherId=27 -Dinstall4j.swt=false \
  -cp "$CLASSES:lib/*" \
  cn.com.voltronic.solar.console.mac.SolarPower
