# pi18 settings controls: design

Date: 2026-11-19. Status: draft, waiting for approval.

## Intent

The owner runs an MPP Solar LV2424 (24 V battery bank, PI18 protocol) with
this component on an ESP32, and sometimes has utility power or a generator on
the AC input. They want to change the inverter's battery and AC settings from
Home Assistant, both by hand and from automations: for example, charge from
the generator when the battery gets low, or move the back-to-utility voltage.

Today the component can read most of these settings but can't usefully
write them:
- **Outputs aren't visible.** Bulk/float voltage and both charging currents
  are ESPHome `output`s, which Home Assistant never sees.
- **48 V only.** The bulk/float outputs only accept 48.0–58.4 V, and the
  example's cut-off select lists 48 V values.
- **Not supported at all:** battery type, back-to-utility/back-to-battery
  voltage, the AC charge and AC output time windows, the LCD sleep time, and
  the inverter clock.

Each setting should become an ordinary Home Assistant entity that shows the
inverter's real value and can be set with the standard actions
(`number.set_value`, `select.select_option`, `time.set_value`,
`button.press`). The owner wants this upstream in syssi/esphome-pipsolar as
well as on their `lv2424` branch.

## Goals

1. A new `number` platform for pi18 covering:
   - battery cut-off, bulk, float, back-to-utility and back-to-battery voltage;
   - max charging current and max AC charging current.
   Each number shows the value the inverter reports and writes it on change.
2. Select types for battery type (`PBT`) and LCD sleep time (`LST`), read back
   from the inverter.
3. Time entities for the AC charge window (`ACCT`) and the AC output window
   (`ACLT`), read back from the inverter.
4. A text sensor showing the inverter clock (`T`), and a button that sets it
   from an ESPHome time source (`DAT`).
5. Fix the 48 V-only defaults of the existing bulk/float outputs.
6. Works on the LV2424, which is verified live. Other PI18 models aren't
   verified, but nothing in the design is LV2424-only.

## Non-goals

- Parallel systems: the parallel ID `k` in `MCHGC`/`MUCHGC` stays 0, as
  today.
- The other ~70 set commands the vendor app knows (generator, second output,
  grid-tie, `PF` factory reset, `CLE`, `SBT`/`BST`, `V`, `F`, `RS`, `PGR`).
- Clock sync logic inside the firmware: an automatic sync is a Home Assistant
  automation that presses the button. An example is in the docs.
- Removing or renaming any existing output, select, switch or sensor.
- A clock-drift sensor.

## Anti-goals

These would make the work a failure even if it builds:
- **Writes nobody asked for.** No setting is written at boot, on reconnect,
  or to "restore" a config value. Only a user or automation action writes.
- **Invalid values sent.** A value outside the entity's range, step or
  allowed list is never sent.
- **Guessed partner values.** `MCHGV` and `BUCD` each carry two settings. If
  the other value isn't known from the inverter yet, nothing is sent.
- **Rewriting what's already set.** Settings are most likely kept in the
  inverter's non-volatile memory, so a write that matches the known value is
  skipped. An automation that fires every minute must not rewrite
  the same value every minute.
- **Slower status readings.** The main status polls must keep their pace
  (about one full cycle every 2 s today).
- **Breaking existing configs.** Every config that validates on `lv2424`
  today still validates and behaves the same.

## Constraints

