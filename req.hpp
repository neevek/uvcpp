/*******************************************************************************
**          File: req.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:20 PM
**   Description: wraps uv_req_t 
*******************************************************************************/
#ifndef UVCPP_REQ_H_
#define UVCPP_REQ_H_
#include <uv.h>
#include <functional>
#include <vector>
#include "resource.hpp"
#include "util.hpp"
#include "defs.h"

namespace uvcpp {

  template <typename T, typename Derived>
  class Req : public Resource<T, Derived> { };

  class WriteReq : public Req<uv_write_t, WriteReq> { };

  class DNSRequest : public Req<uv_getaddrinfo_t, DNSRequest> {
    public:
      using DNSResultVector = std::vector<std::unique_ptr<
        SockAddrStorage, decltype(CPointerDeleter)>>;
      using DNSRequestCallback = std::function<void(std::unique_ptr<DNSResultVector>)>;

      void resolve(const std::string &host, DNSRequestCallback callback) {
        host_ = host;
        callback_ = callback;

        struct addrinfo hint;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_UNSPEC;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;

        int err;
        if ((err = uv_getaddrinfo(
              Loop::get().getRaw(),
              get(),
              resolveCallback,
              host.c_str(),
              nullptr,
              &hint)) != 0) {
          this->reportError("uv_getaddrinfo", err);
        }
      }

    private:
      static void resolveCallback(
          uv_getaddrinfo_t *req, int status, struct addrinfo* res) {
        auto dnsReq = reinterpret_cast<DNSRequest *>(req->data);
        if (status < 0) {
          LOG_E("getaddrinfo(\"%s\"): %s", dnsReq->host_.c_str(), uv_strerror(status));
          uv_freeaddrinfo(res);
          return;
        }

        auto addrVec = std::make_unique<DNSResultVector>();
        for (struct addrinfo *ai = res; ai != nullptr; ai = ai->ai_next) {
          auto addr = Util::makeCStructUniquePtr<SockAddrStorage>();
          NetUtil::copyIPAddress(addr.get(), ai);
          addrVec->emplace_back(std::move(addr));
        }

        uv_freeaddrinfo(res);
        dnsReq->callback_(std::move(addrVec));
      }

    private:
      std::string host_;
      DNSRequestCallback callback_{nullptr};
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_REQ_H_ */
