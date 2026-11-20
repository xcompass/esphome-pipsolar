#pragma once

#include <map>
#include <utility>

#include "esphome/components/pi18/pipsolar.h"
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

namespace esphome::pi18 {
class Pipsolar;

class PipsolarSelect : public Component, public select::Select {
 public:
  void set_parent(Pipsolar *const parent) { this->parent_ = parent; }
  void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }
  void add_mapping(const std::string &key, std::string value) { this->mapping_[key] = std::move(value); }
  void add_status_mapping(const std::string &key, std::string value) { this->status_mapping_[key] = std::move(value); }
  // Skip a write when the option is already the one the inverter reported
  void set_skip_unchanged(bool skip) { this->skip_unchanged_ = skip; }
  // Poll to send again after a write, so the select shows the new reading soon
  void set_refresh_poll(ENUMPollingCommand poll) {
    this->refresh_poll_ = poll;
    this->has_refresh_poll_ = true;
  }
  void dump_config() override;
  void control(const std::string &value) override;
  void map_and_publish(const std::string &value);

 protected:
  std::map<std::string, std::string> mapping_;
  std::map<std::string, std::string> status_mapping_;
  Pipsolar *parent_{nullptr};
  bool optimistic_{false};
  bool skip_unchanged_{false};
  bool has_refresh_poll_{false};
  ENUMPollingCommand refresh_poll_{};
  // The option the inverter last reported; empty before the first reading
  std::string last_reading_;
  // The option last written and not yet confirmed by a later reading; empty if none
  std::string pending_;
};

}  // namespace esphome::pi18
