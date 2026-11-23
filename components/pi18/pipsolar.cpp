#include "pipsolar.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "pipsolar_number.h"
#include "pipsolar_select.h"
#include "pipsolar_time.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::pi18 {

ESPHOME_LOG_TAG(TAG, "pi18");

void Pipsolar::setup() {
  this->state_ = STATE_IDLE;
  this->command_start_millis_ = 0;
}

void Pipsolar::empty_uart_buffer_() {
  uint8_t buf[64];
  size_t avail;
  while ((avail = this->available()) > 0) {
    if (!this->read_array(buf, std::min(avail, sizeof(buf)))) {
      break;
    }
  }
}

void Pipsolar::loop() {
  if (this->state_ == STATE_IDLE) {
    this->empty_uart_buffer_();

    if (this->send_next_command_()) {
      return;
    }

    if (this->send_next_poll_()) {
      return;
    }

    return;
  }
  if (this->state_ == STATE_COMMAND_COMPLETE) {
    const char *command = this->command_queue_[this->command_queue_position_].c_str();
    switch (this->parse_command_reply_(this->read_buffer_, this->read_pos_)) {
      case 1:
        ESP_LOGI(TAG, "command %s accepted", command);
        this->replied_this_interval_ = true;
        break;
      case 0:
        ESP_LOGW(TAG, "command %s refused", command);
        this->replied_this_interval_ = true;
        break;
      default:
        ESP_LOGW(TAG, "command %s: invalid reply (%zu bytes)", command, this->read_pos_);
        break;
    }
    this->command_queue_[this->command_queue_position_] = std::string("");
    this->command_queue_position_ = (command_queue_position_ + 1) % COMMAND_QUEUE_LENGTH;
    this->state_ = STATE_IDLE;
  }

  if (this->state_ == STATE_POLL_CHECKED) {
    ESP_LOGD(TAG, "poll %s decode", this->enabled_polling_commands_[this->last_polling_command_].command);
    this->handle_poll_response_(this->enabled_polling_commands_[this->last_polling_command_].identifier,
                                (const char *) this->read_buffer_);
    this->state_ = STATE_IDLE;
    return;
  }

  if (this->state_ == STATE_POLL_COMPLETE) {
    if (this->check_incoming_crc_()) {
      this->replied_this_interval_ = true;
      // A PI18 inverter refuses a query it doesn't support with ^0 (PI30 sends (NAK)
      const bool refused = (this->read_pos_ == 5 && this->read_buffer_[0] == '^' && this->read_buffer_[1] == '0') ||
                           (this->read_buffer_[0] == '(' && this->read_buffer_[1] == 'N' &&
                            this->read_buffer_[2] == 'A' && this->read_buffer_[3] == 'K');
      auto &poll = this->enabled_polling_commands_[this->last_polling_command_];
      if (refused) {
        ESP_LOGW(TAG, "poll %s refused", poll.command);
        if (++poll.refusals >= POLL_ATTEMPTS_MAX)
          poll.needs_update = false;
        this->handle_poll_error_(poll.identifier);
        this->state_ = STATE_IDLE;
        return;
      }
      poll.refusals = 0;
      poll.needs_update = false;
      poll.answered = true;
      poll.ever_answered = true;
      poll.failed_intervals = 0;
      this->state_ = STATE_POLL_CHECKED;
      return;
    } else {
      this->handle_poll_error_(this->enabled_polling_commands_[this->last_polling_command_].identifier);
      this->state_ = STATE_IDLE;
    }
  }

  if (this->state_ == STATE_COMMAND || this->state_ == STATE_POLL) {
    size_t avail = this->available();
    while (avail > 0) {
      uint8_t buf[64];
      size_t to_read = std::min(avail, sizeof(buf));
      if (!this->read_array(buf, to_read)) {
        break;
      }
      avail -= to_read;
      bool done = false;
      for (size_t i = 0; i < to_read; i++) {
        uint8_t byte = buf[i];

        if (this->read_pos_ >= PIPSOLAR_READ_BUFFER_LENGTH - 1) {
          this->read_pos_ = 0;
          this->empty_uart_buffer_();
          ESP_LOGW(TAG, "response data too long, discarding.");
          done = true;
          break;
        }
        this->read_buffer_[this->read_pos_] = byte;
        this->read_pos_++;

        if (byte == 0x0D) {
          this->read_buffer_[this->read_pos_] = 0;
          this->empty_uart_buffer_();
          if (this->state_ == STATE_POLL) {
            this->state_ = STATE_POLL_COMPLETE;
          }
          if (this->state_ == STATE_COMMAND) {
            this->state_ = STATE_COMMAND_COMPLETE;
          }
          done = true;
          break;
        }
      }
      if (done) {
        break;
      }
    }
  }
  if (this->state_ == STATE_COMMAND) {
    if (millis() - this->command_start_millis_ > this->command_timeout_ms_) {
      const char *command = this->command_queue_[this->command_queue_position_].c_str();
      this->command_start_millis_ = millis();
      ESP_LOGD(TAG, "command %s timeout", command);
      this->command_queue_[this->command_queue_position_] = std::string("");
      this->command_queue_position_ = (command_queue_position_ + 1) % COMMAND_QUEUE_LENGTH;
      this->state_ = STATE_IDLE;
      return;
    }
  }
  if (this->state_ == STATE_POLL) {
    if (millis() - this->command_start_millis_ > this->command_timeout_ms_) {
      auto &poll = this->enabled_polling_commands_[this->last_polling_command_];
      ESP_LOGD(TAG, "poll %s timeout", poll.command);
      if (poll.schedule != PollSchedule::EVERY_UPDATE && ++poll.refusals >= POLL_ATTEMPTS_MAX)
        poll.needs_update = false;
      this->handle_poll_error_(poll.identifier);
      this->state_ = STATE_IDLE;
    }
  }
}

int8_t Pipsolar::parse_command_reply_(uint8_t *reply, size_t length) {
  // ^1 (accepted) or ^0 (refused), two CRC bytes, CR
  if (length != 5 || reply[0] != '^' || (reply[1] != '0' && reply[1] != '1') || reply[4] != 0x0D)
    return -1;
  const uint16_t crc = this->pipsolar_crc_(reply, 2);
  if (reply[2] != (uint8_t) (crc >> 8) || reply[3] != (uint8_t) (crc & 0xff))
    return -1;
  return reply[1] == '1' ? 1 : 0;
}

uint8_t Pipsolar::check_incoming_crc_() {
  if (this->read_pos_ < 3)
    return 0;
  uint16_t crc16;
  crc16 = this->pipsolar_crc_(read_buffer_, read_pos_ - 3);
  if (((uint8_t) ((crc16) >> 8)) == read_buffer_[read_pos_ - 3] &&
      ((uint8_t) ((crc16) & 0xff)) == read_buffer_[read_pos_ - 2]) {
    ESP_LOGD(TAG, "CRC OK");
    read_buffer_[read_pos_ - 1] = 0;
    read_buffer_[read_pos_ - 2] = 0;
    read_buffer_[read_pos_ - 3] = 0;
    return 1;
  }
  ESP_LOGD(TAG, "CRC NOK expected: %X %X but got: %X %X", ((uint8_t) ((crc16) >> 8)), ((uint8_t) ((crc16) & 0xff)),
           read_buffer_[read_pos_ - 3], read_buffer_[read_pos_ - 2]);
  return 0;
}

