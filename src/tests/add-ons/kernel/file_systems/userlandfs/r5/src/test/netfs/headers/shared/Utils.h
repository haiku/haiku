// Utils.h

#ifndef UTILS_H
#define UTILS_H

#ifdef HAIKU_TARGET_PLATFORM_BEOS
#	include <socket.h>
#else
#	include <sys/socket.h>
#	include <unistd.h>
#endif

#include <SupportDefs.h>

#include "Compatibility.h"

template<typename T> T max(const T& a, const T& b) { return (a > b ? a : b); }
template<typename T> T min(const T& a, const T& b) { return (a < b ? a : b); }

// safe_closesocket
/*!	There seems to be race condition on a net_server system, if two threads
	try to close the same socket at the same time. This is work-around. The
	variable which stores the socket ID must be a vint32.
*/
static inline
void
safe_closesocket(vint32& socketVar)
{
	int32 socket = atomic_or(&socketVar, -1);
	if (socket >= 0) {
#ifndef HAIKU_TARGET_PLATFORM_BEOS
		shutdown(socket, SHUTDOWN_BOTH);
#endif
		closesocket(socket);
	}
}

#endif	// UTILS_H
