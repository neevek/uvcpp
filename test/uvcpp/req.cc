#include <gtest/gtest.h>
#include "uvcpp.h"

using namespace uvcpp;

static std::string sasToIP(SockAddrStorage *sas) {
  return NetUtil::ip(reinterpret_cast<SockAddr *>(sas));
}

TEST(Req, DNSRequestResolveLocalHost) {
  DNSRequest req{};
  req.onError([](int status) {
    FAIL() << "failed with status: " << status;
  });

  req.resolve("localhost", [](auto vec) {
    ASSERT_GT(vec->size(), 0);
    for (auto &sas : *vec) {
      auto result = "::1" == sasToIP(sas.get()) ||
        "127.0.0.1" == sasToIP(sas.get());
      ASSERT_TRUE(result);
    }
  });

  Loop::get().run();
}

TEST(Req, DNSRequest0000) {
  DNSRequest req{};
  req.onError([](int status) {
    FAIL() << "failed with status: " << status;
  });

  req.resolve("0.0.0.0", [](auto vec) {
    ASSERT_EQ(vec->size(), 1);
    ASSERT_STREQ("0.0.0.0", sasToIP(vec->at(0).get()).c_str());
  });

  Loop::get().run();
}
