/*******************************************************************************
**          File: req.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:20 PM
**   Description: wraps uv_req_t 
*******************************************************************************/
#ifndef UVCPP_REQ_H_
#define UVCPP_REQ_H_
#include <uv.h>

namespace uvcpp {

  template <typename T>
  class Req {
    public:
      using Type = T;

      Req() = default;
      virtual ~Req() = default;
      Req &&operator=(const Req &) = delete;
      Req &&operator=(const Req &&) = delete;

      Type *get() {
        return &req_;
      }

      void setData(void *data) {
        req_.data = data;
      }

    private:
      Type req_;
  };

  class GetAddressInfoReq : public Req<uv_getaddrinfo_t> { };
  class WriteReq : public Req<uv_write_t> { };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_REQ_H_ */
