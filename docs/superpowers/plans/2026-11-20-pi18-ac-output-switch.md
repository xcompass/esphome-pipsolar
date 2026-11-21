# pi18 AC Output Switch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an `ac_output` switch to the pi18 component that turns the LV2424's AC output on and off through the inverter's AC Output ON/Off Timer (`ACLT`), and keeps it off until switched back on.

**Architecture:**
- The hub (`Pipsolar`) owns the logic: it writes `ACLT` windows from the inverter's own clock (`^P004T`), works out the switch state from readings, guards On with the battery-under-shutdown flag (`^P006FWS`), and re-arms the off window about once a day.
- A thin `PipsolarAcOutputSwitch` entity forwards Home Assistant requests to the hub, and persists "keep off" across ESP32 reboots in its own preference.
- The window time entities become whole-hour only.

**Tech Stack:** ESPHome external component (C++17, Python codegen), GoogleTest via ESPHome's `script/cpp_unit_test.py`, pytest schema tests.

**Spec:** `docs/superpowers/specs/2026-11-20-pi18-ac-output-switch-design.md`. Read it first.

## Global Constraints

**Branch and tooling**
- Branch `pi18-ac-output` (from `lv2424` fe150c6; the design is committed as 09ee67a).
- ESPHome ≥ 2026.3.0 (`require_esphome_version(2026, 3, 0)`). The live device builds with ESPHome 2026.11.0.

**Owner's go-ahead.** Each of these needs the owner's go-ahead at the time it happens:
- pushing;
- flashing the ESP32 (192.168.10.51);
- editing `/usr/share/hassio/homeassistant/esphome/mppsolar.yaml` on root@192.168.1.5;
- any `^S` command reaching the inverter.

**Commits**
- Capitalised imperative subject, no `feat:` prefix, no attribution or Claude-Session trailer.
- Run the `verify` skill right before each commit that changes code. Docs-only and test-only commits skip it.

**Tests run in Docker**
- Same scripts as the previous plan. Mount the worktree as `/repo`:
  ```bash
  S=/private/tmp/claude-504/-Users-compass-projects-esphome-pipsolar/4a04a023-c96b-4277-ade6-728b07873144/scratchpad
  docker run --rm -v /Users/compass/projects/esphome-pipsolar:/repo:ro -v $S/ws2:/ws2 python:3.12-bookworm \
    bash -c 'apt-get update -qq >/dev/null && apt-get install -y -qq rsync >/dev/null; bash /ws2/cpp1.sh'
  ```
- What each script runs:
  - `cpp1.sh`: pi18 C++ tests;
  - `run.sh`: everything CI runs;
  - `cfg.sh`: the example configs;
  - `cc.sh`: ci-custom per component;
  - `host-compile.sh`: a host-platform firmware build of `ws2/host-compile.yaml`.
- Don't delete or reinstall the ESPHome checkout or venv in `ws2`.
- **Known baselines, not regressions:**
  - `run.sh`'s example-config step fails on its 1-character fake secrets.
  - `cfg.sh`'s all-component `ci-custom` step exits 1 on duplicate-constant lints. CI disables those with a `sed` (`len(files) >= 99`).

**Behaviour values (from the spec)**

| Item | Value |
|---|---|
| Off | `^S016ACLT<h-1>00,<h>00`, where `h` is the inverter's current hour and hours are mod 24. At `h = 0` that is `^S016ACLT2300,0000`. |
| On | `^S016ACLT0000,0000` |
| Window semantics | Output on iff `start == end` (timer disabled), or the inverter's minute of day is in `[start, end)`. Windows with `start > end` wrap past midnight. |
| Re-arm | While "keep off" is set, write a fresh off window when the current window contains the inverter's time, or opens within 60 minutes. Never write while a window write is outstanding. |
| On guard | Reject On when the last `^P006FWS` reply's 7th warning flag (battery under shutdown) is set, or no FWS reply has been decoded yet. |
| Switch registers polls | `^P004T` SLOW, `^P007ACLT` SLOW, `^P006FWS` every update |
| Window entities | Reject non-zero minutes with a warning, before anything is sent |

