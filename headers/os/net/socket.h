#ifndef _NET_SOCKET_H
#define _NET_SOCKET_H

#include <BeBuild.h>
#include <in.h> /* in_addr, sockaddr_in */
#include <sys/socket.h> /* sockaddr */
#include <sys/select.h>

/* 
 * Be extension
 */
#define B_UDP_MAX_SIZE (65536 - 1024) 

#endif /* _SOCKET_H */
