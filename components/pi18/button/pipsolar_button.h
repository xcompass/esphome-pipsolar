#pragma once

#include "../pipsolar.h"
#include "esphome/components/button/button.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome::pi18 {

// Sets the inverter clock from an ESPHome time source (^S018DAT)
class PipsolarSyncClockButton : public button::Button {
 public:
  void set_parent(Pipsolar *parent) { this->parent_ = parent; }
  void set_time(time::RealTimeClock *time) { this->time_ = time; }

 protected:
  void press_action() override;
  Pipsolar *parent_{nullptr};
  time::RealTimeClock *time_{nullptr};
};

}  // namespace esphome::pi18