bool Pipsolar::send_next_command_() {
  uint16_t crc16;
  if (!this->command_queue_[this->command_queue_position_].empty()) {
    const char *command = this->command_queue_[this->command_queue_position_].c_str();
    uint8_t byte_command[32];
    size_t length = this->command_queue_[this->command_queue_position_].length();
    if (length > sizeof(byte_command)) {
      ESP_LOGE(TAG, "Command too long: %zu", length);
      this->command_queue_[this->command_queue_position_].clear();
      return false;
    }
    for (size_t i = 0; i < length; i++) {
      byte_command[i] = (uint8_t) this->command_queue_[this->command_queue_position_].at(i);
    }
    this->state_ = STATE_COMMAND;
    this->command_start_millis_ = millis();
    this->empty_uart_buffer_();
    this->read_pos_ = 0;
    crc16 = this->pipsolar_crc_(byte_command, length);
    this->write_str(command);
    this->write(((uint8_t) ((crc16) >> 8)));
    this->write(((uint8_t) ((crc16) & 0xff)));
    this->write(0x0D);
    ESP_LOGD(TAG, "Sending command from queue: %s with length %zu", command, length);
    return true;
  }
  return false;
}

bool Pipsolar::send_next_poll_() {
  uint16_t crc16;
  for (uint8_t i = 0; i < POLLING_COMMANDS_MAX; i++) {
    this->last_polling_command_ = (this->last_polling_command_ + 1) % POLLING_COMMANDS_MAX;
    if (this->enabled_polling_commands_[this->last_polling_command_].length == 0) {
      continue;
    }
    if (!this->enabled_polling_commands_[this->last_polling_command_].needs_update) {
      continue;
    }
    this->enabled_polling_commands_[this->last_polling_command_].sent = true;
    this->state_ = STATE_POLL;
    this->command_start_millis_ = millis();
    this->empty_uart_buffer_();
    this->read_pos_ = 0;
    crc16 = this->pipsolar_crc_(this->enabled_polling_commands_[this->last_polling_command_].command,
                                this->enabled_polling_commands_[this->last_polling_command_].length);
    this->write_array(this->enabled_polling_commands_[this->last_polling_command_].command,
                      this->enabled_polling_commands_[this->last_polling_command_].length);
    this->write(((uint8_t) ((crc16) >> 8)));
    this->write(((uint8_t) ((crc16) & 0xff)));
    this->write(0x0D);
    ESP_LOGD(TAG, "Sending polling command: %s with length %d",
             this->enabled_polling_commands_[this->last_polling_command_].command,
             this->enabled_polling_commands_[this->last_polling_command_].length);
    return true;
  }
  return false;
}

bool Pipsolar::queue_command(const std::string &command) {
  uint8_t next_position = command_queue_position_;
  for (uint8_t i = 0; i < COMMAND_QUEUE_LENGTH; i++) {
    uint8_t testposition = (next_position + i) % COMMAND_QUEUE_LENGTH;
    if (command_queue_[testposition].empty()) {
      command_queue_[testposition] = command;
      ESP_LOGD(TAG, "Command queued successfully: %s at position %d", command.c_str(), testposition);
      return true;
    }
  }
  ESP_LOGD(TAG, "Command queue full dropping command: %s", command.c_str());
  return false;
}

std::string Pipsolar::charge_voltage_command(float bulk_voltage, float float_voltage) {
  if (!std::isfinite(bulk_voltage) || !std::isfinite(float_voltage))
    return "";
  const float bulk_tenths = std::round(bulk_voltage * 10.0f);
  const float float_tenths = std::round(float_voltage * 10.0f);
  // The length field (015) assumes exactly three digits per value
  if (bulk_tenths < 1.0f || bulk_tenths > 999.0f || float_tenths < 1.0f || float_tenths > 999.0f)
    return "";
  char command[24] = {};
  snprintf(command, sizeof(command), "^S015MCHGV%03d,%03d", static_cast<int>(bulk_tenths),
           static_cast<int>(float_tenths));
  return command;
}

bool Pipsolar::set_charge_voltages(float bulk_voltage, float float_voltage) {
  const std::string command = charge_voltage_command(bulk_voltage, float_voltage);
  if (command.empty())
    return false;
  if (!this->queue_command(command))
    return false;
  // Assume the write succeeds; the next PIRI reply corrects this if the inverter refused it
  this->set_known_(NumberSetting::BATTERY_BULK_VOLTAGE, bulk_voltage);
  this->set_known_(NumberSetting::BATTERY_FLOAT_VOLTAGE, float_voltage);
  return true;
}

const char *setting_name(NumberSetting setting) {
  switch (setting) {
    case NumberSetting::BATTERY_UNDER_VOLTAGE:
      return "battery cut-off voltage";
    case NumberSetting::BATTERY_BULK_VOLTAGE:
      return "bulk voltage";
    case NumberSetting::BATTERY_FLOAT_VOLTAGE:
      return "float voltage";
    case NumberSetting::BATTERY_RECHARGE_VOLTAGE:
      return "back to utility voltage";
    case NumberSetting::BATTERY_REDISCHARGE_VOLTAGE:
      return "back to battery voltage";
    case NumberSetting::MAX_CHARGING_CURRENT:
      return "max charging current";
    case NumberSetting::MAX_AC_CHARGING_CURRENT:
      return "max AC charging current";
    default:
      return "setting";
  }
}

namespace {

constexpr int MAX_FIELD_VALUE = 999;  // every setting is sent as three digits

bool is_current(NumberSetting setting) {
  return setting == NumberSetting::MAX_CHARGING_CURRENT || setting == NumberSetting::MAX_AC_CHARGING_CURRENT;
}

// Voltages are sent in tenths, currents in whole amps; -1 when the value can't be sent
int to_units(NumberSetting setting, float value) {
  if (!std::isfinite(value) || std::fabs(value) > 1000.0f)
    return -1;
  return static_cast<int>(std::lround(value * (is_current(setting) ? 1.0f : 10.0f)));
}

// The other half of a setting sent as a pair; the setting itself when it has none
NumberSetting partner_of(NumberSetting setting) {
  switch (setting) {
    case NumberSetting::BATTERY_BULK_VOLTAGE:
      return NumberSetting::BATTERY_FLOAT_VOLTAGE;
    case NumberSetting::BATTERY_FLOAT_VOLTAGE:
      return NumberSetting::BATTERY_BULK_VOLTAGE;
    case NumberSetting::BATTERY_RECHARGE_VOLTAGE:
      return NumberSetting::BATTERY_REDISCHARGE_VOLTAGE;
    case NumberSetting::BATTERY_REDISCHARGE_VOLTAGE:
      return NumberSetting::BATTERY_RECHARGE_VOLTAGE;
    default:
      return setting;
  }
}

// Whether units fit the three-digit field. Only back to battery may be 0 ("full").
bool fits_field(NumberSetting setting, int units) {
  const int min_units = setting == NumberSetting::BATTERY_REDISCHARGE_VOLTAGE ? 0 : 1;
  return units >= min_units && units <= MAX_FIELD_VALUE;
}

// The set command for a value in units; the pair's first and second values for a pair setting
// or "" for a setting this doesn't know
std::string set_command(NumberSetting setting, int units, int partner_units) {
  char command[24] = {};
  switch (setting) {
    case NumberSetting::BATTERY_UNDER_VOLTAGE:
      snprintf(command, sizeof(command), "^S010PSDV%03d", units);
      break;
    case NumberSetting::BATTERY_BULK_VOLTAGE:
      snprintf(command, sizeof(command), "^S015MCHGV%03d,%03d", units, partner_units);
      break;
    case NumberSetting::BATTERY_FLOAT_VOLTAGE:
      snprintf(command, sizeof(command), "^S015MCHGV%03d,%03d", partner_units, units);
      break;
    case NumberSetting::BATTERY_RECHARGE_VOLTAGE:
      snprintf(command, sizeof(command), "^S014BUCD%03d,%03d", units, partner_units);
      break;
    case NumberSetting::BATTERY_REDISCHARGE_VOLTAGE:
      snprintf(command, sizeof(command), "^S014BUCD%03d,%03d", partner_units, units);
      break;
    case NumberSetting::MAX_CHARGING_CURRENT:
      snprintf(command, sizeof(command), "^S013MCHGC0,%03d", units);
      break;
    case NumberSetting::MAX_AC_CHARGING_CURRENT:
      snprintf(command, sizeof(command), "^S014MUCHGC0,%03d", units);
      break;
    default:
      break;
  }
  return command;
}

}  // namespace

