#include <gtest/gtest.h>
#include "common.h"

namespace esphome::pi18::testing {

namespace {

constexpr uint32_t UPDATE_INTERVAL_MS = 30000;  // 2 updates make one slow interval
constexpr const char *SLOW_POLL = "^P004T";
constexpr const char *STATUS_POLL = "^P006MOD";

struct Fixture {
  FakeUart uart;
  TestablePipsolar inv;
  sensor::Sensor mode_id;

  Fixture() {
    this->inv.set_uart_parent(&this->uart);
    this->inv.set_update_interval(UPDATE_INTERVAL_MS);
    this->inv.set_device_mode_id(&this->mode_id);
    this->inv.set_command_timeout_ms(1);
  }

  void add(const char *command, ENUMPollingCommand id, PollSchedule schedule) {
    this->inv.add_polling_command_(command, id, schedule);
  }

  // Answers a send of `command` with `text`, whenever the component sends it
  void answer_every_poll(const std::string &text, int loops = 12) {
    for (int i = 0; i < loops; i++) {
      this->uart.receive(this->inv.reply(text));
      this->inv.loop();
    }
  }

  // Replies ^0 to every send for `loops` loop() calls
  void refuse_everything(int loops = 24) { this->answer_every_poll("^0", loops); }

  // Lets each poll time out; a send and its timeout take two loops
  void run_unanswered(int loops = 24) {
    for (int i = 0; i < loops; i++) {
      delay(2);
      this->inv.loop();
    }
  }

  void one_interval() {
    this->inv.update();
    this->inv.update();
  }
};

}  // namespace

TEST(Pi18PollScheduleTest, SlowPollIsSentOncePerInterval) {
  Fixture f;
  f.add(SLOW_POLL, POLLING_P004T, PollSchedule::SLOW);
  f.answer_every_poll("^D0050");
  EXPECT_EQ(f.uart.count_sent(SLOW_POLL), 1u);
  f.inv.update();
  f.answer_every_poll("^D0050");
  EXPECT_EQ(f.uart.count_sent(SLOW_POLL), 1u);
  f.inv.update();
  f.answer_every_poll("^D0050");
  EXPECT_EQ(f.uart.count_sent(SLOW_POLL), 2u);
}

TEST(Pi18PollScheduleTest, StatusPollsStillRunEveryUpdate) {
  Fixture f;
  f.add(SLOW_POLL, POLLING_P004T, PollSchedule::SLOW);
  f.answer_every_poll("^D0050");
  f.inv.update();
  f.answer_every_poll("^D0050");
  EXPECT_EQ(f.uart.count_sent(STATUS_POLL), 2u);
  EXPECT_EQ(f.uart.count_sent(SLOW_POLL), 1u);
}

TEST(Pi18PollScheduleTest, RefreshSendsASlowPollBeforeItsInterval) {
  Fixture f;
  f.add(SLOW_POLL, POLLING_P004T, PollSchedule::SLOW);
  f.answer_every_poll("^D0050");
  f.inv.request_poll_refresh(POLLING_P004T);
  f.answer_every_poll("^D0050");
  EXPECT_EQ(f.uart.count_sent(SLOW_POLL), 2u);
}

TEST(Pi18PollScheduleTest, OncePollStopsAfterItsFirstAnswer) {
  Fixture f;
  f.add("^P009MCHGCR", POLLING_P009MCHGCR, PollSchedule::ONCE);
  f.answer_every_poll("^D0050");
  for (int i = 0; i < 6; i++) {
    f.one_interval();
    f.answer_every_poll("^D0050");
  }
  EXPECT_EQ(f.uart.count_sent("^P009MCHGCR"), 1u);
}

TEST(Pi18PollScheduleTest, UnansweredSlowPollStopsAfterThreeAttempts) {
  Fixture f;
  f.add(SLOW_POLL, POLLING_P004T, PollSchedule::SLOW);
  f.run_unanswered();
  EXPECT_EQ(f.uart.count_sent(SLOW_POLL), 3u);
  EXPECT_GT(f.uart.count_sent(STATUS_POLL), 3u);  // the status poll keeps being retried
}

TEST(Pi18PollScheduleTest, SlowPollRefusedThreeIntervalsIsDropped) {
  Fixture f;
  f.add(SLOW_POLL, POLLING_P004T, PollSchedule::SLOW);
  f.refuse_everything();
  for (int i = 0; i < 3; i++) {
    f.one_interval();
    f.refuse_everything();
  }
  const size_t sent = f.uart.count_sent(SLOW_POLL);
  EXPECT_EQ(sent, 9u);
  for (int i = 0; i < 3; i++) {
    f.one_interval();
    f.refuse_everything();
  }
  f.inv.request_poll_refresh(POLLING_P004T);
  f.refuse_everything();
  EXPECT_EQ(f.uart.count_sent(SLOW_POLL), sent);
}

TEST(Pi18PollScheduleTest, AnswerResetsTheFailedIntervalCount) {
  Fixture f;
  f.add(SLOW_POLL, POLLING_P004T, PollSchedule::SLOW);
  f.refuse_everything();  // interval 1 refused
  f.one_interval();
  f.refuse_everything();  // interval 2 refused
  f.one_interval();
  f.answer_every_poll("^D0050");  // interval 3 answered
  f.one_interval();
  f.refuse_everything();  // 2 more refused
  f.one_interval();
  f.refuse_everything();
  const size_t sent = f.uart.count_sent(SLOW_POLL);
  f.one_interval();
  f.answer_every_poll("^D0050");
  EXPECT_GT(f.uart.count_sent(SLOW_POLL), sent);
}

}  // namespace esphome::pi18::testing
