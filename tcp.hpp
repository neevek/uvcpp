/*******************************************************************************
**          File: tcp.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:12 PM
**   Description: wraps uv_tcp_t 
*******************************************************************************/
#ifndef UVCPP_TCP_H_
#define UVCPP_TCP_H_
#include "stream.hpp"
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

        dnsReq_.resolve(host, [this](auto vec) {
          for (auto &addr : *vec) {
            auto sa = reinterpret_cast<SockAddr *>(addr.get());
            if (sa->sa_family == AF_INET) {
              this->ipVersion_ = IPVersion::IPV4;
              reinterpret_cast<SockAddr4 *>(sa)->sin_port = htons(this->bindPort_);

            } else if (sa->sa_family == AF_INET6) {
              this->ipVersion_ = IPVersion::IPV6;
              reinterpret_cast<SockAddr6 *>(sa)->sin6_port = htons(this->bindPort_);

            } else {
              LOG_W("unexpected address family: %d", sa->sa_family);
              continue;
            }

            int err;
            if ((err = uv_tcp_bind(this->get(), sa, 0)) != 0) {
              LOG_W("cannot bind on %s:%d, reason: %s",
                  NetUtil::extractIPAddress(addr.get()).c_str(),
                  this->bindPort_, uv_strerror(err));
              continue;
            }

          //if (!tcp->setuid_.empty()) {
            //NetUtil::doSetUID(tcp->setuid_.c_str());
          //}

            LOG_I("server binds on %s:%d",
                NetUtil::extractIPAddress(addr.get()).c_str(), this->bindPort_);
            if (this->bindCallback_) {
              this->bindCallback_(this);
            }
            return;
          }

          LOG_I("failed to bind on %s:%d",
              this->bindHost_.c_str(), this->bindPort_);
        });
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
      DNSRequest dnsReq_;
      std::string bindHost_;
      uint16_t bindPort_{0};
      IPVersion ipVersion_{IPVersion::IPV4};
      BindCallback bindCallback_{nullptr};

      std::string setuid_;
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_TCP_H_ */
