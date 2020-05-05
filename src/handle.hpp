/*******************************************************************************
**          File: handle.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:18 PM
**   Description: wraps uv_handle_t
*******************************************************************************/
#ifndef UVCPP_HANDLE_H_
#define UVCPP_HANDLE_H_
#include "resource.hpp"
#include "nul/buffer.hpp"

namespace uvcpp {

  struct EvClose : public Event { };
  struct EvBufferRecycled : public Event {
    EvBufferRecycled(std::unique_ptr<nul::Buffer> &&buffer) :
      buffer(std::forward<std::unique_ptr<nul::Buffer>>(buffer)) { }
    std::unique_ptr<nul::Buffer> buffer;
  };

  template <typename T, typename Derived>
  class Handle : public Resource<T, Derived> {
    protected:
      Handle(const std::shared_ptr<Loop> &loop) : Resource<T, Derived>(loop) { }

      virtual bool init() {
        this->template once<EvError>([this](const auto &e, auto &handle) {
          this->close();
        });
        return true;
      }

    public:
      template <typename U = Derived, typename ...Args>
      static auto create(const std::shared_ptr<Loop> &loop, Args &&...args) {
        auto handle = Resource<T, U>::template
          create<U, Args...>(loop, std::forward<Args>(args)...);
        return handle->init() ? handle : nullptr;
      }

      void close() {
        auto handle = reinterpret_cast<uv_handle_t *>(this->get());
        if (uv_handle_get_type(handle) == UV_UNKNOWN_HANDLE) {
          LOG_W("handle not initialized");
          return;
        }
        if (!uv_is_closing(handle)) {
          uv_close(handle, closeCallback);
        }
      }

      bool isValid() {
        return uv_is_closing(reinterpret_cast<uv_handle_t *>(this->get())) == 0;
      }

      template<typename E>
      void on(EventCallback<E, Derived> &&callback) {
        static_assert(!std::is_same<E, EvClose>::value,
                      "EvClose is not allowed to be registered with 'on'");

        Resource<T, Derived>::template on<E>(
          std::forward<EventCallback<E, Derived>>(callback));
      }

    private:
      static void closeCallback(uv_handle_t *h) {
        reinterpret_cast<Handle *>(h->data)->template
          publish<EvClose>(EvClose{});
      }
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_HANDLE_H_ */
