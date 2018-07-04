#ifndef LOOP_H_
#define LOOP_H_
#include <uv.h>
#include <memory> 

namespace uvcpp {

  auto LoopDeleter = [](uv_loop_t *p) {
    uv_loop_delete(p);
  };

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

    private:
      Loop() {
        loop_.reset(uv_default_loop());
      }

      ~Loop() = default;
    
    private:
      std::unique_ptr<uv_loop_t, decltype(LoopDeleter)>
        loop_{nullptr, LoopDeleter};
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: LOOP_H_ */
