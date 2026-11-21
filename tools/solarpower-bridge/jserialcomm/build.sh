#!/bin/sh
# Build the patched jSerialComm 2.7.0 SerialPort class that lets SolarPower see the bridge's
# virtual serial port. Downloads the official v2.7.0 source, checks its hash, applies
# getCommPorts.patch and compiles it against SolarPower's own jSerialComm jar.
#
# Usage: build.sh /Applications/SolarPower/lib/jSerialComm2.7.jar [out_dir]
# Needs a JDK 11 or newer (javac). Set JAVAC to pick one, e.g. JAVAC=/path/to/jdk/bin/javac.
set -eu

JAR=${1:?usage: build.sh /path/to/SolarPower/lib/jSerialComm2.7.jar [out_dir]}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${2:-$HERE/classes}
URL=https://raw.githubusercontent.com/Fazecast/jSerialComm/v2.7.0/src/main/java/com/fazecast/jSerialComm/SerialPort.java
SHA256=eb13580c5148667c976b6dd6a170b6c52df65ff6c8d377dd5182792d9eed97c8

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
SRC=$WORK/com/fazecast/jSerialComm/SerialPort.java
mkdir -p "$(dirname "$SRC")"
curl -sSfL -o "$SRC" "$URL"
echo "$SHA256  $SRC" | shasum -a 256 -c - >/dev/null
patch -s "$SRC" "$HERE/getCommPorts.patch"
mkdir -p "$OUT"
"${JAVAC:-javac}" -nowarn --release 11 -cp "$JAR" -d "$OUT" "$SRC"
echo "built $OUT/com/fazecast/jSerialComm/SerialPort.class"
