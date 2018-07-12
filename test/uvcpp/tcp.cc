#include <gtest/gtest.h>
#include "uvcpp.h"
#include <uv.h>

using namespace uvcpp;

TEST(Tcp, Connection) {
  auto server = Tcp::create();
  auto client = Tcp::create();
  ASSERT_TRUE(!!server);
  ASSERT_TRUE(!!client);

  server->on<EvError>([](auto e, auto &tcp) {
    FAIL() << "server failed with status: " << e.status;
  });
  client->on<EvError>([](auto e, auto &tcp) {
    FAIL() << "client failed with status: " << e.status;
  });
  client->on<EvClose>([](auto e, auto &handle) {
    LOG_D("client closed!!!");
  });

  auto serverMsg = std::string{"greet from server!"};
  auto clientMsg = std::string{"greet from client!"};

  std::shared_ptr<Tcp> acceptedClient;

  server->on<EvBind>([&client](auto e, auto &server) {
    server.listen(50);
    client->connect("127.0.0.1", 9000);
  });

  server->on<EvAccept<Tcp>>([&](auto e, auto &server) {
    Buffer buf = {
      .data = (char *)serverMsg.c_str(), .len = serverMsg.size()
    };
    if (!e.client->writeAsync(buf)) {
      return;
    }

    e.client->template on<EvShutdown>([&](auto e, auto &client) {
      LOG_D("received client shutdown");
    });
    e.client->shutdown();

    e.client->readStart();
    e.client->template on<EvRead>([&](auto e, auto &client) {
      ((char *)e.buf)[e.nread] = '\0';
      ASSERT_STREQ(clientMsg.c_str(), e.buf);

      LOG_D("server received: %s", e.buf);

      acceptedClient->close();
    });
    acceptedClient = std::move(e.client);
  });

  const int EXPECTED_WRITE_COUNT = 1;
  int writeCount = 0;
  client->on<EvConnect>([&clientMsg, &writeCount](auto e, auto &client) {
    client.template once<EvWrite>([&](auto e, auto &client) {
      ++writeCount;
    });
    Buffer buf = {
      .data = (char *)clientMsg.c_str(), .len = clientMsg.size()
    };
    if (!client.writeAsync(buf)) {
      return;
    }

    client.template on<EvShutdown>([&](auto e, auto &client) {
      LOG_D("client shutdown");
    });
    client.shutdown();

    client.readStart();
  });

  client->on<EvRead>([&](auto e, auto &client){
    ((char *)e.buf)[e.nread] = '\0';
    ASSERT_STREQ(serverMsg.c_str(), e.buf);

    LOG_D("client received: %s", e.buf);

    client.close();
    server->close();
  });

  server->bind("0.0.0.0", 9000);

  Loop::get().run();

  ASSERT_EQ(writeCount, EXPECTED_WRITE_COUNT);
}