void Pipsolar::set_number(NumberSetting setting, PipsolarNumber *number) {
  this->numbers_[static_cast<size_t>(setting)].entity = number;
  this->add_polling_command_("^P007PIRI", POLLING_P007PIRI);
  if (setting == NumberSetting::MAX_CHARGING_CURRENT)
    this->add_polling_command_("^P009MCHGCR", POLLING_P009MCHGCR, PollSchedule::ONCE);
  else if (setting == NumberSetting::MAX_AC_CHARGING_CURRENT)
    this->add_polling_command_("^P010MUCHGCR", POLLING_P010MUCHGCR, PollSchedule::ONCE);
}

esphome::optional<float> Pipsolar::get_number_setting(NumberSetting setting) const {
  return this->numbers_[static_cast<size_t>(setting)].value;
}

esphome::optional<float> Pipsolar::get_number_reading(NumberSetting setting) const {
  return this->numbers_[static_cast<size_t>(setting)].reading;
}

const std::vector<int> &Pipsolar::get_allowed_currents(NumberSetting setting) const {
  static const std::vector<int> NONE;
  if (setting == NumberSetting::MAX_CHARGING_CURRENT)
    return this->allowed_max_charging_currents_;
  if (setting == NumberSetting::MAX_AC_CHARGING_CURRENT)
    return this->allowed_max_ac_charging_currents_;
  return NONE;
}

void Pipsolar::set_known_(NumberSetting setting, float value) {
  set_pending_(this->numbers_[static_cast<size_t>(setting)], value);
}

bool Pipsolar::keeps_pending_(bool pending) const { return pending && this->write_outstanding(); }

bool Pipsolar::write_outstanding() const {
  if (this->state_ == STATE_COMMAND || this->state_ == STATE_COMMAND_COMPLETE)
    return true;
  return std::any_of(std::begin(this->command_queue_), std::end(this->command_queue_),
                     [](const std::string &command) { return !command.empty(); });
}

void Pipsolar::apply_reading_(NumberSetting setting, esphome::optional<int32_t> raw, float divisor) {
  auto &state = this->numbers_[static_cast<size_t>(setting)];
  if (!raw.has_value())
    return;
  // Divide rather than multiply by the inverse: 540 / 10.0f is exactly 54.0f
  const float reading = static_cast<float>(*raw) / divisor;
  state.reading = reading;
  if (state.entity != nullptr)
    state.entity->publish_state(reading);
  // A reading taken before a write went out would undo it
  if (this->keeps_pending_(state.pending))
    return;
  state.value = reading;
  state.pending = false;
}

WriteResult Pipsolar::write_number_setting(NumberSetting setting, float value) {
  const char *name = setting_name(setting);
  const int units = to_units(setting, value);
  if (!fits_field(setting, units)) {
    ESP_LOGW(TAG, "%s: %.1f can't be sent", name, value);
    return WriteResult::REJECTED;
  }
  const auto &allowed = this->get_allowed_currents(setting);
  if (!allowed.empty() && std::find(allowed.begin(), allowed.end(), units) == allowed.end()) {
    ESP_LOGW(TAG, "%s: %d A isn't an allowed current", name, units);
    return WriteResult::REJECTED;
  }
  const auto known = this->get_number_setting(setting);
  if (known.has_value() && to_units(setting, *known) == units) {
    ESP_LOGD(TAG, "%s is already %d", name, units);
    return WriteResult::UNCHANGED;
  }

  const NumberSetting partner = partner_of(setting);
  int partner_units = 0;
  if (partner != setting) {
    const auto partner_known = this->get_number_setting(partner);
    if (!partner_known.has_value()) {
      ESP_LOGW(TAG, "%s: the paired %s isn't known yet", name, setting_name(partner));
      return WriteResult::REJECTED;
    }
    partner_units = to_units(partner, *partner_known);
    // The partner is sent as it was read, so a reading the command can't hold stops the write
    if (!fits_field(partner, partner_units)) {
      ESP_LOGW(TAG, "%s: the paired %s (%.1f) can't be sent", name, setting_name(partner), *partner_known);
      return WriteResult::REJECTED;
    }
  }

  const std::string command = set_command(setting, units, partner_units);
  if (command.empty()) {
    ESP_LOGW(TAG, "%s: no set command", name);
    return WriteResult::REJECTED;
  }
  if (!this->queue_command(command)) {
    ESP_LOGW(TAG, "%s: command queue full", name);
    return WriteResult::REJECTED;
  }
  this->set_known_(setting, is_current(setting) ? static_cast<float>(units) : units * 0.1f);
  if (partner != setting)
    this->set_known_(partner, *this->get_number_setting(partner));
  return WriteResult::QUEUED;
}

namespace {

TimeWindowEnd window_start_of(TimeWindowEnd end) {
  const bool charge = end == TimeWindowEnd::AC_CHARGE_START || end == TimeWindowEnd::AC_CHARGE_END;
  return charge ? TimeWindowEnd::AC_CHARGE_START : TimeWindowEnd::AC_OUTPUT_START;
}

TimeWindowEnd window_end_of(TimeWindowEnd start) {
  return start == TimeWindowEnd::AC_CHARGE_START ? TimeWindowEnd::AC_CHARGE_END : TimeWindowEnd::AC_OUTPUT_END;
}

bool is_charge_window(TimeWindowEnd end) { return window_start_of(end) == TimeWindowEnd::AC_CHARGE_START; }

ENUMPollingCommand window_poll(TimeWindowEnd end) {
  return is_charge_window(end) ? POLLING_P007ACCT : POLLING_P007ACLT;
}

constexpr uint16_t MINUTES_PER_DAY = 24 * 60;
// Off is rewritten when the window would open within this many minutes
constexpr uint16_t AC_OUTPUT_REARM_MINUTES = 60;
constexpr uint8_t AC_OUTPUT_REARM_WRITES_MAX = 3;

}  // namespace

void Pipsolar::set_time_window_end(TimeWindowEnd end, PipsolarTime *entity) {
  this->times_[static_cast<size_t>(end)].entity = entity;
  if (is_charge_window(end))
    this->add_polling_command_("^P007ACCT", POLLING_P007ACCT, PollSchedule::SLOW);
  else
    this->add_polling_command_("^P007ACLT", POLLING_P007ACLT, PollSchedule::SLOW);
}

esphome::optional<uint16_t> Pipsolar::get_time_setting(TimeWindowEnd end) const {
  return this->times_[static_cast<size_t>(end)].value;
}

// "hhmm" with valid digits only
esphome::optional<uint16_t> Pipsolar::parse_time_(const char *text) {
  for (size_t i = 0; i < 4; i++) {
    if (text[i] < '0' || text[i] > '9')
      return {};
  }
  const uint16_t hour = (text[0] - '0') * 10 + (text[1] - '0');
  const uint16_t minute = (text[2] - '0') * 10 + (text[3] - '0');
  if (hour > 23 || minute > 59)
    return {};
  return hour * 100 + minute;
}

