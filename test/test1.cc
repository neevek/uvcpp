#include "loop.hpp"
#include "handle.hpp"
#include <string>
#include <exception>
#include <thread>
#include <chrono>

#include "log/log.h"

using namespace uvcpp;
using namespace std::chrono_literals;

int main(int argc, const char *argv[]) {
  try {
    std::shared_ptr<Tcp> c;
    auto server = Tcp::create<ServerTcp>();
    server->init();
    server->onAccept([&c](auto client){
      client->onRead([](const char *buf, ssize_t nread){
        ((char *)buf)[nread] = '\0';
        LOG_D(">>> read: %s", buf);
      });
      client->read();
      c = std::move(client);
    });

    server->bind("0.0.0.0", 9000, 50);
    Loop::get().run();

    //std::this_thread::sleep_for(10s);

  } catch (const std::exception &e) {
    printf("%s\n", e.what());
  }
  
  return 0;
}
