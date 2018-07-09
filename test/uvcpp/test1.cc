#include <gtest/gtest.h>
#include "uvcpp.h"

TEST(Test1, TestPass) {
  ASSERT_TRUE(true) << "test pass";
}

TEST(Test2, TestFail) {
  FAIL() << "test fail";
}
