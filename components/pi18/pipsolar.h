#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/time.h"

#include <vector>

namespace esphome::pi18 {

class PipsolarSelect;
class PipsolarNumber;
class PipsolarTime;

enum class NumberSetting : uint8_t {
  BATTERY_UNDER_VOLTAGE,
  BATTERY_BULK_VOLTAGE,
  BATTERY_FLOAT_VOLTAGE,
  BATTERY_RECHARGE_VOLTAGE,
  BATTERY_REDISCHARGE_VOLTAGE,
  MAX_CHARGING_CURRENT,
  MAX_AC_CHARGING_CURRENT,
};
static const size_t NUMBER_SETTING_COUNT = 7;
static_assert(static_cast<size_t>(NumberSetting::MAX_AC_CHARGING_CURRENT) + 1 == NUMBER_SETTING_COUNT,
              "NUMBER_SETTING_COUNT must follow the last NumberSetting");

// A short name for logs, such as "float voltage"
const char *setting_name(NumberSetting setting);

// One end of the AC charge window (^P007ACCT) or the AC output window (^P007ACLT)
enum class TimeWindowEnd : uint8_t { AC_CHARGE_START, AC_CHARGE_END, AC_OUTPUT_START, AC_OUTPUT_END };
static const size_t TIME_WINDOW_END_COUNT = 4;
static_assert(static_cast<size_t>(TimeWindowEnd::AC_OUTPUT_END) + 1 == TIME_WINDOW_END_COUNT,
              "TIME_WINDOW_END_COUNT must follow the last TimeWindowEnd");

enum class WriteResult : uint8_t {
  QUEUED,     // the set command is queued
  UNCHANGED,  // equals the known value; nothing sent
  REJECTED,   // can't be encoded, isn't an allowed current, the partner value is unknown or the queue is full;
              // nothing sent
};

enum ENUMPollingCommand {
  POLLING_P007PIRI = 0,
  POLLING_P005GS = 1,
  POLLING_P006MOD = 2,
  POLLING_P007FLAG = 3,
  POLLING_P006FWS = 4,
  POLLING_P005ET = 5,
  POLLING_P007PGS0 = 6,
  POLLING_P004T = 7,
  POLLING_P007ACCT = 9,
  POLLING_P007ACLT = 10,
  POLLING_P009MCHGCR = 11,
  POLLING_P010MUCHGCR = 12,
};

// EVERY_UPDATE polls run on each update(). SLOW polls run once per
// SLOW_POLL_INTERVAL_MS. ONCE polls run until they are first answered.
enum class PollSchedule : uint8_t { EVERY_UPDATE, SLOW, ONCE };

struct PollingCommand {
  uint8_t *command;
  uint8_t length = 0;
  uint8_t errors;
  // Refusals (^0), plus timeouts for SLOW and ONCE polls, since the last update,
  // interval start or answer
  uint8_t refusals = 0;
  ENUMPollingCommand identifier;
  bool needs_update;
  PollSchedule schedule = PollSchedule::EVERY_UPDATE;
  bool sent = false;           // sent during this interval (SLOW and ONCE)
  bool answered = false;       // answered at least once ever (ONCE) or during this interval (SLOW)
  bool ever_answered = false;  // a query that has answered once is never dropped as unsupported
  uint8_t failed_intervals = 0;
  bool unsupported = false;
};

struct QFLAGValues {
  esphome::optional<bool> silence_buzzer_open_buzzer;
  esphome::optional<bool> overload_bypass_function;
  esphome::optional<bool> lcd_escape_to_default;
  esphome::optional<bool> overload_restart_function;
  esphome::optional<bool> over_temperature_restart_function;
  esphome::optional<bool> backlight_on;
  esphome::optional<bool> alarm_on_when_primary_source_interrupt;
  esphome::optional<bool> fault_code_record;
  esphome::optional<bool> power_saving;
};

// Poll commands are sent as "^" + polling_command_name + CRC + CR
#define PIPSOLAR_ENTITY_(type, name, polling_command) \
 protected: \
  type *name##_{}; /* NOLINT */ \
\
 public: \
  void set_##name(type *name) { /* NOLINT */ \
    this->name##_ = name; \
    this->add_polling_command_("^" #polling_command, POLLING_##polling_command); \
  }

#define PIPSOLAR_SENSOR(name, polling_command) PIPSOLAR_ENTITY_(sensor::Sensor, name, polling_command)
#define PIPSOLAR_SWITCH(name, polling_command) PIPSOLAR_ENTITY_(switch_::Switch, name, polling_command)
#define PIPSOLAR_BINARY_SENSOR(name, polling_command) \
  PIPSOLAR_ENTITY_(binary_sensor::BinarySensor, name, polling_command)
#define PIPSOLAR_TEXT_SENSOR(name, polling_command) PIPSOLAR_ENTITY_(text_sensor::TextSensor, name, polling_command)
#define PIPSOLAR_SELECT(name, polling_command) PIPSOLAR_ENTITY_(PipsolarSelect, name, polling_command)

class Pipsolar : public uart::UARTDevice, public PollingComponent {
  // ^P005GS — general status (28 comma-separated fields)
  PIPSOLAR_SENSOR(grid_voltage, P005GS)
  PIPSOLAR_SENSOR(grid_frequency, P005GS)
  PIPSOLAR_SENSOR(ac_output_voltage, P005GS)
  PIPSOLAR_SENSOR(ac_output_frequency, P005GS)
  PIPSOLAR_SENSOR(ac_output_apparent_power, P005GS)
  PIPSOLAR_SENSOR(ac_output_active_power, P005GS)
  PIPSOLAR_SENSOR(output_load_percent, P005GS)
  PIPSOLAR_SENSOR(battery_voltage, P005GS)
  PIPSOLAR_SENSOR(battery_voltage_scc, P005GS)
  PIPSOLAR_SENSOR(battery_voltage_scc2, P005GS)
  PIPSOLAR_SENSOR(battery_discharge_current, P005GS)
  PIPSOLAR_SENSOR(battery_charging_current, P005GS)
  PIPSOLAR_SENSOR(battery_capacity_percent, P005GS)
  PIPSOLAR_SENSOR(inverter_heat_sink_temperature, P005GS)
  PIPSOLAR_SENSOR(mppt1_charger_temperature, P005GS)
  PIPSOLAR_SENSOR(mppt2_charger_temperature, P005GS)
  PIPSOLAR_SENSOR(pv1_input_power, P005GS)
  PIPSOLAR_SENSOR(pv2_input_power, P005GS)
  PIPSOLAR_SENSOR(pv1_input_voltage, P005GS)
  PIPSOLAR_SENSOR(pv2_input_voltage, P005GS)
  PIPSOLAR_BINARY_SENSOR(setting_value_configuration_state, P005GS)
  PIPSOLAR_TEXT_SENSOR(mppt1_charger_status, P005GS)
  PIPSOLAR_TEXT_SENSOR(mppt2_charger_status, P005GS)
  PIPSOLAR_TEXT_SENSOR(load_connection, P005GS)
  PIPSOLAR_TEXT_SENSOR(battery_power_direction, P005GS)
  PIPSOLAR_TEXT_SENSOR(dc_ac_power_direction, P005GS)
  PIPSOLAR_TEXT_SENSOR(line_power_direction, P005GS)

