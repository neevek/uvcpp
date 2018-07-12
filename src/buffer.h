/*******************************************************************************
**          File: buffer.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-12 Thu 12:28 PM
**   Description: data buffer with the same layout as uv_buf_t 
*******************************************************************************/
#ifndef UVCPP_BUFFER_H_
#define UVCPP_BUFFER_H_ 
#include <cstdlib>
#include <cstring>

namespace uvcpp {
  class Buffer {
    public:
      Buffer(size_t capacity) : len_(0), capacity_(capacity) {
        data_ = new char[capacity];
      }

      ~Buffer() {
        delete [] data_;
      }

      void assign(const char *data, size_t len) {
        memcpy(data_, data, len);
        len_ = len;
      }

      char *getData() const {
        return data_;
      }

      size_t getLen() const {
        return len_;
      }

      size_t getCapacity() const {
        return capacity_;
      }

    private:
      char *data_;
      size_t len_;
      size_t capacity_;
  };

} /* end of namspace: uvcpp */

#endif /* end of include guard: UVCPP_BUFFER_H_ */