**Frames for tests (captured from the LV2424, 2026-11-20)**
- `LV2424_P004T = "^D01720261120130618"` (13:06:18), already in `frames_lv2424.h`.
- `LV2424_P007ACLT = "^D0120000,0000"`, already in `frames_lv2424.h`.
- **New:** `LV2424_P006FWS = "^D03700,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0"`. Its CRC bytes are `3B 0C`, captured via SolarPower. EEPROM-fail is on; everything else is off.
- Build other clock and window replies with small test helpers, labelled synthetic:
  - `^D017` + `20261120` + `HHMM` + `00`;
  - `^D012HHMM,HHMM`;
  - for FWS with battery-under set, the LV2424 frame with its 7th flag set to `1`.

## Review Focus

1. **The inverter clock jumps into the off window** (a clock sync while "keep off" is set). Expected: the next `^P004T` reading re-arms, so any on-blip lasts at most one slow-poll interval. Covered by Task 2's `ClockJumpIntoWindowRearms`.
2. **An off write is refused** (`^0`). Expected: the switch shows the reading again, "keep off" stays, and it retries at most once per `^P004T` reading, not every loop. Covered by Task 2's `RefusedOffRetriesOncePerClockReading`.
3. **The ESP32 reboots while "keep off" is set, and the window already keeps the output off.** Expected: no write until the window comes within 60 minutes of opening. Covered by Task 2's `RestoredKeepOffWritesNothingWhileWindowIsFar`.
4. **The owner sets a schedule by hand while the switch is On.** Expected: never overwritten, even across ESP32 reboots. Covered by Task 2's `KeepOffClearedLeavesHandSetWindowAlone`, and by Task 3 restoring "keep off" only from its own preference.
5. **On is requested before the first FWS reply after boot.** Expected: rejected with a log line, not guessed. Covered by Task 1's `OnRejectedBeforeFirstFwsReply`.

---

### Task 1: Hub AC output control

**Files:**
- Modify:
  - `components/pi18/pipsolar.h`;
  - `components/pi18/pipsolar.cpp`: `handle_p004t_`, `handle_p006fws_`, `write_time_setting` (refactor), new methods.
  - `tests/components/pi18/frames_lv2424.h`: add `LV2424_P006FWS`.
- Create `tests/components/pi18/pi18_ac_output_test.cpp`.

**Interfaces:**
- Consumes: `WriteResult`, `TimeWindowEnd::AC_OUTPUT_START/END`, `get_time_setting`, `get_time_reading`, `queue_command`, `request_poll_refresh`, `set_pending_`, `write_outstanding`, `add_polling_command_`, `TestablePipsolar`, `TestSwitch`, `FakeUart`, all from earlier work.
- Produces, on `Pipsolar`:
  - `void set_ac_output_switch(switch_::Switch *sw)`: stores it and registers the three polls.
  - `WriteResult write_ac_output(bool on)`.
  - `esphome::optional<bool> get_ac_output_state() const`: computed from the `ACLT` *reading* and the inverter clock; empty if either is unknown.
  - `static bool window_contains(uint16_t start_hhmm, uint16_t end_hhmm, uint16_t minute_of_day)`.
  - `esphome::optional<uint16_t> get_inverter_minute_of_day() const`.
- Produces, protected:
  - `WriteResult queue_window_(TimeWindowEnd start, uint16_t start_time, uint16_t end_time)`: one `^S016ACCT/ACLT` command for both ends. It records both ends as pending and refreshes the window poll. `write_time_setting` now calls it.

