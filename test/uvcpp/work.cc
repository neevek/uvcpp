#include <gtest/gtest.h>
#include "uvcpp.h"

using namespace uvcpp;

TEST(Work, Repeat) {
  auto work = Work::createUnique();
  ASSERT_TRUE(!!work);

  work->on<EvError>([](auto e, auto &work) {
    FAIL() << "work failed with status: " << e.status;
  });
  work->on<EvClose>([](auto e, auto &work) {
    LOG_D("work closed");
  });

  const auto CHECK_COUNT = 2;
  auto count = 0;
  work->on<EvWork>([&count](auto e, auto &work) {
    ++count;
  });
  work->on<EvAfterWork>([&count](auto e, auto &work) {
    ++count;
  });

  work->start();

  Loop::get().run();

  ASSERT_EQ(count, CHECK_COUNT);
}

