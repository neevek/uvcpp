#include <gtest/gtest.h>
#include "uvcpp.h"
#include <uv.h>

using namespace uvcpp;

TEST(Tcp, Connection) {
  auto server = Tcp::create();
  auto client = Tcp::create();
  ASSERT_TRUE(!!server);
  ASSERT_TRUE(!!client);

  server->on<EError>([](auto e, auto &tcp) {
    FAIL() << "server failed with status: " << e.status;
  });
  client->on<EError>([](auto e, auto &tcp) {
    FAIL() << "client failed with status: " << e.status;
  });
  client->on<EClose>([](auto e, auto &handle) {
    LOG_E("client closed!!!");
  });

  auto serverMsg = std::string{"greet from server!"};
  auto clientMsg = std::string{"greet from client!"};

  std::shared_ptr<Tcp> acceptedClient;

  server->on<EBind>([&client](auto e, auto &server) {
    server.listen(50);
    client->connect("127.0.0.1", 9000);
  });

  server->on<EAccept<Tcp>>([&](auto e, auto &server) {
    Buffer buf = {
      .base = (char *)serverMsg.c_str(), .len = serverMsg.size()
    };
    while (e.client->write(buf) < 0) { }

    e.client->readStart();
    e.client->template on<ERead>([&](auto e, auto &client) {
      ((char *)e.buf)[e.nread] = '\0';
      ASSERT_STREQ(clientMsg.c_str(), e.buf);

      LOG_D("server received: %s", e.buf);

      acceptedClient->close();
    });
    acceptedClient = std::move(e.client);
  });

  client->on<EConnect>([&clientMsg](auto e, auto &client) {
    Buffer buf = {
      .base = (char *)clientMsg.c_str(), .len = clientMsg.size()
    };
    while (client.write(buf) < 0) { }
    client.readStart();
  });

  client->on<ERead>([&](auto e, auto &client){
    ((char *)e.buf)[e.nread] = '\0';
    ASSERT_STREQ(serverMsg.c_str(), e.buf);

    LOG_D("client received: %s", e.buf);

    client.close();
    server->close();
  });

  server->bind("0.0.0.0", 9000);

  Loop::get().run();
}
