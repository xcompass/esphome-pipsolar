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

size_t aclt_count(const TestablePipsolar &inv) {
  size_t count = 0;
  for (size_t i = 0; i < 10; i++)
    count += inv.queued_command(i).rfind("^S016ACLT", 0) == 0 ? 1 : 0;
  return count;
}

std::string aclt_command(const TestablePipsolar &inv) {
  for (size_t i = 0; i < 10; i++) {
    if (inv.queued_command(i).rfind("^S016ACLT", 0) == 0)
      return inv.queued_command(i);
  }
  return "";
}

// Lets the queued command out and answers it with ^1; the readings are left as they were
void accept_queued_command(TestablePipsolar &inv, FakeUart &uart) {
  if (queued_count(inv) == 0)
    return;
  inv.loop();  // sends it
  uart.receive(inv.reply("^1"));
  inv.loop();  // reads the reply
  inv.loop();  // finishes the command
}

// Runs the polls. Answered polls get the given clock and window readings; otherwise each poll
// times out.
void run_polls(TestablePipsolar &inv, FakeUart &uart, bool answer, const char *hhmm, const char *start, const char *end,
               int loops = 30) {
  for (int i = 0; i < loops; i++) {
    inv.loop();
    if (!inv.polling())
      continue;
    if (!answer) {
      delay(2);
    } else if (inv.polling_identifier() == POLLING_P004T) {
      uart.receive(inv.reply(synthetic_clock(hhmm)));
    } else if (inv.polling_identifier() == POLLING_P007ACLT) {
      uart.receive(inv.reply(synthetic_window(start, end)));
    } else {
      uart.receive(inv.reply(LV2424_P006FWS));
    }
    inv.loop();
  }
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
  EXPECT_EQ(inv.get_time_setting(TimeWindowEnd::AC_OUTPUT_START), 0);
  EXPECT_EQ(inv.get_time_setting(TimeWindowEnd::AC_OUTPUT_END), 0);
  EXPECT_EQ(inv.get_time_reading(TimeWindowEnd::AC_OUTPUT_START), 0);
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

TEST(Pi18AcOutputTest, RearmMovesTheWindowAnHourBeforeItOpens) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  set_inverter(inv, "1030", "1200", "1300");
  EXPECT_EQ(queued_count(inv), 0u);
  inv.handle_p004t_(synthetic_clock("1100").c_str());
  EXPECT_EQ(inv.queued_command(), "^S016ACLT1000,1100");
  EXPECT_EQ(queued_count(inv), 1u);
}

TEST(Pi18AcOutputTest, ClockJumpIntoWindowRearms) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  set_inverter(inv, "1030", "1200", "1300");
  inv.handle_p004t_(synthetic_clock("1230").c_str());
  EXPECT_EQ(inv.queued_command(), "^S016ACLT1100,1200");
}

TEST(Pi18AcOutputTest, KeepOffOverridesHandSetWindow) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  inv.handle_p007aclt_("^D0120000,0000");
  inv.handle_p004t_(synthetic_clock("0910").c_str());
  EXPECT_EQ(inv.queued_command(), "^S016ACLT0800,0900");
}

TEST(Pi18AcOutputTest, KeepOffClearedLeavesHandSetWindowAlone) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p006fws_(LV2424_P006FWS);
  set_inverter(inv, "1306", "1200", "1300");
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::UNCHANGED);
  EXPECT_TRUE(inv.get_ac_output_keep_off());
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::QUEUED);
  EXPECT_FALSE(inv.get_ac_output_keep_off());
  EXPECT_EQ(inv.queued_command(), "^S016ACLT0000,0000");
  accept_queued_command(inv, uart);  // the On write is out and answered: nothing is outstanding
  EXPECT_EQ(queued_count(inv), 0u);
  inv.handle_p007aclt_(synthetic_window("0800", "0900").c_str());  // set by hand afterwards
  inv.handle_p004t_(synthetic_clock("0830").c_str());
  inv.handle_p004t_(synthetic_clock("1100").c_str());
  EXPECT_EQ(queued_count(inv), 0u);
  EXPECT_EQ(uart.count_sent("^S016ACLT"), 1u);  // only the On write
}

TEST(Pi18AcOutputTest, RejectedWriteLeavesKeepOffAsItWas) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::REJECTED);  // window unknown
  EXPECT_TRUE(inv.get_ac_output_keep_off());
}

TEST(Pi18AcOutputTest, RestoredKeepOffWritesNothingWhileWindowIsFar) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  EXPECT_EQ(queued_count(inv), 0u);
  set_inverter(inv, "1400", "1200", "1300");
  EXPECT_EQ(queued_count(inv), 0u);
}

TEST(Pi18AcOutputTest, NoRearmWhileWindowWriteOutstanding) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  set_inverter(inv, "1030", "1200", "1300");
  EXPECT_EQ(inv.write_time_setting(TimeWindowEnd::AC_OUTPUT_START, 11, 0), WriteResult::QUEUED);
  inv.handle_p004t_(synthetic_clock("1100").c_str());
  EXPECT_EQ(aclt_count(inv), 1u);
}

TEST(Pi18AcOutputTest, RefusedOffRetriesOncePerClockReading) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "0910", "0000", "0000");
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT0800,0900");
  for (int i = 0; i < 12; i++) {
    inv.loop();
    uart.receive(inv.reply("^0"));  // refused
    inv.run(uart, 6);
  }
  EXPECT_EQ(uart.count_sent("^S016ACLT0800,0900"), 1u);
  EXPECT_EQ(aclt_count(inv), 0u);
  inv.handle_p007aclt_("^D0120000,0000");  // the reading shows it didn't take
  EXPECT_TRUE(inv.get_ac_output_keep_off());
  EXPECT_EQ(aclt_count(inv), 0u);  // nothing until the next clock reading
  inv.handle_p004t_(synthetic_clock("0911").c_str());
  EXPECT_EQ(aclt_count(inv), 1u);
  EXPECT_EQ(aclt_command(inv), "^S016ACLT0800,0900");
}