- [ ] **Step 1: Add `LV2424_P006FWS` to `frames_lv2424.h` and write the failing tests in `pi18_ac_output_test.cpp`**

  Each test builds `TestablePipsolar inv; TestSwitch sw; inv.set_ac_output_switch(&sw);` and feeds frames through `handle_p004t_`, `handle_p007aclt_` and `handle_p006fws_`.
  - `RegistersClockWindowAndWarningPolls`: the polling list contains `^P004T`, `^P007ACLT` and `^P006FWS`.
  - `ClockHourRecordedWithoutTimeSensor`: after `LV2424_P004T`, `get_inverter_minute_of_day() == 13 * 60 + 6`, with no `inverter_time` text sensor set.
  - `WindowContains`, a table test:
    - `(0, 0, any)` → true;
    - `(1200, 1300, 12*60)` → true;
    - `(1200, 1300, 13*60)` → false;
    - `(2200, 600, 23*60)` → true;
    - `(2200, 600, 7*60)` → false;
    - `(2300, 0, 23*60+59)` → true;
    - `(2300, 0, 0)` → false.
  - `OffWritesTheHourThatJustEnded`: clock 13:06 plus window `0000,0000` → `write_ac_output(false) == QUEUED`. The queued command is `^S016ACLT1200,1300`; `get_time_setting(AC_OUTPUT_START) == 1200` and `AC_OUTPUT_END == 1300`, both pending.
  - `OffAtMidnightWrapsToEleven`: clock 00:15 → queued `^S016ACLT2300,0000`.
  - `OffRejectedWithoutClock`, `OffRejectedWithoutWindowReading`, `OffRejectedWhenQueueFull`: each returns `REJECTED`, with nothing queued for `ACLT`.
  - `OffUnchangedWhenWindowAlreadyKeepsOutputOffForAnHour`: clock 13:06 plus window `1200,1300` → `UNCHANGED`, nothing queued.
  - `OnWritesDisabledWindowWhenOutputIsOff`: clock 13:06, window `1200,1300`, LV2424 FWS → `write_ac_output(true) == QUEUED`, queued `^S016ACLT0000,0000`.
  - `OnUnchangedWhenTimerAlreadyOn`: window `0000,0000` → `UNCHANGED`.
  - `OnRejectedDuringBatteryUnderShutdown`: the FWS frame with its 7th flag `1` → `REJECTED`, nothing queued.
  - `OnRejectedBeforeFirstFwsReply`: clock and window known, no FWS → `REJECTED`.
  - `StateFollowsReadingNotWrite`:
    - clock 13:06 + window `0000,0000` → the switch state is on;
    - after `write_ac_output(false)` and before any new `ACLT` reading → still on;
    - after reading `^D0121200,1300` → off.
  - `StatePublishedOnClockAndWindowReadings`: window `2200,0600` → on at 23:00 and off at 07:00, checking `sw.state` after each `handle_p004t_`.
  - `WindowTimeEntityStillWritesBothEnds`: the existing `write_time_setting` path still queues `^S016ACLT0500,1300`, starting from window `1200,1300` and setting `AC_OUTPUT_START` to 5. This guards the `queue_window_` refactor.

- [ ] **Step 2: Run the tests and see them fail.** Run `cpp1.sh`. Expected: compile errors for the missing methods.

- [ ] **Step 3: Implement**
  - `handle_p004t_`:
    - Parse and validate as today.
    - Always record `inverter_minute_of_day_` (HH×60+MM).
    - Publish the text sensor only if it's set.
    - Then publish the AC output state.
  - `handle_p006fws_`: record the 7th flag into `battery_under_shutdown_` (`optional<bool>`), whether or not the binary sensor is set.
  - `apply_time_window_`: publish the AC output state after an `ACLT` reading.
  - `write_ac_output`, following the table in Global Constraints.
    - **Off:** needs the clock and the `ACLT` value (`get_time_setting`). It returns `UNCHANGED` when the current value already excludes the clock and doesn't open within 60 minutes.
    - **On:** needs the `ACLT` value and a decoded, clear FWS flag. It returns `UNCHANGED` when the value is disabled or already contains the clock.
  - Logging: warnings for rejections; debug for unchanged.

- [ ] **Step 4: Run the tests and see them pass.** `cpp1.sh`: all pass, including the existing ones.

- [ ] **Step 5: `verify`, then commit.** Message: "Add pi18 AC output control through the AC output timer".

### Task 2: Keep the output off (re-arm)

**Files:**
- Modify `components/pi18/pipsolar.h` and `pipsolar.cpp`.
- Test: `tests/components/pi18/pi18_ac_output_test.cpp`.

**Interfaces:**
- Consumes Task 1's methods.
- Produces, on `Pipsolar`:
  - `void set_ac_output_keep_off(bool keep_off)`;
  - `bool get_ac_output_keep_off() const`.
- `write_ac_output` sets "keep off" to `!on` whenever it returns `QUEUED` or `UNCHANGED`.

