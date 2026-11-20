#include <gtest/gtest.h>
#include <string>
#include "common.h"
#include "esphome/components/pi18/pipsolar_select.h"
#include "frames_lv5048.h"

namespace esphome::pi18::testing {

namespace {

// Not captured; the format follows the vendor app (two-digit code). The length field counts the data,
// 2 CRC bytes and the CR: 2 data digits + 3 = 005.
constexpr const char *SYNTHETIC_P006LST_5_MIN = "^D00510";
constexpr const char *SYNTHETIC_P006LST_NONE = "^D00500";

// What the YAML built-in maps produce
void add_battery_type_maps(PipsolarSelect &s) {
  s.add_mapping("AGM", "^S007PBT0");
  s.add_mapping("Flooded", "^S007PBT1");
  s.add_mapping("User", "^S007PBT2");
  s.add_status_mapping("0", "AGM");
  s.add_status_mapping("1", "Flooded");
  s.add_status_mapping("2", "User");
}

void add_lcd_sleep_maps(PipsolarSelect &s) {
  s.add_mapping("None", "^S008LST00");
  s.add_mapping("30 s", "^S008LST01");
  s.add_mapping("60 s", "^S008LST02");
  s.add_mapping("5 min", "^S008LST10");
  s.add_mapping("10 min", "^S008LST20");
  s.add_status_mapping("0", "None");
  s.add_status_mapping("1", "30 s");
  s.add_status_mapping("2", "60 s");
  s.add_status_mapping("10", "5 min");
  s.add_status_mapping("20", "10 min");
}

// LV5048_P007PIRI_BANANNAJOE with battery type (field 13) 0, AGM; the length is unchanged
std::string piri_with_agm() {
  std::string frame = LV5048_P007PIRI_BANANNAJOE;
  const size_t at = frame.find(",2,010,060");
  frame.replace(at + 1, 1, "0");
  return frame;
}

// Commands waiting in any queue slot
size_t queued_count(const TestablePipsolar &inv) {
  size_t count = 0;
  for (size_t i = 0; i < 10; i++)
    count += inv.queued_command(i).empty() ? 0 : 1;
  return count;
}

struct Fixture {
  FakeUart uart;
  TestablePipsolar inv;
  PipsolarSelect battery_type;
  PipsolarSelect lcd_sleep;
  PipsolarSelect priority;

  Fixture() {
    this->inv.set_uart_parent(&this->uart);
    this->battery_type.set_parent(&this->inv);
    this->battery_type.set_skip_unchanged(true);
    this->battery_type.traits.set_options({"AGM", "Flooded", "User"});
    add_battery_type_maps(this->battery_type);
    this->lcd_sleep.set_parent(&this->inv);
    this->lcd_sleep.set_skip_unchanged(true);
    this->lcd_sleep.set_refresh_poll(POLLING_P006LST);
    this->lcd_sleep.traits.set_options({"None", "30 s", "60 s", "5 min", "10 min"});
    add_lcd_sleep_maps(this->lcd_sleep);
    this->priority.set_parent(&this->inv);
    this->priority.traits.set_options({"A", "B"});
    this->priority.add_mapping("A", "^S007POP0");
    this->priority.add_mapping("B", "^S007POP1");
    this->priority.add_status_mapping("0", "A");
    this->priority.add_status_mapping("1", "B");
    this->inv.set_battery_type_select(&this->battery_type);
    this->inv.set_lcd_sleep_time_select(&this->lcd_sleep);
    this->inv.set_output_source_priority_select(&this->priority);
  }

  void select(PipsolarSelect &s, const char *option) { s.make_call().set_option(option).perform(); }

  // Answers whatever poll the component sends next
  void answer(const std::string &text) {
    this->inv.loop();
    this->uart.receive(this->inv.reply(text));
    this->inv.run(this->uart, 6);
  }
};

// Fills every slot of the command queue with an unrelated command
void fill_queue(TestablePipsolar &inv) {
  for (int i = 0; i < 10; i++)
    EXPECT_TRUE(inv.queue_command("^S000FILL"));
  EXPECT_FALSE(inv.queue_command("^S000DROPPED"));
}

}  // namespace

TEST(Pi18SelectTest, BatteryTypeReadFromPiri) {
  Fixture f;
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);  // field 13 = 2
  EXPECT_TRUE(f.battery_type.has_state());
  EXPECT_EQ(std::string(f.battery_type.current_option().c_str()), "User");
}

TEST(Pi18SelectTest, LcdSleepReadFromLst) {
  Fixture f;
  f.inv.handle_p006lst_(SYNTHETIC_P006LST_5_MIN);
  EXPECT_EQ(std::string(f.lcd_sleep.current_option().c_str()), "5 min");
  f.inv.handle_p006lst_(SYNTHETIC_P006LST_NONE);
  EXPECT_EQ(std::string(f.lcd_sleep.current_option().c_str()), "None");
}

TEST(Pi18SelectTest, LcdSleepPollIsRegisteredAsSlow) {
  Fixture f;
  bool found = false;
  for (size_t i = 0; i < TestablePipsolar::POLLING_COMMANDS_MAX; i++)
    found = found || f.inv.polling_command(i) == "^P006LST";
  EXPECT_TRUE(found);
}

TEST(Pi18SelectTest, SelectingTheCurrentOptionSendsNothing) {
  Fixture f;
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);
  f.select(f.battery_type, "User");
  EXPECT_EQ(f.inv.queued_command(), "");
}

