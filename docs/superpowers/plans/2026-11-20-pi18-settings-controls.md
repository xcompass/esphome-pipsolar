# pi18 Settings Controls Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the LV2424's battery, AC-window, LCD-sleep and clock settings
ordinary Home Assistant entities. Each shows the inverter's value and writes
it on change.

**Architecture:**
- **Hub owns the settings.** The `Pipsolar` hub keeps the known value of
  every setting: the last reading, or a pending write. It encodes the `^S`
  commands and applies the shared write rules.
- **Entities are thin.** New entity classes (`PipsolarNumber`,
  `PipsolarTime`, a sync-clock button) validate their own input, call the
  hub, and get read-backs published to them.
- **Poll schedules.** Polls gain a schedule (every update, slow at 60 s, or
  once), so the new setting queries don't slow the status polls.

**Tech Stack:**
- ESPHome external component: C++17, plus Python codegen and schema.
- GoogleTest through ESPHome's `script/cpp_unit_test.py`.
- pytest, ruff, pylint, clang-format, and ESPHome's `ci-custom.py`.

**Spec:** `docs/superpowers/specs/2026-11-19-pi18-settings-controls-design.md`
(approved; commit 6b0ead7). Read it before any task.

## Global Constraints

**Branch and tooling**
- Branch `pi18-settings`, from `lv2424` 7e85d13.
- ESPHome ≥ 2026.3.0 (`require_esphome_version(2026, 3, 0)`). The live device
  builds with ESPHome 2026.10.2.

**Owner's go-ahead**
- Each of these needs the owner's go-ahead at the time it happens:
  - pushing;
  - flashing the ESP32 (192.168.10.51);
  - editing the live config `/usr/share/hassio/homeassistant/esphome/mppsolar.yaml`
    on root@192.168.1.5;
  - any set (`^S`) command that reaches the inverter.
- Read secrets by key name only. Never print their values.

**Commits**
- Use the repo's commit style: a capitalised imperative subject with no
  `feat:` prefix, and no attribution trailer.
- Run `/verify` right before each commit that changes code. Docs-only and
  test-only commits skip it.

**Running the tests**
- Tests run in Docker with the ESPHome dev checkout in the scratchpad.
  `cpp1.sh` runs the pi18 C++ tests; `run.sh` runs everything CI runs;
  `cfg.sh` validates the example configs; `cc.sh` runs `ci-custom` per
  component. Mount the worktree as `/repo` if you work in one:
  ```bash
  S=/private/tmp/claude-504/-Users-compass-projects-esphome-pipsolar/4a04a023-c96b-4277-ade6-728b07873144/scratchpad
  docker run --rm -v /Users/compass/projects/esphome-pipsolar:/repo:ro -v $S/ws2:/ws2 python:3.12-bookworm \
    bash -c 'apt-get update -qq >/dev/null && apt-get install -y -qq rsync >/dev/null; bash /ws2/cpp1.sh'
  ```
- **If `$S/ws2` is gone,** rebuild it the way `ws2/setup.sh` did:
  1. Shallow-clone `esphome/esphome` branch `dev` to `/ws2/esphome`.
  2. Create `/ws2/venv`.
  3. `pip install -r requirements.txt -r requirements_test.txt -e esphome`.
  4. Recreate `cpp1.sh`: activate the venv, rsync `/repo/components/*` and
     `/repo/tests/components/*` into the checkout, then run
     `script/cpp_unit_test.py pi18`.
- **Known baselines, not regressions:**
  - `run.sh` fails the example-config step only because its fake secrets are
    1 character long. `cfg.sh` uses longer ones.
  - `ci-custom` over all components together shows 3 duplicate-constant
    warnings that upstream also has. Per component (`cc.sh`) it is clean.

**Spec values used by several tasks**

| Setting | Command | Read back |
|---|---|---|
| Battery cut-off | `^S010PSDVnnn` | PIRI 10 |
| Bulk + float | `^S015MCHGVbbb,fff` (existing `charge_voltage_command`) | PIRI 11, 12 |
| Back to utility + back to battery | `^S014BUCDrrr,ddd` (back to battery "full" = `000`) | PIRI 8, 9 |
| Max charging current | `^S013MCHGC0,nnn` | PIRI 15 |
| Max AC charging current | `^S014MUCHGC0,nnn` | PIRI 14 |
| Battery type | `^S007PBTn` (0 AGM, 1 Flooded, 2 User) | PIRI 13 |
| LCD sleep | `^S008LSTnn` (`00` None, `01` 30 s, `02` 60 s, `10` 5 min, `20` 10 min) | `^P006LST` |
| AC charge window | `^S016ACCThhmm,hhmm` | `^P007ACCT` |
| AC output window | `^S016ACLThhmm,hhmm` | `^P007ACLT` |
| Clock | `^S018DATyyMMddHHmmss` | `^P004T` |
| Allowed currents | | `^P009MCHGCR`, `^P010MUCHGCR` |

Voltages are sent in tenths and currents in whole amps, each as 3 digits.

- **Default ranges (min–max / step):**
  - 24 V:
    - cut-off 20.4–24.0 / 0.1;
    - bulk and float 24.0–29.2 / 0.1;
    - back to utility 22.0–25.5 / 0.5;
    - back to battery 0 or 24.0–29.0 / 0.5.
  - 48 V:
    - cut-off 40.8–48.0 / 0.1;
    - bulk and float 48.0–58.4 / 0.1;
    - back to utility 44.0–51.0 / 1.0;
    - back to battery 0 or 48.0–58.0 / 1.0.
  - Currents, at either voltage:
    - max charging 10–90 / 10;
    - max AC charging 2–90 / 1.
