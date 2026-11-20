#include <gtest/gtest.h>
#include <string>
#include "common.h"
#include "frames_lv2424.h"
#include "frames_lv5048.h"

namespace esphome::pi18::testing {

namespace {

constexpr const char *READ_CLOCK = "^P004T";

// A valid local time, with the day fields ESPTime::is_valid() checks
ESPTime make_time(int year, int month, int day, int hour, int minute, int second) {
  ESPTime now{};
  now.year = year;
  now.month = month;
  now.day_of_month = day;
  now.hour = hour;
  now.minute = minute;
  now.second = second;
  now.day_of_week = 6;  // Friday: the check only wants 1 to 7
  now.day_of_year = 324;
  return now;
}

size_t queued_count(const TestablePipsolar &inv) {
  size_t count = 0;
  for (size_t i = 0; i < 10; i++)
    count += inv.queued_command(i).empty() ? 0 : 1;
  return count;
}
}  // namespace

TEST(Pi18ClockTest, InverterTimeIsFormatted) {
  TestablePipsolar inv;
  text_sensor::TextSensor clock;
  inv.set_inverter_time(&clock);
  inv.handle_p004t_(LV5048_P004T);
  EXPECT_STREQ(clock.state.c_str(), "2023-03-20 11:46:13");
}

TEST(Pi18ClockTest, Lv2424InverterTimeIsFormatted) {
  TestablePipsolar inv;
  text_sensor::TextSensor clock;
  inv.set_inverter_time(&clock);
  inv.handle_p004t_(LV2424_P004T);
  EXPECT_STREQ(clock.state.c_str(), "2026-11-20 13:06:18");
}

TEST(Pi18ClockTest, PollErrorKeepsTheLastTime) {
  TestablePipsolar inv;
  text_sensor::TextSensor clock;
  inv.set_inverter_time(&clock);
  inv.handle_p004t_(LV5048_P004T);
  inv.handle_poll_error_(POLLING_P004T);
  EXPECT_STREQ(clock.state.c_str(), "2023-03-20 11:46:13");
}

TEST(Pi18ClockTest, ReplyThatIsNotFourteenDigitsPublishesNothing) {
  TestablePipsolar inv;
  text_sensor::TextSensor clock;
  inv.set_inverter_time(&clock);
  inv.handle_p004t_("^D0152023032011461");
  inv.handle_p004t_("^D0172023032011461x");
  EXPECT_FALSE(clock.has_state());
}

TEST(Pi18ClockTest, ClockCommandUsesTwoDigitYear) {
  EXPECT_EQ(Pipsolar::clock_command(make_time(2026, 11, 20, 3, 0, 5)), "^S018DAT261120030005");
}

TEST(Pi18ClockTest, InvalidTimeSendsNothing) {
  TestablePipsolar inv;
  EXPECT_EQ(Pipsolar::clock_command(ESPTime{}), "");
  EXPECT_FALSE(inv.sync_clock(ESPTime{}));
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18ClockTest, SyncQueuesTheCommand) {
  TestablePipsolar inv;
  EXPECT_TRUE(inv.sync_clock(make_time(2026, 11, 20, 3, 0, 5)));
  EXPECT_EQ(inv.queued_command(0), "^S018DAT261120030005");
}

TEST(Pi18ClockTest, FullQueueDropsTheSyncAndRefreshesNothing) {
  FakeUart uart;
  TestablePipsolar inv;
  text_sensor::TextSensor clock;
  inv.set_uart_parent(&uart);
  inv.set_inverter_time(&clock);
  // Let the first clock poll go out and be answered, so no refresh is pending
  for (int i = 0; i < 6; i++) {
    inv.loop();
    uart.receive(inv.reply(LV5048_P004T));
    inv.run(uart, 6);
  }
  ASSERT_EQ(uart.count_sent("^P004T"), 1u);
  for (int i = 0; i < 10; i++)
    EXPECT_TRUE(inv.queue_command("^S000FILL"));
  EXPECT_FALSE(inv.sync_clock(make_time(2026, 11, 20, 3, 0, 5)));
  for (int i = 0; i < 40; i++) {  // drains the queue
    inv.loop();
    uart.receive(inv.reply("^1"));
    inv.run(uart, 6);
  }
  EXPECT_EQ(uart.count_sent("^S000FILL"), 10u);
  EXPECT_EQ(uart.count_sent("^S018DAT261120030005"), 0u);
  EXPECT_EQ(uart.count_sent("^P004T"), 1u);  // the dropped sync asked for no refresh
}

TEST(Pi18ClockTest, InverterTimeRegistersTheClockPoll) {
  TestablePipsolar inv;
  text_sensor::TextSensor clock;
  inv.set_inverter_time(&clock);
  EXPECT_EQ(inv.polling_command(0), READ_CLOCK);
  EXPECT_EQ(inv.polling_command(1), "");
}

TEST(Pi18ClockTest, SyncRefreshesTheClockPoll) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  text_sensor::TextSensor clock;
  inv.set_inverter_time(&clock);
  // Drain the first poll round
  inv.update();
  for (int i = 0; i < 12; i++) {
    inv.loop();
    uart.receive(inv.reply(LV5048_P004T));
    inv.run(uart, 6);
  }
  const size_t before = uart.count_sent(READ_CLOCK);
  EXPECT_TRUE(inv.sync_clock(make_time(2026, 11, 20, 3, 0, 5)));
  for (int i = 0; i < 12; i++) {
    inv.loop();
    uart.receive(inv.reply("^1"));
    inv.run(uart, 6);
  }
  EXPECT_EQ(uart.count_sent("^S018DAT261120030005"), 1u);
  EXPECT_GT(uart.count_sent(READ_CLOCK), before);
}

}  // namespace esphome::pi18::testing
