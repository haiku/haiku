
#ifndef _COMPAT_H
#define _COMPAT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#if IPPROTO_TCP != 6
/* net_server */

#else

# define closesocket close

# ifdef BONE_VERSION
/* BONE */

# else
/* Haiku ? */

# endif

#endif

#endif /* _COMPAT_H */