- **Poll limits:**
  - `POLLING_COMMANDS_MAX` = 16.
  - Slow polls run every 60 s.
  - A slow or once-only poll that fails 3 consecutive intervals is dropped
    until reboot.
  - `POLL_ATTEMPTS_MAX` stays 3.

## Review Focus

1. **An automation re-sends the same value**, every minute or twice before
   the inverter has answered. Expected: one command, then nothing.
   *Test:* Task 3, `SecondIdenticalWriteWhilePendingIsSkipped`.
2. **A slow poll gets no reply at all** (timeouts, not `^0`). Expected:
   at most 3 sends per interval, and status polls keep running in between.
   *Test:* Task 2, `UnansweredSlowPollStopsAfterThreeAttempts`.
3. **Two writes to one pair in quick succession** (bulk, then float).
   Expected: the second command carries the first's new value.
   *Test:* Task 3, `PairWritesInQuickSuccessionCombine`.
4. **A typed value between steps or in the "full" gap** (back to battery
   12.0, or 28.15 V). Expected: no command, and the entity snaps back.
   *Test:* Task 4, `OffStepAndFullGapValuesAreRejected`.
5. **The inverter refuses a write (`^0`).** Expected: the entity reverts to
   the inverter's value on the first reading after the refusal.
   *Test:* Task 3, `RefusedWriteRevertsOnNextReading`.

---

### Task 1: Probe the unverified queries on the LV2424

Live and read-only. The owner's go-ahead is needed before flashing.

**Files:**
- Create: `tests/components/pi18/frames_lv2424.h`
- Modify: `docs/pi18-protocol.md` (the "Other queries" table and "Not implemented")

**Interfaces:**
- Produces frames named `LV2424_P007PIRI`, `LV2424_P004T`, `LV2424_P006LST`,
  `LV2424_P007ACCT`, `LV2424_P007ACLT`, `LV2424_P009MCHGCR` and
  `LV2424_P010MUCHGCR`. Each is a `static const char[]` holding the reply
  without CRC and CR, like `frames_lv5048.h`.
- A query that got only `^0` gets no frame. Its name goes in the header comment instead.
- Later tasks drop what depends on a missing frame:
  - `LST` → the LCD sleep half of Task 5;
  - `ACCT`/`ACLT` → Task 6;
  - `T` → Task 7.

- [ ] **Step 1: Write the probe config**

  Copy the live `mppsolar.yaml` to `mppsolar-probe.yaml` in the same
  directory, keeping the node name. Then change it as follows.
  - Set `pi18: update_interval: 1h`. One status round runs at boot, then the
    bus is quiet.
  - Add `debug:` to `uart_0`:
    - `direction: BOTH`;
    - `dummy_receiver: false`;
    - `after: delimiter: "\r"`;
    - `sequence: - lambda: UARTDebug::log_string(direction, bytes);`.
  - Add an `interval: 5s` lambda that waits 30 s, then sends each query 3
    times, one per tick:
    - the queries are `^P004T`, `^P006LST`, `^P007ACCT`, `^P007ACLT`,
      `^P009MCHGCR` and `^P010MUCHGCR`;
    - the CRC is `crc16be` over the query, with each CRC byte equal to
      `0x28`, `0x0D` or `0x0A` incremented by one;
    - each query ends with CR.

- [ ] **Step 2: Validate the config**

  Run: `docker exec app_5c53de3b_esphome esphome config /config/esphome/mppsolar-probe.yaml`

  Expected: `Configuration is valid!`

- [ ] **Step 3: With the owner's go-ahead, flash it and capture the log**

  1. Flash: `docker exec app_5c53de3b_esphome esphome run /config/esphome/mppsolar-probe.yaml --device 192.168.10.51 --no-logs`
  2. Capture the log, with `timeout` inside the container:
     `docker exec app_5c53de3b_esphome timeout 150 esphome logs /config/esphome/mppsolar-probe.yaml --device 192.168.10.51`
  3. Save the log to `$S/live-probe.log`.

  Expected: three `<<<` replies for each query, plus the boot-time PIRI
  reply.

- [ ] **Step 4: Restore the device**

  Flash `mppsolar.yaml` back the same way, delete `mppsolar-probe.yaml`, and
  check the config repo's `git status` is clean.

  Expected: the device runs the same firmware as before, and every status
  entity updates again.

- [ ] **Step 5: Record the results**

  - **Frames.** Write the replies into `frames_lv2424.h`. Add a header
    comment naming the date, the device, and any query that only got `^0`.
  - **Protocol doc.** In `docs/pi18-protocol.md`, mark each query as
    verified (or refused) on an LV2424.
  - **Format differences.** Where a reply format differs from the app's
    description, the doc follows the inverter, and the later tasks parse
    what was captured.

- [ ] **Step 6: Commit** (docs and tests only, so no `/verify`)

  ```bash
  git add tests/components/pi18/frames_lv2424.h docs/pi18-protocol.md
  git commit -m "Record the LV2424's replies to the PI18 settings queries"
  ```

---

### Task 2: Slow and once-only poll schedules

