#ifndef _NET_SOCKET_H
#define _NET_SOCKET_H

#include <sys/socketvar.h>
#include <mbuf.h>

//uint32 sb_max;

/* Function prototypes */

/* These are the ones we export to libnet.so */

int     initsocket(struct socket **);
int     socreate  (int, struct socket *, int, int);//XXX
int     soshutdown(void *, int);
int     soclose   (void *);

int     sobind    (void *, char *, int);
int     solisten  (void *, int);
int     soconnect (void *, char *, int);
int     soaccept  (struct socket *, struct socket **, void *, int *);

int     writeit   (void *, struct iovec *, int);
int     readit    (void *, struct iovec *, int *);
int     sendit    (void *, struct msghdr *, int, int *);
int     recvit    (void *, struct msghdr *, char *, int *);

//int     so_ioctl (void *, int, void *, size_t);
int     sosysctl  (int *, uint, void *, size_t *, void *, size_t);
int     sosetopt  (void *, int, int, const void *, size_t);
int     sogetopt  (void *, int, int, void *, size_t *);
int 	soo_ioctl(void *sp, int cmd, caddr_t data);

int     sogetpeername(void *, struct sockaddr *, int *);
int     sogetsockname(void *, struct sockaddr *, int *);


/* these are all private to the stack...although may be shared with 
 * other network modules.
 */

int     sosend(struct socket *so, struct mbuf *addr, struct uio *uio, 
               struct mbuf *top, struct mbuf *control, int flags);

struct socket *sonewconn(struct socket *head, int connstatus);
int 	set_socket_event_callback(void *, socket_event_callback, void *, int);
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