  // ^P007PIRI — device ratings (26 fields, ×0.1 for voltages/currents/frequencies;
  // field 25, the grid regulation standard, isn't parsed)
  PIPSOLAR_SENSOR(grid_rating_voltage, P007PIRI)
  PIPSOLAR_SENSOR(grid_rating_current, P007PIRI)
  PIPSOLAR_SENSOR(ac_output_rating_voltage, P007PIRI)
  PIPSOLAR_SENSOR(ac_output_rating_frequency, P007PIRI)
  PIPSOLAR_SENSOR(ac_output_rating_current, P007PIRI)
  PIPSOLAR_SENSOR(ac_output_rating_apparent_power, P007PIRI)
  PIPSOLAR_SENSOR(ac_output_rating_active_power, P007PIRI)
  PIPSOLAR_SENSOR(battery_rating_voltage, P007PIRI)
  PIPSOLAR_SENSOR(battery_recharge_voltage, P007PIRI)
  PIPSOLAR_SENSOR(battery_redischarge_voltage, P007PIRI)
  PIPSOLAR_SENSOR(battery_under_voltage, P007PIRI)
  PIPSOLAR_SENSOR(battery_bulk_voltage, P007PIRI)
  PIPSOLAR_SENSOR(battery_float_voltage, P007PIRI)
  PIPSOLAR_SENSOR(battery_type, P007PIRI)
  PIPSOLAR_SENSOR(current_max_ac_charging_current, P007PIRI)
  PIPSOLAR_SENSOR(current_max_charging_current, P007PIRI)
  PIPSOLAR_SENSOR(input_voltage_range, P007PIRI)
  PIPSOLAR_SENSOR(output_source_priority, P007PIRI)
  PIPSOLAR_SENSOR(charger_source_priority, P007PIRI)
  PIPSOLAR_SENSOR(parallel_max_num, P007PIRI)
  PIPSOLAR_SENSOR(machine_type, P007PIRI)
  PIPSOLAR_SENSOR(topology, P007PIRI)
  PIPSOLAR_SENSOR(output_mode, P007PIRI)
  PIPSOLAR_SENSOR(solar_power_priority, P007PIRI)
  PIPSOLAR_SENSOR(mppt_string, P007PIRI)

