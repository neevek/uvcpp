/*******************************************************************************
**          File: fs_event.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2020-04-12 Sun 03:22 PM
**   Description: a wrapper of uv_fs_event_t
*******************************************************************************/
#ifndef UVCPP_FS_EVENT_H_
#define UVCPP_FS_EVENT_H_
#include "handle.hpp"
#include <thread>

namespace uvcpp {
  struct EvFsEvent : public Event {
    enum class Event : int {
      kRename = 1,
      kChange = 2
    };

    EvFsEvent(const char *path, Event events, int status) :
      path(path ? path : ""), events(events), status(status) { }

    std::string path; // could be empty
    Event events;
    int status;
  };

  class FsEvent : public Handle<uv_fs_event_t, FsEvent> {
    friend class Resource;
    friend class Handle;

    public:
      enum class Flag : unsigned int {
        kWatchEntry = 1,
        kStat       = 2,
        kRecursive  = 4
      };

      friend FsEvent::Flag operator|(FsEvent::Flag lhs, FsEvent::Flag rhs) {
        typedef typename std::underlying_type<FsEvent::Flag>::type underlying;
        return static_cast<FsEvent::Flag>(
          static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
      }

    protected:
      FsEvent(const std::shared_ptr<Loop> &loop) : Handle(loop) {}

      virtual bool init() override {
        if (uv_fs_event_init(this->getLoop()->getRaw(), get()) != 0) {
          LOG_E("uv_fs_event_init failed");
          return false;
        }
        return true;
      }

    public:
      void start(const std::string &path, Flag flags) {
        if (path.empty()) {
          this->reportError("path is empty", 0);
          return;
        }

        int err;
        if ((err = uv_fs_event_start(
              static_cast<uv_fs_event_t *>(this->get()),
              onFsEventCallback, path.c_str(),
              static_cast<unsigned int>(flags))) != 0) {
          this->reportError("uv_fs_event_start", err);
        }
      }

      void stop() {
        int err;
        if ((err = uv_fs_event_stop(
              static_cast<uv_fs_event_t *>(this->get()))) != 0) {
          this->reportError("uv_fs_event_stop", err);
        }
      }

    private:
      static void onFsEventCallback(
        uv_fs_event_t *handle, const char *filename, int events, int status) {
        size_t size = 2048;
        char path[size];

        auto ret = uv_fs_event_getpath(handle, path, &size);
        if (ret == UV_ENOBUFS) {
          static_cast<FsEvent *>(handle->data)->reportError(
            "uv_fs_event_getpath failed, buffer size too small", ret);

        } else {
          static_cast<FsEvent *>(handle->data)->template
            publish<EvFsEvent>(EvFsEvent{
              path, static_cast<EvFsEvent::Event>(events), status
            });
        }
      }
  };

} /* end of namespace: uvcpp */

#endif /* end of include guard: UVCPP_FS_EVENT_H_ */
