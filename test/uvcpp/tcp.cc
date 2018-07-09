#include <gtest/gtest.h>
#include "uvcpp.h"

TEST(Tcp, Functionalities) {
  uvcpp::Loop::get().run();
  ASSERT_TRUE(false) << "loop is null";
}
