#include <gtest/gtest.h>
#include "../common.h"
#include "../frames_lv5048.h"
#include "esphome/components/pi18/output/pipsolar_output.h"

namespace esphome::pi18::testing {

// LV5048_P007PIRI_BANANNAJOE reports bulk 55.4 V and float 54.0 V.

TEST(Pi18OutputTest, BulkVoltageKeepsTheCurrentFloatVoltage) {
  TestablePipsolar inv;
  inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);
  PipsolarOutput out;
  out.set_parent(&inv);
  out.set_charge_voltage(ChargeVoltage::BULK_VOLTAGE);
  out.set_possible_values({56.4f});
  out.set_value(56.4f);
  EXPECT_EQ(inv.queued_command(), "^S015MCHGV564,540");
}

TEST(Pi18OutputTest, FloatVoltageKeepsTheCurrentBulkVoltage) {
  TestablePipsolar inv;
  inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);
  PipsolarOutput out;
  out.set_parent(&inv);
  out.set_charge_voltage(ChargeVoltage::FLOAT_VOLTAGE);
  out.set_possible_values({53.0f});
  out.set_value(53.0f);
  EXPECT_EQ(inv.queued_command(), "^S015MCHGV554,530");
}

TEST(Pi18OutputTest, ChargeVoltageWaitsForTheFirstPiriReply) {
  TestablePipsolar inv;
  PipsolarOutput out;
  out.set_parent(&inv);
  out.set_charge_voltage(ChargeVoltage::BULK_VOLTAGE);
  out.set_possible_values({56.4f});
  out.set_value(56.4f);
  EXPECT_EQ(inv.queued_command(), "");
}

TEST(Pi18OutputTest, ValueOutsideThePossibleValuesIsNotSent) {
  TestablePipsolar inv;
  inv.handle_p007piri_(LV5048_P007PIRI_BANANNAJOE);
  PipsolarOutput out;
  out.set_parent(&inv);
  out.set_charge_voltage(ChargeVoltage::BULK_VOLTAGE);
  out.set_possible_values({56.4f});
  out.set_value(57.0f);
  EXPECT_EQ(inv.queued_command(), "");
}

TEST(Pi18OutputTest, ChargingCurrentIsSentAsThreeDigits) {
  TestablePipsolar inv;
  PipsolarOutput out;
  out.set_parent(&inv);
  out.set_set_command("^S013MCHGC0,%03.0f");
  out.set_possible_values({60.0f});
  out.set_value(60.0f);
  EXPECT_EQ(inv.queued_command(), "^S013MCHGC0,060");
}

}  // namespace esphome::pi18::testing
