/*******************************************************************************
**          File: defs.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-06 Fri 05:46 PM
**   Description: some structs and macros used in the library
*******************************************************************************/
#ifndef DEFS_H_
#define DEFS_H_
#include <memory>
#include <uv.h>

namespace uvcpp {
  auto ByteArrayDeleter = [](unsigned char *p) { delete [] p; };
  using ByteArray = std::unique_ptr<unsigned char, decltype(ByteArrayDeleter)>;
  using Buffer = uv_buf_t;

  enum class IPVersion {
    IPV4,
    IPV6
  };

  using IPAddr = struct sockaddr;
  using IPv4Addr = struct sockaddr_in;
  using IPv6Addr = struct sockaddr_in6;

} /* end of namspace: uvcpp */

#endif /* end of include guard: DEFS_H_ */
