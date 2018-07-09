/*******************************************************************************
**          File: resource.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 07:30 PM
**   Description: base of uv_handle_t and uv_req_t
*******************************************************************************/
#ifndef UVCPP_RESOURCE_H_
#define UVCPP_RESOURCE_H_
#include <uv.h>
#include <memory>
#include <string>
#include <type_traits>
#include <functional>
#include "loop.hpp"
#include "log/log.h"

namespace uvcpp {
  template <typename T, typename Derived>
  class Resource {
    public:
      using Type = T;
      using ErrorCallback = std::function<void(int err)>;

      explicit Resource() {
        setData(this);
      }
      virtual ~Resource() = default;
      Resource &&operator=(const Resource &) = delete;
      Resource &&operator=(const Resource &&) = delete;

      Type *get() {
        return &resource_;
      }

      void setData(void *data) {
        resource_.data = data;
      }

      void onError(ErrorCallback callback) {
        errorCallback_ = callback; 
      }

      static auto create() {
        using ResourceType = 
          typename std::enable_if_t<
          std::is_base_of<Resource<typename Derived::Type, Derived>, Derived>::value, Derived>;
        return std::make_unique<ResourceType>();
      }

    protected:
      void reportError(const char *funName, int err) {
        LOG_E("%s failed: %s", funName, uv_strerror(err));
        if (errorCallback_) {
          errorCallback_(err);
        }
      }
    
    private:
      Type resource_;
      ErrorCallback errorCallback_{nullptr};
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_RESOURCE_H_ */
