#pragma once

#include "esphome/components/number/number.h"
#include "esphome/components/pi18/pipsolar.h"

namespace esphome::pi18 {

// A battery or charging setting. It shows what the inverter last reported, never a value
// that is still waiting to be written.
class PipsolarNumber : public number::Number {
 public:
  void set_parent(Pipsolar *parent) { this->parent_ = parent; }
  void set_setting(NumberSetting setting) { this->setting_ = setting; }
  // Values above 0 and below this are rejected; 0 means "full" (back to battery only)
  void set_min_nonzero_value(float value) { this->min_nonzero_value_ = value; }

 protected:
  void control(float value) override;
  // Shows the last reading again, so Home Assistant drops a value that wasn't written
  void republish_reading_();

  Pipsolar *parent_{nullptr};
  NumberSetting setting_{NumberSetting::BATTERY_UNDER_VOLTAGE};
  float min_nonzero_value_{0.0f};
};

}  // namespace esphome::pi18
