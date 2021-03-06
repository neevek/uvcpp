/*******************************************************************************
**          File: timer.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-10 Tue 10:44 AM
**   Description: wraps uv_timer_t 
*******************************************************************************/
#ifndef UVCPP_TIMER_H_
#define UVCPP_TIMER_H_
#include "handle.hpp"

namespace uvcpp {
  struct EvTimer : public Event { };

  class Timer : public Handle<uv_timer_t, Timer> {
    friend class Resource;
    friend class Handle;

    protected:
      Timer(const std::shared_ptr<Loop> &loop) : Handle(loop) { }

      virtual bool init() override {
        if (uv_timer_init(this->getLoop()->getRaw(), get()) != 0) {
          LOG_E("uv_timer_init failed");
          return false;
        }
        return true;
      }

    public:
      void start(uint64_t timeout, uint64_t repeat) {
        int err;
        if ((err = uv_timer_start(
              reinterpret_cast<uv_timer_t *>(this->get()),
              onTimerCallback, timeout, repeat)) != 0) {
          this->reportError("uv_timer_start", err);
        }
      }

      void stop() {
        int err;
        if ((err = uv_timer_stop(
              reinterpret_cast<uv_timer_t *>(this->get()))) != 0) {
          this->reportError("uv_timer_stop", err);
        }
      }
    
    private:
      static void onTimerCallback(uv_timer_t *t) {
        reinterpret_cast<Timer *>(t->data)->template
          publish<EvTimer>(EvTimer{});
      }
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_TIMER_H_ */
