#include "uvcpp.h"
#include <string>
#include <exception>
#include <thread>
#include <chrono>

#include "log/log.h"

using namespace uvcpp;
using namespace std::chrono_literals;

void test1() {
  auto client = Tcp::create();
  if (client) {
    client->connect("0.0.0.0", 1080, [](auto c){
        LOG_D("connected");
        //Buffer buf = { .base = (char *)"SOMETHING", .len = 9 };
        //while (client->write(buf) < 0) {}
        });
  }
  Loop::get().run();
}

void test2() {
  std::shared_ptr<Tcp> c;
  auto server = Tcp::create();
  if (!server) {
    server->onError([](int status){
        LOG_E("error occured: %d", status);
        });
  }
  //server->onAccept([&c](auto client){
  //client->onRead([](const char *buf, ssize_t nread){
  //((char *)buf)[nread] = '\0';
  //LOG_D(">>> read: %s", buf);
  //});
  //client->read();
  //c = std::move(client);
  //});

  auto cc = Tcp::create();

  server->bind("0.0.0.0", 9000, [&](auto s){
      s->listen(50, [&c](auto client){
          Buffer buf = { .base = (char *)"hello world", .len = 11 };
          while (client->write(buf) < 0) {}

          client->readStart();
          client->onRead([](const char *buf, ssize_t nread){
              ((char *)buf)[nread] = '\0';
              LOG_D(">>> read: %s", buf);
              });
          c = std::move(client);
          });

      if (cc) {
        cc->connect("0.0.0.0", 9000, [](auto c){
          Buffer buf = { .base = (char *)"SOMETHING", .len = 9 };
          while (c->write(buf) < 0) {}
        });
      }
      });
  Loop::get().run();

  //std::this_thread::sleep_for(10s);
}

int main(int argc, const char *argv[]) {
  test2();
  return 0;
}
