#ifndef _NET_SOCKET_H
#define _NET_SOCKET_H

#include <sys/socketvar.h>
#include <mbuf.h>

/* Function prototypes */

/* These functions are exported through the core module */

int socket_init			(struct socket **nso);
int socket_create		(struct socket *so, int dom, int type, int proto);
int socket_shutdown		(struct socket *so, int how); // XXX this one is not used at all
int socket_close		(struct socket *so);

int socket_bind			(struct socket *so, char *, int);
int socket_listen		(struct socket *so, int backlog);
int socket_connect		(struct socket *so, char *, int);
int socket_accept		(struct socket *so, struct socket **nso, void *, int *);

int socket_writev		(struct socket *so, struct iovec *, int flags);
int socket_readv		(struct socket *so, struct iovec *, int *flags);
int socket_send			(struct socket *so, struct msghdr *, int flags, int *retsize);
int socket_recv			(struct socket *so, struct msghdr *, caddr_t namelenp, int *retsize);

int socket_setsockopt	(struct socket *so, int, int, const void *, size_t);
int socket_getsockopt	(struct socket *so, int, int, void *, size_t *);
int socket_ioctl		(struct socket *so, int cmd, caddr_t data);

int socket_getpeername	(struct socket *so, struct sockaddr *, int *);
int socket_getsockname	(struct socket *so, struct sockaddr *, int *);

int socket_socketpair		(struct socket *so, struct socket **nso);

int socket_set_event_callback(struct socket *so, socket_event_callback, void *, int);


/* these are all private to the stack...although may be shared with 
 * other network modules.
 */


struct socket *sonewconn(struct socket *head, int connstatus);
int	soreserve (struct socket *so, uint32 sndcc, uint32 rcvcc);

void	sockbuf_release(struct sockbuf *sb);
int     sockbuf_reserve(struct sockbuf *sb, uint32 cc);
void	sockbuf_drop(struct sockbuf *sb, int len);
void    sockbuf_droprecord(struct sockbuf *sb);
void    sockbuf_flush(struct sockbuf *sb);
int     sockbuf_wait(struct sockbuf *sb);
void	sockbuf_append(struct sockbuf *sb, struct mbuf *m);
int     sockbuf_appendaddr(struct sockbuf *sb, struct sockaddr *asa,
            struct mbuf *m0, struct mbuf *control);
int     sockbuf_appendcontrol(struct sockbuf *sb, struct mbuf *m0,
            struct mbuf *control);
void    sockbuf_appendrecord(struct sockbuf *sb, struct mbuf *m0);
void    sockbuf_compress(struct sockbuf *sb, struct mbuf *m, struct mbuf *n);
void    sockbuf_insertoob(struct sockbuf *, struct mbuf *);

void    sowakeup(struct socket *so, struct sockbuf *sb);

int     sodisconnect(struct socket *);

void    socket_set_hasoutofband(struct socket *so);
void    socket_set_cantsendmore(struct socket *so);
void    socket_set_cantrcvmore(struct socket *so);
void    socket_set_connected(struct socket *so);
void    socket_set_connecting(struct socket *so);
void    socket_set_disconnected(struct socket *so);
void    socket_set_disconnecting(struct socket *so);

int     sorflush(struct socket *so);

int     sockbuf_lock(struct sockbuf *sb);
int     nsleep(sem_id chan, char *msg, int timeo);
void    wakeup(sem_id chan);

#endif
