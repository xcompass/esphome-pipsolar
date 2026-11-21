# pi18 AC output switch: design

## Intent

The owner wants Home Assistant to switch the LV2424's 110 V output on and off, both by hand and
from automations. Today it can't: the vendor's own on/off command (`LON0`) is refused.

## What the inverter can do (verified live on the LV2424, 2026-11-20)

- `^S007LON0` (SolarPower's "Real-time control → AC output control") gets `^0`: 4 of 4 tries, on
  battery and on grid. `LON1` is accepted, but it does nothing useful while `LON0` is refused.
- `ACLT` is the vendor's "AC Output ON/Off Timer" (manual programs 32/33, "Scheduled time for AC
  output on/off"). The output is on only inside `[start, end)`. `0000,0000` disables the timer,
  so the output is always on.
- **Whole hours only.** `1500,0000` and `0000,0000` got `^1`; `1438,0000` and `1446,0000` got
  `^0` three times each.
- **Off is immediate.** A window that excludes the current hour cut the output within about 1 s,
  grid pass-through included (15:57, on grid).
- **On is immediate.** Writing `0000,0000` while the output was off turned it on within about 3 s
  (16:57).
- **The start time turns the output on.** At the window start the output switched on by itself
  (16:00), and it did so even during a low-battery cut-off standby (15:00). In that case the
  output dropped again 26 s later with the low-battery alarm.
- **There is no "always off" window.** `0100,0100` got `^1` but was stored as `0000,0000`.
- The window is checked against the inverter's own clock (`^P004T`). That clock was 30 minutes
  off before the first sync.

## Goals

- An `ac_output` switch on the pi18 `switch` platform:
  - On turns the output on now.
  - Off turns it off now and keeps it off until switched on.
- It works for both manual use and automations, and it shows the output state the timer gives.
- It fails safe: if the ESP32 stops, the output comes back on by itself within a day.

## Non-goals

- A real on/off command. The LV2424 has none; the rocker relay plan stays separate.
- AC output scheduling. The existing `ac_output_start_time`/`ac_output_end_time` entities already
  do that.
- Other models. The switch relies on the `ACLT` behaviour measured on the LV2424. The README says
  so.

## Anti-goals

- Switching the output on during a low-battery cut-off. That turned a normal low-battery standby
  into an alarm today.
- Wearing out the inverter's settings memory with frequent writes. It already reports an
  "EEPROM fail" warning.

## Approaches considered

1. **Component switch (chosen by the owner).** Native `ac_output` switch with the logic in the hub,
   using the inverter's clock. It has a low-battery guard and re-arms itself.
2. **Home Assistant scripts.** Two scripts plus a daily automation, writing the existing window
   entities with HA's clock. No firmware change, but no guard, HA-side logic, and two writes per
   change that pass through odd half-windows.
3. **Hardware relay on the output.** Out of scope here.

## Behaviour

**Off**
- Read the inverter's current hour `h` from the latest `^P004T` reply.
- Write `^S016ACLT<h-1>00,<h>00`, with hours mod 24. This is the hour that just ended, so the
  output goes off now and would next come on at `h-1` tomorrow, 23 hours later.
- Reject the request (warning log, switch republishes its state) if:
  - the inverter's time is unknown;
  - the `ACLT` window hasn't been read yet;
  - the command queue is full.

**Keeping it off (re-arm)**
- While the switch is off, the hub checks each time a new `^P004T` reading arrives.
- If the window it wrote would open within the next hour (the current hour is `start-1`), it
  writes a new `[h-1, h)`.
- The new window also excludes the current hour, so there's no on-blip.
- That's about one inverter write every 23 hours.
- If the ESP32 is down when the window opens, the output turns on by itself. That's the
  fail-safe.

**On**
- Write `^S016ACLT0000,0000`, but only when the timer currently keeps the output off. Turning it on
  when it's already on writes nothing.
- **Low-battery guard:** refuse On (warning log, switch republishes off) while the last `^P006FWS`
  reply has the battery-under-shutdown warning set. The guard can't cover the inverter turning
  the output on by itself at a window start; the README says so.

**State shown**
- The switch shows On when the last `ACLT` reading is `0000,0000`, or when the inverter's current
  hour is inside the window. Otherwise it shows Off.
- It follows the last *reading*, not the write, like the other new controls. A write shows up
  once the next `ACLT` reading confirms it. The hub refreshes `ACLT` right after a write, as it
  does for the window entities.

**The switch's own setting**
- It persists what was last asked for (Off keeps re-arming), using ESPHome's switch restore with
  default `RESTORE_DEFAULT_ON`.
- After an ESP32 reboot, an Off restore re-arms once the inverter's time and the `ACLT` window are
  known.
- An On restore writes nothing, so a reboot never clobbers a window the owner set by hand.

**Shared write rules**
- The switch writes both ends in one command. It records both ends as pending, using the same
  pending/reading rule as the window entities, so the two stay consistent.
- A full command queue rejects the write.

**Polls**
- The switch needs `^P004T` and `^P007ACLT`. It registers them (SLOW, as today) even when the
  clock sensor or window entities aren't configured.

**Window entities: whole hours**
- `ac_charge_*` and `ac_output_*` reject non-zero minutes before sending, with a warning log, and
  republish their reading.
- The LV2424 refuses them anyway; this makes the reason visible.

## Interaction with hand-set windows

While the switch is Off, it owns the AC output timer. Editing the window entities is overwritten
at the next re-arm, or immediately if the edit puts the current hour inside the window. While
the switch is On, it leaves the timer alone, so hand-set schedules work as before. The switch
then just shows whether the schedule has the output on.

## Docs

- **README:** the switch, its timer mechanism, the whole-hour rule, the fail-safe, the
  low-battery guard and its limit, the clock dependency (pair it with the daily clock sync), and
  that it's verified on the LV2424 only.
- **`docs/pi18-protocol.md`:**
  - `ACLT` semantics from the tests above;
  - `LON0` refused 4 of 4;
  - equal start and end stored as `0000,0000`;
  - correct the line saying the vendor never reads FLAG field 8 (it reads it as flag I, machine
    type).

## Left to the builder

- Class and method names.
- Exactly how the battery-under-shutdown bit reaches the hub when no binary sensor is configured.
- The restore plumbing.
- Test structure.
