/*******************************************************************************
**          File: prepare.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-10 Tue 11:09 AM
**   Description: wraps uv_prepare_t
*******************************************************************************/
#ifndef UVCPP_PREPARE_H_
#define UVCPP_PREPARE_H_
#include "handle.hpp"

namespace uvcpp {
  struct EvPrepare : public Event { };

  class Prepare : public Handle<uv_prepare_t, Prepare> {
    public:
      Prepare(Loop &loop) : Handle(loop) { }

      virtual bool init() override {
        if (uv_prepare_init(this->getLoop().getRaw(), get()) != 0) {
          LOG_E("uv_prepare_init failed");
          return false;
        }
        return true;
      }

      void start() {
        int err;
        if ((err = uv_prepare_start(
                reinterpret_cast<uv_prepare_t *>(this->get()),
                onPrepareCallback)) != 0) {
          this->reportError("uv_prepare_start", err);
        }
      }

      void stop() {
        int err;
        if ((err = uv_prepare_stop(
                reinterpret_cast<uv_prepare_t *>(this->get()))) != 0) {
          this->reportError("uv_prepare_stop", err);
        }
      }
    
    private:
      static void onPrepareCallback(uv_prepare_t *t) {
        reinterpret_cast<Prepare *>(t->data)->template
          publish<EvPrepare>(EvPrepare{});
      }
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_PREPARE_H_ */
