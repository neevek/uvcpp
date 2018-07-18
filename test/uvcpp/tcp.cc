#include <gtest/gtest.h>
#include "uvcpp.h"
#include <uv.h>

using namespace uvcpp;

TEST(Tcp, Connection) {
  Loop loop;
  ASSERT_TRUE(loop.init());

  auto server = Tcp::createUnique(loop);
  auto client = Tcp::createUnique(loop);
  ASSERT_TRUE(!!server);
  ASSERT_TRUE(!!client);

  server->on<EvError>([](const auto &e, auto &tcp) {
    FAIL() << "server failed with status: " << e.status;
  });
  client->on<EvError>([](const auto &e, auto &tcp) {
    FAIL() << "client failed with status: " << e.status;
  });
  client->on<EvClose>([](const auto &e, auto &handle) {
    LOG_D("client closed: isValid=%d", handle.isValid());
  });

  auto serverMsg = std::string{"greet from server!"};
  auto clientMsg = std::string{"greet from client!"};

  std::shared_ptr<Tcp> acceptedClient;

  server->on<EvBind>([&client](const auto &e, auto &server) {
    server.listen(50);
    client->connect("127.0.0.1", 9000);
  });

  server->on<EvAccept<Tcp>>([&](const auto &e, auto &server) {
    auto buf = std::make_unique<Buffer>(serverMsg.size() + 1);
    buf->assign(serverMsg.c_str(), serverMsg.size());
    if (!e.client->writeAsync(std::move(buf))) {
      return;
    }

    e.client->template on<EvShutdown>([&](const auto &e, auto &client) {
      LOG_D("received client shutdown");
    });
    e.client->shutdown();

    e.client->readStart();
    e.client->template on<EvRead>([&](const auto &e, auto &client) {
      ((char *)e.buf)[e.nread] = '\0';
      ASSERT_STREQ(clientMsg.c_str(), e.buf);

      LOG_D("server received: %s", e.buf);

      acceptedClient->close();
    });
    acceptedClient = std::move(const_cast<EvAccept<Tcp> &>(e).client);
  });

  const int EXPECTED_WRITE_COUNT = 1;
  int writeCount = 0;
  client->on<EvConnect>([&clientMsg, &writeCount](const auto &e, auto &client) {
    client.template on<EvBufferRecycled>([&](const auto &e, auto &client) {
      LOG_D("buffer recycled: %p", e.buffer.get());
    });
    client.template once<EvWrite>([&](const auto &e, auto &client) {
      ++writeCount;
    });

    auto buf = std::make_unique<Buffer>(clientMsg.size() + 1);
    buf->assign(clientMsg.c_str(), clientMsg.size());
    if (!client.writeAsync(std::move(buf))) {
      return;
    }

    client.template on<EvShutdown>([&](const auto &e, auto &client) {
      LOG_D("client shutdown");
    });
    client.shutdown();

    client.readStart();
  });

  client->on<EvRead>([&](const auto &e, auto &client){
    ((char *)e.buf)[e.nread] = '\0';
    ASSERT_STREQ(serverMsg.c_str(), e.buf);

    LOG_D("client received: %s", e.buf);

    client.close();
    server->close();
  });

  server->bind("0.0.0.0", 9000);

  loop.run();

  ASSERT_EQ(writeCount, EXPECTED_WRITE_COUNT);
}

TEST(Tcp, TestFail) {
  Loop loop;
  ASSERT_TRUE(loop.init());

  auto client = Tcp::createUnique(loop);
  ASSERT_TRUE(!!client);

  client->once<EvConnect>([](const auto &e, auto &client) {
    // will no reach here
    FAIL();
  });

  client->on<EvRead>([&](const auto &e, auto &client){
    // will no reach here
    FAIL();
  });

  client->on<EvError>([&](const auto &e, auto &client){
    client.close();
  });

  client->connect("nonexist", 1234);

  loop.run();
}