  // ^P006MOD — device mode
  PIPSOLAR_TEXT_SENSOR(device_mode, P006MOD)
  PIPSOLAR_SENSOR(device_mode_id, P006MOD)

  // ^P007FLAG — 9 enable/disable flags
  PIPSOLAR_BINARY_SENSOR(silence_buzzer_open_buzzer, P007FLAG)
  PIPSOLAR_BINARY_SENSOR(overload_bypass_function, P007FLAG)
  PIPSOLAR_BINARY_SENSOR(lcd_escape_to_default, P007FLAG)
  PIPSOLAR_BINARY_SENSOR(overload_restart_function, P007FLAG)
  PIPSOLAR_BINARY_SENSOR(over_temperature_restart_function, P007FLAG)
  PIPSOLAR_BINARY_SENSOR(backlight_on, P007FLAG)
  PIPSOLAR_BINARY_SENSOR(alarm_on_when_primary_source_interrupt, P007FLAG)
  PIPSOLAR_BINARY_SENSOR(fault_code_record, P007FLAG)
  // Field 8. SolarPower doesn't read it, so its meaning is unconfirmed; the
  // ^S006PEI/PDI command is the machine type setting, not power saving.
  PIPSOLAR_BINARY_SENSOR(power_saving, P007FLAG)

  // ^P006FWS — fault/warning status
  PIPSOLAR_SENSOR(fault_code, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_line_fail, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_output_circuit_short, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_over_temperature, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_fan_lock, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_battery_voltage_high, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_battery_low_alarm, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_battery_under_shutdown, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_over_load, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_eeprom_failed, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_power_limit, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_pv1_voltage_high, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_pv2_voltage_high, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_mppt1_overload, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_mppt2_overload, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_scc1_battery_too_low_to_charge, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_scc2_battery_too_low_to_charge, P006FWS)
  PIPSOLAR_BINARY_SENSOR(warning_battery_open, P006FWS)

  // ^P005ET — total generated energy (8-digit, kWh)
  PIPSOLAR_SENSOR(total_generated_energy, P005ET)

