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
      using ConnectCallback = std::function<void(Tcp *)>;

      virtual bool init() override {
        if (uv_tcp_init(Loop::get().getRaw(), get()) != 0) {
          LOG_E("failed to init Tcp");
          return false;
        }
        return true;
      }

      bool bind(SockAddr *sa, BindCallback callback) {
        if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6) {
          //// the port starts from sa_data
          //*reinterpret_cast<uint16_t *>(sa->sa_data) = htons(port);

          memcpy(&sds_, sa, sa->sa_family == AF_INET ?
              sizeof(SockAddr4) : sizeof(SockAddr6));

          int err;
          if ((err = uv_tcp_bind(get(), sa, 0)) != 0) {
            LOG_W("cannot bind on %s:%d, reason: %s",
                getIP().c_str(), getPort(), uv_strerror(err));
            return false;
          }

          LOG_D("server binds on: %s:%d", getIP().c_str(), getPort());
          if (bindCallback_) {
            bindCallback_(this);
          }

          if (callback) {
            bindCallback_ = callback;
          }

          //if (!tcp->setuid_.empty()) {
            //NetUtil::doSetUID(tcp->setuid_.c_str());
          //}
          return true;
        }

        return false;
      }

      void bind(const std::string &host, uint16_t port, BindCallback callback) {
        bindCallback_ = callback;

        SockAddrStorage sas;
        if (NetUtil::convertIPAddress(host, port, &sas)) {
          LOG_D("host is IP address: %s", host.c_str());
          bind(reinterpret_cast<SockAddr *>(&sas), callback);
          return;
        }

        if (!dnsReq_) {
          dnsReq_ = std::make_unique<DNSRequest>();
        }
        dnsReq_->resolve(host, [this, host, port](auto vec) {
          for (auto &addr : *vec) {
            if (!this->bind(reinterpret_cast<SockAddr *>(addr.get()), nullptr)) {
              continue;
            }
            return;
          }
          LOG_I("failed to bind on %s:%d", host.c_str(), port);
        });
      }

      bool connect(SockAddr *sa, ConnectCallback callback) {
        connectCallback_ = callback;

        if (!connectReq_) {
          connectReq_ = std::make_unique<ConnectReq>();
        }

        //// the port starts from sa_data
        //*reinterpret_cast<uint16_t *>(sa->sa_data) = htons(port);
        connectReq_->setData(this);

        int err;
        if ((err = uv_tcp_connect(connectReq_->get(), get(), sa, onConnect)) != 0) {
          this->reportError("uv_tcp_connect", err);
          return false;
        }
        LOG_E(">>>>>>>>>>>> handletype: %d", get()->type);
        return true;
      }

      void connect(const std::string &host, uint16_t port, ConnectCallback callback) {
        SockAddrStorage sas;
        if (NetUtil::convertIPAddress(host, port, &sas)) {
          LOG_D("host is IP address: %s", host.c_str());
          connect(reinterpret_cast<SockAddr *>(&sas), callback);

        } else {
          LOG_E("failed to convert IP address: %s", host.c_str());
        }
      }

      void setKeepAlive(bool enable) {
        int err;
        if ((err = uv_tcp_keepalive(get(), enable ? 1 : 0, 60)) != 0) {
          this->reportError("uv_tcp_keepalive", err);
        }
      }

      void setNoDelay(bool enable) {
        int err;
        if ((err = uv_tcp_nodelay(get(), enable ? 1 : 0)) != 0) {
          this->reportError("uv_tcp_nodelay", err);
        }
      }

      void setSockOption(int option, void *value, socklen_t optionLength) {
        uv_os_fd_t fd;
        if (uv_fileno(reinterpret_cast<uv_handle_t *>(get()),
              &fd) == UV_EBADF) {
          LOG_W("uv_fileno failed on server_tcp");
        } else {
          int err;
          if ((err = setsockopt(fd, SOL_SOCKET, option, value, optionLength)) == -1) {
            this->reportError("setsockopt", err);
          }
        }
      }

      std::string getIP() {
        return NetUtil::ip(reinterpret_cast<SockAddr *>(&sds_));
      }

      uint16_t getPort() {
        return NetUtil::port(reinterpret_cast<SockAddr *>(&sds_));
      }

    protected:
      virtual void doAccept(AcceptCallback<Tcp> acceptCallback) override {
        auto client = Tcp::create();
        if (!client) {
          return;
        }

        int err;
        if ((err = uv_accept(reinterpret_cast<uv_stream_t *>(get()),
                reinterpret_cast<uv_stream_t *>(client->get()))) != 0) {
          LOG_E("uv_accept failed: %s", uv_strerror(err));
          return;
        }

        int len = sizeof(client->sds_);
        if ((err = uv_tcp_getpeername(client->get(),
                reinterpret_cast<SockAddr *>(&client->sds_), &len)) != 0) {
          LOG_E("uv_tcp_getpeername failed: %s", uv_strerror(err));
          return;
        }

        LOG_V("from client: %s:%d", client->getIP().c_str(), client->getPort());
        if (acceptCallback) {
          acceptCallback(std::move(client));
        }
      }

    private:
      static void onConnect(uv_connect_t *req, int status) {
        auto tcp = reinterpret_cast<Tcp *>(req->data);
        if (status < 0) {
          tcp->reportError("uv_tcp_connect", status);
          return;
        }

        if (tcp->connectCallback_) {
          tcp->connectCallback_(tcp);
        }
      }

    private:
      std::unique_ptr<DNSRequest> dnsReq_{nullptr};
      std::unique_ptr<ConnectReq> connectReq_{nullptr};
      BindCallback bindCallback_{nullptr};
      ConnectCallback connectCallback_{nullptr};
      SockAddrStorage sds_;

      std::string setuid_;
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_TCP_H_ */