- ESPHome ≥ 2026.3.0 (the component's `require_esphome_version`). The live
  device runs 2026.10.2. The `number`, `datetime` (type `time`), `button` and
  `time` platforms all exist there.
- **Entity ranges are fixed at build time.** Home Assistant reads a number's
  min/max/step when the API connects. The first PIRI reply can arrive after
  Home Assistant has connected, so the inverter's reported battery rating
  can't set the ranges at runtime.
- **One serial line.** `loop()` sends one queued set command or one poll at
  a time. `COMMAND_QUEUE_LENGTH` is 10. `POLLING_COMMANDS_MAX` is 10, and 6 of
  those slots are in use on the live config.
- **Unverified queries.** The LV2424 has never been sent `T`, `LST`, `ACCT`,
  `ACLT`, `MCHGCR` or `MUCHGCR`. Their reply formats come from the decompiled
  vendor app (see `docs/pi18-protocol.md`), and the app has known bugs for
  this model class.
- **Upstream review.** Upstream (syssi) reviews every PR. Small PRs that each
  stand alone review better than one large one.
- **The fork is ahead of upstream.** `lv2424` carries 8 commits that upstream
  `main` (ee9ed2f) doesn't have, and this design builds on them:
  - 9e2744e: checks `^1`/`^0` replies to set commands;
  - 7d5c189: the working `MCHGV` write and shared bulk/float cache, and
    removal of the `power_saving` switch;
  - ee01b7c: refused-poll handling, the fault description, and the
    `FakeUart` test helper;
  - ca2a46c, a02de44, 664d9ec: `docs/pi18-protocol.md`;
  - 51fbaee: removes `power_saving` from the ESP8266 example;
  - 7e85d13: the FLAG switch fix (upstream PR #261, open).
- **Repo rules.** Run the C++ host tests, schema tests, ruff, pylint,
  clang-format and per-component `ci-custom.py` before each commit.
- **Owner rules.** Commits, pushes, PRs, flashing the ESP32 and any write to
  the inverter each need the owner's go-ahead at the time.

## Approaches considered

| | Approach | Trade-off |
|---|---|---|
| A | Keep the outputs. The owner writes template numbers in YAML that read the PIRI sensors and call `output.pi18.set_level`. | Least component code. About 15 lines of lambda YAML per setting, nothing reusable upstream, and clock/LCD/time windows still need component code. |
| **B** | **Native `number` platform in the component, plus selects, time entities and a button.** | **Chosen.** The most code (new platforms, tests, docs). In return each setting takes 3 lines of YAML, appears in Home Assistant with units and limits, works directly in automations, and can go upstream. |
| C | Selects for every setting. | Reuses the select platform. Voltage lists of 50+ options are awkward on a dashboard, and automations must pass exact strings such as `"28.4V"`. |

The owner picked B.

## Design

### Step 0: probe the unverified queries

Before writing feature code, build a throwaway firmware that sends each
unverified query 3 times after boot and logs the raw replies. About 1% of
polls get a passing `^0`, so one refusal doesn't prove a query is
unsupported. The queries are:
- `^P004T`
- `^P006LST`
- `^P007ACCT`
- `^P007ACLT`
- `^P009MCHGCR`
- `^P010MUCHGCR`

All of these are read-only. Flash it only with the owner's go-ahead,
capture the log, then flash the normal firmware back. Record the replies in
`docs/pi18-protocol.md`.

What the probe decides:
- **A query the LV2424 refuses (`^0`) or doesn't answer:** drop the
  feature that depends on it from this work, and say so in the protocol doc.
  Nothing could test it. Clock and LCD sleep need `T`/`LST`; the time windows
  need `ACCT`/`ACLT`.
- **`MCHGCR`/`MUCHGCR` answer:** their lists become the allowed values for the
  current numbers, and the owner's config ranges are set from them. If they
  don't answer, the current numbers validate only against their configured
  range.
- **A reply format that differs from the app's:** the parser follows the
  inverter, not the app.

### Number platform (`components/pi18/number/`)

```yaml
number:
  - platform: pi18
    pipsolar_id: inverter0
    battery_rated_voltage: 24   # 24 or 48; picks the default ranges
    battery_under_voltage:
      name: Battery cut-off voltage
    battery_bulk_voltage:
      name: Bulk charge voltage
    battery_float_voltage:
      name: Float charge voltage
    battery_recharge_voltage:
      name: Back to utility voltage
    battery_redischarge_voltage:
      name: Back to battery voltage
    current_max_charging_current:
      name: Max charging current
    current_max_ac_charging_current:
      name: Max AC charging current
```

| Key | Command | Read back from | 24 V default min–max / step | 48 V default |
|---|---|---|---|---|
| `battery_under_voltage` | `^S010PSDVnnn` | PIRI 10 | 20.4–24.0 / 0.1 V | 40.8–48.0 / 0.1 |
| `battery_bulk_voltage` | `^S015MCHGVbbb,fff` (pair) | PIRI 11 | 24.0–29.2 / 0.1 V | 48.0–58.4 / 0.1 |
| `battery_float_voltage` | same (pair) | PIRI 12 | 24.0–29.2 / 0.1 V | 48.0–58.4 / 0.1 |
| `battery_recharge_voltage` | `^S014BUCDrrr,ddd` (pair) | PIRI 8 | 22.0–25.5 / 0.5 V | 44.0–51.0 / 1.0 |
| `battery_redischarge_voltage` | same (pair) | PIRI 9 | 0 or 24.0–29.0 / 0.5 V | 0 or 48.0–58.0 / 1.0 |
| `current_max_charging_current` | `^S013MCHGC0,nnn` | PIRI 15 | 10–90 / 10 A | same |
| `current_max_ac_charging_current` | `^S014MUCHGC0,nnn` | PIRI 14 | 2–90 / 1 A | same |

How the ranges work:
- **Where the defaults come from.** The 24 V ranges are those the vendor app
  offers for this model class. The 48 V ranges are the 24 V ones doubled, and
  are unverified. The current ranges are the existing output defaults.
- **Overrides.** Each entity accepts `min_value`, `max_value` and `step` to
  override its default.
- **`battery_rated_voltage` is required** when any voltage number is
  configured.
- **Back to battery "full".** On the LCD and in the app, back-to-battery can
  be "FULL" (battery fully charged), which is sent as `000`. The number shows
  and accepts **0** for this. Its range is therefore 0 to the maximum, and the
  component rejects values between 0 and the real minimum.
- **Allowed current values.** When the inverter has answered
  `MCHGCR`/`MUCHGCR`, a current value must also be in that list.

How the numbers behave:
- **Read-back.** On every PIRI reply each number publishes the inverter's
  value. A missing field (failed poll) leaves it unchanged, as the
  selects and switches do today.
- **No optimistic state.** After a write the number keeps showing the old value
  until the next PIRI reply shows what the inverter accepted, about 2 s later.
- **Shared write rules** apply (below).

### Selects

Two new select types on the existing `select` platform. Unlike the existing
types, each has a built-in options map and status map, so the YAML needs only
a name. A user-supplied `optionsmap`/`statusmap` replaces the built-in one.

| Key | Options → command | Read back from |
|---|---|---|
| `battery_type` | AGM `^S007PBT0`, Flooded `^S007PBT1`, User `^S007PBT2` | PIRI 13 (values with no option leave the select unchanged, as today) |
| `lcd_sleep_time` | None `00`, 30 s `01`, 60 s `02`, 5 min `10`, 10 min `20`, sent as `^S008LSTnn` | `LST` poll |

Caveats:
- **Voltage writes may need battery type "User".** MPP Solar manuals say
  bulk/float voltages can only be changed when the battery type is "User".
  This is unverified on the LV2424. The component doesn't block such writes.
  The inverter's `^0` is logged and read-back shows the real value.
- **Changing the battery type may reset the voltages.** The docs warn about
  this.

### Time windows (`components/pi18/datetime/`, type `time`)

```yaml
datetime:
  - platform: pi18
    pipsolar_id: inverter0
    ac_charge_start_time:
      name: AC charge start
    ac_charge_end_time:
      name: AC charge end
    ac_output_start_time:
      name: AC output start
    ac_output_end_time:
      name: AC output end
```

- **Commands.** Start and end of one window are a pair:
  `^S016ACCThhmm,hhmm` and `^S016ACLThhmm,hhmm`. Writing one fills in the
  other from the last read value, under the same pair rules as `MCHGV`.
  Seconds are ignored.
- **Read-back** comes from the `ACCT`/`ACLT` polls (slow polls, below).
- **Equal start and end.** What the inverter does when start equals end
  (e.g. `0000,0000`) is unknown. The probe and the manual decide whether the
  docs call it "no window" or "always". The component publishes whatever the
  inverter reports.

### Clock

- **Text sensor `inverter_time`** (on the text_sensor platform). It comes from
  the `T` slow poll and is published as `YYYY-MM-DD HH:MM:SS`.
- **Button `sync_clock`** (new `button` platform). It requires
  `time_id` pointing at an ESPHome `time` component. On press:
  - if the time source isn't valid yet, it logs a warning and sends nothing;
  - otherwise it queues `^S018DATyyMMddHHmmss` in the time source's local
    time, then refreshes `T`.
- **Owner's config needs a time source.** The live config has none, so it gets
  `time: - platform: homeassistant`.

### Shared write rules (all numbers, selects, time entities)

1. **Validate.** The value must be in range, on the step and (for currents)
   in the allowed list when known. Otherwise: log a warning, send nothing,
   and republish the current known value so Home Assistant snaps back.
2. **Skip unchanged.** If the value equals the known value, log at debug and
   send nothing. The known value is the last read value, or a pending write
   (rule 4).
3. **Fill in the partner.** For a pair (`MCHGV`, `BUCD`, `ACCT`, `ACLT`), take
   the partner from its known value. If it's unknown, log a warning and send
   nothing.
4. **Record a pending write.** After queuing, store the written value(s) as
   pending. A second write right after (for example, an automation setting
   bulk then float) then pairs with the first instead of overwriting it.
   This generalises what `set_charge_voltages()` already does.
   - Only a reading whose poll was sent after the write went out replaces
     pending values with what the inverter reports. A reading already in
     flight when the write was queued doesn't, so it can't undo the pending
     value with the old one.
   - A refused write therefore reverts on the next reading after it.
5. **Refresh.** After a write, mark that setting's poll for an immediate
   refresh. PIRI already runs every cycle.

`MCHGV` keeps one shared cache, so the existing bulk/float outputs and the new
numbers can't send conflicting pairs.

These rules apply to the new entities only. The existing outputs, selects and
switches behave as they do now.

### Polling changes

- **Room for more polls.** Raise `POLLING_COMMANDS_MAX` from 10 to 16. The
  new polls are `T`, `LST`, `ACCT`, `ACLT`, `MCHGCR` and `MUCHGCR`; each is
  registered only when an entity that needs it is configured.
- **Slow polls.** `T`, `LST`, `ACCT` and `ACLT` refresh every 60 s and right
  after a write, instead of on every `update()`. The existing polls keep their
  current behaviour.
- **Once-only polls.** `MCHGCR` and `MUCHGCR` are polled every 60 s until
  their first successful reply, then never again until reboot.
- **Unsupported polls.** If a slow or once-only poll is refused (`^0`) or
  gets no reply on 3 consecutive intervals:
  - log once that the inverter doesn't support it;
  - stop polling it until reboot.
  Its entities stay unknown. The current numbers then validate against their
  configured range only.

### Existing outputs and selects

- **Outputs keep working.** The bulk/float `possible_values` default now
  covers 24.0–29.2 V and 48.0–58.4 V instead of 48 V only. Values that don't
  fit the inverter's rating are refused by the inverter, as before.
- **Selects and switches are unchanged.**
- **The example config** (`esp32-pi18-example.yaml`) shows the new number,
  select, datetime and button entries. The README and `docs/pi18-protocol.md`
  ("Not implemented") are updated to match.

### Owner's config and automations

The owner's `mppsolar.yaml` gains the blocks above, with these values:
- `battery_rated_voltage: 24`;
- the current ranges from the probe;
- a Home Assistant time source.

The docs include two example automations:

```yaml
# Charge from the generator when the battery is low
- action: number.set_value
  target: {entity_id: number.mppsolar_back_to_utility_voltage}
  data: {value: 24.0}
# Daily clock sync
- trigger: {trigger: time, at: "03:00:00"}
  action: {action: button.press, target: {entity_id: button.mppsolar_sync_clock}}
```

## Testing

- **C++ host tests** (`tests/components/pi18/`) for every write rule:
  - the exact command string;
  - skip unchanged;
  - pair fill, and refusal when the partner is unknown;
  - out-of-range, off-step and not-in-list rejection;
  - pending-then-read-back;
  - the back-to-battery 0 = full case.
  Also for each new parser (`T`, `LST`, `ACCT`/`ACLT`, `MCHGCR`/`MUCHGCR`,
  PIRI 13 for battery type), for slow and once-only poll cadence, and for
  dropping an unsupported slow poll. Frames come from the probe where
  available. All tests are written first and seen failing.
- **Schema tests** (`tests/test_component_schemas.py`) for the new platforms
  and keys, `battery_rated_voltage` being required, and the widened output
  defaults.
- **The repo's full checks**, as in the Constraints section.
- **Live on the LV2424.** Each step needs the owner's go-ahead.
  - Probe. Flash the feature build and confirm every entity shows the
    inverter's value.
  - Float voltage: change 28.2 → 28.1 → 28.2 V and see `^1` and read-back.
  - LCD sleep: change it and set it back.
  - Sync the clock.
  - Set the same value again and confirm no command is sent.
  - Battery type and cut-off aren't test-written.

## Delivery

The owner chose to build on the fork first and upstream afterwards.

1. **Build on `lv2424`.**
   - Work on a feature branch from `lv2424`, one commit per feature:
     1. the number platform and 48 V output fix;
     2. the battery type and LCD sleep selects;
     3. the AC time windows;
     4. the clock.
     Slow polls land with the first commit that needs them.
   - When the branch is done and verified live, fast-forward `lv2424` to it
     and flash.
   - Pushing and flashing each need the owner's go-ahead.
2. **Upstream the existing fork fixes**, each as its own small PR from
   upstream `main`, after #261. Whether the `power_saving` removal goes
   upstream as it stands is decided when that PR is opened. The fixes:
   - set-command reply checking;
   - the `MCHGV`/SolarPower fixes;
   - refused polls and the fault description;
   - the protocol notes.
3. **Upstream the four features** as separate PRs once the fixes they build
   on are merged, rebasing as needed.
4. **This document stays in the fork.** It is not part of any upstream PR.

## Calls made in this design (owner may override)

- **Back to battery "full" is the number 0**, not a separate switch or a
  select.
- **`battery_rated_voltage` is a required YAML option**, not read from
  the inverter. This is because Home Assistant fixes ranges at connect time.
  (This corrects the earlier message, which said the inverter's rating would
  pick the ranges.)
- **Clock sync is a button only.** Automatic sync is a Home Assistant
  automation.
- **Features whose query the LV2424 refuses are dropped**, not built
  untested.
- **Slow-poll interval is a fixed 60 s**, not a config option.
- **Voltage writes aren't blocked** when the battery type isn't "User".
- **The probe sends each query 3 times** before calling it unsupported.
- **Unanswered `MCHGCR`/`MUCHGCR` are dropped** like other unsupported polls.
- **Only a reading taken after a write replaces its pending value.**
- **The write rules apply to the new entities only.**

## Left to the builder

- **Class layout.** For example, one `PipsolarNumber` class with a type enum
  versus one class per command family; or how the hub stores known/pending
  values.
- **The poll-cadence mechanism** (per-poll interval and "once" flags, or
  something else), provided the behaviour above holds.
- **Build size.** Whether `number`, `datetime` and `button` join `AUTO_LOAD`
  or are guarded so builds that don't use them stay small.
- **Entity metadata.** Unit, device class, entity category and icon
  defaults.
- **Text.** Exact log wording and doc wording.
