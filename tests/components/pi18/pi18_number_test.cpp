#include <gtest/gtest.h>
#include <string>
#include "common.h"
#include "esphome/components/pi18/pipsolar_number.h"
#include "frames_lv5048.h"

namespace esphome::pi18::testing {

namespace {

// Wires a number to a hub that has had a PIRI reply.
// LV5048_P007PIRI_BANANNAJOE reports bulk 55.4 V, float 54.0 V, back to utility 50.0 V,
// back to battery 52.0 V and cut-off 48.0 V.
struct NumberFixture {
  TestablePipsolar inv;
  PipsolarNumber number;

  NumberFixture(NumberSetting setting, float min, float max, float step) {
    this->number.set_parent(&this->inv);
    this->number.set_setting(setting);
    this->number.traits.set_min_value(min);
    this->number.traits.set_max_value(max);
    this->number.traits.set_step(step);
    this->inv.set_number(setting, &this->number);
    this->inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);
  }

  void set(float value) { this->number.make_call().set_value(value).perform(); }
};

}  // namespace

TEST(Pi18NumberTest, ReadingIsPublished) {
  NumberFixture f(NumberSetting::BATTERY_FLOAT_VOLTAGE, 48.0f, 58.4f, 0.1f);
  EXPECT_FLOAT_EQ(f.number.state, 54.0f);
  EXPECT_TRUE(f.number.has_state());
}

TEST(Pi18NumberTest, ValueOnStepIsWritten) {
  NumberFixture f(NumberSetting::BATTERY_FLOAT_VOLTAGE, 48.0f, 58.4f, 0.1f);
  f.set(53.5f);
  EXPECT_EQ(f.inv.queued_command(), "^S015MCHGV554,535");
  // No optimistic state: the number shows the old reading until the next PIRI reply
  EXPECT_FLOAT_EQ(f.number.state, 54.0f);
  EXPECT_NEAR(*f.inv.get_number_setting(NumberSetting::BATTERY_FLOAT_VOLTAGE), 53.5f, 0.01f);
}

TEST(Pi18NumberTest, PendingWriteDoesNotHideTheReading) {
  NumberFixture f(NumberSetting::BATTERY_FLOAT_VOLTAGE, 48.0f, 58.4f, 0.1f);
  f.set(53.5f);
  f.inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);
  EXPECT_FLOAT_EQ(f.number.state, 54.0f);
  EXPECT_NEAR(*f.inv.get_number_setting(NumberSetting::BATTERY_FLOAT_VOLTAGE), 53.5f, 0.01f);
}

TEST(Pi18NumberTest, OffStepValueIsRejected) {
  NumberFixture f(NumberSetting::BATTERY_FLOAT_VOLTAGE, 48.0f, 58.4f, 0.1f);
  f.number.publish_state(30.0f);  // as if the UI had moved it
  f.set(53.55f);
  EXPECT_EQ(f.inv.queued_command(), "");
  EXPECT_FLOAT_EQ(f.number.state, 54.0f);
}

TEST(Pi18NumberTest, FullGapIsRejectedAndZeroMeansFull) {
  NumberFixture f(NumberSetting::BATTERY_REDISCHARGE_VOLTAGE, 0.0f, 58.0f, 1.0f);
  f.number.set_min_nonzero_value(48.0f);
  f.number.publish_state(12.0f);  // as if the UI had moved it
  f.set(12.0f);
  EXPECT_EQ(f.inv.queued_command(), "");
  EXPECT_FLOAT_EQ(f.number.state, 52.0f);
  f.set(0.0f);
  EXPECT_EQ(f.inv.queued_command(), "^S014BUCD500,000");
  EXPECT_FLOAT_EQ(f.number.state, 52.0f);
}

TEST(Pi18NumberTest, TwentyFourVoltFloatIsWrittenWithTheBulkFromTheReading) {
  NumberFixture f(NumberSetting::BATTERY_FLOAT_VOLTAGE, 24.0f, 29.2f, 0.1f);
  // LV5048_P007PIRI_BANANNAJOE with bulk 29.0 V and float 28.2 V; same length
  std::string frame = LV5048_P007PIRI_BANANNAJOE;
  const size_t at = frame.find("554,540");
  ASSERT_NE(at, std::string::npos);
  frame.replace(at, 7, "290,282");
  f.inv.handle_p007piri_(frame.c_str());
  EXPECT_FLOAT_EQ(f.number.state, 28.2f);
  f.set(28.1f);
  EXPECT_EQ(f.inv.queued_command(0), "^S015MCHGV290,281");
  f.set(28.2f);  // back to the reading, but the lower write is still pending
  EXPECT_EQ(f.inv.queued_command(1), "^S015MCHGV290,282");
  EXPECT_FLOAT_EQ(f.number.state, 28.2f);
}

TEST(Pi18NumberTest, UnchangedValueKeepsState) {
  NumberFixture f(NumberSetting::BATTERY_FLOAT_VOLTAGE, 48.0f, 58.4f, 0.1f);
  f.set(54.0f);
  EXPECT_EQ(f.inv.queued_command(), "");
  EXPECT_FLOAT_EQ(f.number.state, 54.0f);
}

TEST(Pi18NumberTest, HubRejectionSnapsBackToTheReading) {
  // 65 A is on the step and in range, but not in the inverter's list of allowed currents
  NumberFixture f(NumberSetting::MAX_CHARGING_CURRENT, 10.0f, 90.0f, 5.0f);
  f.inv.handle_p009mchgcr_(LV5048_P009MCHGCR);
  f.number.publish_state(30.0f);  // as if the UI had moved it
  f.set(65.0f);
  EXPECT_EQ(f.inv.queued_command(), "");
  EXPECT_FLOAT_EQ(f.number.state, 60.0f);
}

}  // namespace esphome::pi18::testing
