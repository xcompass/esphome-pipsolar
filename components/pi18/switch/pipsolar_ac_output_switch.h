#pragma once

#include "../pipsolar.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::pi18 {

// Turns the inverter's AC output on and off through its AC output timer. The hub publishes the
// state from the timer readings, so this entity never publishes the requested state itself.
class PipsolarAcOutputSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(Pipsolar *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;

 protected:
  void write_state(bool state) override;
  // "Keep off" has its own preference: the switch shows the timer's actual state, so ESPHome's
  // switch restore would save that instead of the request.
  ESPPreferenceObject keep_off_pref_;
  Pipsolar *parent_{nullptr};
};

}  // namespace esphome::pi18
