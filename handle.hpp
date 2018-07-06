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
  template <typename T, typename Derived>
  class Handle : public Resource<T, Derived> {
    public:
      virtual bool init() = 0;

      void close() {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(this->get()))) {
          uv_close((uv_handle_t *)this->get(), closeCallback);
        }
      }

    private:
      static void closeCallback(uv_handle_t *handle) {
        LOG_D("close");
      }
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_HANDLE_H_ */
