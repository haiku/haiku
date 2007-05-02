/*
 * kernel-level support for sockets, includes userland support as well for testing.
 * François Revol.
 */

#ifndef _KSOCKET_H
#define _KSOCKET_H

#include <sys/socket.h>

#ifndef _KERNEL_MODE /* userland wrapper */

#define ksocket socket
#define kbind bind
#define kconnect connect
#define kgetsockname getsockname
#define kgetpeername getpeername
#define kaccept accept
#define ksendmsg sendmsg
#define krecvmsg recvmsg
#define krecvfrom recvfrom
#define ksendto sendto
#define krecv recv
#define ksend send
#define klisten listen
#define kshutdown shutdown
#define kclosesocket close
#define ksocket_init() ({B_OK;})
#define ksocket_cleanup() ({B_OK;})
#define kmessage(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define KSOCKET_MODULE_DECL /* nothing */

#elif defined(__HAIKU__)

/* Haiku socket module */
#include <os/drivers/socket_interface.h>

extern struct socket_module_info *gSocket;
#define ksocket (gSocket->socket)
#define kbind (gSocket->bind)
#define kconnect (gSocket->connect)
#define kgetsockname (gSocket->getsockname)
#define kgetpeername (gSocket->getpeername)
#define kaccept (gSocket->accept)
//#define kaccept(_fd, _addr, _sz) ({int thesock; thesock = (gSocket->accept)(_fd, _addr, _sz); dprintf("kaccept(%d, , ) = %d\n", _fd, thesock); thesock; })
#define ksendmsg (gSocket->sendmsg)
#define krecvmsg (gSocket->recvmsg)
#define krecvfrom (gSocket->recvfrom)
#define ksendto (gSocket->sendto)
#define krecv (gSocket->recv)
#define ksend (gSocket->send)
#define klisten (gSocket->listen)
#define kshutdown (gSocket->shutdown)
#define kclosesocket close
#define kmessage(fmt, ...) dprintf("ksocket: " fmt "\n", ##__VA_ARGS__)

extern status_t ksocket_init (void);
extern status_t ksocket_cleanup (void);

#define KSOCKET_MODULE_DECL \
struct socket_module_info *gSocket; \
status_t ksocket_init (void) { \
	return get_module(B_SOCKET_MODULE_NAME, (module_info **)&gSocket); \
} \
 \
status_t ksocket_cleanup (void) { \
	return put_module(B_SOCKET_MODULE_NAME); \
}

#elif defined(BONE_VERSION)

/* BONE socket module */
#include <sys/socket_module.h>

extern bone_socket_info_t *gSocket;
#define ksocket (gSocket->socket)
//#define ksocket(_fam, _typ, _pro) ({int thesock; thesock = (gSocket->socket)(_fam, _typ, _pro); dprintf("ksocket(%d, %d, %d) = %d\n", _fam, _typ, _pro, thesock); thesock;})
#define kbind (gSocket->bind)
#define kconnect (gSocket->connect)
#define kgetsockname (gSocket->getsockname)
#define kgetpeername (gSocket->getpeername)
#define kaccept (gSocket->accept)
//#define kaccept(_fd, _addr, _sz) ({int thesock; thesock = (gSocket->accept)(_fd, _addr, _sz); dprintf("kaccept(%d, , ) = %d\n", _fd, thesock); thesock; })
#define ksendmsg _ERROR_no_sendmsg_in_BONE
#define krecvmsg _ERROR_no_recvmsg_in_BONE
#define krecvfrom (gSocket->recvfrom)
#define ksendto (gSocket->sendto)
#define krecv (gSocket->recv)
#define ksend (gSocket->send)
#define klisten (gSocket->listen)
#define kshutdown (gSocket->shutdown)
#define kclosesocket close
#define kmessage(fmt, ...) dprintf("ksocket: " fmt "\n", ##__VA_ARGS__)

extern status_t ksocket_init (void);
extern status_t ksocket_cleanup (void);

#define KSOCKET_MODULE_DECL \
bone_socket_info_t *gSocket; \
status_t ksocket_init (void) { \
	return get_module(BONE_SOCKET_MODULE, (module_info **)&gSocket); \
} \
 \
status_t ksocket_cleanup (void) { \
	return put_module(BONE_SOCKET_MODULE); \
}

#else /* _KERNEL_MODE, !BONE_VERSION */

#error feel free to put back ksocketd support if you dare

#endif /* _KERNEL_MODE, BONE_VERSION */

#endif /* _KSOCKET_H */
