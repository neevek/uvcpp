/*******************************************************************************
**          File: stream.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:11 PM
**   Description: wraps uv_stream_t 
*******************************************************************************/
#ifndef UVCPP_STREAM_H_
#define UVCPP_STREAM_H_
#include "handle.hpp"
#include "req.hpp"
#include "defs.h"
#include "nul/buffer.hpp"
#include <deque>

namespace uvcpp {
  template <typename StreamType>
  struct EvAccept : public Event {
    EvAccept(std::unique_ptr<StreamType> client) : client(std::move(client)) { }
    std::unique_ptr<StreamType> client;
  };

  struct EvRead : public Event {
    EvRead(const char *buf, ssize_t nread) :
      buf(buf), nread(nread) { }

    const char *buf;
    ssize_t nread;
  };

  struct EvWrite : public Event { };
  struct EvShutdown : public Event { };
  struct EvBufferRecycled : public Event {
    EvBufferRecycled(std::unique_ptr<nul::Buffer> &&buffer) :
      buffer(std::forward<std::unique_ptr<nul::Buffer>>(buffer)) { }
    std::unique_ptr<nul::Buffer> buffer;
  };

  template <typename T, typename Derived>
    class Stream : public Handle<T, Derived> {
    public:
      const static auto BUF_SIZE = 8192; 

      Stream(const std::shared_ptr<Loop> &loop) : Handle<T, Derived>(loop) { }
      bool listen(int backlog) {
        int err;
        if ((err = uv_listen(reinterpret_cast<uv_stream_t *>(this->get()),
                backlog, onConnectCallback)) != 0) {
          this->reportError("uv_listen", err);
          return false;
        }
        return true;
      }

      void readStart() {
        int err;
        if ((err = uv_read_start(
                reinterpret_cast<uv_stream_t *>(this->get()),
                onAllocCallback, onReadCallback)) != 0) {
          this->reportError("uv_read_start", err);
        }
      }

      void readStop() {
        int err;
        if ((err = uv_read_stop(reinterpret_cast<uv_stream_t *>(this->get()))) != 0) {
          this->reportError("uv_read_stop", err);
        }
      }

      void shutdown() {
        if (!shutdownReq_) {
          shutdownReq_ = ShutdownReq::createUnique(this->getLoop());
        }

        int err;
        if ((err = uv_shutdown(
                shutdownReq_->get(),
                reinterpret_cast<uv_stream_t *>(this->get()),
                onShutdownCallback)) != 0) {
          this->reportError("uv_shutdown", err);
        }
      }

      /**
       * buffer.base should only be released when this method returns
       * false or the EvWrite event is received
       */
      bool writeAsync(std::unique_ptr<nul::Buffer> buffer) {
        auto rawBuffer = buffer->asPod();

        auto req = WriteReq::createUnique(this->getLoop(), std::move(buffer));
        auto rawReq = req->get();

        pendingReqs_.push_back(std::move(req));

        int err;
        if ((err = uv_write(
                rawReq,
                reinterpret_cast<uv_stream_t *>(this->get()),
                reinterpret_cast<uv_buf_t *>(rawBuffer),
                1, onWriteCallback)) != 0) {
          this->reportError("uv_write", err);
          return false;
        }
        return true;
      }

      /**
       * > 0: number of bytes written (can be less than the supplied buffer size).
       * < 0: negative error code (UV_EAGAIN is returned if no data can be sent immediately).
       */
      int writeSync(const nul::Buffer &buf) {
        //uv_buf_t buf = { .base = const_cast<char *>(data), .len = len };
        int err;
        if ((err = uv_try_write(
            reinterpret_cast<uv_stream_t *>(this->get()),
            reinterpret_cast<const uv_buf_t *>(&buf), 1)) < 0) {
          this->reportError("uv_try_write", err);
        }
        return err;
      }

      virtual bool init() override {
        if (!Handle<T, Derived>::init()) {
          return false;
        }

        this->template once<EvError>([this](const auto &e, auto &st){
          if (!pendingReqs_.empty()) {
            for (auto &r : pendingReqs_) {
              this->template publish<EvBufferRecycled>(
                EvBufferRecycled{ std::move(r->buffer) });
            }
          }
        });
        return true;
      }

    protected:
      virtual void doAccept() = 0;

      static void onAllocCallback(
          uv_handle_t *handle, std::size_t size, uv_buf_t *buf) {
        auto st = reinterpret_cast<Stream *>(handle->data);
        buf->base = st->readBuf_;
        buf->len = sizeof(st->readBuf_);
      }

      static void onReadCallback(
          uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
        if (nread == 0) { // EAGAIN || EWOULDBLOCK
          return;
        }

        auto st = reinterpret_cast<Stream *>(handle->data);
        if (nread < 0) {
          if (nread != UV_EOF) {
            LOG_E("TCP read failed: %s", uv_strerror(nread));
          }
          st->close();
          return;
        }

        st->template publish<EvRead>(EvRead{ buf->base, nread });
      }

      static void onWriteCallback(uv_write_t *req, int status) {
        auto st = reinterpret_cast<Stream *>(req->handle->data);
        if (status < 0) {
          st->reportError("write", status);

        } else {
          if (!st->pendingReqs_.empty()) {
            auto req = std::move(st->pendingReqs_.front());
            st->pendingReqs_.pop_front();

            st->template publish<EvBufferRecycled>(
              EvBufferRecycled{ std::move(req->buffer) });
          }

          st->template publish<EvWrite>(EvWrite{});
        }
      }

      static void onConnectCallback(uv_stream_t* stream, int status) {
        auto st = reinterpret_cast<Stream *>(stream->data);

        if (status < 0) {
          st->reportError("connect", status);
        } else {
          st->doAccept();
        }
      }

      static void onShutdownCallback(uv_shutdown_t *r, int status) {
        auto req = reinterpret_cast<Stream *>(r->handle->data);
        req->template publish<EvShutdown>(EvShutdown{});
      }

    private:
      std::deque<std::unique_ptr<WriteReq>> pendingReqs_{};
      std::unique_ptr<ShutdownReq> shutdownReq_{nullptr};
      char readBuf_[BUF_SIZE];
  };
  
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_STREAM_H_ */
