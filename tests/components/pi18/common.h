#pragma once
#include <deque>
#include <string>
#include <vector>
#include "esphome/components/pi18/pipsolar.h"
#include "esphome/components/uart/uart_component.h"

namespace esphome::pi18::testing {

class TestablePipsolar : public Pipsolar {
 public:
  using Pipsolar::handle_p005et_;
  using Pipsolar::handle_p006fws_;
  using Pipsolar::handle_p005gs_;
  using Pipsolar::handle_p006mod_;
  using Pipsolar::handle_p007flag_;
  using Pipsolar::handle_p007pgs0_;
  using Pipsolar::handle_p007piri_;
  using Pipsolar::handle_poll_error_;
  using Pipsolar::parse_command_reply_;
  using Pipsolar::POLLING_COMMANDS_MAX;

  // Registered poll command n as text, or "" when that slot is unused
  std::string polling_command(size_t n) const {
    const auto &entry = this->enabled_polling_commands_[n];
    return entry.length == 0 ? std::string() : std::string((const char *) entry.command, entry.length);
  }

  // The command queue holds at most one entry per test; returns "" when empty
  const std::string &queued_command() const { return this->command_queue_[0]; }
};

// switch_::Switch is abstract; tests only need publish_state()
class TestSwitch : public switch_::Switch {
 protected:
  void write_state(bool state) override {}
};

// Stands in for the UART bus: receive() queues bytes for the component to read,
// and everything the component writes is kept for count_sent()
class FakeUart : public uart::UARTComponent {
 public:
  void receive(const std::vector<uint8_t> &bytes) { this->rx_.insert(this->rx_.end(), bytes.begin(), bytes.end()); }

  size_t count_sent(const std::string &text) const {
    size_t count = 0;
    for (size_t pos = this->sent_.find(text); pos != std::string::npos; pos = this->sent_.find(text, pos + 1))
      count++;
    return count;
  }

  void write_array(const uint8_t *data, size_t len) override { this->sent_.append((const char *) data, len); }
  bool peek_byte(uint8_t *data) override {
    if (this->rx_.empty())
      return false;
    *data = this->rx_.front();
    return true;
  }
  bool read_array(uint8_t *data, size_t len) override {
    if (len > this->rx_.size())
      return false;
    for (size_t i = 0; i < len; i++) {
      data[i] = this->rx_.front();
      this->rx_.pop_front();
    }
    return true;
  }
  size_t available() override { return this->rx_.size(); }
  uart::UARTFlushResult flush() override { return uart::UARTFlushResult::UART_FLUSH_RESULT_SUCCESS; }
  void check_logger_conflict() override {}

 protected:
  std::deque<uint8_t> rx_;
  std::string sent_;
};

}  // namespace esphome::pi18::testing
