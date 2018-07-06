/*******************************************************************************
**          File: tcp.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:12 PM
**   Description: wraps uv_tcp_t 
*******************************************************************************/
#ifndef UVCPP_TCP_H_
#define UVCPP_TCP_H_
#include "handle.hpp"
#include "req.hpp"
#include "util.hpp"
#include "defs.h"

namespace uvcpp {
  class Tcp : public Stream<uv_tcp_t, Tcp> {
    public:
      using BindCallback = std::function<void(Tcp *)>;

      virtual bool init() override {
        if (uv_tcp_init(Loop::get().getRaw(), get()) != 0) {
          LOG_E("failed to init Tcp");
          return false;
        }
        return true;
      }

      void bind(const std::string &host, uint16_t port, BindCallback callback) {
        bindHost_ = host;
        bindPort_ = port;
        bindCallback_ = callback;
        getAddrReq_.setData(this);

        struct addrinfo hint;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_UNSPEC;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;

        int err;
        if ((err = uv_getaddrinfo(
              Loop::get().getRaw(),
              getAddrReq_.get(),
              getAddressInfoReqCallback,
              host.c_str(),
              nullptr, 
              &hint)) != 0) {
          this->reportError("uv_getaddrinfo", err);
        }
      }

      void setKeepAlive(bool keepAlive) {
        int err;
        if ((err = uv_tcp_keepalive(get(), keepAlive ? 1 : 0, 60)) != 0) {
          LOG_E("uv_tcp_keepalive failed: %s", uv_strerror(err));
        }
      }

      void setSockOption(int option, void *value, socklen_t optionLength) {
        uv_os_fd_t fd;
        if (uv_fileno(reinterpret_cast<uv_handle_t *>(get()),
              &fd) == UV_EBADF) {
          LOG_W("uv_fileno failed on server_tcp");
        } else {
          if (setsockopt(fd, SOL_SOCKET, option, value, optionLength) == -1) {
            LOG_W("setsockopt failed: %s", strerror(errno));
          }
        }
      }

    protected:
      virtual void doAccept(AcceptCallback<Tcp> acceptCallback) override {
        auto client = Tcp::create();
        if (!client->init()) {
          return;
        }

        int err;
        if ((err = uv_accept(reinterpret_cast<uv_stream_t *>(get()),
                reinterpret_cast<uv_stream_t *>(client->get()))) != 0) {
          LOG_E("uv_accept failed: %s", uv_strerror(err));
          return;
        }

        struct sockaddr_storage addr;
        int len = sizeof(addr);
        if ((err = uv_tcp_getpeername(client->get(),
                reinterpret_cast<sockaddr *>(&addr), &len)) != 0) {
          LOG_E("uv_tcp_getpeername failed: %s", uv_strerror(err));
          return;
        }

        client->bindHost_ = NetUtil::extractIPAddress(&addr);
        client->bindPort_ = reinterpret_cast<sockaddr_in *>(&addr)->sin_port;

        LOG_V("from client: %s:%d", client->bindHost_.c_str(), client->bindPort_);

        if (acceptCallback) {
          acceptCallback(std::move(client));
        }
      }

    private:
      bool doBind(IPAddr *addr, char *ipstr) {
        int err;
        if ((err = uv_tcp_bind(get(), addr, 0)) != 0) {
          LOG_W("uv_tcp_bind on %s:%d failed: %s", ipstr, bindPort_, uv_strerror(err));
          return false;
        }

        if (bindCallback_) {
          bindCallback_(this);
        }
        return true;
      }

      static void getAddressInfoReqCallback(
          uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
        auto tcp = reinterpret_cast<Tcp *>(req->data);
        if (status < 0) {
          LOG_E("getaddrinfo(\"%s\"): %s",
              tcp->bindHost_.c_str(), uv_strerror(status));
          uv_freeaddrinfo(res);
          return;
        }

        struct sockaddr_storage addr;
        char ipstr[INET6_ADDRSTRLEN];
        for (struct addrinfo *ai = res; ai != nullptr; ai = ai->ai_next) {
          if (NetUtil::fillIPAddress(reinterpret_cast<struct sockaddr *>(&addr),
                htons(tcp->bindPort_), ipstr, sizeof(ipstr), ai) != 0) {
            continue;
          }

          if (ai->ai_family == AF_INET) {
            tcp->ipVersion_ = IPVersion::IPV4;
          } else if (ai->ai_family == AF_INET6) {
            tcp->ipVersion_ = IPVersion::IPV6;
          } else {
            LOG_W("unexpected address family: %d", ai->ai_family);
            continue;
          }

          if (!tcp->doBind(reinterpret_cast<IPAddr *>(&addr), ipstr)) {
            continue;
          }

          LOG_I("server bound on%s:%d", ipstr, tcp->bindPort_);
          uv_freeaddrinfo(res);

          //if (!tcp->setuid_.empty()) {
            //NetUtil::doSetUID(tcp->setuid_.c_str());
          //}
          return;
        }

        LOG_E("failed to bind on port: %d", tcp->bindPort_);
      }

    private:
      GetAddressInfoReq getAddrReq_;
      std::string bindHost_;
      uint16_t bindPort_{0};
      IPVersion ipVersion_{IPVersion::IPV4};
      BindCallback bindCallback_{nullptr};

      std::string setuid_;
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_TCP_H_ */
