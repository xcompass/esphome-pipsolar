#!/usr/bin/env python3
"""Mac side of the inverter serial bridge.

Opens a pseudo-terminal, links it at --link, and relays bytes both ways between it and the ESP32's
`serial_proxy` (ESPHome native API, encrypted). SolarPower (launched with the patched jSerialComm)
opens the link as its only serial port.

The API encryption key is read from stdin and never printed; see README.md for how to pipe it
in from Home Assistant's secrets.yaml.

--trace logs every chunk with a timestamp and direction (> to the inverter, < from it), so the exact
bytes SolarPower sends can be read afterwards.
"""

import argparse
import asyncio
import contextlib
import datetime as dt
import os
import pathlib
import pty
import signal
import sys
import tty

import aioesphomeapi


def printable(data: bytes) -> str:
    return "".join(chr(b) if 32 <= b < 127 else f"\\x{b:02X}" for b in data)


def local_now() -> dt.datetime:
    return dt.datetime.now(tz=dt.UTC).astimezone()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True, help="ESPHome device address")
    parser.add_argument(
        "--name", default="inverter", help="serial_proxy name on the ESP32"
    )
    parser.add_argument(
        "--link", required=True, help="path of the symlink to create for the pty"
    )
    parser.add_argument("--baud", type=int, default=2400)
    parser.add_argument("--trace", help="file to log traffic to")
    return parser.parse_args()


async def main() -> int:
    args = parse_args()
    key = sys.stdin.readline().strip()
    if not key:
        print("no API key on stdin", file=sys.stderr)
        return 2

    cli = aioesphomeapi.APIClient(args.host, 6053, None, noise_psk=key)
    await cli.connect(login=True)
    info = await cli.device_info()
    names = [p.name for p in info.serial_proxies]
    if args.name not in names:
        print(
            f"serial_proxy '{args.name}' not found on the device (found {names});"
            " is the bridge firmware flashed?",
            file=sys.stderr,
        )
        await cli.disconnect()
        return 1
    instance = names.index(args.name)

    master, slave = pty.openpty()
    # Keep the slave open ourselves so the master never sees EIO between SolarPower sessions.
    tty.setraw(slave)
    link = pathlib.Path(args.link)
    if os.path.lexists(link):
        link.unlink()
    link.symlink_to(os.ttyname(slave))

    with contextlib.ExitStack() as stack:
        trace = (
            stack.enter_context(pathlib.Path(args.trace).open("a", encoding="utf-8"))
            if args.trace
            else None
        )
        counts = {">": 0, "<": 0}

        def log(direction: str, data: bytes) -> None:
            counts[direction] += len(data)
            if trace:
                stamp = local_now().strftime("%H:%M:%S.%f")[:-3]
                trace.write(f"{stamp} {direction} {printable(data)}\n")
                trace.flush()

        def on_data(msg) -> None:
            if getattr(msg, "instance", instance) != instance:
                return
            data = bytes(msg.data)
            log("<", data)
            os.write(master, data)

        def on_pty_readable() -> None:
            try:
                data = os.read(master, 1024)
            except OSError:
                return
            if data:
                log(">", data)
                cli.serial_proxy_write(instance, data)

        cli.subscribe_serial_proxy_data(on_data)
        await cli.serial_proxy_subscribe_await_response(instance)
        await cli.serial_proxy_configure_await_response(instance, args.baud)
        loop = asyncio.get_running_loop()
        loop.add_reader(master, on_pty_readable)
        print(
            f"bridge up: {link} -> {os.ttyname(slave)} <-> {args.host}"
            f" serial_proxy '{args.name}' at {args.baud} baud. Ctrl-C to stop.",
            flush=True,
        )

        stop = asyncio.Event()
        loop.add_signal_handler(signal.SIGINT, stop.set)
        loop.add_signal_handler(signal.SIGTERM, stop.set)
        while not stop.is_set():
            try:
                await asyncio.wait_for(stop.wait(), timeout=30)
            except TimeoutError:
                print(
                    f"{local_now():%H:%M:%S} bytes to inverter {counts['>']},"
                    f" from inverter {counts['<']}",
                    flush=True,
                )

        loop.remove_reader(master)
        with contextlib.suppress(Exception):  # best effort on shutdown
            await cli.serial_proxy_unsubscribe_await_response(instance)
        await cli.disconnect()
    if link.is_symlink():
        link.unlink()
    print("bridge stopped", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
