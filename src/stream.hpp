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

  struct EvShutdown : public Event {
    EvShutdown(int status) : status(status) { }
    int status;
  };

  template <typename T, typename Derived>
    class Stream : public Handle<T, Derived> {
    public:

      const static auto BUF_SIZE = 4096; 

      void listen(int backlog) {
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

      void shutdown() {

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
      virtual void doAccept() = 0;

      static void onAllocCallback(
          uv_handle_t *handle, size_t size, uv_buf_t *buf) {
        auto st = reinterpret_cast<Stream *>(handle->data);
        buf->base = st->readBuf_;
        buf->len = sizeof(st->readBuf_);
      }

      static void onReadCallback(
          uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
        reinterpret_cast<Stream *>(handle->data)->template
          publish<EvRead>(EvRead{ buf->base, nread });
      }

      static void onConnectCallback(uv_stream_t* stream, int status) {
        auto st = reinterpret_cast<Stream *>(stream->data);

        if (status < 0) {
          st->reportError("connect", status);
        } else {
          st->doAccept();
        }
      }

    private:
      char readBuf_[BUF_SIZE];
  };
  
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_STREAM_H_ */
