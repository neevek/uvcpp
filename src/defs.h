/*******************************************************************************
**          File: defs.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 05:46 PM
**   Description: some structs and macros used in the library
*******************************************************************************/
#ifndef DEFS_H_
#define DEFS_H_
#include <memory>
#include <cstdlib>
#include "uv.h"

namespace uvcpp {
  #define STRINGIFY(s) #s

  enum class IPVersion {
    IPV4,
    IPV6
  };

  using SockAddr = struct sockaddr;
  using SockAddrStorage = struct sockaddr_storage;
  using SockAddr4 = struct sockaddr_in;
  using SockAddr6 = struct sockaddr_in6;

} /* end of namspace: uvcpp */

#endif /* end of include guard: DEFS_H_ */