  // ^P007PGS0 — parallel unit aggregate status
  PIPSOLAR_SENSOR(total_ac_output_apparent_power, P007PGS0)
  PIPSOLAR_SENSOR(total_ac_output_active_power, P007PGS0)
  PIPSOLAR_SENSOR(total_output_load_percent, P007PGS0)
  PIPSOLAR_SENSOR(total_battery_charging_current, P007PGS0)

  // Raw response text sensors (one per command)
  PIPSOLAR_TEXT_SENSOR(last_p005gs, P005GS)
  PIPSOLAR_TEXT_SENSOR(last_p007piri, P007PIRI)
  PIPSOLAR_TEXT_SENSOR(last_p006mod, P006MOD)
  PIPSOLAR_TEXT_SENSOR(last_p007flag, P007FLAG)
  PIPSOLAR_TEXT_SENSOR(fault_description, P006FWS)
  // The YAML key keeps its old name so existing configs still validate
  PIPSOLAR_TEXT_SENSOR(last_p005fws, P006FWS)
  PIPSOLAR_TEXT_SENSOR(last_p005et, P005ET)

  // Selects — user defines optionsmap/statusmap in YAML with ^S commands
  PIPSOLAR_SELECT(output_source_priority_select, P007PIRI)
  PIPSOLAR_SELECT(charger_source_priority_select, P007PIRI)
  PIPSOLAR_SELECT(solar_power_priority_select, P007PIRI)
  PIPSOLAR_SELECT(input_voltage_range_select, P007PIRI)
  PIPSOLAR_SELECT(machine_type_select, P007PIRI)
  PIPSOLAR_SELECT(current_max_charging_current_select, P007PIRI)
  PIPSOLAR_SELECT(current_max_ac_charging_current_select, P007PIRI)
  PIPSOLAR_SELECT(battery_under_voltage_select, P007PIRI)
  PIPSOLAR_SELECT(battery_type_select, P007PIRI)

  // Inverter clock (^P004T) is read slowly, and again after every sync
  void set_inverter_time(text_sensor::TextSensor *sensor) {
    this->inverter_time_ = sensor;
    this->add_polling_command_("^P004T", POLLING_P004T, PollSchedule::SLOW);
  }

  // The AC output switch shows whether the AC output timer has the output on. It needs the clock,
  // the output window and the warnings, so it registers their polls.
  void set_ac_output_switch(switch_::Switch *sw);
  // Turns the output on or off through the AC output timer, which the LV2424 accepts in whole
  // hours only. Off writes the hour that just ended; On disables the timer. On is refused during
  // the battery under shutdown warning, or before the first warnings reply.
  WriteResult write_ac_output(bool on);
  // While set, each ^P004T reading re-arms the off window when it contains the inverter's time or
  // opens within the hour. Set by write_ac_output (QUEUED or UNCHANGED); never writes by itself.
  void set_ac_output_keep_off(bool keep_off) { this->ac_output_keep_off_ = keep_off; }
  bool get_ac_output_keep_off() const { return this->ac_output_keep_off_; }
  // On when the last ACLT reading has the output on at the inverter's clock; empty if either is unknown
  esphome::optional<bool> get_ac_output_state() const;
  // Output is on iff the timer is disabled (start == end) or the minute of day is in [start, end),
  // which wraps past midnight when start > end. Times are hour * 100 + minute.
  static bool window_contains(uint16_t start_hhmm, uint16_t end_hhmm, uint16_t minute_of_day);
  // From the last ^P004T reading; empty until read
  esphome::optional<uint16_t> get_inverter_minute_of_day() const { return this->inverter_minute_of_day_; }

  // "^S018DATyyMMddHHmmss" for a local time; empty when the time isn't valid
  static std::string clock_command(const ESPTime &now);
  // Queues the clock command and refreshes the clock reading. False when the time isn't
  // valid or the queue was full, with nothing sent.
  bool sync_clock(const ESPTime &now);

