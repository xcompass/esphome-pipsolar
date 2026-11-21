"""Derive a serial-bridge firmware config from a pi18 device config.

Drops the pi18 hub and every pi18 entity (so nothing else talks on the inverter's serial line)
and adds ESPHome's built-in serial_proxy on uart_0. Everything else (Wi-Fi, API key, OTA,
SmartShunt) stays. Writes <out_dir>/<device>-bridge.yaml and links secrets.yaml next to it.

Usage (inside the ESPHome container):
  python3 make_bridge_config.py /config/esphome/<device>.yaml /tmp/bridge
"""

import pathlib
import re
import sys

src = pathlib.Path(sys.argv[1])
out_dir = pathlib.Path(sys.argv[2])
lines = src.read_text(encoding="utf-8").splitlines(keepends=True)

TOP = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*:")
PI18_ONLY_SECTIONS = {"pi18", "number", "select", "datetime", "button"}

# Split into top-level sections (keeping leading comments with the section they precede).
sections, current = [], []
for line in lines:
    if TOP.match(line) and current:
        sections.append(current)
        current = []
    current.append(line)
sections.append(current)


def key_of(section):
    for line in section:
        if TOP.match(line):
            return line.split(":", 1)[0]
    return None


def drop_pi18_items(section):
    """Remove `  - platform: pi18` list items from a platform section."""
    head, items, item = [], [], None
    for line in section:
        if line.startswith("  - "):
            if item is not None:
                items.append(item)
            item = [line]
        elif item is None:
            head.append(line)
        else:
            item.append(line)
    if item is not None:
        items.append(item)
    kept = [it for it in items if not it[0].strip().startswith("- platform: pi18")]
    return head, kept


out = []
for section in sections:
    key = key_of(section)
    if key in PI18_ONLY_SECTIONS:
        continue
    if key in ("sensor", "binary_sensor", "text_sensor", "switch"):
        head, kept = drop_pi18_items(section)
        if not kept:
            continue
        out.extend(head)
        for item in kept:
            out.extend(item)
        continue
    out.extend(section)

text = "".join(out).rstrip("\n") + "\n"
assert "platform: pi18" not in text and "\npi18:" not in text, "pi18 left behind"
assert "\nuart:" in text and "id: uart_0" in text, "uart_0 missing"
text += """
# Serial bridge (temporary): the inverter's RS-232 line, proxied over the encrypted native API.
serial_proxy:
  - uart_id: uart_0
    name: "inverter"
    port_type: RS232
"""
out_dir.mkdir(parents=True, exist_ok=True)
out = out_dir / f"{src.stem}-bridge.yaml"
out.write_text(text, encoding="utf-8")
link = out_dir / "secrets.yaml"
if not link.exists():
    pathlib.Path(link).symlink_to(src.parent / "secrets.yaml")
print("wrote", out)
