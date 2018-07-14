#include <gtest/gtest.h>
#include "uvcpp.h"

using namespace uvcpp;

TEST(Prepare, LoopCount) {
  auto prepare = Prepare::createUnique();
  ASSERT_TRUE(!!prepare);

  prepare->on<EvError>([](auto e, auto &prepare) {
    FAIL() << "prepare failed with status: " << e.status;
  });
  prepare->on<EvClose>([](auto e, auto &prepare) {
    LOG_D("prepare closed");
  });

  const auto CHECK_COUNT = 1;
  auto count = 0;
  prepare->on<EvPrepare>([&count](auto e, auto &prepare) {
    LOG_D("counting: %d", count);
    if (++count == CHECK_COUNT) {
      prepare.stop();
      prepare.close();
    }
  });

  prepare->start();

  Loop::get().run();

  ASSERT_EQ(count, CHECK_COUNT);
}