void Pipsolar::apply_time_window_(TimeWindowEnd start, uint16_t start_time, uint16_t end_time) {
  const uint16_t readings[] = {start_time, end_time};
  const TimeWindowEnd ends[] = {start, window_end_of(start)};
  for (size_t i = 0; i < 2; i++) {
    auto &state = this->times_[static_cast<size_t>(ends[i])];
    state.reading = readings[i];
#ifdef USE_DATETIME_TIME
    if (state.entity != nullptr)
      state.entity->publish_time(readings[i] / 100, readings[i] % 100);
#endif
    // A reading taken before a write went out would undo it
    if (this->keeps_pending_(state.pending))
      continue;
    state.value = readings[i];
    state.pending = false;
  }
  if (start == TimeWindowEnd::AC_OUTPUT_START)
    this->publish_ac_output_state_();
}

// "^D012hhmm,hhmm"
void Pipsolar::handle_p007acct_(const char *message) {
  this->handle_time_window_(TimeWindowEnd::AC_CHARGE_START, message);
}

void Pipsolar::handle_p007aclt_(const char *message) {
  this->handle_time_window_(TimeWindowEnd::AC_OUTPUT_START, message);
}

void Pipsolar::handle_time_window_(TimeWindowEnd start, const char *message) {
  // "^D012" then "hhmm,hhmm"; a missing or short reply has no times
  const size_t length = strlen(message);
  esphome::optional<uint16_t> start_time, end_time;
  if (length >= 5 + 9 && message[9] == ',') {
    start_time = parse_time_(message + 5);
    end_time = parse_time_(message + 10);
  }
  // A window with one bad time is dropped whole
  if (!start_time.has_value() || !end_time.has_value())
    return;
  this->apply_time_window_(start, *start_time, *end_time);
}

WriteResult Pipsolar::write_time_setting(TimeWindowEnd end, uint8_t hour, uint8_t minute) {
  if (hour > 23 || minute > 59) {
    ESP_LOGW(TAG, "%02d:%02d isn't a time", hour, minute);
    return WriteResult::REJECTED;
  }
  if (minute != 0) {
    ESP_LOGW(TAG, "The inverter only accepts whole hours, so %02d:%02d was not sent", hour, minute);
    return WriteResult::REJECTED;
  }
  const uint16_t time = hour * 100 + minute;
  if (this->get_time_setting(end) == time) {
    ESP_LOGD(TAG, "The window end is already %04d", time);
    return WriteResult::UNCHANGED;
  }
  const TimeWindowEnd start = window_start_of(end);
  const TimeWindowEnd partner = end == start ? window_end_of(start) : start;
  const auto partner_known = this->get_time_setting(partner);
  if (!partner_known.has_value()) {
    ESP_LOGW(TAG, "The other end of the window isn't known yet");
    return WriteResult::REJECTED;
  }
  const uint16_t start_time = end == start ? time : *partner_known;
  const uint16_t end_time = end == start ? *partner_known : time;
  return this->queue_window_(start, start_time, end_time);
}

WriteResult Pipsolar::queue_window_(TimeWindowEnd start, uint16_t start_time, uint16_t end_time) {
  char command[32];
  snprintf(command, sizeof(command), "^S016%s%04u,%04u", is_charge_window(start) ? "ACCT" : "ACLT",
           static_cast<unsigned>(start_time), static_cast<unsigned>(end_time));
  if (!this->queue_command(command)) {
    ESP_LOGW(TAG, "Command queue full");
    return WriteResult::REJECTED;
  }
  set_pending_(this->times_[static_cast<size_t>(start)], start_time);
  set_pending_(this->times_[static_cast<size_t>(window_end_of(start))], end_time);
  this->request_poll_refresh(window_poll(start));
  return WriteResult::QUEUED;
}

void Pipsolar::set_ac_output_switch(switch_::Switch *sw) {
  this->ac_output_switch_ = sw;
  this->add_polling_command_("^P004T", POLLING_P004T, PollSchedule::SLOW);
  this->add_polling_command_("^P007ACLT", POLLING_P007ACLT, PollSchedule::SLOW);
  this->add_polling_command_("^P006FWS", POLLING_P006FWS);
}

bool Pipsolar::window_contains(uint16_t start_hhmm, uint16_t end_hhmm, uint16_t minute_of_day) {
  if (start_hhmm == end_hhmm)
    return true;
  const uint16_t start = start_hhmm / 100 * 60 + start_hhmm % 100;
  const uint16_t end = end_hhmm / 100 * 60 + end_hhmm % 100;
  if (start < end)
    return minute_of_day >= start && minute_of_day < end;
  return minute_of_day >= start || minute_of_day < end;
}

esphome::optional<bool> Pipsolar::get_ac_output_state() const {
  const auto start = this->get_time_reading(TimeWindowEnd::AC_OUTPUT_START);
  const auto end = this->get_time_reading(TimeWindowEnd::AC_OUTPUT_END);
  if (!start.has_value() || !end.has_value() || !this->inverter_minute_of_day_.has_value())
    return {};
  return window_contains(*start, *end, *this->inverter_minute_of_day_);
}

void Pipsolar::publish_ac_output_state_() {
  if (this->ac_output_switch_ == nullptr)
    return;
  const auto state = this->get_ac_output_state();
  if (state.has_value())
    this->ac_output_switch_->publish_state(*state);
}

WriteResult Pipsolar::write_ac_output(bool on) {
  this->ac_output_rearm_writes_ = 0;
  const WriteResult result = this->write_ac_output_(on);
  if (result == WriteResult::QUEUED || result == WriteResult::UNCHANGED)
    this->ac_output_keep_off_ = !on;
  return result;
}

WriteResult Pipsolar::write_ac_output_(bool on) {
  const auto start = this->get_time_setting(TimeWindowEnd::AC_OUTPUT_START);
  const auto end = this->get_time_setting(TimeWindowEnd::AC_OUTPUT_END);
  if (!start.has_value() || !end.has_value()) {
    ESP_LOGW(TAG, "The AC output window isn't known yet");
    return WriteResult::REJECTED;
  }
  const bool disabled = *start == *end;
  const auto minute = this->inverter_minute_of_day_;
  if (on) {
    // While keep off is set the window is the switch's own, so On removes it even when the clock
    // sits inside it: otherwise the output would go off again at the next hour.
    const bool own_window = this->ac_output_keep_off_ && !disabled;
    if (disabled || (!own_window && minute.has_value() && window_contains(*start, *end, *minute))) {
      ESP_LOGD(TAG, "The AC output timer already has the output on");
      return WriteResult::UNCHANGED;
    }
    if (!minute.has_value()) {
      ESP_LOGW(TAG, "The inverter's time isn't known yet");
      return WriteResult::REJECTED;
    }
    if (!this->battery_under_shutdown_.has_value()) {
      ESP_LOGW(TAG, "Not turning the AC output on: the warnings haven't been read yet");
      return WriteResult::REJECTED;
    }
    if (*this->battery_under_shutdown_) {
      ESP_LOGW(TAG, "Not turning the AC output on: the battery is under shutdown voltage");
      return WriteResult::REJECTED;
    }
    return this->queue_window_(TimeWindowEnd::AC_OUTPUT_START, 0, 0);
  }
  if (!minute.has_value()) {
    ESP_LOGW(TAG, "The inverter's time isn't known yet");
    return WriteResult::REJECTED;
  }
  return this->write_ac_output_off_(*start, *end, *minute);
}

bool Pipsolar::ac_output_off_write_needed_(uint16_t start, uint16_t end, uint16_t minute) {
  const bool disabled = start == end;
  const uint16_t start_minute = start / 100 * 60 + start % 100;
  const bool opens_soon = (start_minute + MINUTES_PER_DAY - minute) % MINUTES_PER_DAY <= AC_OUTPUT_REARM_MINUTES;
  return disabled || window_contains(start, end, minute) || opens_soon;
}