  // Switches — on/off commands are full ^S<len><cmd> strings
  PIPSOLAR_SWITCH(output_source_priority_switch, P007PIRI)
  PIPSOLAR_SWITCH(solar_power_priority_switch, P007PIRI)
  PIPSOLAR_SWITCH(charger_source_priority_solarfirst_switch, P007PIRI)
  PIPSOLAR_SWITCH(charger_source_priority_utility_switch, P007PIRI)
  PIPSOLAR_SWITCH(charger_source_priority_solaronly_switch, P007PIRI)
  PIPSOLAR_SWITCH(silence_buzzer_open_buzzer_switch, P007FLAG)
  PIPSOLAR_SWITCH(overload_bypass_function_switch, P007FLAG)
  PIPSOLAR_SWITCH(lcd_escape_to_default_switch, P007FLAG)
  PIPSOLAR_SWITCH(overload_restart_function_switch, P007FLAG)
  PIPSOLAR_SWITCH(over_temperature_restart_function_switch, P007FLAG)
  PIPSOLAR_SWITCH(backlight_on_switch, P007FLAG)
  PIPSOLAR_SWITCH(alarm_on_when_primary_source_interrupt_switch, P007FLAG)
  PIPSOLAR_SWITCH(fault_code_record_switch, P007FLAG)

  // False when the queue was full and the command was dropped
  bool queue_command(const std::string &command);

  // Any command queued, or one being sent or answered
  bool write_outstanding() const;

  // Sends the poll again soon. Does nothing for a poll that isn't registered or
  // that the inverter doesn't support.
  void request_poll_refresh(ENUMPollingCommand id);

  // Registers the number entity for a setting and the polls that feed it: ^P007PIRI, and for the
  // two currents their allowed-value lists (^P009MCHGCR, ^P010MUCHGCR) until first answered.
  void set_number(NumberSetting setting, PipsolarNumber *number);
  // Queues the set command for a setting (voltages in V, currents in A). A pair setting (bulk and
  // float, back to utility and back to battery) is sent together with its partner's known value.
  WriteResult write_number_setting(NumberSetting setting, float value);
  // The pending write if any, else the last reading; empty until the first PIRI reply
  esphome::optional<float> get_number_setting(NumberSetting setting) const;
  // The last PIRI reading, never a pending write; empty until the first PIRI reply
  esphome::optional<float> get_number_reading(NumberSetting setting) const;
  // Allowed values from ^P009MCHGCR / ^P010MUCHGCR; empty until answered or for a voltage
  const std::vector<int> &get_allowed_currents(NumberSetting setting) const;

  // Registers the time entity for one end of a window and the slow poll that feeds it
  // (^P007ACCT for the charge window, ^P007ACLT for the output window).
  void set_time_window_end(TimeWindowEnd end, PipsolarTime *entity);
  // Queues ^S016ACCT or ^S016ACLT. Start and end are sent together, the other end as it is known.
  WriteResult write_time_setting(TimeWindowEnd end, uint8_t hour, uint8_t minute);
  // The pending write if any, else the last reading, as hour * 100 + minute; empty until read
  esphome::optional<uint16_t> get_time_setting(TimeWindowEnd end) const;
  // The last reading, never a pending write
  esphome::optional<uint16_t> get_time_reading(TimeWindowEnd end) const {
    return this->times_[static_cast<size_t>(end)].reading;
  }

  // Bulk and float charge voltages (V): the pending write if any, else the last PIRI reading;
  // empty until the first PIRI reply.
  esphome::optional<float> get_battery_bulk_voltage_setting() const {
    return this->get_number_setting(NumberSetting::BATTERY_BULK_VOLTAGE);
  }
  esphome::optional<float> get_battery_float_voltage_setting() const {
    return this->get_number_setting(NumberSetting::BATTERY_FLOAT_VOLTAGE);
  }
  // Queues ^S015MCHGV, which always sets both voltages. Returns false, and queues
  // nothing, when a voltage is outside 0.1-99.9 V or the command queue is full.
  bool set_charge_voltages(float bulk_voltage, float float_voltage);
  // "^S015MCHGV<bulk>,<float>" in 0.1 V with three digits each, or "" if a value doesn't fit
  static std::string charge_voltage_command(float bulk_voltage, float float_voltage);

