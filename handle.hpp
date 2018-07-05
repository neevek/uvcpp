#ifndef HANDLE_H_
#define HANDLE_H_
#include <stdexcept>
#include <string>
#include <type_traits>
#include <functional>
#include "loop.hpp"
#include "util.hpp"
#include "log/log.h"

namespace uvcpp {
  template <typename T>
  class Handle {
    public:
      using Type = T;

      Handle() {
        handle_.data = this;
      }
      virtual ~Handle() = default;
      Handle &&operator=(const Handle &) = delete;
      Handle &&operator=(const Handle &&) = delete;

      virtual bool init() = 0;

      void close() {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(&handle_))) {
          uv_close((uv_handle_t *)&handle_, closeCallback);
        }
      }

      Type *get() {
        return &handle_;
      }

      template <typename U>
      static auto create() {
        using HandleType = 
          typename std::enable_if<std::is_base_of<Handle<typename U::Type>, U>::value, U>::type;
        return std::make_unique<HandleType>();
      }

    private:
      static void closeCallback(uv_handle_t *handle) {
        LOG_D("close");
      }
    
    private:
      Type handle_;
  };

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


  template <typename T>
  class Stream : public Handle<T> {
    public:
      using AcceptCallback = std::function<void(std::unique_ptr<Stream>)>;
      using ReadCallback = std::function<void(const char *buf, ssize_t nread)>;
      using ErrorCallback = std::function<void(int err)>;
      const static auto BUF_SIZE = 4096; 

      void onRead(ReadCallback reader) {
        readCallback = reader;
      }

      void listen(int backlog, AcceptCallback acceptCallback) {
        acceptCallback_ = acceptCallback;
        int err;
        if ((err = uv_listen(reinterpret_cast<uv_stream_t *>(this->get()),
                backlog, onConnectCallback)) != 0) {
          LOG_W("uv_listen failed: %s", uv_strerror(err));
        }
      }

      int read() {
        int err;
        if ((err = uv_read_start(
                reinterpret_cast<uv_stream_t *>(this->get()),
                onAllocCallback, onReadCallback)) != 0) {
          LOG_E("uv_read_start failed: %s", uv_strerror(err));
          if (!errorCallback_) {
            errorCallback_(err);
          }
        }
        return err;
      }

    protected:
      virtual void doAccept(AcceptCallback acceptCallback) = 0;

    private:
      static void onAllocCallback(
          uv_handle_t *handle, size_t size, uv_buf_t *buf) {
        auto st = reinterpret_cast<Stream *>(handle->data);
        buf->base = reinterpret_cast<char *>(st->buf_.get());
        buf->len = BUF_SIZE;
      }

      static void onReadCallback(
          uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
        auto st = reinterpret_cast<Stream *>(handle->data);
        if (st->readCallback) {
          st->readCallback(buf->base, nread);
        }
      }

      static void onConnectCallback(uv_stream_t* stream, int status) {
        if (status < 0) {
          LOG_E("uv_listen failed: %s", uv_strerror(status));
          return;
        }

        auto st = reinterpret_cast<Stream *>(stream->data);
        st->doAccept(st->acceptCallback_);
      }

    private:
      AcceptCallback acceptCallback_{nullptr};
      ReadCallback readCallback{nullptr};
      ErrorCallback errorCallback_{nullptr};
      ByteArray buf_{new unsigned char[BUF_SIZE], ByteArrayDeleter};
  };

  class Tcp : public Stream<uv_tcp_t> {
    public:
      using BindCallback = std::function<void(Tcp *)>;

      enum class IPVersion {
        IPV4,
        IPV6
      };

      using IPAddr = struct sockaddr;
      using IPv4Addr = struct sockaddr_in;
      using IPv6Addr = struct sockaddr_in6;

      bool init() {
        if (uv_tcp_init(Loop::get().getRaw(), get()) != 0) {
          LOG_E("failed to init ServerTcp");
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
          LOG_E("uv_getaddrinfo failed: %s", uv_strerror(err));
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

  class ClientTcp : public Tcp {
    public:
      void setIPAndPort(std::string ip, uint16_t port) {
        ip_ = ip;
        port_ = port;
      }

      std::string getIP() const {
        return ip_;
      }

      uint16_t getPort() const {
        return port_;
      }

    protected:
      virtual void doAccept(AcceptCallback acceptCallback) override { }

    private:
      std::string ip_;
      uint16_t port_;
  };

  class ServerTcp : public Tcp {
    protected:
      virtual void doAccept(AcceptCallback acceptCallback) override {
        auto client = Tcp::create<ClientTcp>();
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

        auto ip = NetUtil::extractIPAddress(&addr);
        auto port = reinterpret_cast<sockaddr_in *>(&addr)->sin_port;
        client->setIPAndPort(ip, port);

        LOG_V("from client: %s:%d", ip.c_str(), port);

        if (acceptCallback) {
          acceptCallback(std::move(client));
        }
      }
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: HANDLE_H_ */