WriteResult Pipsolar::write_ac_output_off_(uint16_t start, uint16_t end, uint16_t minute) {
  if (!ac_output_off_write_needed_(start, end, minute)) {
    ESP_LOGD(TAG, "The AC output timer already keeps the output off");
    return WriteResult::UNCHANGED;
  }
  const uint16_t hour = minute / 60;
  return this->queue_window_(TimeWindowEnd::AC_OUTPUT_START, (hour + 23) % 24 * 100, hour * 100);
}

void Pipsolar::rearm_ac_output_() {
  if (!this->ac_output_keep_off_)
    return;
  const auto start = this->get_time_setting(TimeWindowEnd::AC_OUTPUT_START);
  const auto end = this->get_time_setting(TimeWindowEnd::AC_OUTPUT_END);
  if (!start.has_value() || !end.has_value() || !this->inverter_minute_of_day_.has_value())
    return;
  if (this->write_outstanding() || this->times_[static_cast<size_t>(TimeWindowEnd::AC_OUTPUT_START)].pending ||
      this->times_[static_cast<size_t>(TimeWindowEnd::AC_OUTPUT_END)].pending)
    return;
  const uint16_t minute = *this->inverter_minute_of_day_;
  if (!ac_output_off_write_needed_(*start, *end, minute)) {
    this->ac_output_rearm_writes_ = 0;  // the window keeps the output off
    return;
  }
  // Writes that don't take effect (refused, or not stored) would otherwise repeat on every reading
  // and wear out the inverter's settings memory. The window's own start then turns the output back on.
  if (this->ac_output_rearm_writes_ >= AC_OUTPUT_REARM_WRITES_MAX) {
    if (this->ac_output_rearm_writes_ == AC_OUTPUT_REARM_WRITES_MAX) {
      ESP_LOGE(TAG, "The AC output off window doesn't hold after %u writes, no longer re-arming it",
               AC_OUTPUT_REARM_WRITES_MAX);
      this->ac_output_rearm_writes_++;
    }
    return;
  }
  if (this->write_ac_output_off_(*start, *end, minute) == WriteResult::QUEUED) {
    this->ac_output_rearm_writes_++;
    ESP_LOGI(TAG, "Re-armed the AC output off window");
  }
}

std::vector<int> Pipsolar::parse_current_list_(const char *message) {
  std::vector<int> values;
  const size_t length = strlen(message);
  size_t pos = std::min<size_t>(length, 5);  // "^D" and the 3-digit length
  while (pos < length) {
    const size_t begin = pos;
    while (pos < length && message[pos] != ',')
      pos++;
    const auto value = parse_number<int>(std::string(message, begin, pos - begin));  // NOLINT
    if (value.has_value())
      values.push_back(*value);
    if (pos < length)
      pos++;
  }
  return values;
}

void Pipsolar::handle_p009mchgcr_(const char *message) {
  auto values = parse_current_list_(message);
  if (!values.empty())
    this->allowed_max_charging_currents_ = std::move(values);
}

void Pipsolar::handle_p010muchgcr_(const char *message) {
  auto values = parse_current_list_(message);
  if (!values.empty())
    this->allowed_max_ac_charging_currents_ = std::move(values);
}

void Pipsolar::handle_poll_response_(ENUMPollingCommand polling_command, const char *message) {
  switch (polling_command) {
    case POLLING_P007PIRI:
      handle_p007piri_(message);
      break;
    case POLLING_P004T:
      handle_p004t_(message);
      break;
    case POLLING_P005GS:
      handle_p005gs_(message);
      break;
    case POLLING_P006MOD:
      handle_p006mod_(message);
      break;
    case POLLING_P007FLAG:
      handle_p007flag_(message);
      break;
    case POLLING_P006FWS:
      handle_p006fws_(message);
      break;
    case POLLING_P005ET:
      handle_p005et_(message);
      break;
    case POLLING_P007PGS0:
      handle_p007pgs0_(message);
      break;
    case POLLING_P007ACCT:
      handle_p007acct_(message);
      break;
    case POLLING_P007ACLT:
      handle_p007aclt_(message);
      break;
    case POLLING_P009MCHGCR:
      handle_p009mchgcr_(message);
      break;
    case POLLING_P010MUCHGCR:
      handle_p010muchgcr_(message);
      break;
    default:
      break;
  }
}

void Pipsolar::handle_poll_error_(ENUMPollingCommand polling_command) {
  this->handle_poll_response_(polling_command, "");
}

void Pipsolar::handle_p004t_(const char *message) {
  // "^D017" and 14 digits: yyyyMMddHHmmss
  constexpr size_t PREFIX_LENGTH = 5;
  constexpr size_t DIGITS = 14;
  if (strlen(message) != PREFIX_LENGTH + DIGITS)
    return;
  const char *d = message + PREFIX_LENGTH;
  for (size_t i = 0; i < DIGITS; i++) {
    if (d[i] < '0' || d[i] > '9')
      return;
  }
  const uint16_t hour = (d[8] - '0') * 10 + (d[9] - '0');
  const uint16_t minute = (d[10] - '0') * 10 + (d[11] - '0');
  if (hour <= 23 && minute <= 59)
    this->inverter_minute_of_day_ = hour * 60 + minute;
  if (this->inverter_time_ != nullptr) {
    char text[20];
    snprintf(text, sizeof(text), "%.4s-%.2s-%.2s %.2s:%.2s:%.2s", d, d + 4, d + 6, d + 8, d + 10, d + 12);
    this->inverter_time_->publish_state(text);
  }
  this->publish_ac_output_state_();
  this->rearm_ac_output_();
}

std::string Pipsolar::clock_command(const ESPTime &now) {
  if (!now.is_valid())
    return "";
  char command[24];
  snprintf(command, sizeof(command), "^S018DAT%02d%02d%02d%02d%02d%02d", now.year % 100, now.month, now.day_of_month,
           now.hour, now.minute, now.second);
  return command;
}

bool Pipsolar::sync_clock(const ESPTime &now) {
  const std::string command = clock_command(now);
  if (command.empty() || !this->queue_command(command))
    return false;
  this->request_poll_refresh(POLLING_P004T);
  return true;
}