  void setup() override;
  void loop() override;
  void dump_config() override;
  void update() override;

 protected:
  static const size_t PIPSOLAR_READ_BUFFER_LENGTH = 200;
  static const size_t COMMAND_QUEUE_LENGTH = 10;
  static const size_t COMMAND_TIMEOUT = 5000;
  static const size_t POLLING_COMMANDS_MAX = 16;
  static const uint32_t SLOW_POLL_INTERVAL_MS = 60000;
  // A SLOW or ONCE poll that fails this many intervals in a row is dropped until reboot
  static const uint8_t UNSUPPORTED_AFTER_INTERVALS = 3;
  // Like SolarPower, a poll the inverter refuses is tried at most this often per update
  static const uint8_t POLL_ATTEMPTS_MAX = 3;

  void add_polling_command_(const char *command, ENUMPollingCommand id,
                            PollSchedule schedule = PollSchedule::EVERY_UPDATE);
  PollingCommand *find_polling_command_(ENUMPollingCommand id);
  void empty_uart_buffer_();
  uint8_t check_incoming_crc_();
  // Reply to a ^S set command: 1 accepted (^1), 0 refused (^0), -1 not a valid reply
  int8_t parse_command_reply_(uint8_t *reply, size_t length);
  uint16_t pipsolar_crc_(uint8_t *msg, uint8_t len);
  bool send_next_command_();
  bool send_next_poll_();

  void handle_poll_response_(ENUMPollingCommand polling_command, const char *message);
  void handle_poll_error_(ENUMPollingCommand polling_command);
  void handle_p005gs_(const char *message);
  void handle_p007piri_(const char *message);
  void handle_p004t_(const char *message);
  void handle_p006mod_(const char *message);
  void handle_p007flag_(const char *message);
  void handle_p006fws_(const char *message);
  void publish_fault_description_(int fault_code);
  void handle_p005et_(const char *message);
  void handle_p007pgs0_(const char *message);
  void handle_p007acct_(const char *message);
  void handle_p007aclt_(const char *message);
  void handle_p009mchgcr_(const char *message);
  void handle_p010muchgcr_(const char *message);

  // Publishes a PIRI field (raw / divisor) to the setting's number. It also becomes the known
  // value, unless a write to it is still outstanding.
  void apply_reading_(NumberSetting setting, esphome::optional<int32_t> raw, float divisor);
  void set_known_(NumberSetting setting, float value);
  // A reading doesn't replace the known value of a setting whose write is pending and still outstanding,
  // since the reading may predate the write
  bool keeps_pending_(bool pending) const;
  // The known value becomes a write the inverter hasn't confirmed by a reading
  template<typename State, typename Value> static void set_pending_(State &state, Value value) {
    state.value = value;
    state.pending = true;
  }
  // Publishes a window reading to both ends; it becomes the known value unless a write is outstanding
  void apply_time_window_(TimeWindowEnd start, uint16_t start_time, uint16_t end_time);
  void handle_time_window_(TimeWindowEnd start, const char *message);
  // Queues one ^S016ACCT/ACLT command for both ends and records both as pending
  WriteResult queue_window_(TimeWindowEnd start, uint16_t start_time, uint16_t end_time);
  void publish_ac_output_state_();
  WriteResult write_ac_output_(bool on);
  // Writes the hour that just ended as the off window, unless the window already keeps the output
  // off for the next hour. Shared by the Off request and the re-arm.
  static bool ac_output_off_write_needed_(uint16_t start, uint16_t end, uint16_t minute);
  WriteResult write_ac_output_off_(uint16_t start, uint16_t end, uint16_t minute);
  // Runs once per ^P004T reading while keep off is set; skipped while a window write is outstanding
  void rearm_ac_output_();
  static esphome::optional<uint16_t> parse_time_(const char *text);
  static std::vector<int> parse_current_list_(const char *message);