TEST(Pi18SelectTest, OtherOptionIsQueued) {
  Fixture f;
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);
  f.select(f.battery_type, "AGM");
  EXPECT_EQ(f.inv.queued_command(), "^S007PBT0");
}

TEST(Pi18SelectTest, NewOptionIsQueuedAndRefreshesLst) {
  Fixture f;
  f.inv.handle_p006lst_(SYNTHETIC_P006LST_NONE);
  // Drain the first round of polls so the LST poll is not pending
  f.inv.update();
  for (int i = 0; i < 12; i++)
    f.answer("^0");
  const size_t before = f.uart.count_sent("^P006LST");
  f.select(f.lcd_sleep, "5 min");
  EXPECT_EQ(f.inv.queued_command(), "^S008LST10");
  for (int i = 0; i < 12; i++)
    f.answer("^1");
  EXPECT_EQ(f.uart.count_sent("^S008LST10"), 1u);
  EXPECT_GT(f.uart.count_sent("^P006LST"), before);
}

TEST(Pi18SelectTest, ExistingSelectsStillAlwaysWrite) {
  Fixture f;
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);  // output source priority is 0 -> "A"
  EXPECT_EQ(std::string(f.priority.current_option().c_str()), "A");
  f.select(f.priority, "A");
  EXPECT_EQ(f.inv.queued_command(), "^S007POP0");
}

TEST(Pi18SelectTest, RevertBeforeReadingIsSent) {
  Fixture f;
  f.inv.handle_p007piri_(piri_with_agm().c_str());
  f.select(f.battery_type, "User");
  f.select(f.battery_type, "AGM");  // back to what the last reading says, but a write is pending
  EXPECT_EQ(f.inv.queued_command(0), "^S007PBT2");
  EXPECT_EQ(f.inv.queued_command(1), "^S007PBT0");
}

TEST(Pi18SelectTest, InFlightReadingKeepsThePendingOption) {
  // Only PIRI is registered, so the poll in flight is PIRI and its reply reaches the battery type select
  FakeUart uart;
  TestablePipsolar inv;
  PipsolarSelect battery_type;
  inv.set_uart_parent(&uart);
  battery_type.set_parent(&inv);
  battery_type.set_skip_unchanged(true);
  battery_type.traits.set_options({"AGM", "Flooded", "User"});
  add_battery_type_maps(battery_type);
  inv.set_battery_type_select(&battery_type);
  inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);  // User
  inv.loop();                                        // PIRI sent, reply not yet read
  ASSERT_EQ(uart.count_sent("^P007PIRI"), 1u);
  battery_type.make_call().set_option("AGM").perform();
  uart.receive(inv.reply(LV5048_P007PIRI_BANANNAJOE));  // the old reading arrives
  inv.run(uart, 6);                                     // ... and the write is sent
  uart.receive(inv.reply("^1"));
  inv.run(uart, 6);
  EXPECT_EQ(uart.count_sent("^S007PBT0"), 1u);
  // The select shows the inverter's reading, but the pending write is still the known value
  EXPECT_EQ(std::string(battery_type.current_option().c_str()), "User");
  battery_type.make_call().set_option("AGM").perform();
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18SelectTest, RefusedWriteCanBeRetriedWhileNothingHasBeenRead) {
  FakeUart uart;
  TestablePipsolar inv;
  PipsolarSelect lcd_sleep;
  inv.set_uart_parent(&uart);
  lcd_sleep.set_parent(&inv);
  lcd_sleep.set_skip_unchanged(true);
  lcd_sleep.set_refresh_poll(POLLING_P006LST);
  lcd_sleep.traits.set_options({"None", "30 s", "60 s", "5 min", "10 min"});
  add_lcd_sleep_maps(lcd_sleep);
  inv.set_lcd_sleep_time_select(&lcd_sleep);
  lcd_sleep.make_call().set_option("5 min").perform();
  for (int i = 0; i < 12; i++) {  // every reply, the write's included, is refused
    inv.loop();
    uart.receive(inv.reply("^0"));
    inv.run(uart, 6);
  }
  EXPECT_EQ(uart.count_sent("^S008LST10"), 1u);
  lcd_sleep.make_call().set_option("5 min").perform();
  EXPECT_EQ(queued_count(inv), 1u);
}

TEST(Pi18SelectTest, RefusedSelectWriteReverts) {
  Fixture f;
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);  // User
  f.select(f.battery_type, "AGM");
  f.answer("^0");  // the inverter refuses the write
  EXPECT_EQ(f.uart.count_sent("^S007PBT0"), 1u);
  f.select(f.battery_type, "AGM");  // no reading yet: the pending option is still the known one
  EXPECT_EQ(queued_count(f.inv), 0u);
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);  // the next reading: still User
  f.select(f.battery_type, "AGM");
  EXPECT_EQ(queued_count(f.inv), 1u);
}

TEST(Pi18SelectTest, FullQueueDoesNotMarkTheOptionPending) {
  Fixture f;
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);  // User
  fill_queue(f.inv);
  f.select(f.battery_type, "AGM");
  EXPECT_EQ(std::string(f.battery_type.current_option().c_str()), "User");
  for (int i = 0; i < 40; i++)
    f.answer("^1");  // drains the queue
  EXPECT_EQ(f.uart.count_sent("^S007PBT0"), 0u);
  f.select(f.battery_type, "AGM");  // not "already selected": the first write never queued
  for (int i = 0; i < 12; i++)
    f.answer("^1");
  EXPECT_EQ(f.uart.count_sent("^S007PBT0"), 1u);
}

}  // namespace esphome::pi18::testing