void Pipsolar::handle_p005gs_(const char *message) {
  if (this->last_p005gs_)
    this->last_p005gs_->publish_state(message);

  size_t pos = 0;
  skip_start_(message, &pos);

  read_sensor_(message, &pos, this->grid_voltage_, 0.1f);
  read_sensor_(message, &pos, this->grid_frequency_, 0.1f);
  read_sensor_(message, &pos, this->ac_output_voltage_, 0.1f);
  read_sensor_(message, &pos, this->ac_output_frequency_, 0.1f);
  read_sensor_(message, &pos, this->ac_output_apparent_power_);
  read_sensor_(message, &pos, this->ac_output_active_power_);
  read_sensor_(message, &pos, this->output_load_percent_);
  read_sensor_(message, &pos, this->battery_voltage_, 0.1f);
  read_sensor_(message, &pos, this->battery_voltage_scc_, 0.1f);
  read_sensor_(message, &pos, this->battery_voltage_scc2_, 0.1f);
  read_sensor_(message, &pos, this->battery_discharge_current_, -1.0f);
  read_sensor_(message, &pos, this->battery_charging_current_);
  read_sensor_(message, &pos, this->battery_capacity_percent_);
  read_sensor_(message, &pos, this->inverter_heat_sink_temperature_);
  read_sensor_(message, &pos, this->mppt1_charger_temperature_);
  read_sensor_(message, &pos, this->mppt2_charger_temperature_);
  read_sensor_(message, &pos, this->pv1_input_power_);
  read_sensor_(message, &pos, this->pv2_input_power_);
  read_sensor_(message, &pos, this->pv1_input_voltage_, 0.1f);
  read_sensor_(message, &pos, this->pv2_input_voltage_, 0.1f);

  // Missing fields (failed poll) stay empty instead of defaulting to 0, which would
  // publish "abnormal", "disconnect" and "donothing" for an inverter that is not answering.
  const auto config_state = this->read_flag_(message, &pos);
  const auto mppt1_status = this->read_int_(message, &pos);
  const auto mppt2_status = this->read_int_(message, &pos);
  const auto load_conn = this->read_int_(message, &pos);
  const auto bat_dir = this->read_int_(message, &pos);
  const auto dc_ac_dir = this->read_int_(message, &pos);
  const auto line_dir = this->read_int_(message, &pos);

  this->publish_binary_sensor_(config_state, this->setting_value_configuration_state_);
  this->publish_mppt_status_(this->mppt1_charger_status_, mppt1_status);
  this->publish_mppt_status_(this->mppt2_charger_status_, mppt2_status);

  if (this->load_connection_ && load_conn.has_value())
    this->load_connection_->publish_state(*load_conn == 1 ? "connect" : "disconnect");

  if (this->battery_power_direction_ && bat_dir.has_value()) {
    switch (*bat_dir) {
      case 0:
        this->battery_power_direction_->publish_state("donothing");
        break;
      case 1:
        this->battery_power_direction_->publish_state("charge");
        break;
      case 2:
        this->battery_power_direction_->publish_state("discharge");
        break;
      default:
        break;
    }
  }
  if (this->dc_ac_power_direction_ && dc_ac_dir.has_value()) {
    switch (*dc_ac_dir) {
      case 0:
        this->dc_ac_power_direction_->publish_state("donothing");
        break;
      case 1:
        this->dc_ac_power_direction_->publish_state("AC-DC");
        break;
      case 2:
        this->dc_ac_power_direction_->publish_state("DC-AC");
        break;
      default:
        break;
    }
  }
  this->publish_direction_(this->line_power_direction_, line_dir);
}

void Pipsolar::handle_p007piri_(const char *message) {
  if (this->last_p007piri_)
    this->last_p007piri_->publish_state(message);

  size_t pos = 0;
  skip_start_(message, &pos);

  read_sensor_(message, &pos, this->grid_rating_voltage_, 0.1f);
  read_sensor_(message, &pos, this->grid_rating_current_, 0.1f);
  read_sensor_(message, &pos, this->ac_output_rating_voltage_, 0.1f);
  read_sensor_(message, &pos, this->ac_output_rating_frequency_, 0.1f);
  read_sensor_(message, &pos, this->ac_output_rating_current_, 0.1f);
  read_sensor_(message, &pos, this->ac_output_rating_apparent_power_);
  read_sensor_(message, &pos, this->ac_output_rating_active_power_);
  read_sensor_(message, &pos, this->battery_rating_voltage_, 0.1f);
  const auto recharge_v_raw = this->read_int_(message, &pos);
  this->publish_sensor_(this->battery_recharge_voltage_, recharge_v_raw, 0.1f);
  const auto redischarge_v_raw = this->read_int_(message, &pos);
  this->publish_sensor_(this->battery_redischarge_voltage_, redischarge_v_raw, 0.1f);

  // Missing fields (failed poll) must not become 0: that would report 0 V / 0 A settings
  // and flip the charger priority switches to "utility" (code 0).
  const auto bat_under_v_raw = this->read_int_(message, &pos);
  this->publish_sensor_(this->battery_under_voltage_, bat_under_v_raw, 0.1f);

  // Known even without sensors: ^S015MCHGV sets both voltages, so changing one resends the other
  const auto bulk_v_raw = this->read_int_(message, &pos);
  this->publish_sensor_(this->battery_bulk_voltage_, bulk_v_raw, 0.1f);
  const auto float_v_raw = this->read_int_(message, &pos);
  this->publish_sensor_(this->battery_float_voltage_, float_v_raw, 0.1f);

  const auto bat_type = this->read_int_(message, &pos);
  const auto max_ac_charging_i = this->read_int_(message, &pos);
  const auto max_charging_i = this->read_int_(message, &pos);
  const auto input_v_range = this->read_int_(message, &pos);
  const auto out_src_priority = this->read_int_(message, &pos);
  const auto charger_src_priority = this->read_int_(message, &pos);
  const auto parallel_max = this->read_int_(message, &pos);
  const auto machine_type = this->read_int_(message, &pos);
  const auto topology = this->read_int_(message, &pos);
  const auto output_mode = this->read_int_(message, &pos);
  const auto solar_priority = this->read_int_(message, &pos);
  const auto mppt_string = this->read_int_(message, &pos);

  this->apply_reading_(NumberSetting::BATTERY_RECHARGE_VOLTAGE, recharge_v_raw, 10.0f);
  this->apply_reading_(NumberSetting::BATTERY_REDISCHARGE_VOLTAGE, redischarge_v_raw, 10.0f);
  this->apply_reading_(NumberSetting::BATTERY_UNDER_VOLTAGE, bat_under_v_raw, 10.0f);
  this->apply_reading_(NumberSetting::BATTERY_BULK_VOLTAGE, bulk_v_raw, 10.0f);
  this->apply_reading_(NumberSetting::BATTERY_FLOAT_VOLTAGE, float_v_raw, 10.0f);
  this->apply_reading_(NumberSetting::MAX_AC_CHARGING_CURRENT, max_ac_charging_i, 1.0f);
  this->apply_reading_(NumberSetting::MAX_CHARGING_CURRENT, max_charging_i, 1.0f);

  this->publish_sensor_(this->battery_type_, bat_type);
  this->publish_sensor_(this->current_max_ac_charging_current_, max_ac_charging_i);
  this->publish_sensor_(this->current_max_charging_current_, max_charging_i);
  this->publish_sensor_(this->input_voltage_range_, input_v_range);
  this->publish_sensor_(this->output_source_priority_, out_src_priority);
  this->publish_sensor_(this->charger_source_priority_, charger_src_priority);
  this->publish_sensor_(this->parallel_max_num_, parallel_max);
  this->publish_sensor_(this->machine_type_, machine_type);
  this->publish_sensor_(this->topology_, topology);
  this->publish_sensor_(this->output_mode_, output_mode);
  this->publish_sensor_(this->solar_power_priority_, solar_priority);
  this->publish_sensor_(this->mppt_string_, mppt_string);

  this->publish_select_(this->output_source_priority_select_, out_src_priority);
  this->publish_select_(this->charger_source_priority_select_, charger_src_priority);
  this->publish_select_(this->solar_power_priority_select_, solar_priority);
  this->publish_select_(this->input_voltage_range_select_, input_v_range);
  this->publish_select_(this->machine_type_select_, machine_type);
  this->publish_select_(this->current_max_charging_current_select_, max_charging_i);
  this->publish_select_(this->current_max_ac_charging_current_select_, max_ac_charging_i);
  this->publish_select_(this->battery_under_voltage_select_, bat_under_v_raw);
  this->publish_select_(this->battery_type_select_, bat_type);

  // Each switch is on when the field holds the code it writes when turned on (^S007POP1, ^S007PSP1, ^S009PCP0,n)
  this->publish_switch_(this->output_source_priority_switch_, out_src_priority, 1);
  this->publish_switch_(this->solar_power_priority_switch_, solar_priority, 1);
  // Charger source priority: 0 = solar first, 1 = solar and utility, 2 = only solar
  this->publish_switch_(this->charger_source_priority_solarfirst_switch_, charger_src_priority, 0);
  this->publish_switch_(this->charger_source_priority_utility_switch_, charger_src_priority, 1);
  this->publish_switch_(this->charger_source_priority_solaronly_switch_, charger_src_priority, 2);
}