**Files:**
- Modify: `components/pi18/pipsolar.h`, `components/pi18/pipsolar.cpp`
- Modify: `tests/components/pi18/common.h`
- Create: `tests/components/pi18/pi18_poll_schedule_test.cpp`

**Interfaces:**
- Produces:
  - **New enum values** in `ENUMPollingCommand`:
    - `POLLING_P004T = 7`
    - `POLLING_P006LST = 8`
    - `POLLING_P007ACCT = 9`
    - `POLLING_P007ACLT = 10`
    - `POLLING_P009MCHGCR = 11`
    - `POLLING_P010MUCHGCR = 12`
  - **Schedule type:** `enum class PollSchedule : uint8_t { EVERY_UPDATE, SLOW, ONCE };`
  - **Registration** (protected):
    `void add_polling_command_(const char *command, ENUMPollingCommand id, PollSchedule schedule = PollSchedule::EVERY_UPDATE);`
  - **Refresh** (public):
    `void request_poll_refresh(ENUMPollingCommand id);`
    It sends the poll again soon. It does nothing for an unsupported poll.
  - **Constants:**
    - `static const uint32_t SLOW_POLL_INTERVAL_MS = 60000;`
    - `static const uint8_t UNSUPPORTED_AFTER_INTERVALS = 3;`
    - `POLLING_COMMANDS_MAX = 16`
  - **Test helpers** in `TestablePipsolar`:
    - `using Pipsolar::add_polling_command_;`
    - `std::vector<uint8_t> reply(const std::string &text) const` returns
      text + `pipsolar_crc_` bytes + CR.
    - `const std::string &queued_command(size_t n = 0) const`.
    - `void run(FakeUart &uart, int loops = 6)` calls `loop()` that many
      times.

- [ ] **Step 1: Write the failing tests**

  The tests go in `pi18_poll_schedule_test.cpp`. Each uses
  `set_update_interval(30000)`, so 2 updates make one slow interval, and
  registers `^P006LST` as `PollSchedule::SLOW` next to a `device_mode_id`
  sensor (an every-update poll). Assertions:

  ```cpp
  TEST(Pi18PollScheduleTest, SlowPollIsSentOncePerInterval)
  // answered once; then update(); run → count_sent("^P006LST") == 1; update(); run → == 2
  TEST(Pi18PollScheduleTest, StatusPollsStillRunEveryUpdate)
  // across 2 updates: count_sent("^P006MOD") == 2 while "^P006LST" == 1
  TEST(Pi18PollScheduleTest, RefreshSendsASlowPollBeforeItsInterval)
  // after the first answer: request_poll_refresh(POLLING_P006LST); run → count_sent("^P006LST") == 2
  TEST(Pi18PollScheduleTest, OncePollStopsAfterItsFirstAnswer)
  // ^P009MCHGCR as ONCE, answered; 6 more intervals → count_sent == 1
  TEST(Pi18PollScheduleTest, UnansweredSlowPollStopsAfterThreeAttempts)
  // no reply at all (let COMMAND_TIMEOUT pass with delay(5100) between loops, or
  // shorten it via a protected test hook): within one interval count_sent("^P006LST") == 3
  // and "^P006MOD" is still sent in between
  TEST(Pi18PollScheduleTest, SlowPollRefusedThreeIntervalsIsDropped)
  // ^0 to every send for 3 intervals; then 3 more intervals → no further sends;
  // request_poll_refresh doesn't revive it
  TEST(Pi18PollScheduleTest, AnswerResetsTheFailedIntervalCount)
  // refused 2 intervals, answered on the 3rd, refused 2 more → still polled
  ```

  Also update `PollCommandsCarryTheirLength` in `pi18_lv5048_test.cpp`. It
  must iterate the new `POLLING_COMMANDS_MAX` and still find 7 commands.

- [ ] **Step 2: Run them and see them fail**

  Run: `cpp1.sh` (command in Global Constraints).

  Expected: a compile error naming `PollSchedule` and `request_poll_refresh`.

- [ ] **Step 3: Implement the schedules in `pipsolar.h/.cpp`**

  - **Per poll, keep:** the schedule; this interval's attempts (refusals,
    plus timeouts for SLOW/ONCE); whether it was answered; the count of
    consecutive failed intervals; and an unsupported flag.
  - **`update()`:**
    - EVERY_UPDATE polls behave exactly as today.
    - Every `max(1, SLOW_POLL_INTERVAL_MS / max(get_update_interval(), 1))`
      updates, start a new interval for SLOW/ONCE polls. A poll sent but not
      answered in the last interval counts one failed interval.
    - At `UNSUPPORTED_AFTER_INTERVALS` failed intervals, log one `ESP_LOGW`
      ("… not supported by this inverter, no longer polled") and mark the
      poll unsupported.
    - A ONCE poll that has answered is never marked again.
  - **Timeouts:** a timed-out SLOW/ONCE poll counts an attempt, and stops
    for the interval at `POLL_ATTEMPTS_MAX`.

- [ ] **Step 4: Run all pi18 tests**

  Run: `cpp1.sh`

  Expected: every test passes, the existing 129 included.

- [ ] **Step 5: Commit** (`/verify` first)

  ```bash
  git add components/pi18/pipsolar.h components/pi18/pipsolar.cpp tests/components/pi18/
  git commit -m "Add slow and once-only pi18 poll schedules"
  ```

