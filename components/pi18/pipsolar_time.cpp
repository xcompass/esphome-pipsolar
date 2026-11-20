#include "pipsolar_time.h"

#ifdef USE_DATETIME_TIME

#include "esphome/core/log.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::pi18 {

ESPHOME_LOG_TAG(TAG, "pi18.time");

void PipsolarTime::publish_time(uint8_t hour, uint8_t minute) {
  this->hour_ = hour;
  this->minute_ = minute;
  this->second_ = 0;
  this->publish_state();
}

void PipsolarTime::control(const datetime::TimeCall &call) {
  if (this->parent_ == nullptr)
    return;
  // Seconds are ignored; a field the call doesn't set stays as displayed
  const auto hour = call.get_hour();
  const auto minute = call.get_minute();
  if ((!hour.has_value() || !minute.has_value()) && !this->has_state()) {
    ESP_LOGW(TAG, "The current time isn't known yet");
    return;
  }
  const uint8_t new_hour = hour.has_value() ? *hour : this->hour_;
  const uint8_t new_minute = minute.has_value() ? *minute : this->minute_;
  if (this->parent_->write_time_setting(this->end_, new_hour, new_minute) == WriteResult::QUEUED)
    return;
  // Shows the last reading again, so Home Assistant drops a time that wasn't written
  const auto reading = this->parent_->get_time_reading(this->end_);
  if (reading.has_value())
    this->publish_time(*reading / 100, *reading % 100);
}

}  // namespace esphome::pi18

#endif  // USE_DATETIME_TIME
