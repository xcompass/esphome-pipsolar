#include "pipsolar_select.h"
#include "esphome/core/log.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::pi18 {

ESPHOME_LOG_TAG(TAG, "pi18.select");

void PipsolarSelect::dump_config() { LOG_SELECT(TAG, "PI18 Controller Select", this); }

void PipsolarSelect::control(const std::string &value) {
  ESP_LOGD(TAG, "got option: %s", value.c_str());
  const auto it = this->mapping_.find(value);
  if (it == this->mapping_.end()) {
    ESP_LOGD(TAG, "could not find option %s in mapping", value.c_str());
    return;
  }
  if (this->skip_unchanged_ && value == (this->pending_.empty() ? this->last_reading_ : this->pending_)) {
    ESP_LOGD(TAG, "option %s is already selected, nothing sent", value.c_str());
    return;
  }
  ESP_LOGD(TAG, "found mapped option %s for option %s", it->second.c_str(), value.c_str());
  this->parent_->queue_command(it->second);
  if (this->has_refresh_poll_)
    this->parent_->request_poll_refresh(this->refresh_poll_);
  if (this->skip_unchanged_)
    this->pending_ = value;
  if (this->optimistic_)
    this->publish_state(value);
}

void PipsolarSelect::map_and_publish(const std::string &value) {
  ESP_LOGD(TAG, "got value: %s", value.c_str());
  const auto it = this->status_mapping_.find(value);
  if (it == this->status_mapping_.end()) {
    ESP_LOGD(TAG, "could not find option %s in mapping", value.c_str());
    return;
  }
  ESP_LOGD(TAG, "found mapped option %s for option %s", it->second.c_str(), value.c_str());
  this->last_reading_ = it->second;
  // A reading taken before a write went out must not undo it; a refused write reverts here
  if (this->parent_ == nullptr || !this->parent_->write_outstanding())
    this->pending_.clear();
  this->publish_state(it->second);
}

}  // namespace esphome::pi18
