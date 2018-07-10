/*******************************************************************************
**          File: handle.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:18 PM
**   Description: wraps uv_handle_t
*******************************************************************************/
#ifndef UVCPP_HANDLE_H_
#define UVCPP_HANDLE_H_
#include "resource.hpp"

namespace uvcpp {

  struct EvClose : public Event { };

  template <typename T, typename Derived>
  class Handle : public Resource<T, Derived> {
    public:
      void close() {
        this->setData(this);
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(this->get()))) {
          uv_close((uv_handle_t *)this->get(), closeCallback);
        }
      }

      static auto create() {
        using HandleType = 
          typename std::enable_if_t<
          std::is_base_of<Handle<typename Derived::Type, Derived>, Derived>::value, Derived>;
        auto handle = std::make_unique<HandleType>();
        return handle->init() ? std::move(handle) : nullptr;
      }

      virtual bool init() = 0;

    private:
      static void closeCallback(uv_handle_t *h) {
        reinterpret_cast<Handle *>(h->data)->template
          publish<EvClose>(EvClose{});
      }
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_HANDLE_H_ */
