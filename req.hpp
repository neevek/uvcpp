/*******************************************************************************
**          File: req.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:20 PM
**   Description: wraps uv_req_t 
*******************************************************************************/
#ifndef UVCPP_REQ_H_
#define UVCPP_REQ_H_
#include <uv.h>
#include "resource.hpp"

namespace uvcpp {

  template <typename T, typename Derived>
  class Req : public Resource<T, Derived> { };

  class GetAddressInfoReq : public Req<uv_getaddrinfo_t, GetAddressInfoReq> { };

  class WriteReq : public Req<uv_write_t, WriteReq> { };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_REQ_H_ */
