#pragma once

#include "esphome/core/defines.h"

#ifdef USE_DATETIME_TIME

#include "esphome/components/datetime/time_entity.h"
#include "esphome/components/pi18/pipsolar.h"

namespace esphome::pi18 {

// One end of the AC charge or AC output window. It shows what the inverter last reported, never
// a time that is still waiting to be written.
class PipsolarTime : public datetime::TimeEntity {
 public:
  void set_parent(Pipsolar *parent) { this->parent_ = parent; }
  void set_window_end(TimeWindowEnd end) { this->end_ = end; }
  void publish_time(uint8_t hour, uint8_t minute);

 protected:
  void control(const datetime::TimeCall &call) override;

  Pipsolar *parent_{nullptr};
  TimeWindowEnd end_{TimeWindowEnd::AC_CHARGE_START};
};

}  // namespace esphome::pi18

#endif  // USE_DATETIME_TIME
