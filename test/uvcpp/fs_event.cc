#include <gtest/gtest.h>
#include "uvcpp.h"
#include <fstream>

using namespace uvcpp;

TEST(FsEvent, TestFileChange) {
  auto destroyed = false;
  auto count = 0;
  {
    auto loop = std::make_shared<Loop>();
    ASSERT_TRUE(loop->init());

    auto fsEvent = FsEvent::create(loop);
    ASSERT_TRUE(!!fsEvent);

    fsEvent->once<EvError>([](const auto &e, auto &fsEvent) {
      FAIL() << "fsEvent failed with status: " << e.status << ", msg: " << e.message;
    });
    fsEvent->once<EvClose>([fe = fsEvent](const auto &e, auto &fsEvent) {
      LOG_D("fsEvent closed: %li", fe.use_count());
    });
    fsEvent->once<EvDestroy>([&destroyed, fe = &fsEvent](const auto &e, auto &fsEvent) {
      LOG_D("fsEvent destroyed: %li", fe->use_count());
      destroyed = true;
    });

    fsEvent->on<EvFsEvent>([&count](const auto &e, auto &fsEvent) {
      LOG_D("haha changed: %d", e.events);
      if (e.events == EvFsEvent::Event::kChange) {
        if (++count >= 2) {
          fsEvent.stop();
          fsEvent.close();
        }
      }
    });

    auto testFile = "./test_fs_event.txt";
    int fd = open(testFile, O_RDWR|O_CREAT, 0644);
    if (fd != -1) {
      close(fd);
    }

    fsEvent->start(testFile, FsEvent::Flag::kRecursive);

    LOG_D("will run thread");
    auto t = std::thread([&](){
      auto changeFile = [&]() {
        LOG_D("will change file: %s", testFile);
        std::ofstream os(testFile, std::ios::binary);
        os << "test";
        os.close();
      };
      changeFile();
      changeFile();
      unlink(testFile);
    });

    LOG_D("will run loop");

    loop->run();
    t.join();
  }

  ASSERT_TRUE(destroyed);
  ASSERT_TRUE(count == 2);
}