---

### Task 3: Number settings in the hub

**Files:**
- Modify: `components/pi18/pipsolar.h`, `components/pi18/pipsolar.cpp`
- Create: `tests/components/pi18/pi18_settings_test.cpp`

**Interfaces:**
- Consumes (Task 2):
  - `add_polling_command_(…, PollSchedule::ONCE)`
  - `request_poll_refresh`
  - test helpers `reply()`, `queued_command(n)` and `run()`
- Produces (in `pipsolar.h`):
  ```cpp
  enum class NumberSetting : uint8_t {
    BATTERY_UNDER_VOLTAGE, BATTERY_BULK_VOLTAGE, BATTERY_FLOAT_VOLTAGE,
    BATTERY_RECHARGE_VOLTAGE, BATTERY_REDISCHARGE_VOLTAGE,
    MAX_CHARGING_CURRENT, MAX_AC_CHARGING_CURRENT,
  };
  enum class WriteResult : uint8_t { QUEUED, UNCHANGED, REJECTED };
  class PipsolarNumber;  // Task 4
  // Registers ^P007PIRI; for the two currents also ^P009MCHGCR / ^P010MUCHGCR as ONCE
  void set_number(NumberSetting setting, PipsolarNumber *number);
  WriteResult write_number_setting(NumberSetting setting, float value);
  // Pending write if any, else the last reading; empty until the first PIRI reply
  esphome::optional<float> get_number_setting(NumberSetting setting) const;
  // Allowed values from MCHGCR / MUCHGCR; empty until answered
  const std::vector<int> &get_allowed_currents(NumberSetting setting) const;
  ```
  Also protected handlers `handle_p009mchgcr_(const char *)` and
  `handle_p010muchgcr_(const char *)`, plus `using` lines for them in
  `TestablePipsolar`.
- Existing `get_battery_bulk_voltage_setting()`,
  `get_battery_float_voltage_setting()` and `set_charge_voltages()` keep
  their signatures. They read and write the same BULK/FLOAT known values,
  and `set_charge_voltages` marks them pending.
- Until Task 4 adds `PipsolarNumber`, `set_number` stores only the pointer.
  It publishes through `PipsolarNumber::publish_state` from Task 4 on.

- [ ] **Step 1: Write the failing tests**

  These use `LV5048_P007PIRI_BANANNAJOE` (48 V: recharge 50.0, redischarge
  52.0, cut-off 48.0, bulk 55.4, float 54.0, AC charging 10, charging 60).
  Use `LV2424_P009MCHGCR` and `LV2424_P010MUCHGCR` where Task 1 captured
  them; otherwise use `LV5048_P009MCHGCR`.

  ```cpp
  TEST(Pi18SettingsTest, PiriSetsKnownValues)                 // all 7 get_number_setting values
  TEST(Pi18SettingsTest, CutOffWriteUsesTenths)               // 47.5 → "^S010PSDV475", QUEUED
  TEST(Pi18SettingsTest, BackToUtilityKeepsBackToBattery)     // RECHARGE 49.0 → "^S014BUCD490,520"
  TEST(Pi18SettingsTest, BackToBatteryFullIsZero)             // REDISCHARGE 0 → "^S014BUCD500,000"
  TEST(Pi18SettingsTest, ChargingCurrentWrites)               // 80 → "^S013MCHGC0,080"; AC 20 → "^S014MUCHGC0,020"
  TEST(Pi18SettingsTest, UnchangedValueSendsNothing)          // CUT_OFF 48.0 → UNCHANGED, queue empty
  TEST(Pi18SettingsTest, SecondIdenticalWriteWhilePendingIsSkipped)  // 47.5 twice → QUEUED then UNCHANGED, one command
  TEST(Pi18SettingsTest, PairWritesInQuickSuccessionCombine)  // BULK 55.0, FLOAT 53.5 → 2nd command "^S015MCHGV550,535"
  TEST(Pi18SettingsTest, PairWithUnknownPartnerIsRejected)    // no PIRI yet: RECHARGE 49.0 → REJECTED, queue empty
  TEST(Pi18SettingsTest, CurrentNotInAllowedListIsRejected)   // after MCHGCR: 65 → REJECTED; 70 → QUEUED
  TEST(Pi18SettingsTest, CurrentAllowedWhileListUnknown)      // no MCHGCR reply: 65 → QUEUED
  TEST(Pi18SettingsTest, ValueThatCannotBeEncodedIsRejected)  // CUT_OFF 100.0 / NAN → REJECTED
  TEST(Pi18SettingsTest, ReadingInFlightKeepsThePendingValue) // loop-driven: PIRI sent, then write 47.5,
      // then old PIRI reply arrives → get_number_setting == 47.5
  TEST(Pi18SettingsTest, RefusedWriteRevertsOnNextReading)    // loop-driven: write sent, ^0, next PIRI → 48.0
  TEST(Pi18SettingsTest, AcceptedWriteIsConfirmedByNextReading) // ^1, next PIRI (frame edited to 475) → 47.5
  ```

- [ ] **Step 2: Run them and see them fail**

  Run: `cpp1.sh`

  Expected: a compile error naming `NumberSetting` and `write_number_setting`.

