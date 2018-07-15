/*******************************************************************************
**          File: loop.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:18 PM
**   Description: wraps uv_loop_t 
*******************************************************************************/
#ifndef UVCPP_LOOP_H_
#define UVCPP_LOOP_H_
#include <uv.h>
#include <memory> 
#include <functional>

namespace uvcpp {

  class Loop final {
    public:
      static const Loop &get() {
        static Loop instance{};
        return instance;
      }

      void run() const {
        uv_run(loop_.get(), UV_RUN_DEFAULT);
      }

      uv_loop_t *getRaw() const {
        return loop_.get();
      }

      void stop() const {
        uv_stop(loop_.get());
      }

    private:
      Loop() {
        loop_.reset(uv_default_loop());
      }

      ~Loop() = default;
    
    private:
      // cannot use decytype(lambda) because a warning
      // ref: https://stackoverflow.com/a/40557850/668963
      using LoopDeleter = std::function<void(uv_loop_t *p)>;
      std::unique_ptr<uv_loop_t, LoopDeleter>
        loop_{nullptr, [](uv_loop_t *p) { uv_loop_delete(p); }};
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_LOOP_H_ */
