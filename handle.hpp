/*******************************************************************************
**          File: handle.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:18 PM
**   Description: wraps uv_handle_t
*******************************************************************************/
#ifndef UVCPP_HANDLE_H_
#define UVCPP_HANDLE_H_
#include <stdexcept>
#include <string>
#include <type_traits>
#include <functional>
#include "loop.hpp"
#include "util.hpp"
#include "log/log.h"

namespace uvcpp {
  template <typename T, typename Derived>
  class Handle {
    public:
      using Type = T;
      using ErrorCallback = std::function<void(int err)>;

      explicit Handle() {
        handle_.data = this;
      }
      virtual ~Handle() = default;
      Handle &&operator=(const Handle &) = delete;
      Handle &&operator=(const Handle &&) = delete;

      virtual bool init() = 0;

      void close() {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(&handle_))) {
          uv_close((uv_handle_t *)&handle_, closeCallback);
        }
      }

      Type *get() {
        return &handle_;
      }

      void onError(ErrorCallback callback) {
        errorCallback_ = callback; 
      }

      static auto create() {
        using HandleType = 
          typename std::enable_if_t<
          std::is_base_of<Handle<typename Derived::Type, Derived>, Derived>::value, Derived>;
        return std::make_unique<HandleType>();
      }

    protected:
      void reportError(const char *funName, int err) {
        LOG_E("%s failed: %s", funName, uv_strerror(err));
        if (errorCallback_) {
          errorCallback_(err);
        }
      }

    private:
      static void closeCallback(uv_handle_t *handle) {
        LOG_D("close");
      }
    
    private:
      Type handle_;
      ErrorCallback errorCallback_{nullptr};
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_HANDLE_H_ */
