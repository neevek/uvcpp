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
#include "buffer.h"

namespace uvcpp {
  struct EvWork : public Event { };

  struct EvAfterWork : public Event { };

  struct EvDNSResult : public Event {
    using DNSResultVector = std::vector<std::unique_ptr<
      SockAddrStorage, CPointerDeleterType>>;
    EvDNSResult(DNSResultVector &&dnsResults) :
      dnsResults(std::move(dnsResults)) { }
    DNSResultVector dnsResults;
  };

  template <typename T, typename Derived>
  class Req : public Resource<T, Derived> {
    public:
      Req(Loop &loop) : Resource<T, Derived>(loop) { }
      void cancel() {
        int err;
        if ((err = uv_cancel(reinterpret_cast<uv_req_t *>(this->get()))) < 0) {
          this->reportError("uv_cancel", err);
        }
      }
  };

  class WriteReq : public Req<uv_write_t, WriteReq> {
    public:
      WriteReq(Loop &loop, std::unique_ptr<Buffer> buffer) :
        Req(loop), buffer(std::move(buffer)) { }
      std::unique_ptr<Buffer> buffer;
  };

  class ConnectReq : public Req<uv_connect_t, ConnectReq> {
    public:
      ConnectReq(Loop &loop) : Req(loop) { }
  };

  class ShutdownReq : public Req<uv_shutdown_t, ShutdownReq> {
    public:
      ShutdownReq(Loop &loop) : Req(loop) { }
  };

  class Work : public Req<uv_work_t, Work> {
    public:
      Work(Loop &loop) : Req(loop) { }

      void start() {
        int err;
        if ((err = uv_queue_work(
              this->getLoop().getRaw(),
              reinterpret_cast<uv_work_t *>(this->get()),
              onWorkCallback, onAfterWorkCallback)) != 0) {
          this->reportError("uv_queue_work", err);
        }
      }
    
    private:
      static void onWorkCallback(uv_work_t *w) {
        reinterpret_cast<Work *>(w->data)->template
          publish<EvWork>(EvWork{});
      }

      static void onAfterWorkCallback(uv_work_t *w, int status) {
        reinterpret_cast<Work *>(w->data)->template
          publish<EvAfterWork>(EvAfterWork{});
      }
  };

  class DNSRequest : public Req<uv_getaddrinfo_t, DNSRequest> {
    public:
      DNSRequest(Loop &loop) : Req(loop) { }

      void resolve(
          const std::string &host,
          bool ipv4Only = false) {
        host_ = host;

        struct addrinfo hint;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = ipv4Only ? AF_INET : AF_UNSPEC;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;

        int err;
        if ((err = uv_getaddrinfo(
              getLoop().getRaw(),
              this->get(),
              onResolveCallback,
              host.c_str(),
              nullptr,
              &hint)) != 0) {
          this->reportError("uv_getaddrinfo", err);
        }
      }

    private:
      static void onResolveCallback(
          uv_getaddrinfo_t *req, int status, struct addrinfo* res) {
        auto dnsReq = reinterpret_cast<DNSRequest *>(req->data);
        if (status < 0) {
          LOG_E("getaddrinfo(\"%s\"): %s", dnsReq->host_.c_str(), uv_strerror(status));
          uv_freeaddrinfo(res);
          return;
        }

        auto addrVec = EvDNSResult::DNSResultVector{};
        for (auto ai = res; ai != nullptr; ai = ai->ai_next) {
          auto addr = Util::makeCStructUniquePtr<SockAddrStorage>();
          NetUtil::copyIPAddress(addr.get(), ai);
          addrVec.push_back(std::move(addr));
        }

        uv_freeaddrinfo(res);
        dnsReq->template publish<EvDNSResult>(EvDNSResult{ std::move(addrVec) });
      }

    private:
      std::string host_;
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_REQ_H_ */
