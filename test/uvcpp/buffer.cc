#include <gtest/gtest.h>
#include "uvcpp.h"

using namespace uvcpp;

TEST(Prepare, LoopCount) {
  auto a = std::make_unique<Buffer>(90);
  auto b = std::move(a);

  ASSERT_TRUE(a == nullptr);
  ASSERT_TRUE(!a);
  ASSERT_EQ(b->getCapacity(), 90);
}