- [ ] **Step 3: Implement in `pipsolar.h/.cpp`**

  - **Known values.** Keep known values and pending flags per
    `NumberSetting`. Bulk and float replace the existing two optionals, so
    the outputs share them.
  - **Unchanged check.** Compare in tenths for voltages and whole amps for
    currents, using `lroundf`.
  - **PIRI handling.** `handle_p007piri_` updates each known value unless
    that value is pending and a write is still outstanding. Outstanding means
    any queued command, or `state_` is `STATE_COMMAND` or
    `STATE_COMMAND_COMPLETE`. When nothing is outstanding, the reading
    replaces the value and clears the pending flag.
  - **Publishing.** After that, publish each registered number's known value.
  - **Command format.** Use the spec table, with 3-digit fields.
    `charge_voltage_command` stays the MCHGV encoder.
  - **Allowed currents.** The MCHGCR/MUCHGCR handlers parse the
    comma-separated integers. An empty or failed reply leaves the list
    unchanged.

- [ ] **Step 4: Run all pi18 tests**

  Run: `cpp1.sh`

  Expected: everything passes, including `pi18_output_test.cpp` and the
  existing charge-voltage tests.

- [ ] **Step 5: Commit** (`/verify` first)

  ```bash
  git add components/pi18/pipsolar.h components/pi18/pipsolar.cpp tests/components/pi18/
  git commit -m "Track pi18 battery and charging settings for writing"
  ```

---

### Task 4: The `number` platform and the 48 V output fix

**Files:**
- Create: `components/pi18/pipsolar_number.h`, `components/pi18/pipsolar_number.cpp`
  (top level, like `pipsolar_select.*`, because the hub publishes to it)
- Create: `components/pi18/number/__init__.py`
- Modify: `components/pi18/__init__.py`. Add `"number"` to `AUTO_LOAD`.
- Modify: `components/pi18/const.py`. Add `CONF_BATTERY_RECHARGE_VOLTAGE`,
  `CONF_BATTERY_REDISCHARGE_VOLTAGE` and `CONF_BATTERY_RATED_VOLTAGE`, moving
  any existing duplicates out of the sensor module.
- Modify: `components/pi18/output/__init__.py`. Set `_VOLTAGE_STEPS` to
  24.0–29.2 plus 48.0–58.4 V, in 0.1 V steps.
- Modify: `esp32-pi18-example.yaml`, `docs/pi18-protocol.md` ("Not implemented"), `README.md` (pi18 section)
- Test: `tests/components/pi18/pi18_number_test.cpp`, `tests/test_component_schemas.py`

**Interfaces:**
- Consumes (Task 3): `NumberSetting`, `WriteResult`, `write_number_setting`,
  `get_number_setting` and `set_number`.
- Produces:
  ```cpp
  class PipsolarNumber : public number::Number {
   public:
    void set_parent(Pipsolar *parent);
    void set_setting(NumberSetting setting);
    // Values above 0 and below this are rejected; 0 means "full" (back to battery only)
    void set_min_nonzero_value(float value);
   protected:
    void control(float value) override;
  };
  ```
- Python:
  - `number.TYPES`: a dict from each config key to a `NumberSetting` enum
    member. The keys are:
    - `battery_under_voltage`
    - `battery_bulk_voltage`
    - `battery_float_voltage`
    - `battery_recharge_voltage`
    - `battery_redischarge_voltage`
    - `current_max_charging_current`
    - `current_max_ac_charging_current`
  - `number.VOLTAGE_RANGES[24|48][key] = (min, max, step)`, using the values
    in Global Constraints.
  - `number.CURRENT_RANGES[key] = (min, max, step)`.

- [ ] **Step 1: Write the failing tests**

  C++ tests (`pi18_number_test.cpp`). Each sets traits (min, max, step),
  sets a parent that has had a PIRI reply, and calls
  `make_call().set_value(v).perform()`:

  ```cpp
  TEST(Pi18NumberTest, ReadingIsPublished)                    // after PIRI: number.state == 54.0 (float)
  TEST(Pi18NumberTest, ValueOnStepIsWritten)                  // float 53.5 → "^S015MCHGV554,535"
  TEST(Pi18NumberTest, OffStepAndFullGapValuesAreRejected)    // float 53.55 → no command, state stays 54.0;
      // redischarge traits 0–58 / 1.0 with min_nonzero 48: 12.0 → no command; 0 → "^S014BUCD500,000"
  TEST(Pi18NumberTest, UnchangedValueKeepsState)              // float 54.0 → no command, state 54.0
  ```

  Python tests (`TestPi18NumberTypes`):

  ```python
  def test_voltage_numbers_require_battery_rated_voltage()   # cv.Invalid mentioning battery_rated_voltage
  def test_current_numbers_do_not_need_battery_rated_voltage()
  def test_24v_defaults_match_the_spec()                      # VOLTAGE_RANGES[24] == the 5 tuples above
  def test_48v_defaults_match_the_spec()
  def test_output_voltage_defaults_cover_24v_and_48v()        # 24.0, 29.2, 48.0, 58.4 in _VOLTAGE_STEPS; 30.0 not
  ```

- [ ] **Step 2: Run them and see them fail**

  Run `cpp1.sh`, then `run.sh` and read its `== pytest` section.

  Expected: C++ fails to compile (no `pipsolar_number.h`). pytest fails to
  import `components.pi18.number`.

