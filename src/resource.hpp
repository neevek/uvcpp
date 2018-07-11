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
#include <vector>
#include "loop.hpp"
#include "log/log.h"

namespace uvcpp {

  struct Event { };
  struct EvError : public Event {
    EvError(int status) : status(status) { }
    int status;
  };

  template <typename E, typename Derived>
  using EventCallback = std::function<void(E event, Derived &handle)>;

  struct CallbackInterface { };

  template <typename E, typename Derived>
  struct Callback : public CallbackInterface {
    Callback(EventCallback<E, Derived> callback) : callback(callback) { }
    EventCallback<E, Derived> callback;
  };

  template <typename T, typename Derived>
  class Resource {
    public:
      using Type = T;

      explicit Resource() {
        resource_.data = this;
      }
      virtual ~Resource() = default;
      Resource &&operator=(const Resource &) = delete;
      Resource &&operator=(const Resource &&) = delete;

      Type *get() {
        return &resource_;
      }

      template<typename E, typename = std::enable_if_t<std::is_base_of<Event, E>::value, E>>
      void on(EventCallback<E, Derived> &&callback) {
        auto index = getEventTypeIndex<E>();
        if (index >= callbacks_.size()) {
          callbacks_.resize(index + 1);
        }
        callbacks_[index] = std::make_unique<Callback<E, Derived>>(
            std::forward<EventCallback<E, Derived>>(callback));
      }

      template<typename E, typename = std::enable_if_t<std::is_base_of<Event, E>::value, E>>
      void publish(E &&event) {
        auto index = getEventTypeIndex<E>();
        if (index < callbacks_.size()) {
          auto &callback = callbacks_[index];
          if (callback) {
            static_cast<Callback<E, Derived> *>(callback.get())
              ->callback(std::forward<E>(event), static_cast<Derived &>(*this));
          }
        }
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
        publish<EvError>(EvError{ err });
      }

    private:
      static std::size_t countEventTypeIndex() {
        static std::size_t index = 0;
        return index++;
      }

      template <typename>
      static std::size_t getEventTypeIndex() {
        static std::size_t index = countEventTypeIndex();
        return index;
      }
    
    private:
      Type resource_;
      std::vector<std::unique_ptr<CallbackInterface>> callbacks_{};
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_RESOURCE_H_ */