- [ ] **Step 1: Write the failing tests**
  - `RearmMovesTheWindowAnHourBeforeItOpens`:
    - keep off set, window `1200,1300`;
    - clock 10:30 → nothing queued;
    - clock 11:00 → queued `^S016ACLT1000,1100`, which excludes 11:xx.
  - `ClockJumpIntoWindowRearms`: keep off set, window `1200,1300`, clock 12:30 → queued `^S016ACLT1100,1200`.
  - `KeepOffOverridesHandSetWindow`: keep off set, window `0000,0000` read at clock 09:10 → queued `^S016ACLT0800,0900`.
  - `KeepOffClearedLeavesHandSetWindowAlone`: after `write_ac_output(true)` returns `UNCHANGED`/`QUEUED`, window `0800,0900` at 08:30 or at 11:00 → nothing queued.
  - `RestoredKeepOffWritesNothingWhileWindowIsFar`: `set_ac_output_keep_off(true)` before any reading, then window `1200,1300` and clock 14:00 → nothing queued.
  - `NoRearmWhileWindowWriteOutstanding`: keep off set, an `ACLT` write queued and not yet sent, then clock 11:00 → still exactly one `ACLT` command in the queue.
  - `RefusedOffRetriesOncePerClockReading`:
    - keep off set; the off write is sent through `FakeUart`, answered `^0`, and the next `ACLT` reading shows `0000,0000`;
    - no new write until the next `^P004T` reading;
    - then exactly one new write.

- [ ] **Step 2: Run the tests and see them fail.**

- [ ] **Step 3: Implement**
  - Re-arm check: run it from `handle_p004t_`, once per clock reading, and **not** from `loop()` or the `ACLT` handler, so a refused write retries at most once a minute.
  - Re-arm write: use the same off-window computation as `write_ac_output(false)`.

- [ ] **Step 4: Run the tests and see them pass.**

- [ ] **Step 5: `verify`, then commit.** Message: "Keep the pi18 AC output off by re-arming the timer window".

### Task 3: Switch entity and YAML

**Files:**
- Create `components/pi18/switch/pipsolar_ac_output_switch.h` and `.cpp`.
- Modify:
  - `components/pi18/switch/__init__.py`;
  - `components/pi18/const.py`: add `CONF_AC_OUTPUT = "ac_output"`;
  - `tests/test_component_schemas.py`;
  - `ws2/host-compile.yaml` in the scratchpad: add `ac_output`.
- Test: `tests/components/pi18/pi18_ac_output_test.cpp`, for the entity's `write_state`.

**Interfaces:**
- Consumes `write_ac_output`, `get_ac_output_state`, `set_ac_output_keep_off` and `get_ac_output_keep_off`.
- Produces `PipsolarAcOutputSwitch : public switch_::Switch, public Component` with:
  - `void set_parent(Pipsolar *parent)`;
  - `void setup() override`;
  - `void dump_config() override`;
  - `void write_state(bool state) override` (protected).

- [ ] **Step 1: Write the failing tests**
  - C++ `SwitchForwardsAndRepublishesOnRejection`: with no clock known, `turn_off()` leaves nothing queued, and the published state stays as before. Use the entity directly; `setup()` isn't called in unit tests.
  - pytest:
    - `ac_output` is accepted;
    - `inverted: true` is rejected (`block_inverted`);
    - the default `restore_mode` is `DISABLED`.

- [ ] **Step 2: Run the tests and see them fail.**

- [ ] **Step 3: Implement**
  - **Python:** `switch.switch_schema(PipsolarAcOutputSwitch, icon="mdi:power-socket-us", block_inverted=True, default_restore_mode="DISABLED")`. Codegen calls `new_switch`, `register_component`, `set_parent` and `paren.set_ac_output_switch(var)`.
  - **C++:**
    - `setup()` loads "keep off" from a `global_preferences->make_preference<bool>(this->get_preference_hash() ^ 0x41434f55)` and passes it to the hub.
    - `write_state` calls `write_ac_output(state)`. If the result isn't `REJECTED`, it saves `!state` to that preference. On `REJECTED`, it republishes `get_ac_output_state()` if known.
    - The switch **never** publishes the requested state itself; the hub publishes readings.
  - **Ruling to record in the ledger:** "keep off" has its own preference, rather than ESPHome's switch restore. The switch publishes the timer's actual state, so ESPHome's restore would save that instead of the request. This is the restore plumbing the spec leaves to the builder.

