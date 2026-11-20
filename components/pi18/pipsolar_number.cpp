#include "pipsolar_number.h"
#include "esphome/core/log.h"

#include <cmath>

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::pi18 {

ESPHOME_LOG_TAG(TAG, "pi18.number");

// A value is on the step when it is within this fraction of a step of one
static const float STEP_TOLERANCE = 0.001f;

void PipsolarNumber::republish_reading_() {
  const auto reading = this->parent_->get_number_reading(this->setting_);
  if (reading.has_value())
    this->publish_state(*reading);
}

void PipsolarNumber::control(float value) {
  if (this->parent_ == nullptr)
    return;
  const float step = this->traits.get_step();
  const float tolerance = step * STEP_TOLERANCE;
  if (step > 0.0f) {
    const float from_min = value - this->traits.get_min_value();
    if (std::fabs(from_min - std::round(from_min / step) * step) > tolerance) {
      ESP_LOGW(TAG, "'%s': %.2f isn't a multiple of %.2f", this->get_name().c_str(), value, step);
      this->republish_reading_();
      return;
    }
  }
  if (value > tolerance && value < this->min_nonzero_value_ - tolerance) {
    ESP_LOGW(TAG, "'%s': %.2f is between full (0) and %.2f", this->get_name().c_str(), value, this->min_nonzero_value_);
    this->republish_reading_();
    return;
  }
  if (this->parent_->write_number_setting(this->setting_, value) != WriteResult::QUEUED)
    this->republish_reading_();
}

}  // namespace esphome::pi18
