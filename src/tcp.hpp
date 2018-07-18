/*******************************************************************************
**          File: tcp.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:12 PM
**   Description: wraps uv_tcp_t 
*******************************************************************************/
#ifndef UVCPP_TCP_H_
#define UVCPP_TCP_H_
#include "stream.hpp"

namespace uvcpp {
  struct EvBind : public Event { };
  struct EvConnect : public Event { };

  class Tcp : public Stream<uv_tcp_t, Tcp> {
    public:
      Tcp(Loop &loop) : Stream(loop) { }

      virtual bool init() override {
        if (!Stream::init() || uv_tcp_init(this->getLoop().getRaw(), get()) != 0) {
          LOG_E("failed to init Tcp");
          return false;
        }
        return true;
      }

      bool bind(SockAddr *sa) {
        if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6) {
          //// the port starts from sa_data
          //*reinterpret_cast<uint16_t *>(sa->sa_data) = htons(port);

          memcpy(&sas_, sa, sa->sa_family == AF_INET ?
              sizeof(SockAddr4) : sizeof(SockAddr6));

          int err;
          if ((err = uv_tcp_bind(get(), sa, 0)) != 0) {
            LOG_W("cannot bind on %s:%d, reason: %s",
                getIP().c_str(), getPort(), uv_strerror(err));
            return false;
          }

          LOG_D("server bound on: %s:%d", getIP().c_str(), getPort());
          publish<EvBind>(EvBind{});

          return true;
        }

        return false;
      }

      void bind(const std::string &host, uint16_t port) {
        SockAddrStorage sas;
        if (NetUtil::convertIPAddress(host, port, &sas)) {
          LOG_D("binding on ip address: %s", host.c_str());
          bind(reinterpret_cast<SockAddr *>(&sas));
          return;
        }

        if (!dnsReq_) {
          dnsReq_ = DNSRequest::createUnique(this->getLoop());
        }

        dnsReq_->once<EvDNSResult>([this, host, port](const auto &e, auto &tcp){
          for (auto &addr : e.dnsResults) {
            if (!this->bind(reinterpret_cast<SockAddr *>(addr.get()))) {
              continue;
            }
            return;
          }
          LOG_E("failed to bind on %s:%d", host.c_str(), port);
        });

        dnsReq_->resolve(host);
      }

      bool connect(SockAddr *sa) {
        if (!connectReq_) {
          connectReq_ = ConnectReq::createUnique(this->getLoop());
        }

        //// the port starts from sa_data
        //*reinterpret_cast<uint16_t *>(sa->sa_data) = htons(port);
        //connectReq_->setData(this);

        int err;
        if ((err = uv_tcp_connect(connectReq_->get(), get(), sa, onConnect)) != 0) {
          this->reportError("uv_tcp_connect", err);
          return false;
        }
        return true;
      }

      void connect(const std::string &host, uint16_t port) {
        if (NetUtil::convertIPAddress(host, port, &sas_)) {
          LOG_D("connecting to ip address: %s", host.c_str());
          connect(reinterpret_cast<SockAddr *>(&sas_));

        } else {
          LOG_E("failed to convert ip address: %s", host.c_str());
          close();
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
        return NetUtil::ip(reinterpret_cast<SockAddr *>(&sas_));
      }

      uint16_t getPort() {
        return NetUtil::port(reinterpret_cast<SockAddr *>(&sas_));
      }

    protected:
      virtual void doAccept() override {
        auto client = Tcp::createUnique(this->getLoop());
        if (!client) {
          return;
        }

        int err;
        if ((err = uv_accept(reinterpret_cast<uv_stream_t *>(get()),
                reinterpret_cast<uv_stream_t *>(client->get()))) != 0) {
          LOG_E("uv_accept failed: %s", uv_strerror(err));
          return;
        }

        int len = sizeof(client->sas_);
        if ((err = uv_tcp_getpeername(client->get(),
                reinterpret_cast<SockAddr *>(&client->sas_), &len)) != 0) {
          LOG_E("uv_tcp_getpeername failed: %s", uv_strerror(err));
          return;
        }

        LOG_V("client: %s:%d", client->getIP().c_str(), client->getPort());
        publish<EvAccept<Tcp>>(EvAccept<Tcp>{ std::move(client) });
      }

    private:
      static void onConnect(uv_connect_t *req, int status) {
        auto tcp = reinterpret_cast<Tcp *>(req->handle->data);
        if (status < 0) {
          tcp->reportError("uv_tcp_connect", status);
          return;
        }
        tcp->template publish<EvConnect>(EvConnect{});
      }

    private:
      std::unique_ptr<DNSRequest> dnsReq_{nullptr};
      std::unique_ptr<ConnectReq> connectReq_{nullptr};
      SockAddrStorage sas_;

      std::string setuid_;
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_TCP_H_ */
