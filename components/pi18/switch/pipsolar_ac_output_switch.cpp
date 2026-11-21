#include "pipsolar_ac_output_switch.h"
#include "esphome/core/log.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::pi18 {

ESPHOME_LOG_TAG(TAG, "pi18.ac_output_switch");

// Distinguishes this preference from ESPHome's own switch restore for the same entity
static constexpr uint32_t KEEP_OFF_PREFERENCE_SALT = 0x41434f55;

void PipsolarAcOutputSwitch::setup() {
  this->keep_off_pref_ = this->make_entity_preference<bool>(KEEP_OFF_PREFERENCE_SALT);
  bool keep_off = false;
  // Only the owner's last request is restored. Nothing is written to the inverter here.
  if (this->keep_off_pref_.load(&keep_off) && keep_off) {
    this->parent_->set_ac_output_keep_off(true);
  }
}

void PipsolarAcOutputSwitch::dump_config() { LOG_SWITCH("", "PI18 AC Output Switch", this); }

void PipsolarAcOutputSwitch::write_state(bool state) {
  if (this->parent_->write_ac_output(state) != WriteResult::REJECTED) {
    const bool keep_off = !state;
    this->keep_off_pref_.save(&keep_off);
    return;
  }
  // Drop the request in Home Assistant by showing the reading again
  const auto reading = this->parent_->get_ac_output_state();
  if (reading.has_value()) {
    this->publish_state(*reading);
  }
}

}  // namespace esphome::pi18
