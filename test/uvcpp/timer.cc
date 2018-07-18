#include <gtest/gtest.h>
#include "uvcpp.h"

using namespace uvcpp;

TEST(Timer, Repeat) {
  auto timer = Timer::createUnique();
  ASSERT_TRUE(!!timer);

  timer->on<EvError>([](const auto &e, auto &timer) {
    FAIL() << "timer failed with status: " << e.status;
  });
  timer->on<EvClose>([](const auto &e, auto &timer) {
    LOG_D("timer closed");
  });

  const auto CHECK_COUNT = 5;
  auto count = 0;
  timer->on<EvTimer>([&count](const auto &e, auto &timer) {
    LOG_D("counting: %d", count);
    if (++count == CHECK_COUNT) {
      timer.stop();
      timer.close();
    }
  });

  timer->start(0, 10);

  Loop::get().run();

  ASSERT_EQ(count, CHECK_COUNT);
}

