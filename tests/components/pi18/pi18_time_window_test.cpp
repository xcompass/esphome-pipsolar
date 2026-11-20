#include <gtest/gtest.h>
#include <string>
#include "common.h"
#include "esphome/components/pi18/pipsolar_time.h"
#include "frames_lv2424.h"
#include "frames_lv5048.h"

namespace esphome::pi18::testing {

namespace {

// Not captured from an inverter: synthetic ^P007ACCT / ^P007ACLT replies (hhmm,hhmm)
constexpr const char *SYNTHETIC_WINDOW_2300_0600 = "^D0122300,0600";
constexpr const char *SYNTHETIC_WINDOW_HOUR_24 = "^D0122460,0600";
constexpr const char *SYNTHETIC_WINDOW_MINUTE_60 = "^D0122360,0600";
constexpr const char *SYNTHETIC_WINDOW_NOT_DIGITS = "^D012ab00,0600";

size_t queued_count(const TestablePipsolar &inv) {
  size_t count = 0;
  for (size_t i = 0; i < 10; i++)
    count += inv.queued_command(i).empty() ? 0 : 1;
  return count;
}

// Fills every slot of the command queue with an unrelated command
void fill_queue(TestablePipsolar &inv) {
  for (int i = 0; i < 10; i++)
    EXPECT_TRUE(inv.queue_command("^S000FILL"));
  EXPECT_FALSE(inv.queue_command("^S000DROPPED"));
}

// Sends everything queued, each answered with ^1
void drain_queue(TestablePipsolar &inv, FakeUart &uart) {
  for (int i = 0; i < 40; i++) {
    inv.loop();
    uart.receive(inv.reply("^1"));
    inv.run(uart, 6);
  }
}
}  // namespace

TEST(Pi18TimeWindowTest, WindowsReadFromAcctAndAclt) {
  TestablePipsolar inv;
  EXPECT_FALSE(inv.get_time_setting(TimeWindowEnd::AC_CHARGE_START).has_value());
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  inv.handle_p007aclt_(LV2424_P007ACLT);  // captured: 00:00 to 00:00
  EXPECT_EQ(*inv.get_time_setting(TimeWindowEnd::AC_CHARGE_START), 2300);
  EXPECT_EQ(*inv.get_time_setting(TimeWindowEnd::AC_CHARGE_END), 600);
  EXPECT_EQ(*inv.get_time_setting(TimeWindowEnd::AC_OUTPUT_START), 0);
  EXPECT_EQ(*inv.get_time_setting(TimeWindowEnd::AC_OUTPUT_END), 0);
}

TEST(Pi18TimeWindowTest, FactoryZeroWindowsPublishMidnightForBothEnds) {
  TestablePipsolar inv;
  PipsolarTime start, end;
  start.set_parent(&inv);
  start.set_window_end(TimeWindowEnd::AC_CHARGE_START);
  end.set_parent(&inv);
  end.set_window_end(TimeWindowEnd::AC_CHARGE_END);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_START, &start);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_END, &end);
  inv.handle_p007acct_(LV2424_P007ACCT);
  EXPECT_TRUE(start.has_state());
  EXPECT_TRUE(end.has_state());
  EXPECT_EQ(start.hour, 0);
  EXPECT_EQ(start.minute, 0);
  EXPECT_EQ(end.hour, 0);
  EXPECT_EQ(end.minute, 0);
}

TEST(Pi18TimeWindowTest, EntitiesShowTheReading) {
  TestablePipsolar inv;
  PipsolarTime start, end;
  start.set_parent(&inv);
  start.set_window_end(TimeWindowEnd::AC_CHARGE_START);
  end.set_parent(&inv);
  end.set_window_end(TimeWindowEnd::AC_CHARGE_END);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_START, &start);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_END, &end);
  EXPECT_EQ(inv.polling_command(0), "^P007ACCT");
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  EXPECT_TRUE(start.has_state());
  EXPECT_EQ(start.hour, 23);
  EXPECT_EQ(start.minute, 0);
  EXPECT_EQ(start.second, 0);
  EXPECT_EQ(end.hour, 6);
}

TEST(Pi18TimeWindowTest, StartWriteKeepsTheEnd) {
  TestablePipsolar inv;
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 22, 30), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACCT2230,0600");
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_END, 7, 5), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(1), "^S016ACCT2230,0705");
}

TEST(Pi18TimeWindowTest, OutputWindowUsesAclt) {
  TestablePipsolar inv;
  inv.handle_p007aclt_(SYNTHETIC_WINDOW_2300_0600);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_OUTPUT_START, 22, 30), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT2230,0600");
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_OUTPUT_END, 5, 0), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(1), "^S016ACLT2230,0500");
}

