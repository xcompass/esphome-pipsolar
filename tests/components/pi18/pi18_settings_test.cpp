#include <gtest/gtest.h>
#include <string>
#include "common.h"
#include "frames_lv5048.h"

namespace esphome::pi18::testing {

namespace {

// Not captured from an inverter: a synthetic ^P010MUCHGCR reply (allowed AC charging currents,
// 3-digit amps). 4 values of 3 digits and 3 commas = 15 bytes, plus the 3-digit length itself.
constexpr const char *SYNTHETIC_P010MUCHGCR = "^D018010,020,030,040";

// LV5048_P007PIRI_BANANNAJOE with the battery cut-off (field 10) changed from 48.0 V to 47.5 V.
// Same length, so the length field is unchanged.
constexpr const char *PIRI_CUT_OFF_475 =
    "^D0892300,243,2300,500,243,5600,5600,480,500,520,475,554,540,2,010,060,0,0,2,9,1,0,0,0,1,00";

// Without a time-out, a loop-driven test needs one loop() to send and some more to receive
struct Fixture {
  FakeUart uart;
  TestablePipsolar inv;

  Fixture() { this->inv.set_uart_parent(&this->uart); }

  // The component sends the next command or poll, then reads this reply
  void exchange(const std::string &text) {
    this->inv.loop();
    this->uart.receive(this->inv.reply(text));
    this->inv.run(this->uart, 6);
  }
};

// PIRI known without a loop; registers nothing
void read_piri(TestablePipsolar &inv) { inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE); }

}  // namespace

TEST(Pi18SettingsTest, PiriSetsKnownValues) {
  TestablePipsolar inv;
  EXPECT_FALSE(inv.get_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE).has_value());
  EXPECT_FALSE(inv.get_number_setting(NumberSetting::MAX_AC_CHARGING_CURRENT).has_value());
  read_piri(inv);
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::BATTERY_RECHARGE_VOLTAGE), 50.0f, 0.01f);
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::BATTERY_REDISCHARGE_VOLTAGE), 52.0f, 0.01f);
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE), 48.0f, 0.01f);
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::BATTERY_BULK_VOLTAGE), 55.4f, 0.01f);
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::BATTERY_FLOAT_VOLTAGE), 54.0f, 0.01f);
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::MAX_AC_CHARGING_CURRENT), 10.0f, 0.01f);
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::MAX_CHARGING_CURRENT), 60.0f, 0.01f);
}

TEST(Pi18SettingsTest, CutOffWriteUsesTenths) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 47.5f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S010PSDV475");
  EXPECT_NEAR(*inv.get_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE), 47.5f, 0.01f);
}

TEST(Pi18SettingsTest, BackToUtilityKeepsBackToBattery) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_RECHARGE_VOLTAGE, 49.0f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S014BUCD490,520");
}

TEST(Pi18SettingsTest, BackToBatteryFullIsZero) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_REDISCHARGE_VOLTAGE, 0.0f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S014BUCD500,000");
}

TEST(Pi18SettingsTest, ChargingCurrentWrites) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::MAX_CHARGING_CURRENT, 80.0f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(0), "^S013MCHGC0,080");
  EXPECT_EQ(inv.write_number_setting(NumberSetting::MAX_AC_CHARGING_CURRENT, 20.0f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(1), "^S014MUCHGC0,020");
}

TEST(Pi18SettingsTest, UnchangedValueSendsNothing) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 48.0f), WriteResult::UNCHANGED);
  EXPECT_EQ(inv.queued_command(), "");
}

TEST(Pi18SettingsTest, SecondIdenticalWriteWhilePendingIsSkipped) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 47.5f), WriteResult::QUEUED);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 47.5f), WriteResult::UNCHANGED);
  EXPECT_EQ(inv.queued_command(0), "^S010PSDV475");
  EXPECT_EQ(inv.queued_command(1), "");
}

TEST(Pi18SettingsTest, PairWritesInQuickSuccessionCombine) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_BULK_VOLTAGE, 55.0f), WriteResult::QUEUED);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_FLOAT_VOLTAGE, 53.5f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(0), "^S015MCHGV550,540");
  EXPECT_EQ(inv.queued_command(1), "^S015MCHGV550,535");
}

TEST(Pi18SettingsTest, PairWithUnknownPartnerIsRejected) {
  TestablePipsolar inv;
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_RECHARGE_VOLTAGE, 49.0f), WriteResult::REJECTED);
  EXPECT_EQ(inv.queued_command(), "");
}

TEST(Pi18SettingsTest, CurrentNotInAllowedListIsRejected) {
  TestablePipsolar inv;
  read_piri(inv);
  inv.handle_p009mchgcr_(LV5048_P009MCHGCR);
  EXPECT_EQ(inv.get_allowed_currents(NumberSetting::MAX_CHARGING_CURRENT).size(), 12u);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::MAX_CHARGING_CURRENT, 65.0f), WriteResult::REJECTED);
  EXPECT_EQ(inv.queued_command(), "");
  EXPECT_EQ(inv.write_number_setting(NumberSetting::MAX_CHARGING_CURRENT, 70.0f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(), "^S013MCHGC0,070");
}