TEST(Pi18AcOutputTest, AnsweredSlowPollsSurviveALinkOutage) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  inv.set_update_interval(30000);  // 2 updates make one slow interval
  inv.set_command_timeout_ms(1);
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  run_polls(inv, uart, true, "1306", "0000", "0000");
  ASSERT_TRUE(sw.has_state());
  EXPECT_TRUE(sw.state);
  for (int interval = 0; interval < 5; interval++) {  // more than UNSUPPORTED_AFTER_INTERVALS
    inv.update();
    inv.update();
    run_polls(inv, uart, false, "", "", "");
  }
  const size_t sent = uart.count_sent("^P004T");
  inv.update();
  inv.update();
  run_polls(inv, uart, true, "1100", "1200", "1300");
  EXPECT_GT(uart.count_sent("^P004T"), sent);
  EXPECT_FALSE(sw.state);  // 11:00 is outside the new 12:00-13:00 window
}

TEST(Pi18AcOutputTest, RearmStopsAfterThreeWritesThatDoNotTakeEffect) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  set_inverter(inv, "0910", "0000", "0000");
  inv.handle_p004t_(synthetic_clock("0910").c_str());  // the first reading with the window known
  accept_queued_command(inv, uart);
  for (const char *hhmm : {"0911", "0912", "0913", "0914", "0915"}) {
    inv.handle_p007aclt_("^D0120000,0000");  // the reading shows the write didn't store
    inv.handle_p004t_(synthetic_clock(hhmm).c_str());
    accept_queued_command(inv, uart);
  }
  EXPECT_EQ(uart.count_sent("^S016ACLT"), 3u);
  // A user request starts over
  inv.handle_p007aclt_("^D0120000,0000");
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::QUEUED);
  accept_queued_command(inv, uart);
  EXPECT_EQ(uart.count_sent("^S016ACLT"), 4u);
  inv.handle_p007aclt_("^D0120000,0000");
  inv.handle_p004t_(synthetic_clock("0916").c_str());
  accept_queued_command(inv, uart);
  EXPECT_EQ(uart.count_sent("^S016ACLT"), 5u);
}

TEST(Pi18AcOutputTest, RearmCountResetsOnceTheWindowKeepsTheOutputOff) {
  FakeUart uart;
  TestablePipsolar inv;
  inv.set_uart_parent(&uart);
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  set_inverter(inv, "0910", "0000", "0000");
  inv.handle_p004t_(synthetic_clock("0910").c_str());
  accept_queued_command(inv, uart);
  inv.handle_p007aclt_("^D0120000,0000");
  inv.handle_p004t_(synthetic_clock("0911").c_str());
  accept_queued_command(inv, uart);
  inv.handle_p007aclt_("^D0120800,0900");  // this one took
  inv.handle_p004t_(synthetic_clock("0912").c_str());
  EXPECT_EQ(queued_count(inv), 0u);
  for (const char *hhmm : {"1010", "1011", "1012", "1013"}) {
    inv.handle_p007aclt_("^D0120000,0000");
    inv.handle_p004t_(synthetic_clock(hhmm).c_str());
    accept_queued_command(inv, uart);
  }
  EXPECT_EQ(uart.count_sent("^S016ACLT"), 5u);  // 2 + 3, not 2 + 2
}

TEST(Pi18AcOutputTest, OnWhileKeepOffRemovesTheSwitchsOwnWindowEvenWithTheClockInsideIt) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.handle_p006fws_(LV2424_P006FWS);
  set_inverter(inv, "1230", "1200", "1300");
  inv.set_ac_output_keep_off(true);
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S016ACLT0000,0000");
  EXPECT_FALSE(inv.get_ac_output_keep_off());
}

TEST(Pi18AcOutputTest, OnRejectedAfterAFailedFwsPoll) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "1306", "1200", "1300");
  inv.handle_p006fws_(LV2424_P006FWS);
  inv.handle_p006fws_("^D0");  // short reply
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::REJECTED);
  EXPECT_EQ(queued_count(inv), 0u);
  inv.handle_p006fws_(LV2424_P006FWS);
  EXPECT_EQ(inv.write_ac_output(true), WriteResult::QUEUED);
}

TEST(Pi18AcOutputTest, RearmAcrossMidnightFromJustAfter) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  set_inverter(inv, "2300", "0000", "0100");
  inv.handle_p004t_(synthetic_clock("2300").c_str());
  EXPECT_EQ(inv.queued_command(), "^S016ACLT2200,2300");
}

TEST(Pi18AcOutputTest, RearmAcrossMidnightFromJustBefore) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  inv.set_ac_output_keep_off(true);
  set_inverter(inv, "2200", "2300", "0000");
  inv.handle_p004t_(synthetic_clock("2200").c_str());
  EXPECT_EQ(inv.queued_command(), "^S016ACLT2100,2200");
}

TEST(Pi18AcOutputTest, OffUnchangedAtTheWrap) {
  TestablePipsolar inv;
  TestSwitch sw;
  inv.set_ac_output_switch(&sw);
  set_inverter(inv, "0015", "2300", "0000");
  EXPECT_EQ(inv.write_ac_output(false), WriteResult::UNCHANGED);
  EXPECT_EQ(queued_count(inv), 0u);
}

}  // namespace esphome::pi18::testing