void Pipsolar::handle_p006mod_(const char *message) {
  if (this->last_p006mod_) {
    this->last_p006mod_->publish_state(message);
  }
  // ^D005<0><mode_char>  — mode digit is at index 6
  if (strlen(message) < 7) {
    // A failed or refused poll passes "". Usually the inverter is off (it powers down at dusk in
    // standby): clear the mode, as the status sensors clear, rather than keep the last one.
    if (this->device_mode_id_)
      this->device_mode_id_->publish_state(NAN);
    if (this->device_mode_)
      this->device_mode_->publish_state("");
    return;
  }
  const char mode_char = message[6];
  const int mode_id = mode_char - '0';
  if (this->device_mode_id_)
    this->device_mode_id_->publish_state(mode_id);
  if (this->device_mode_) {
    // Names for 0-6 follow MPP Solar's SolarPower. An LV2424 reports 7 just before it
    // powers off; publishing "Unknown mode" keeps the sensor from showing the previous mode.
    const char *name;
    switch (mode_char) {
      case '0':
        name = "Power on mode";
        break;
      case '1':
        name = "Standby mode";
        break;
      case '2':
        name = "Bypass mode";
        break;
      case '3':
        name = "Battery mode";
        break;
      case '4':
        name = "Fault mode";
        break;
      case '5':
        name = "Hybrid mode";
        break;
      case '6':
        name = "Charge mode";
        break;
      default:
        name = "Unknown mode";
        break;
    }
    this->device_mode_->publish_state(name);
  }
}

void Pipsolar::handle_p007flag_(const char *message) {
  if (this->last_p007flag_)
    this->last_p007flag_->publish_state(message);
  if (strlen(message) < 6)
    return;

  size_t pos = 0;
  skip_start_(message, &pos);

  struct Flag {
    binary_sensor::BinarySensor *sensor;
    switch_::Switch *sw;
  };
  const Flag flags[] = {
      {this->silence_buzzer_open_buzzer_, this->silence_buzzer_open_buzzer_switch_},
      {this->overload_bypass_function_, this->overload_bypass_function_switch_},
      {this->lcd_escape_to_default_, this->lcd_escape_to_default_switch_},
      {this->overload_restart_function_, this->overload_restart_function_switch_},
      {this->over_temperature_restart_function_, this->over_temperature_restart_function_switch_},
      {this->backlight_on_, this->backlight_on_switch_},
      {this->alarm_on_when_primary_source_interrupt_, this->alarm_on_when_primary_source_interrupt_switch_},
      {this->fault_code_record_, this->fault_code_record_switch_},
      {this->power_saving_, nullptr},
  };
  // Firmwares that report fewer flags leave the remaining sensors unknown and switches unchanged
  for (const auto &flag : flags) {
    const auto enabled = this->read_flag_(message, &pos);
    this->publish_binary_sensor_(enabled, flag.sensor);
    if (flag.sw != nullptr && enabled.has_value())
      flag.sw->publish_state(*enabled);
  }
}

// Fault text from MPP Solar's SolarPower (Events_P181TO2K.xml for 1-4 kW units,
// Events_P18.xml for the others). The tables differ only in 10-12, and 11 has a
// different meaning in each.
struct FaultText {
  int code;
  const char *text;
};
static const FaultText FAULT_TEXTS[] = {
    {0, "No fault"},
    {1, "Fan is locked"},
    {2, "Over temperature"},
    {3, "Battery voltage is too high"},
    {4, "Battery voltage is too low"},
    {5, "Output short circuited or over temperature"},
    {6, "Output voltage is too high"},
    {7, "Over load time out"},
    {8, "Bus voltage is too high"},
    {9, "Bus soft start failed"},
    {10, "PV current over"},
    {11, "PV voltage over or main relay failed"},
    {12, "Charge current over"},
    {51, "Over current inverter"},
    {52, "Bus soft start failed"},
    {53, "Inverter soft start failed"},
    {54, "Self-test failed"},
    {55, "Over DC voltage on output of inverter"},
    {56, "Battery connection is open"},
    {57, "Current sensor failed"},
    {58, "Output voltage is too low"},
    {60, "Inverter negative power"},
    {71, "Parallel version different"},
    {72, "Output circuit failed"},
    {80, "CAN communication failed"},
    {81, "Parallel host line lost"},
    {82, "Parallel synchronized signal lost"},
    {83, "Parallel battery voltage detect different"},
    {84, "Parallel line voltage or frequency detect different"},
    {85, "Parallel line input current unbalanced"},
    {86, "Parallel output setting different"},
};

void Pipsolar::publish_fault_description_(int fault_code) {
  if (this->fault_description_ == nullptr)
    return;
  for (const auto &fault : FAULT_TEXTS) {
    if (fault.code == fault_code) {
      this->fault_description_->publish_state(fault.text);
      return;
    }
  }
  char unknown[32];
  snprintf(unknown, sizeof(unknown), "Unknown fault %d", fault_code);
  this->fault_description_->publish_state(unknown);
}

void Pipsolar::handle_p006fws_(const char *message) {
  if (this->last_p005fws_)
    this->last_p005fws_->publish_state(message);
  // A failed poll blocks On until a good reply arrives
  this->battery_under_shutdown_.reset();
  if (strlen(message) < 6)
    return;

  size_t pos = 0;
  skip_start_(message, &pos);

  // A missing fault code must not read as 0 ("no fault"); the description keeps its last text
  const auto fault_code = this->read_int_(message, &pos);
  this->publish_sensor_(this->fault_code_, fault_code);
  if (fault_code.has_value())
    this->publish_fault_description_(*fault_code);

  binary_sensor::BinarySensor *warn_sensors[] = {
      this->warning_line_fail_,
      this->warning_output_circuit_short_,
      this->warning_over_temperature_,
      this->warning_fan_lock_,
      this->warning_battery_voltage_high_,
      this->warning_battery_low_alarm_,
      this->warning_battery_under_shutdown_,
      this->warning_over_load_,
      this->warning_eeprom_failed_,
      this->warning_power_limit_,
      this->warning_pv1_voltage_high_,
      this->warning_pv2_voltage_high_,
      this->warning_mppt1_overload_,
      this->warning_mppt2_overload_,
      this->warning_scc1_battery_too_low_to_charge_,
      this->warning_scc2_battery_too_low_to_charge_,
  };
  // Firmwares that report fewer warnings leave the remaining sensors unknown instead of off
  constexpr size_t BATTERY_UNDER_SHUTDOWN = 6;
  size_t flag = 0;
  for (auto *sensor : warn_sensors) {
    const auto warning = this->read_flag_(message, &pos);
    if (flag++ == BATTERY_UNDER_SHUTDOWN)
      this->battery_under_shutdown_ = warning;
    this->publish_binary_sensor_(warning, sensor);
  }

  // Field 17: battery connection is open (SolarPower code 1056). Firmware that sends
  // only 17 fields leaves the sensor unknown.
  this->publish_binary_sensor_(this->read_flag_(message, &pos), this->warning_battery_open_);
}

void Pipsolar::handle_p005et_(const char *message) {
  if (this->last_p005et_)
    this->last_p005et_->publish_state(message);
  if (strlen(message) < 6)
    return;
  size_t pos = 0;
  skip_start_(message, &pos);
  read_sensor_(message, &pos, this->total_generated_energy_);
}

