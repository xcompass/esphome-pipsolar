#include <gtest/gtest.h>
#include <string>
#include "common.h"
#include "frames_lv2424.h"

namespace esphome::pi18::testing {

namespace {

// Not captured from an inverter: synthetic ^P004T reply for 2026-11-20 at HH:MM:00
std::string synthetic_clock(const char *hhmm) { return std::string("^D01720261120") + hhmm + "00"; }

// Not captured from an inverter: synthetic ^P007ACLT reply
std::string synthetic_window(const char *start, const char *end) { return std::string("^D012") + start + "," + end; }

// Not captured from an inverter: the LV2424 ^P006FWS reply with its 7th flag (battery under
// shutdown) set to 1
constexpr const char *SYNTHETIC_FWS_BATTERY_UNDER = "^D03700,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0";

size_t queued_count(const TestablePipsolar &inv) {
  size_t count = 0;
  for (size_t i = 0; i < 10; i++)
    count += inv.queued_command(i).empty() ? 0 : 1;
  return count;
}

void fill_queue(TestablePipsolar &inv) {
  for (int i = 0; i < 10; i++)
    EXPECT_TRUE(inv.queue_command("^S000FILL"));
  EXPECT_FALSE(inv.queue_command("^S000DROPPED"));
}

// The inverter at hhmm with the given AC output window
void set_inverter(TestablePipsolar &inv, const char *hhmm, const char *start, const char *end) {
  inv.handle_p004t_(synthetic_clock(hhmm).c_str());
  inv.handle_p007aclt_(synthetic_window(start, end).c_str());
}

}  // namespace

TEST(Pi18AcOutputTest, RegistersClockWindowAndWarningPolls) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  EXPECT_EQ(inv.polling_command(0), "^P004T");
  EXPECT_EQ(inv.polling_command(1), "^P007ACLT");
  EXPECT_EQ(inv.polling_command(2), "^P006FWS");
}

TEST(Pi18AcOutputTest, ClockHourRecordedWithoutTimeSensor) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  EXPECT_FALSE(inv.get_inverter_minute_of_day().has_value());
  inv.handle_p004t_(LV2424_P004T);
  EXPECT_EQ(inv.get_inverter_minute_of_day(), 13 * 60 + 6);
}

TEST(Pi18AcOutputTest, WindowContains) {
  EXPECT_TRUE(Pipsolar::window_contains(0, 0, 0));
  EXPECT_TRUE(Pipsolar::window_contains(0, 0, 12 * 60));
  EXPECT_TRUE(Pipsolar::window_contains(1200, 1300, 12 * 60));
  EXPECT_FALSE(Pipsolar::window_contains(1200, 1300, 13 * 60));
  EXPECT_TRUE(Pipsolar::window_contains(2200, 600, 23 * 60));
  EXPECT_FALSE(Pipsolar::window_contains(2200, 600, 7 * 60));
  EXPECT_TRUE(Pipsolar::window_contains(2300, 0, 23 * 60 + 59));
  EXPECT_FALSE(Pipsolar::window_contains(2300, 0, 0));
}

TEST(Pi18AcOutputTest, OffWritesTheHourThatJustEnded) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p004t_(LV2424_P004T);
  inv.handle_p007aclt_(LV2424_P007ACLT);
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT1200,1300");
  EXPECT_EQ(inv.get_time_setting(TimeWindowEnd::AC_OUTPUT_START), 1200);
  EXPECT_EQ(inv.get_time_setting(TimeWindowEnd::AC_OUTPUT_END), 1300);
  EXPECT_EQ(inv.get_time_reading(TimeWindowEnd::AC_OUTPUT_START), 0);
}

TEST(Pi18AcOutputTest, OffAtMidnightWrapsToEleven) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "0015", "0000", "0000");
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT2300,0000");
}

TEST(Pi18AcOutputTest, OffRejectedWithoutClock) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p007aclt_(LV2424_P007ACLT);
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::REJECTED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18AcOutputTest, OffRejectedWithoutWindowReading) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p004t_(LV2424_P004T);
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::REJECTED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18AcOutputTest, OffRejectedWhenQueueFull) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p004t_(LV2424_P004T);
  inv.handle_p007aclt_(LV2424_P007ACLT);
  fill_queue(inv);
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::REJECTED);
  for (size_t i = 0; i < 10; i++)
    EXPECT_EQ(inv.queued_command(i), "^S000FILL");
  EXPECT_FALSE(inv.get_time_setting(TimeWindowEnd::AC_OUTPUT_END) == 1300);
}

TEST(Pi18AcOutputTest, OffUnchangedWhenWindowAlreadyKeepsOutputOffForAnHour) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "1306", "1200", "1300");
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::UNCHANGED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18AcOutputTest, OffRewritesWhenWindowOpensWithinAnHour) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "1306", "1400", "1500");  // opens in 54 minutes
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT1200,1300");
}

TEST(Pi18AcOutputTest, OnWritesDisabledWindowWhenOutputIsOff) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "1306", "1200", "1300");
  inv.handle_p006fws_(LV2424_P006FWS);
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT0000,0000");
}

TEST(Pi18AcOutputTest, OnUnchangedWhenTimerAlreadyOn) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p004t_(LV2424_P004T);
  inv.handle_p007aclt_(LV2424_P007ACLT);
  inv.handle_p006fws_(LV2424_P006FWS);
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::UNCHANGED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18AcOutputTest, OnRejectedDuringBatteryUnderShutdown) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "1306", "1200", "1300");
  inv.handle_p006fws_(SYNTHETIC_FWS_BATTERY_UNDER);
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::REJECTED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18AcOutputTest, OnRejectedBeforeFirstFwsReply) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "1306", "1200", "1300");
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::REJECTED);
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18AcOutputTest, StateFollowsReadingNotWrite) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p004t_(LV2424_P004T);
  inv.handle_p007aclt_(LV2424_P007ACLT);
  ASSERT_TRUE(sw.has_state());
  EXPECT_TRUE(sw.state);
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::QUEUED);
  EXPECT_TRUE(sw.state);
  inv.handle_p007aclt_("^D0121200,1300");
  EXPECT_FALSE(sw.state);
}

TEST(Pi18AcOutputTest, StatePublishedOnClockAndWindowReadings) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p007aclt_("^D0122200,0600");
  EXPECT_FALSE(sw.has_state());  // no clock yet
  inv.handle_p004t_(synthetic_clock("2300").c_str());
  EXPECT_TRUE(sw.state);
  inv.handle_p004t_(synthetic_clock("0700").c_str());
  EXPECT_FALSE(sw.state);
}

TEST(Pi18AcOutputTest, WindowTimeEntityStillWritesBothEnds) {
  TestablePipsolar inv;
  inv.handle_p007aclt_("^D0121200,1300");
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_OUTPUT_START, 5, 0), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT0500,1300");
}

}  // namespace esphome::pi18::testing
