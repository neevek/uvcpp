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
      bool init() {
        return uv_loop_init(&loop_) == 0;
      }

      uv_loop_t *getRaw() {
        return &loop_;
      }

      void run() {
        uv_run(&loop_, UV_RUN_DEFAULT);
      }

      void close() {
        uv_stop(&loop_);
        uv_loop_close(&loop_);
      }
    
    private:
      uv_loop_t loop_;
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_LOOP_H_ */