- [ ] **Step 4: Run the tests and see them pass.** Run `cpp1.sh`, `run.sh`, `cc.sh` and `host-compile.sh`. Expected: `compile_exit=0`, with no pi18 warnings.

- [ ] **Step 5: `verify`, then commit.** Message: "Add the pi18 ac_output switch".

### Task 4: Whole-hour window entities

**Files:**
- Modify `components/pi18/pipsolar.cpp` (`write_time_setting`).
- Test: `tests/components/pi18/pi18_time_window_test.cpp`.

- [ ] **Step 1: Write the failing tests**
  - `MinutesAreRejected`: starting from a known window, `write_time_setting(AC_OUTPUT_START, 14, 38) == REJECTED`, with nothing queued.
  - `WholeHourStillWrites`: `(14, 0)` → `QUEUED`.
  - Update any existing test that writes non-zero minutes, so it uses whole hours or expects the rejection.
- [ ] **Step 2: Run them and see them fail.**
- [ ] **Step 3: Implement.** Reject when `minute != 0`, with the warning "The inverter only accepts whole hours". The entity already republishes the reading on `REJECTED`.
- [ ] **Step 4: Run `cpp1.sh` and see them pass.**
- [ ] **Step 5: `verify`, then commit.** Message: "Accept whole hours only for the pi18 AC time windows".

### Task 5: Docs and example

**Files:** `README.md`, `docs/pi18-protocol.md`, `esp32-pi18-example.yaml`.

- [ ] **Step 1: README.** Add an `ac_output` section covering:
  - the timer mechanism;
  - On/Off behaviour and timing (about 1 s off, about 3 s on);
  - that Off cuts grid pass-through too;
  - whole hours;
  - the 23-hour fail-safe;
  - the low-battery guard, and its limit: the inverter's own window start turns the output on even during a cut-off;
  - the clock dependency, with a daily `sync_clock` automation;
  - that the switch owns the timer while Off;
  - that it's verified on the LV2424 only.

  Also note the whole-hour rule in the window entities section.
- [ ] **Step 2: `docs/pi18-protocol.md`.** Add:
  - the `ACLT` semantics from the spec's "verified live" list;
  - `LON0` refused 4 out of 4, including from SolarPower itself;
  - equal start and end stored as `0000,0000`.

  Correct "never reads FLAG field 8": the vendor reads it as flag I, machine type (`P18ComUSBProcessor.java:1171`).
- [ ] **Step 3: Example.** Add `ac_output` to `esp32-pi18-example.yaml`'s pi18 switch block, with a one-line comment.
- [ ] **Step 4:** run `cfg.sh` (example configs valid), then commit: "Document the pi18 ac_output switch". This is docs only, so no `verify`.

### Task 6: Whole-branch review

- [ ] **Step 1:** Run `run.sh`, `cfg.sh`, `cc.sh` and `host-compile.sh` on the tip. Expected: the baselines only.
- [ ] **Step 2:** One final review of `fe150c6..HEAD` on the most capable model, with the spec and the Review Focus list. One fix wave, then a scoped re-review.

### Task 7: Live verification on the LV2424 (owner's go-ahead at each step)

- [ ] **Step 1: Prepare.** Push `pi18-ac-output`. Point the live config at it and add `ac_output` to the pi18 switch block. Validate, then flash.
- [ ] **Step 2: Check the state.** The switch shows On with window `0000,0000`.
- [ ] **Step 3: Off.** Expect:
  - `^S016ACLT<h-1>00,<h>00` accepted;
  - output 0 V within seconds;
  - the switch shows Off after the next `ACLT` reading.
- [ ] **Step 4: On.** Expect `^S016ACLT0000,0000` accepted, the output back within about 3 s, and the switch On.
- [ ] **Step 5: Reboot test.** Off, then reflash or reboot the ESP32. Expect:
  - no write while the window is far from opening;
  - the switch shows Off.

  Then On to finish. Re-arm itself is covered by unit tests; a live re-arm takes about 23 hours.

### Task 8: Land

- [ ] **Step 1:** With the owner's go-ahead, fast-forward `lv2424` to `pi18-ac-output` and push it. Point the live config back at `lv2424` and flash.
- [ ] **Step 2:** Update the memory notes `mppsolar-ha-deployment.md` and `pi18-vendor-protocol.md`.