  text_sensor::TextSensor *inverter_time_{};
  switch_::Switch *ac_output_switch_{};
  esphome::optional<uint16_t> inverter_minute_of_day_;
  bool ac_output_keep_off_{false};
  // consecutive re-arm writes the window reading hasn't confirmed
  uint8_t ac_output_rearm_writes_{0};
  esphome::optional<bool> battery_under_shutdown_;  // from the last ^P006FWS reply

  void publish_mppt_status_(text_sensor::TextSensor *sensor, esphome::optional<int32_t> value);
  void publish_direction_(text_sensor::TextSensor *sensor, esphome::optional<int32_t> value);
  void publish_binary_sensor_(esphome::optional<bool> b, binary_sensor::BinarySensor *sensor);
  // Publishes NAN when the value is missing (failed poll or empty field)
  void publish_sensor_(sensor::Sensor *sensor, esphome::optional<int32_t> value, float scale = 1.0f);
  // Does nothing when the value is missing: selects and switches cannot be invalidated
  void publish_select_(PipsolarSelect *select, esphome::optional<int32_t> value);
  // On when value equals on_value; does nothing when the value is missing
  void publish_switch_(switch_::Switch *sw, esphome::optional<int32_t> value, int32_t on_value);

  void skip_start_(const char *message, size_t *pos);
  void skip_field_(const char *message, size_t *pos);
  std::string read_field_(const char *message, size_t *pos);
  esphome::optional<int32_t> read_int_(const char *message, size_t *pos);
  // Missing field -> empty optional; otherwise non-zero -> true
  esphome::optional<bool> read_flag_(const char *message, size_t *pos);
  void read_sensor_(const char *message, size_t *pos, sensor::Sensor *sensor, float scale = 1.0f);

  struct NumberState {
    esphome::optional<float> value;    // last reading, or the pending write
    esphome::optional<float> reading;  // last reading
    bool pending = false;              // value is a write the inverter hasn't confirmed by a reading
    PipsolarNumber *entity = nullptr;
  };
  NumberState numbers_[NUMBER_SETTING_COUNT];

  struct TimeState {
    esphome::optional<uint16_t> value;    // last reading, or the pending write
    esphome::optional<uint16_t> reading;  // last reading
    bool pending = false;                 // value is a write the inverter hasn't confirmed by a reading
    PipsolarTime *entity = nullptr;
  };
  TimeState times_[TIME_WINDOW_END_COUNT];
  std::vector<int> allowed_max_charging_currents_;
  std::vector<int> allowed_max_ac_charging_currents_;

  std::string command_queue_[COMMAND_QUEUE_LENGTH];
  uint8_t command_queue_position_ = 0;
  uint8_t read_buffer_[PIPSOLAR_READ_BUFFER_LENGTH];
  size_t read_pos_{0};

  uint32_t command_start_millis_ = 0;
  uint32_t command_timeout_ms_ = COMMAND_TIMEOUT;  // only tests change it
  uint32_t updates_in_interval_ = 0;
  uint8_t state_ = 0;  // STATE_IDLE
  enum State {
    STATE_IDLE = 0,
    STATE_POLL = 1,
    STATE_COMMAND = 2,
    STATE_POLL_COMPLETE = 3,
    STATE_COMMAND_COMPLETE = 4,
    STATE_POLL_CHECKED = 5,
  };

  uint8_t last_polling_command_ = 0;
  PollingCommand enabled_polling_commands_[POLLING_COMMANDS_MAX];
};

}  // namespace esphome::pi18

#undef PIPSOLAR_ENTITY_
#undef PIPSOLAR_SENSOR
#undef PIPSOLAR_SWITCH
#undef PIPSOLAR_BINARY_SENSOR
#undef PIPSOLAR_TEXT_SENSOR
#undef PIPSOLAR_SELECT
