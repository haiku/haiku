#ifndef _NET_SOCKET_H
#define _NET_SOCKET_H

#include <BeBuild.h>

#include <netinet/in.h> /* in_addr, sockaddr_in */
#include <sys/socket.h> /* sockaddr */
#include <sys/select.h>

#define OBOS_NET_STACK_VERSION	0x0100      /* 1.0 */

/* 
 * Be extension
 */
#define B_UDP_MAX_SIZE (65536 - 1024) 

#endif /* _NET_SOCKET_H */