void Pipsolar::publish_mppt_status_(text_sensor::TextSensor *sensor, esphome::optional<int32_t> value) {
  if (!sensor || !value.has_value())
    return;
  switch (*value) {
    case 0:
      sensor->publish_state("abnormal");
      break;
    case 1:
      sensor->publish_state("normal but not charging");
      break;
    case 2:
      sensor->publish_state("charging");
      break;
    default:
      break;
  }
}

void Pipsolar::publish_direction_(text_sensor::TextSensor *sensor, esphome::optional<int32_t> value) {
  if (!sensor || !value.has_value())
    return;
  switch (*value) {
    case 0:
      sensor->publish_state("donothing");
      break;
    case 1:
      sensor->publish_state("input");
      break;
    case 2:
      sensor->publish_state("output");
      break;
    default:
      break;
  }
}

void Pipsolar::publish_binary_sensor_(esphome::optional<bool> b, binary_sensor::BinarySensor *sensor) {
  if (sensor) {
    if (b.has_value()) {
      sensor->publish_state(b.value());
    } else {
      sensor->invalidate_state();
    }
  }
}

void Pipsolar::publish_sensor_(sensor::Sensor *sensor, esphome::optional<int32_t> value, float scale) {
  if (sensor)
    sensor->publish_state(value.has_value() ? *value * scale : NAN);
}

void Pipsolar::publish_select_(PipsolarSelect *select, esphome::optional<int32_t> value) {
  if (select == nullptr || !value.has_value())
    return;
  select->map_and_publish(std::to_string(*value));  // NOLINT
}

void Pipsolar::publish_switch_(switch_::Switch *sw, esphome::optional<int32_t> value, int32_t on_value) {
  if (sw && value.has_value())
    sw->publish_state(*value == on_value);
}

void Pipsolar::dump_config() {
  ESP_LOGCONFIG(TAG, "PI18 enabled polling commands:");
  for (auto &enabled_polling_command : this->enabled_polling_commands_) {
    if (enabled_polling_command.length != 0) {
      ESP_LOGCONFIG(TAG, "%s", enabled_polling_command.command);
    }
  }
}

void Pipsolar::update() {
  const uint32_t interval_ms = std::max<uint32_t>(this->get_update_interval(), 1);
  const uint32_t interval_updates = std::max<uint32_t>(1, SLOW_POLL_INTERVAL_MS / interval_ms);
  const bool new_interval = ++this->updates_in_interval_ >= interval_updates;
  // A query only counts as unsupported when it fails while the inverter answers others. When
  // nothing answers, the inverter is off (it powers down at dusk in standby) or the link is down.
  const bool inverter_replied = this->replied_this_interval_;
  if (new_interval) {
    this->updates_in_interval_ = 0;
    this->replied_this_interval_ = false;
  }

  for (auto &poll : this->enabled_polling_commands_) {
    if (poll.length == 0)
      continue;
    if (poll.schedule == PollSchedule::EVERY_UPDATE) {
      poll.needs_update = true;
      poll.refusals = 0;
      continue;
    }
    if (!new_interval || poll.unsupported)
      continue;
    if (poll.schedule == PollSchedule::ONCE && poll.answered)
      continue;
    if (poll.sent && !poll.answered && !poll.ever_answered && inverter_replied &&
        ++poll.failed_intervals >= UNSUPPORTED_AFTER_INTERVALS) {
      ESP_LOGW(TAG, "%s not supported by this inverter, no longer polled", poll.command);
      poll.unsupported = true;
      poll.needs_update = false;
      continue;
    }
    poll.sent = false;
    poll.answered = false;
    poll.refusals = 0;
    poll.needs_update = true;
  }
}

PollingCommand *Pipsolar::find_polling_command_(ENUMPollingCommand id) {
  for (auto &poll : this->enabled_polling_commands_) {
    if (poll.length != 0 && poll.identifier == id)
      return &poll;
  }
  return nullptr;
}

void Pipsolar::request_poll_refresh(ENUMPollingCommand id) {
  PollingCommand *poll = this->find_polling_command_(id);
  if (poll == nullptr || poll->unsupported)
    return;
  poll->needs_update = true;
  poll->refusals = 0;
}

void Pipsolar::add_polling_command_(const char *command, ENUMPollingCommand id, PollSchedule schedule) {
  for (auto &enabled_polling_command : this->enabled_polling_commands_) {
    if (enabled_polling_command.length == strlen(command)) {
      uint8_t len = strlen(command);
      if (memcmp(enabled_polling_command.command, command, len) == 0) {
        return;
      }
    }
    if (enabled_polling_command.length == 0) {
      size_t length = strlen(command);
      enabled_polling_command.command = new uint8_t[length + 1];  // NOLINT(cppcoreguidelines-owning-memory)
      for (size_t i = 0; i < length + 1; i++) {
        enabled_polling_command.command[i] = (uint8_t) command[i];
      }
      enabled_polling_command.errors = 0;
      enabled_polling_command.identifier = id;
      enabled_polling_command.schedule = schedule;
      enabled_polling_command.length = length;
      enabled_polling_command.needs_update = true;
      return;
    }
  }
}

uint16_t Pipsolar::pipsolar_crc_(uint8_t *msg, uint8_t len) {
  uint16_t crc = crc16be(msg, len);
  uint8_t crc_low = crc & 0xff;
  uint8_t crc_high = crc >> 8;
  if (crc_low == 0x28 || crc_low == 0x0d || crc_low == 0x0a)
    crc_low++;
  if (crc_high == 0x28 || crc_high == 0x0d || crc_high == 0x0a)
    crc_high++;
  crc = (crc_high << 8) | crc_low;
  return crc;
}

void Pipsolar::handle_p007pgs0_(const char *message) {
  size_t pos = 0;
  skip_start_(message, &pos);

  for (int i = 0; i < 9; i++)
    skip_field_(message, &pos);

  read_sensor_(message, &pos, this->total_ac_output_apparent_power_);
  read_sensor_(message, &pos, this->total_ac_output_active_power_);
  skip_field_(message, &pos);
  read_sensor_(message, &pos, this->total_output_load_percent_);

  for (int i = 0; i < 3; i++)
    skip_field_(message, &pos);

  read_sensor_(message, &pos, this->total_battery_charging_current_);
}

void Pipsolar::skip_start_(const char *message, size_t *pos) {
  const size_t len = strlen(message);
  *pos = (len >= 5) ? 5 : len;
}

void Pipsolar::skip_field_(const char *message, size_t *pos) {
  while (message[*pos] != '\0' && message[*pos] != ',')
    (*pos)++;
  if (message[*pos] == ',')
    (*pos)++;
}

std::string Pipsolar::read_field_(const char *message, size_t *pos) {
  const size_t begin = *pos;
  while (message[*pos] != '\0' && message[*pos] != ',')
    (*pos)++;
  std::string field(message, begin, *pos - begin);  // NOLINT
  if (message[*pos] == ',')
    (*pos)++;
  return field;
}

esphome::optional<int32_t> Pipsolar::read_int_(const char *message, size_t *pos) {
  return parse_number<int32_t>(this->read_field_(message, pos));
}

esphome::optional<bool> Pipsolar::read_flag_(const char *message, size_t *pos) {
  const auto value = this->read_int_(message, pos);
  if (!value.has_value())
    return {};
  return *value != 0;
}

void Pipsolar::read_sensor_(const char *message, size_t *pos, sensor::Sensor *sensor, float scale) {
  if (sensor != nullptr) {
    auto val = parse_number<float>(read_field_(message, pos));
    sensor->publish_state(val.has_value() ? val.value() * scale : NAN);
  } else {
    skip_field_(message, pos);
  }
}

}  // namespace esphome::pi18