- [ ] **Step 3: Implement**

  - **`control()`:**
    - Reject a value off the step from the traits' min (tolerance
      step/1000), or above 0 and below the min_nonzero value. Log
      `ESP_LOGW` and republish the known value.
    - Otherwise call `write_number_setting`. Republish the known value
      unless the result is `QUEUED`.
  - **Schema:** `cv.Optional(CONF_BATTERY_RATED_VOLTAGE): cv.one_of(24, 48, int=True)`.
    - Each key uses `number.number_schema(PipsolarNumber, unit_of_measurement=…, device_class=…)`
      plus optional `min_value`, `max_value` and `step`.
    - An `cv.All` validator requires the rated voltage when any voltage key
      is present.
  - **`to_code`:**
    - `number.new_number(conf, min_value=…, max_value=…, step=…)`.
    - For back to battery, the traits minimum is 0, and
      `set_min_nonzero_value` gets the configured or default minimum.
    - Then `set_parent`, `set_setting` and `paren.set_number(...)`.
  - **Docs:**
    - The example config gets a `number:` block with `battery_rated_voltage: 48`
      and every key.
    - The protocol doc drops `PSDV`, `MCHGV`, `BUCD`, `MCHGC`, `MUCHGC`,
      `MCHGCR` and `MUCHGCR` from "Not implemented".
    - The README pi18 section lists the number platform. It includes the
      spec's `number.set_value` automation example (back to utility 24.0 V
      for generator charging).

- [ ] **Step 4: Run every check**

  Run `run.sh`, `cfg.sh` and `cc.sh`.

  Expected:
  - all C++ tests and pytest pass;
  - ruff, pylint and clang-format are clean;
  - both example configs are valid;
  - `ci-custom` is clean per component.

- [ ] **Step 5: Commit** (`/verify` first)

  ```bash
  git add components/pi18 tests esp32-pi18-example.yaml docs/pi18-protocol.md README.md
  git commit -m "Add pi18 number entities for battery and charging settings"
  ```

---

### Task 5: Battery type and LCD sleep selects