TEST(Pi18TimeWindowTest, UnknownPartnerIsRejected) {
  TestablePipsolar inv;
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 22, 30), WriteResult::REJECTED);
  inv.handle_p007aclt_(SYNTHETIC_WINDOW_2300_0600);  // the other window doesn't help
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 22, 30), WriteResult::REJECTED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18TimeWindowTest, OutOfRangeWriteIsRejected) {
  TestablePipsolar inv;
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 24, 0), WriteResult::REJECTED);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 12, 60), WriteResult::REJECTED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18TimeWindowTest, InvalidTimesAreTreatedAsMissing) {
  TestablePipsolar inv;
  PipsolarTime start;
  start.set_parent(&inv);
  start.set_window_end(TimeWindowEnd::AC_CHARGE_START);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_START, &start);
  for (const char *frame :
       {SYNTHETIC_WINDOW_HOUR_24, SYNTHETIC_WINDOW_MINUTE_60, SYNTHETIC_WINDOW_NOT_DIGITS, "", "^D012"}) {
    inv.handle_p007acct_(frame);
    EXPECT_FALSE(start.has_state()) << frame;
    EXPECT_FALSE(inv.get_time_setting(TimeWindowEnd::AC_CHARGE_START).has_value()) << frame;
    EXPECT_FALSE(inv.get_time_setting(TimeWindowEnd::AC_CHARGE_END).has_value()) << frame;
  }
}

TEST(Pi18TimeWindowTest, SameTimeSendsNothing) {
  TestablePipsolar inv;
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 23, 0), WriteResult::UNCHANGED);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_END, 6, 0), WriteResult::UNCHANGED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18TimeWindowTest, ReadingBeforeTheWriteIsSentDoesNotUndoIt) {
  TestablePipsolar inv;
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  ASSERT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 22, 30), WriteResult::QUEUED);
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);  // an old reading while the write is queued
  EXPECT_EQ(*inv.get_time_setting(TimeWindowEnd::AC_CHARGE_START), 2230);
  // Writing the same value again sends nothing
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 22, 30), WriteResult::UNCHANGED);
  // The partner is sent as already known, so the end write after the start keeps 22:30
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_END, 7, 0), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(1), "^S016ACCT2230,0700");
}

TEST(Pi18TimeWindowTest, EntityControlWritesAndIgnoresSeconds) {
  TestablePipsolar inv;
  PipsolarTime start;
  start.set_parent(&inv);
  start.set_window_end(TimeWindowEnd::AC_CHARGE_START);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_START, &start);
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  start.make_call().set_time(22, 30, 45).perform();
  EXPECT_EQ(inv.queued_command(), "^S016ACCT2230,0600");
  // The entity keeps showing the reading until the inverter reports the change
  EXPECT_EQ(start.hour, 23);
  EXPECT_EQ(start.minute, 0);
  EXPECT_EQ(start.second, 0);
}

TEST(Pi18TimeWindowTest, EntityControlTakesMissingFieldsFromTheState) {
  TestablePipsolar inv;
  PipsolarTime start;
  start.set_parent(&inv);
  start.set_window_end(TimeWindowEnd::AC_CHARGE_START);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_START, &start);
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  start.make_call().set_minute(15).perform();
  EXPECT_EQ(inv.queued_command(), "^S016ACCT2315,0600");
}

TEST(Pi18TimeWindowTest, UnchangedControlRepublishesTheReading) {
  TestablePipsolar inv;
  PipsolarTime end;
  end.set_parent(&inv);
  end.set_window_end(TimeWindowEnd::AC_CHARGE_END);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_END, &end);
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  int published = 0;
  end.add_on_state_callback([&published]() { published++; });
  end.make_call().set_time(6, 0, 0).perform();
  EXPECT_EQ(queued_count(inv), 0u);
  EXPECT_EQ(published, 1);
}

TEST(Pi18TimeWindowTest, WindowPollsAreRegisteredAsSlow) {
  TestablePipsolar inv;
  PipsolarTime charge_start, output_end;
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_START, &charge_start);
  inv.set_time_window_end(TimeWindowEnd::AC_OUTPUT_END, &output_end);
  EXPECT_EQ(inv.polling_command(0), "^P007ACCT");
  EXPECT_EQ(inv.polling_command(1), "^P007ACLT");
  EXPECT_EQ(inv.polling_command(2), "");
}

TEST(Pi18TimeWindowTest, WriteRefreshesThePoll) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  PipsolarTime start;
  start.set_parent(&inv);
  start.set_window_end(TimeWindowEnd::AC_CHARGE_START);
  inv.set_time_window_end(TimeWindowEnd::AC_CHARGE_START, &start);
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  // Drain the first poll round
  inv.update();
  for (int i = 0; i < 12; i++) {
    inv.loop();
    uart.receive(inv.reply("^0"));
    inv.run(uart, 6);
  }
  const size_t before = uart.count_sent("^P007ACCT");
  start.make_call().set_time(22, 30, 0).perform();
  for (int i = 0; i < 12; i++) {
    inv.loop();
    uart.receive(inv.reply("^1"));
    inv.run(uart, 6);
  }
  EXPECT_EQ(uart.count_sent("^S016ACCT2230,0600"), 1u);
  EXPECT_GT(uart.count_sent("^P007ACCT"), before);
}

TEST(Pi18TimeWindowTest, FullQueueRejectsAWriteWithoutPending) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  inv.handle_p007acct_(SYNTHETIC_WINDOW_2300_0600);
  fill_queue(inv);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 22, 30), WriteResult::REJECTED);
  EXPECT_EQ(*inv.get_time_setting(TimeWindowEnd::AC_CHARGE_START), 2300);
  drain_queue(inv, uart);
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_CHARGE_START, 22, 30), WriteResult::QUEUED);
}

}  // namespace esphome::pi18::testing
