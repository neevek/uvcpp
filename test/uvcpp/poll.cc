#include <gtest/gtest.h>
#include "uvcpp.h"
#include <vector>
#include <sys/un.h>
//#include <netdb.h>

using namespace uvcpp;

TEST(Poll, UnixDomainSocket) {
  auto sockPath = "testsockpath";

  auto loop = std::make_shared<Loop>();
  ASSERT_TRUE(loop->init());

  auto serverPoll = Poll::createUnique(loop);
  int sSock = socket(AF_UNIX, SOCK_STREAM, 0);
  ASSERT_NE(sSock, -1);

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sockPath, sizeof(addr.sun_path) - 1);

  unlink(sockPath);

  ASSERT_NE(bind(sSock, (struct sockaddr*)&addr, sizeof(addr)), -1);
  ASSERT_NE(listen(sSock, 1), -1);

  std::vector<std::unique_ptr<Poll>> v;

  serverPoll->initWithSockHandle(sSock);
  serverPoll->on<EvPoll>([sSock, &v](const auto &e, auto &serverPoll){
    struct sockaddr_un c_addr;
    socklen_t len = sizeof(c_addr);
    int cSock = accept(sSock, (struct sockaddr *)&c_addr, &len);
    ASSERT_NE(cSock, -1);

    auto p = Poll::createUnique(serverPoll.getLoop());
    p->initWithSockHandle(cSock);
    p->template on<EvPoll>([&](const auto &e, auto &p){
      char buf[1024];
      read(cSock, buf, sizeof(buf));
      LOG_D("received from client: %s", buf);

      const char *msg = "hello from server!";
      write(cSock, msg, strlen(msg) + 1);

      p.stop();
      p.close();
      close(cSock);

      serverPoll.stop();
      serverPoll.close();
      close(sSock);
    });
    p->poll(Poll::Event::READABLE);
    v.push_back(std::move(p));

  });
  serverPoll->poll(Poll::Event::READABLE);



  // client poll
  auto clientPoll = Poll::createUnique(loop);
  int clientSock = socket(AF_UNIX, SOCK_STREAM, 0);
  ASSERT_NE(clientSock, -1);
  ASSERT_NE(connect(clientSock, (struct sockaddr*)&addr, sizeof(addr)), -1);
  unlink(sockPath);

  clientPoll->initWithSockHandle(clientSock);
  clientPoll->on<EvPoll>([clientSock, &v](const auto &e, auto &clientPoll){
    if (e.events & Poll::Event::WRITABLE) {

      const char *msg = "hello from client!";
      write(clientSock, msg, strlen(msg) + 1);
      clientPoll.poll(Poll::Event::READABLE);

    } else {
      char buf[1024];
      read(clientSock, buf, sizeof(buf));
      LOG_D("received from server: %s", buf);

      clientPoll.stop();
      clientPoll.close();
      close(clientSock);
    }
  });
  clientPoll->poll(
    static_cast<Poll::Event>(Poll::Event::READABLE | Poll::Event::WRITABLE));


  loop->run();
}
