#include "pipsolar_button.h"
#include "esphome/core/log.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::pi18 {

ESPHOME_LOG_TAG(TAG, "pi18.button");

void PipsolarSyncClockButton::press_action() {
  if (this->parent_ == nullptr || this->time_ == nullptr)
    return;
  if (!this->parent_->sync_clock(this->time_->now())) {
    ESP_LOGW(TAG, "The clock was not set: the time source isn't valid yet, or the command queue is full");
  }
}

}  // namespace esphome::pi18
