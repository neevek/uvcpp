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

namespace uvcpp {

  template <typename T, typename Derived>
  class Stream : public Handle<T, Derived> {
    public:
      template <typename U>
      using AcceptCallback = std::function<void(std::unique_ptr<Derived>)>;
      using ReadCallback = std::function<void(const char *buf, ssize_t nread)>;
      const static auto BUF_SIZE = 4096; 

      void onRead(ReadCallback callback) {
        readCallback_ = callback;
      }

      void onAccept(AcceptCallback<Derived> callback) {
        acceptCallback_ = callback;
      }

      void listen(int backlog) {
        if (!acceptCallback_) {
          LOG_E("AcceptCallback is not set");
          return;
        }

        int err;
        if ((err = uv_listen(reinterpret_cast<uv_stream_t *>(this->get()),
                backlog, onConnectCallback)) != 0) {
          this->reportError("uv_listen", err);
        }
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

      int write(const Buffer buf) {
        return write(&buf, 1);
      }

      /**
       * > 0: number of bytes written (can be less than the supplied buffer size).
       * < 0: negative error code (UV_EAGAIN is returned if no data can be sent immediately).
       */
      int write(const Buffer bufs[], unsigned int nBufs) {
        //uv_buf_t buf = { .base = const_cast<char *>(data), .len = len };
        int err;
        if ((err = uv_try_write(
            reinterpret_cast<uv_stream_t *>(this->get()), bufs, nBufs)) < 0) {
          this->reportError("uv_try_write", err);
        }
        return err;
      }

    protected:
      virtual void doAccept(AcceptCallback<Derived> callback) = 0;

      static void onAllocCallback(
          uv_handle_t *handle, size_t size, uv_buf_t *buf) {
        auto st = reinterpret_cast<Stream *>(handle->data);
        buf->base = st->readBuf_;
        buf->len = sizeof(st->readBuf_);
      }

      static void onReadCallback(
          uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
        auto st = reinterpret_cast<Stream *>(handle->data);
        if (st->readCallback_) {
          st->readCallback_(buf->base, nread);
        }
      }

      static void onConnectCallback(uv_stream_t* stream, int status) {
        auto st = reinterpret_cast<Stream *>(stream->data);

        if (status < 0) {
          st->reportError("connect", status);
        } else {
          st->doAccept(st->acceptCallback_);
        }
      }

    private:
      AcceptCallback<Derived> acceptCallback_{nullptr};
      ReadCallback readCallback_{nullptr};
      char readBuf_[BUF_SIZE];
  };
  
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_STREAM_H_ */
