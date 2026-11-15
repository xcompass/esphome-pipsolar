#pragma once

#include "../pipsolar.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/component.h"

#include <vector>

namespace esphome::pi18 {

class Pipsolar;

// Bulk and float voltage share one command (^S015MCHGV) that always carries both
enum class ChargeVoltage : uint8_t { NONE, BULK_VOLTAGE, FLOAT_VOLTAGE };

class PipsolarOutput : public output::FloatOutput {
 public:
  PipsolarOutput() {}
  void set_parent(Pipsolar *parent) { this->parent_ = parent; }
  // printf template taking the level as a float, e.g. "^S013MCHGC0,%03.0f"
  void set_set_command(const char *command) { this->set_command_ = command; }
  void set_set_command(const std::string &command) = delete;
  void set_charge_voltage(ChargeVoltage charge_voltage) { this->charge_voltage_ = charge_voltage; }
  void set_possible_values(std::vector<float> possible_values) { this->possible_values_ = std::move(possible_values); }
  void set_value(float value) { this->write_state(value); }

 protected:
  void write_state(float state) override;
  void write_charge_voltage_(float state);
  const char *set_command_{nullptr};
  ChargeVoltage charge_voltage_{ChargeVoltage::NONE};
  Pipsolar *parent_;
  std::vector<float> possible_values_;
};

template<typename... Ts> class SetOutputAction : public Action<Ts...> {
 public:
  SetOutputAction(PipsolarOutput *output) : output_(output) {}

  TEMPLATABLE_VALUE(float, level)

  void play(const Ts &...x) override { this->output_->set_value(this->level_.value(x...)); }

 protected:
  PipsolarOutput *output_;
};

}  // namespace esphome::pi18
