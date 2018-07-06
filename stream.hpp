/*******************************************************************************
**          File: stream.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 03:11 PM
**   Description: wraps uv_stream_t 
*******************************************************************************/
#ifndef UVCPP_STREAM_H_
#define UVCPP_STREAM_H_
#include "handle.hpp"

namespace uvcpp {

  template <typename T, typename Derived>
  class Stream : public Handle<T, Derived> {
    public:
      template <typename U>
      using AcceptCallback = std::function<void(std::unique_ptr<Derived>)>;
      using ReadCallback = std::function<void(const char *buf, ssize_t nread)>;
      using ErrorCallback = std::function<void(int err)>;
      const static auto BUF_SIZE = 4096; 

      void onRead(ReadCallback reader) {
        readCallback = reader;
      }

      void listen(int backlog, AcceptCallback<Derived> acceptCallback) {
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
      virtual void doAccept(AcceptCallback<Derived> acceptCallback) = 0;

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
      AcceptCallback<Derived> acceptCallback_{nullptr};
      ReadCallback readCallback{nullptr};
      ErrorCallback errorCallback_{nullptr};
      ByteArray buf_{new unsigned char[BUF_SIZE], ByteArrayDeleter};
  };
  
} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_STREAM_H_ */
