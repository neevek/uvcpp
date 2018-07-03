#ifndef HANDLE_H_
#define HANDLE_H_
#include <uv.h>

namespace uvcpp {
  class Handle {
    public:
      Handle();
      virtual ~Handle();
    
    private:
      uv_handle_t *handle_;
  };
} /* end of namspace: uvcpp */

#endif /* end of include guard: HANDLE_H_ */