TEST(Pi18SettingsTest, AcChargingListComesFromItsOwnReply) {
  TestablePipsolar inv;
  read_piri(inv);
  inv.handle_p010muchgcr_(SYNTHETIC_P010MUCHGCR);
  const auto &list = inv.get_allowed_currents(NumberSetting::MAX_AC_CHARGING_CURRENT);
  ASSERT_EQ(list.size(), 4u);
  EXPECT_EQ(list[0], 10);
  EXPECT_TRUE(inv.get_allowed_currents(NumberSetting::MAX_CHARGING_CURRENT).empty());
  EXPECT_EQ(inv.write_number_setting(NumberSetting::MAX_AC_CHARGING_CURRENT, 25.0f), WriteResult::REJECTED);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::MAX_AC_CHARGING_CURRENT, 30.0f), WriteResult::QUEUED);
}

TEST(Pi18SettingsTest, EmptyAllowedListReplyLeavesTheListUnchanged) {
  TestablePipsolar inv;
  inv.handle_p009mchgcr_(LV5048_P009MCHGCR);
  inv.handle_p009mchgcr_("");
  EXPECT_EQ(inv.get_allowed_currents(NumberSetting::MAX_CHARGING_CURRENT).size(), 12u);
}

TEST(Pi18SettingsTest, CurrentAllowedWhileListUnknown) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::MAX_CHARGING_CURRENT, 65.0f), WriteResult::QUEUED);
}

TEST(Pi18SettingsTest, ValueThatCannotBeEncodedIsRejected) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 100.0f), WriteResult::REJECTED);
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, NAN), WriteResult::REJECTED);
  EXPECT_EQ(inv.queued_command(), "");
}

TEST(Pi18SettingsTest, SetNumberRegistersThePolls) {
  TestablePipsolar inv;
  inv.set_number(NumberSetting::BATTERY_UNDER_VOLTAGE, nullptr);
  inv.set_number(NumberSetting::MAX_CHARGING_CURRENT, nullptr);
  inv.set_number(NumberSetting::MAX_AC_CHARGING_CURRENT, nullptr);
  EXPECT_EQ(inv.polling_command(0), "^P007PIRI");
  EXPECT_EQ(inv.polling_command(1), "^P009MCHGCR");
  EXPECT_EQ(inv.polling_command(2), "^P010MUCHGCR");
  EXPECT_EQ(inv.polling_command(3), "");
}

TEST(Pi18SettingsTest, ChargeVoltageOutputsShareTheKnownValues) {
  TestablePipsolar inv;
  read_piri(inv);
  EXPECT_TRUE(inv.set_charge_voltages(56.0f, 55.0f));
  EXPECT_EQ(inv.write_number_setting(NumberSetting::BATTERY_FLOAT_VOLTAGE, 54.5f), WriteResult::QUEUED);
  EXPECT_EQ(inv.queued_command(1), "^S015MCHGV560,545");
}

TEST(Pi18SettingsTest, ReadingInFlightKeepsThePendingValue) {
  Fixture f;
  f.inv.set_number(NumberSetting::BATTERY_UNDER_VOLTAGE, nullptr);
  f.inv.loop();  // PIRI sent, reply not yet read
  read_piri(f.inv);
  EXPECT_EQ(f.inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 47.5f), WriteResult::QUEUED);
  f.uart.receive(f.inv.reply(LV5048_P007PIRI_BANANNAJOE));  // the old reading arrives
  f.inv.run(f.uart, 6);
  EXPECT_NEAR(*f.inv.get_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE), 47.5f, 0.01f);
}

TEST(Pi18SettingsTest, RefusedWriteRevertsOnNextReading) {
  Fixture f;
  f.inv.set_number(NumberSetting::BATTERY_UNDER_VOLTAGE, nullptr);
  f.exchange(LV5048_P007PIRI_BANANNAJOE);
  ASSERT_EQ(f.inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 47.5f), WriteResult::QUEUED);
  f.exchange("^0");  // the inverter refuses the write
  EXPECT_EQ(f.uart.count_sent("^S010PSDV475"), 1u);
  EXPECT_NEAR(*f.inv.get_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE), 47.5f, 0.01f);
  f.inv.update();
  f.exchange(LV5048_P007PIRI_BANANNAJOE);
  EXPECT_NEAR(*f.inv.get_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE), 48.0f, 0.01f);
}

TEST(Pi18SettingsTest, AcceptedWriteIsConfirmedByNextReading) {
  Fixture f;
  f.inv.set_number(NumberSetting::BATTERY_UNDER_VOLTAGE, nullptr);
  f.exchange(LV5048_P007PIRI_BANANNAJOE);
  ASSERT_EQ(f.inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 47.5f), WriteResult::QUEUED);
  f.exchange("^1");
  f.inv.update();
  f.exchange(PIRI_CUT_OFF_475);
  EXPECT_NEAR(*f.inv.get_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE), 47.5f, 0.01f);
  // Confirmed: a write back to 48.0 is a real change again
  EXPECT_EQ(f.inv.write_number_setting(NumberSetting::BATTERY_UNDER_VOLTAGE, 48.0f), WriteResult::QUEUED);
}

}  // namespace esphome::pi18::testing