**Files:**
- Modify: `components/pi18/select/__init__.py`, `components/pi18/const.py`
  (`CONF_BATTERY_TYPE` if it isn't already shared, `CONF_LCD_SLEEP_TIME`)
- Modify: `components/pi18/pipsolar_select.h/.cpp`, `components/pi18/pipsolar.h/.cpp`
- Modify: the example config, the protocol doc and the README, as in Task 4
- Test: `tests/components/pi18/pi18_select_test.cpp`, `tests/test_component_schemas.py`

**Interfaces:**
- Consumes: `request_poll_refresh` and `PollSchedule::SLOW` (Task 2).
- Produces:
  - **Hub setters:**
    - `PIPSOLAR_SELECT(battery_type_select, P007PIRI)`;
    - a hand-written `void set_lcd_sleep_time_select(PipsolarSelect *select)`
      that registers `^P006LST` as SLOW.
  - **LST handler:** protected `handle_p006lst_(const char *)`, with a
    `using` line in the test class.
  - **`PipsolarSelect` additions:**
    - `void set_skip_unchanged(bool skip)`;
    - `void set_refresh_poll(ENUMPollingCommand poll)`.
    Both are set only for the two new types.
  - **Python:** `select.DEFAULT_MAPS = {key: (optionsmap, statusmap)}`, with
    the values from the Global Constraints table. The status maps are keyed
    `"0"`, `"1"`, `"2"` and `"10"`, `"20"`, as `publish_select_` sends
    `std::to_string`.

- [ ] **Step 1: Write the failing tests**

  ```cpp
  TEST(Pi18SelectTest, BatteryTypeReadFromPiri)        // BANANNAJOE field 13 = 2 → "User"
  TEST(Pi18SelectTest, LcdSleepReadFromLst)            // LV2424_P006LST → its option
  TEST(Pi18SelectTest, SelectingTheCurrentOptionSendsNothing)
  TEST(Pi18SelectTest, NewOptionIsQueuedAndRefreshesLst) // "5 min" → "^S008LST10"; LST sent again on next loop
  TEST(Pi18SelectTest, ExistingSelectsStillAlwaysWrite)  // output_source_priority same option → still queued
  ```
  ```python
  def test_new_selects_have_built_in_maps()   # schema accepts battery_type: {name: x} without optionsmap
  def test_built_in_maps_can_be_replaced()    # a user optionsmap wins
  ```

- [ ] **Step 2: Run them and see them fail**

  Run: `cpp1.sh` and pytest.

  Expected: compile and import failures naming the new setters and keys.

- [ ] **Step 3: Implement**

  - **`to_code`:** use `conf.get(CONF_OPTIONSMAP)` or the built-in map for
    the two new types. Existing types keep requiring their maps.
  - **`PipsolarSelect::control`:**
    - When skip-unchanged is set and the option equals the current option,
      log at debug and return.
    - After queuing, call `request_poll_refresh` if a refresh poll is set.
  - **PIRI handler:** publish `battery_type_select_` from field 13.

- [ ] **Step 4: Run every check**

  Run `run.sh`, `cfg.sh` and `cc.sh`.

  Expected: all clean, as in Task 4.

- [ ] **Step 5: Commit** (`/verify` first)

  ```bash
  git commit -am "Add pi18 battery type and LCD sleep time selects"
  ```

  Stage new files explicitly first.

---

### Task 6: AC charge and AC output windows

Skip this task if Task 1 found `ACCT`/`ACLT` unsupported.

**Files:**
- Create: `components/pi18/pipsolar_time.h`, `components/pi18/pipsolar_time.cpp`.
  Their contents are wrapped in `#ifdef USE_DATETIME_TIME`.
- Create: `components/pi18/datetime/__init__.py`
- Create: `tests/components/pi18/__init__.py`. It needs
  `override_manifest(manifest)`, whose `to_code` adds the define
  `USE_DATETIME_TIME`. Add any other defines the datetime sources need to
  compile in the test build.
- Modify: `components/pi18/__init__.py` (`"datetime"` in `AUTO_LOAD`), `pipsolar.h/.cpp`, `const.py`
- Modify: the example config, the protocol doc and the README
- Test: `tests/components/pi18/pi18_time_window_test.cpp`, `tests/test_component_schemas.py`

**Interfaces:**
- Consumes: `WriteResult`, the pending rules (Task 3), and the SLOW schedule
  (Task 2).
- Produces:
  ```cpp
  enum class TimeWindowEnd : uint8_t { AC_CHARGE_START, AC_CHARGE_END, AC_OUTPUT_START, AC_OUTPUT_END };
  class PipsolarTime;  // forward-declared in pipsolar.h; the pointer is stored unguarded
  void set_time_window_end(TimeWindowEnd end, PipsolarTime *entity);  // registers ^P007ACCT or ^P007ACLT as SLOW
  WriteResult write_time_setting(TimeWindowEnd end, uint8_t hour, uint8_t minute);
  esphome::optional<uint16_t> get_time_setting(TimeWindowEnd end) const;  // hour * 100 + minute
  // protected: handle_p007acct_(const char *), handle_p007aclt_(const char *)

  class PipsolarTime : public datetime::TimeEntity {
   public:
    void set_parent(Pipsolar *parent);
    void set_window_end(TimeWindowEnd end);
    void publish_time(uint8_t hour, uint8_t minute);  // seconds 0
   protected:
    void control(const datetime::TimeCall &call) override;
  };
  ```
  The config keys are `ac_charge_start_time`, `ac_charge_end_time`,
  `ac_output_start_time` and `ac_output_end_time`.

- [ ] **Step 1: Write the failing tests**

  ```cpp
  TEST(Pi18TimeWindowTest, WindowsReadFromAcctAndAclt)     // LV2424 frames (or "^D0122300,0600") → 23:00 and 06:00
  TEST(Pi18TimeWindowTest, StartWriteKeepsTheEnd)          // AC_CHARGE_START 22:30 → "^S016ACCT2230,0600"
  TEST(Pi18TimeWindowTest, OutputWindowUsesAclt)           // → "^S016ACLThhmm,hhmm"
  TEST(Pi18TimeWindowTest, UnknownPartnerIsRejected)       // before any ACCT reply → REJECTED
  TEST(Pi18TimeWindowTest, InvalidTimesAreTreatedAsMissing) // "^D0122460,0600" and "^D012ab00,0600" → nothing published
  TEST(Pi18TimeWindowTest, SameTimeSendsNothing)
  TEST(Pi18TimeWindowTest, EntityControlWritesAndIgnoresSeconds) // TimeCall 22:30:45 → "…2230,…"
  ```
  ```python
  def test_datetime_keys_present()   # the 4 keys in datetime TYPES
  ```

- [ ] **Step 2: Run them and see them fail**

  Run: `cpp1.sh` and pytest.

  Expected: compile and import failures naming `TimeWindowEnd`.

- [ ] **Step 3: Implement**

  - **Pairs:** use the Task 3 pending and outstanding-write rules.
  - **After a write:** `request_poll_refresh` for that window's poll.
  - **`control()`:** take any field missing from the call from the current
    state.
  - **Platform:** the datetime platform uses `datetime.time_schema(PipsolarTime)`
    and `datetime.new_datetime(conf)`.

- [ ] **Step 4: Run every check**

  Run `run.sh`, `cfg.sh` and `cc.sh`.

  Expected: all clean.

- [ ] **Step 5: Commit** (`/verify` first)

  ```bash
  git commit -m "Add pi18 time entities for the AC charge and output windows"
  ```

  Stage new files explicitly.

---

### Task 7: Inverter clock text sensor and sync button

Skip this task if Task 1 found `T` unsupported.

**Files:**
- Create: `components/pi18/button/__init__.py`, `components/pi18/button/pipsolar_button.h/.cpp`
- Modify: `components/pi18/text_sensor/__init__.py`, `pipsolar.h/.cpp`, `const.py`
- Modify: the example config (with `time: - platform: homeassistant` on
  ESP32 and `sntp` on ESP8266, or whatever the examples can validate), the
  protocol doc, and the README with the daily sync automation from the spec
- Test: `tests/components/pi18/pi18_clock_test.cpp`, `tests/test_component_schemas.py`

**Interfaces:**
- Consumes: `PollSchedule::SLOW` and `request_poll_refresh` (Task 2).
- Produces:
  ```cpp
  void set_inverter_time(text_sensor::TextSensor *sensor);  // registers ^P004T as SLOW
  static std::string clock_command(const ESPTime &now);     // "^S018DATyyMMddHHmmss"; "" if !now.is_valid()
  bool sync_clock(const ESPTime &now);                      // queues it and refreshes T; false if invalid
  // protected: handle_p004t_(const char *)

  class PipsolarSyncClockButton : public button::Button {
   public:
    void set_parent(Pipsolar *parent);
    void set_time(time::RealTimeClock *time);
   protected:
    void press_action() override;  // a warning and nothing sent when the time isn't valid
  };
  ```
  The config keys are text sensor `inverter_time` and button `sync_clock`
  (required `time_id`).

- [ ] **Step 1: Write the failing tests**

  ```cpp
  TEST(Pi18ClockTest, InverterTimeIsFormatted)      // LV2424_P004T (or LV5048_P004T) → "2023-03-20 11:46:13"
  TEST(Pi18ClockTest, PollErrorKeepsTheLastTime)
  TEST(Pi18ClockTest, ClockCommandUsesTwoDigitYear) // 2026-11-20 03:00:05 → "^S018DAT261120030005"
  TEST(Pi18ClockTest, InvalidTimeSendsNothing)      // ESPTime{} → sync_clock false, queue empty
  TEST(Pi18ClockTest, SyncRefreshesTheClockPoll)    // ^P004T sent again after the command
  ```
  ```python
  def test_sync_clock_requires_time_id()
  def test_inverter_time_text_sensor_present()
  ```

- [ ] **Step 2: Run them and see them fail**

  Run: `cpp1.sh` and pytest.

  Expected: compile and import failures.

- [ ] **Step 3: Implement**

  `handle_p004t_` publishes `YYYY-MM-DD HH:MM:SS` from the 14 digits. A
  reply that isn't 14 digits publishes nothing.

- [ ] **Step 4: Run every check**

  Run `run.sh`, `cfg.sh` and `cc.sh`.

  Expected: all clean.

- [ ] **Step 5: Commit** (`/verify` first)

  ```bash
  git commit -m "Add a pi18 inverter clock sensor and sync button"
  ```

---

### Task 8: Whole-branch review

- [ ] **Step 1: Run every check on the branch tip**

  Run `run.sh`, `cfg.sh` and `cc.sh`.

  Expected: the baselines only.

- [ ] **Step 2: Get a fresh review**

  Dispatch a fresh `cpp-reviewer` and a `python-reviewer` on
  `git diff 7e85d13..HEAD -- components tests`, with the spec path.

- [ ] **Step 3: Fix and commit**

  Fix CRITICAL and HIGH findings test-first. Commit each fix (`/verify`
  first).

---

### Task 9: Live verification on the LV2424

Every step needs the owner's go-ahead.

- [ ] **Step 1: Point the live config at the branch**

  - Push `pi18-settings` to origin.
  - In the live `mppsolar.yaml`, set the `external_components` ref to
    `pi18-settings`.
  - Add these blocks:
    - `time: - platform: homeassistant`;
    - `number:` with `battery_rated_voltage: 24`, the current ranges from
      Task 1's lists, and all keys;
    - `select:` with `battery_type` and `lcd_sleep_time`;
    - `datetime:` with the 4 window keys;
    - `inverter_time` and `sync_clock`.
  - Validate with `esphome config`.

  Expected: the config is valid.

- [ ] **Step 2: Flash and capture the log**

  Capture 120 s (`timeout` inside the container).

  Expected:
  - each new entity shows the inverter's value: cut-off 23.6, bulk 28.6,
    float 28.2, back to utility 25.5, back to battery 0;
  - no `refused` or `not supported` warnings except for queries Task 1
    found unsupported;
  - the status polls keep their ~2 s cycle.

- [ ] **Step 3: Test float voltage writes**

  Set float 28.1, then 28.2, through the Home Assistant API.

  Expected:
  - `^S015MCHGV286,281` with `command … accepted`, read-back 28.1;
  - then `^S015MCHGV286,282`, read-back 28.2.

- [ ] **Step 4: Test that an unchanged value sends nothing**

  Set float 28.2 again.

  Expected: no `^S` in the log.

- [ ] **Step 5: Test LCD sleep**

  Change LCD sleep to another option, then back.

  Expected: `^1` both times, with read-back after each.

- [ ] **Step 6: Sync the clock**

  Press `sync_clock`.

  Expected: `^S018DAT…` accepted, and `inverter_time` is within a few
  seconds of Home Assistant time.

  Battery type and cut-off aren't test-written.

---

### Task 10: Land on `lv2424`

- [ ] **Step 1: Regroup the commits**

  `git reset --soft` and recommit, so the branch reads:
  1. the design and plan;
  2. the probe results;
  3. one commit for Tasks 2–4 ("Add pi18 number entities for battery and
     charging settings");
  4. the selects;
  5. the windows;
  6. the clock;
  7. any review fixes, folded into the commit they fix.

  Then confirm the code is unchanged.

  Run: `git diff <pre-regroup tip>..HEAD`

  Expected: empty.

- [ ] **Step 2: Move `lv2424` to the branch**

  With the owner's go-ahead, `git branch -f lv2424 pi18-settings` (a
  fast-forward), then push `lv2424`.

- [ ] **Step 3: Point the live config back and flash**

  Set the live config's ref back to `lv2424`, flash, and commit the config
  repo change.

  Expected: the same entities and values as in Task 9.

- [ ] **Step 4: Update memory notes**

  Update `mppsolar-ha-deployment.md`: the new lv2424 layout, the flash date,
  and the entities added. Update `pi18-vendor-protocol.md`: which queries
  the LV2424 answers.
