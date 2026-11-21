#include <gtest/gtest.h>
#include <string>
#include "../common.h"
#include "esphome/components/pi18/switch/pipsolar_ac_output_switch.h"

namespace esphome::pi18::testing {

namespace {

size_t queued_count(const TestablePipsolar &inv) {
  size_t count = 0;
  for (size_t i = 0; i < 10; i++)
    count += inv.queued_command(i).empty() ? 0 : 1;
  return count;
}

}  // namespace

// write_state is protected; the entity is driven through turn_on()/turn_off() like Home Assistant does.
// setup() isn't called here, so the preference stays default-constructed (a no-op on save).
TEST(Pi18AcOutputSwitchTest, SwitchForwardsAndRepublishesOnRejection) {
  TestablePipsolar inv;
  PipsolarAcOutputSwitch sw;
  sw.set_parent(&inv);
  inv.set_ac_output_switch(&sw);

  // No clock yet: the off write is rejected, nothing is queued, the state is left alone
  sw.publish_state(true);
  sw.turn_off();
  EXPECT_EQ(queued_count(inv), 0u);
  EXPECT_TRUE(sw.state);
  EXPECT_FALSE(inv.get_ac_output_keep_off());

  // Not captured from an inverter: synthetic clock (10:10) with a window 08:00-09:00, so the output is off
  inv.handle_p004t_("^D01720261120101000");
  inv.handle_p007aclt_("^D0120800,0900");
  EXPECT_FALSE(sw.state);

  // A rejected On request (no ^P006FWS reply decoded yet) is dropped by showing the reading again
  sw.publish_state(true);
  sw.turn_on();
  EXPECT_EQ(queued_count(inv), 0u);
  EXPECT_FALSE(sw.state);

  // Timer disabled at 09:10, so the output is on
  inv.handle_p004t_("^D01720261120091000");
  inv.handle_p007aclt_("^D0120000,0000");
  EXPECT_TRUE(sw.state);

  // An accepted Off is forwarded; the switch doesn't publish the request itself
  sw.turn_off();
  EXPECT_EQ(inv.queued_command(), "^S016ACLT0800,0900");
  EXPECT_TRUE(sw.state);
  EXPECT_TRUE(inv.get_ac_output_keep_off());
}

}  // namespace esphome::pi18::testing
