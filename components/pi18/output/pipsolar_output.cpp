#include "pipsolar_output.h"
#include "esphome/core/log.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::pi18 {

ESPHOME_LOG_TAG(TAG, "pi18.output");

void PipsolarOutput::write_state(float state) {
  if (std::find(this->possible_values_.begin(), this->possible_values_.end(), state) == this->possible_values_.end()) {
    ESP_LOGD(TAG, "Will not write %.1f: not in the list of allowed values", state);
    return;
  }
  if (this->charge_voltage_ != ChargeVoltage::NONE) {
    this->write_charge_voltage_(state);
    return;
  }
  char tmp[24];
  snprintf(tmp, sizeof(tmp), this->set_command_, state);
  ESP_LOGD(TAG, "Will write: %s out of value %.1f", tmp, state);
  this->parent_->queue_command(std::string(tmp));
}

void PipsolarOutput::write_charge_voltage_(float state) {
  const bool is_bulk = this->charge_voltage_ == ChargeVoltage::BULK_VOLTAGE;
  const char *name = is_bulk ? "bulk" : "float";
  esphome::optional<float> bulk = this->parent_->get_battery_bulk_voltage_setting();
  esphome::optional<float> flt = this->parent_->get_battery_float_voltage_setting();
  if (is_bulk) {
    bulk = state;
  } else {
    flt = state;
  }
  if (!bulk.has_value() || !flt.has_value()) {
    ESP_LOGW(TAG, "Not setting the %s voltage yet: the other voltage is unknown until the first PIRI reply", name);
    return;
  }
  if (!this->parent_->set_charge_voltages(*bulk, *flt)) {
    ESP_LOGW(TAG, "Not setting the %s voltage: %.1f / %.1f V is out of range or the command queue is full", name, *bulk,
             *flt);
    return;
  }
  ESP_LOGD(TAG, "Will write %s voltage %.1f V (bulk %.1f V, float %.1f V)", name, state, *bulk, *flt);
}

}  // namespace esphome::pi18
