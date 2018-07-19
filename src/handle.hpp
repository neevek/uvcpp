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
      Handle(const std::shared_ptr<Loop> &loop) : Resource<T, Derived>(loop) { }

      void close() {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(this->get()))) {
          uv_close((uv_handle_t *)this->get(), closeCallback);
        }
      }

      bool isValid() {
        return uv_is_closing(reinterpret_cast<uv_handle_t *>(this->get())) == 0;
      }

      virtual bool init() {
        this->template once<EvError>([this](const auto &e, auto &handle){
          this->close();
        });
        return true;
      }

      template <typename U = Derived, typename ...Args>
      static auto createUnique(const std::shared_ptr<Loop> &loop, Args ...args) {
        auto handle = Resource<T, U>::template
          createUnique<U, Args...>(loop, std::forward<Args>(args)...);
        return handle->init() ? std::move(handle) : nullptr;
      }

      template <typename U = Derived, typename ...Args>
      static auto createShared(const std::shared_ptr<Loop> &loop, Args ...args) {
        auto handle = Resource<T, U>::template
          createShared<U, Args...>(loop, std::forward<Args>(args)...);
        return handle->init() ? handle : nullptr;
      }

    private:
      static void closeCallback(uv_handle_t *h) {
        reinterpret_cast<Handle *>(h->data)->template
          publish<EvClose>(EvClose{});
      }
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_HANDLE_H_ */
