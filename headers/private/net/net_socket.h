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

int socket_set_event_callback(struct socket *so, socket_event_callback, void *, int);


/* these are all private to the stack...although may be shared with 
 * other network modules.
 */

int     sosend(struct socket *so, struct mbuf *addr, struct uio *uio, 
               struct mbuf *top, struct mbuf *control, int flags);

struct socket *sonewconn(struct socket *head, int connstatus);
int	soreserve (struct socket *so, uint32 sndcc, uint32 rcvcc);

void    sbrelease (struct sockbuf *sb);
int     sbreserve (struct sockbuf *sb, uint32 cc);
void	sbdrop (struct sockbuf *sb, int len);
void    sbdroprecord (struct sockbuf *sb);
void    sbflush (struct sockbuf *sb);
int     sbwait (struct sockbuf *sb);
void	sbappend (struct sockbuf *sb, struct mbuf *m);
int     sbappendaddr (struct sockbuf *sb, struct sockaddr *asa,
            struct mbuf *m0, struct mbuf *control);
int     sbappendcontrol (struct sockbuf *sb, struct mbuf *m0,
            struct mbuf *control);
void    sbappendrecord (struct sockbuf *sb, struct mbuf *m0);
void    sbcheck (struct sockbuf *sb);
void    sbcompress (struct sockbuf *sb, struct mbuf *m, struct mbuf *n);
void    sbinsertoob(struct sockbuf *, struct mbuf *);

int     soreceive (struct socket *so, struct mbuf **paddr, struct uio *uio,
            struct mbuf **mp0, struct mbuf **controlp, int *flagsp);
void    sowakeup(struct socket *so, struct sockbuf *sb);
int     sbwait(struct sockbuf *sb);

int     sodisconnect(struct socket *);
void    sofree(struct socket *);

void    sohasoutofband(struct socket *so);
void    socantsendmore(struct socket *so);
void    socantrcvmore(struct socket *so);
void    soisconnected (struct socket *so);
void    soisconnecting (struct socket *so);
void    soisdisconnected (struct socket *so);
void    soisdisconnecting (struct socket *so);
void    soqinsque (struct socket *head, struct socket *so, int q);
int     soqremque (struct socket *so, int q);
int     sorflush(struct socket *so);

int     sb_lock(struct sockbuf *sb);
int     nsleep(sem_id chan, char *msg, int timeo);
void    wakeup(sem_id chan);

#endif
