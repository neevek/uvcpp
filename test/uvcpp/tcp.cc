#include <gtest/gtest.h>
#include "uvcpp.h"
#include <uv.h>

using namespace uvcpp;

TEST(Tcp, Connection) {
  auto server = Tcp::create();
  auto client = Tcp::create();
  ASSERT_TRUE(!!server);
  ASSERT_TRUE(!!client);

  server->onError([](int status){
    FAIL() << "server failed with status: " << status;
  });
  server->onClose([](auto &handle) {
    LOG_D("server handle closed");
  });
  client->onError([](int status){
    FAIL() << "client failed with status: " << status;
  });
  client->onClose([](auto &handle) {
    LOG_D("client handle closed");
  });

  auto serverMsg = std::string{"greet from server!"};
  auto clientMsg = std::string{"greet from client!"};

  std::shared_ptr<Tcp> acceptedClient;

  server->onBind([&client](auto &server) {
    server.listen(50);
    client->connect("127.0.0.1", 9000);
  });

  server->onAccept([&](auto c) {
    Buffer buf = {
      .base = (char *)serverMsg.c_str(), .len = serverMsg.size()
    };
    while (c->write(buf) < 0) { }

    c->readStart();
    c->onRead([&](const char *buf, ssize_t nread){
      ((char *)buf)[nread] = '\0';
      ASSERT_STREQ(clientMsg.c_str(), buf);

      acceptedClient->close();
    });
    acceptedClient = std::move(c);
  });

  client->onConnect([&clientMsg](auto &client) {
    Buffer buf = {
      .base = (char *)clientMsg.c_str(), .len = clientMsg.size()
    };
    while (client.write(buf) < 0) { }

    client.readStart();
  });

  client->onRead([&](const char *buf, ssize_t nread){
    ((char *)buf)[nread] = '\0';
    ASSERT_STREQ(serverMsg.c_str(), buf);

    client->close();
    server->close();
  });

  server->bind("0.0.0.0", 9000);

  Loop::get().run();
}
