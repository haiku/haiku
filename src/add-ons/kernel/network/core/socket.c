/* socket "server" */

#include <stdio.h>
#include <kernel/OS.h>
#include <iovec.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "sys/socket.h"
#include "sys/socketvar.h"
#include "sys/sockio.h"
#include "sys/protosw.h"
#include "pools.h"
#include "net/if.h"
#include "netinet/in.h"
#include "netinet/in_pcb.h"
#include "net_misc.h"
#include "protocols.h"
#include "sys/net_uio.h"
#ifdef _KERNEL_
#include <KernelExport.h>
#include "core_module.h"
#endif

#define   SBLOCKWAIT(f)   (((f) & MSG_DONTWAIT) ? M_NOWAIT : M_WAITOK)

/* Private prototypes */
static int checkevent(struct socket *so);

/* Static global objects... */
static pool_ctl *spool;
static benaphore sockets_lock;

/* OpenBSD sets this at 128??? */
static int somaxconn = SOMAXCONN;

/* for now - should be moved to be_error.h */
#define EDESTADDRREQ EINVAL

int sockets_init(void)
{
	if (!spool)
		pool_init(&spool, sizeof(struct socket));

	if (!spool)
		return ENOMEM;
	
	INIT_BENAPHORE(sockets_lock, "sockets_lock");
	return CHECK_BENAPHORE(sockets_lock);
}


void sockets_shutdown(void)
{
	pool_destroy(spool);
	
	UNINIT_BENAPHORE(sockets_lock);
}


/* uiomove! */

int uiomove(caddr_t cp, int n, struct uio *uio)
{
	struct iovec *iov;
	uint cnt;
	int error = 0;
	void *ptr = NULL;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;

		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		switch (uio->uio_segflg) {
			/* XXX - once "properly" in kernel space, revisit and
			 * fix this for kernel moves...
			 */
			case UIO_USERSPACE:
			case UIO_SYSSPACE:
				if (uio->uio_rw == UIO_READ)
					ptr = memcpy(iov->iov_base, cp, cnt);
				else
					ptr = memcpy(cp, iov->iov_base, cnt);

				if (!ptr)
					return (errno);			
				break;
		}
		iov->iov_base = (caddr_t)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

int initsocket(void **sp)
{
	struct socket *so;
	
	so = (struct socket*)pool_get(spool);
	
	if (so == NULL) {
		printf("initsocket: ENOMEM\n");
		return ENOMEM;
	}

	memset(so, 0, sizeof(*so));

	*sp = so;

	return 0;
}

int socreate(int dom, void *sp, int type, int proto)
{
	struct protosw *prm = NULL; /* protocol module */
	struct socket *so = (struct socket*)sp;
	int error;

	if (so == NULL) {
		printf("socreate: EINVAL\n");
		return EINVAL;
	}
	
	if (proto)
		prm = pffindproto(dom, proto, type);
	else
		prm = pffindtype(dom, type);

	if (!prm || !prm->pr_userreq) {
		printf("socreate: EPROTONOSUPPORT\n");
		return EPROTONOSUPPORT;
	}
	
	if (prm->pr_type != type) {
		printf("socreate: EPROTOTYPE\n");
		return EPROTOTYPE;
	}
	
	so->so_type = type;
	so->so_proto = prm;
	/* Our sem's... don't like using so many here - find another way :( */
	so->so_rcv.sb_pop   = create_sem(0, "so_rcv.sb_pop sem");
	so->so_snd.sb_pop   = create_sem(0, "so_snd.sb_pop sem");
	so->so_timeo        = create_sem(0, "so_timeo sem");
	so->so_rcv.sb_sleep = create_sem(0, "so_rcv.sb_sleep sem");
	so->so_snd.sb_sleep = create_sem(0, "so_snd.sb_sleep sem");
	
	if (so->so_rcv.sb_pop < 0 ||
	    so->so_rcv.sb_sleep < 0 ||
	    so->so_snd.sb_pop < 0 ||
	    so->so_snd.sb_sleep < 0 ||
	    so->so_timeo < 0)
		return ENOMEM;
	    
#ifdef _KERNEL_
	set_sem_owner(so->so_rcv.sb_pop,   B_SYSTEM_TEAM);
	set_sem_owner(so->so_snd.sb_pop,   B_SYSTEM_TEAM);
	set_sem_owner(so->so_timeo,        B_SYSTEM_TEAM);
	set_sem_owner(so->so_rcv.sb_sleep, B_SYSTEM_TEAM);
	set_sem_owner(so->so_snd.sb_pop,   B_SYSTEM_TEAM);
#endif

	error = prm->pr_userreq(so, PRU_ATTACH, NULL, (struct mbuf *)proto, NULL);
	if (error) {
		so->so_state |= SS_NOFDREF; /* so we free the socket */
		sofree(so);
		return error;
	}
	
	return 0;
}


int soreserve(struct socket *so, uint32 sndcc, uint32 rcvcc)
{
	if (sbreserve(&so->so_snd, sndcc) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, rcvcc) == 0)
		goto bad2;

	if (so->so_rcv.sb_lowat == 0)
		so->so_rcv.sb_lowat = 1;
	if (so->so_snd.sb_lowat == 0)
		so->so_snd.sb_lowat = MCLBYTES;
	if (so->so_snd.sb_lowat > so->so_snd.sb_hiwat)
		so->so_snd.sb_lowat = so->so_snd.sb_hiwat;

	return (0);

bad2:
	sbrelease(&so->so_snd);
bad:
	printf("soreserve: ENOBUFS\n");
	return (ENOBUFS);
}


int sobind(void *sp, caddr_t data, int len)
{
	int error;
	struct mbuf *nam;
	struct socket *so = (struct socket*)sp;

	nam = m_get(MT_SONAME);
	if (!nam) {
		printf("sobind: ENOMEM\n");
		return ENOMEM;
	}

	nam->m_len = len;
	memcpy(mtod(nam, char*), data, len);

	/* xxx - locking! */
	error = (*so->so_proto->pr_userreq) (so, PRU_BIND, NULL, nam, NULL);

	m_freem(nam);

	return error;
}

int solisten(void *sp, int backlog)
{
	struct socket *so = (struct socket *)sp;
	int error;

	error = so->so_proto->pr_userreq(so, PRU_LISTEN, NULL, NULL, NULL);
	if (error)
		return error;
        
	if (so->so_q == 0)
		so->so_options |= SO_ACCEPTCONN;
	if (backlog < 0 || backlog > somaxconn)
		backlog = somaxconn;
	/* OpenBSD defines a minimum of 80...hmmm... */
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = backlog;
	return 0;
}

int soconnect(void *sp, caddr_t data, int len)
{
	struct socket *so = (struct socket *)sp;
	struct mbuf *nam = m_get(MT_SONAME);
	int error;

	if (!nam)
		return ENOMEM;

	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING))
		return EALREADY;

	if ((so->so_options & SO_ACCEPTCONN))
		return (EOPNOTSUPP);

	nam->m_len = len;
	memcpy(mtod(nam, char*), data, len);

	/*
	 * If protocol is connection-based, can only connect once.
	 * Otherwise, if connected, try to disconnect first.
	 * This allows user to disconnect by connecting to, e.g.,
	 * a null address.
	 */
	if ((so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING)) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	    (error = sodisconnect(so)))) {
		error = EISCONN;
	} else {
		error = so->so_proto->pr_userreq(so, PRU_CONNECT,
		                                 NULL, nam, NULL);
	}
	
	if (error) {
		goto bad;
	}
	
	if ((so->so_state & SS_NBIO) && (so->so_state && SS_ISCONNECTING)) {
		m_freem(nam);
		return EINPROGRESS;
	}
	
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0)
		if ((error = nsleep(so->so_timeo, "soconnect", 0)))
			break;
	
	if (error == 0) {
		error = so->so_error;
		so->so_error = 0;
	}

bad:
	so->so_state &= ~SS_ISCONNECTING;
	m_freem(nam);

	return error;
}

struct socket *sonewconn(struct socket *head, int connstatus)
{
	struct socket *so;
	int soqueue = connstatus ? 1 : 0;

	if (head->so_qlen + head->so_q0len > 3 * head->so_qlimit / 2)
		return NULL;
	if (initsocket((void**)&so) < 0)
		return NULL;

	so->so_type    = head->so_type;
	so->so_options = head->so_options & ~SO_ACCEPTCONN;
	so->so_linger  = head->so_linger;
	so->so_state   = head->so_state | SS_NOFDREF;
	so->so_proto   = head->so_proto;
	so->so_timeo   = head->so_timeo;

	soreserve(so, head->so_snd.sb_hiwat, head->so_rcv.sb_hiwat);
	soqinsque(head, so, soqueue);

	if ((*so->so_proto->pr_userreq)(so, PRU_ATTACH, NULL, NULL, NULL)) {
		soqremque(so, soqueue);
		pool_put(spool, so);
		return NULL;
	}
	if(connstatus) {
		sorwakeup(head);
		wakeup(head->so_timeo);
		so->so_state |= connstatus;
	}
	return so;
}

int soshutdown(void *sp, int how)
{
	struct socket *so = (struct socket*)sp;
	struct protosw *pr = so->so_proto;
	
	how++;
	if (how & SHUT_RD)
		sorflush(so);
	if (how & SHUT_WR)
		return pr->pr_userreq(so, PRU_SHUTDOWN, NULL, NULL, NULL);
	return 0;
}

int sendit(void *sp, struct msghdr *mp, int flags, int *retsize)
{
	struct socket *so = (struct socket *)sp;
	struct uio auio;
	struct iovec *iov;
	int i;
	struct mbuf *to;
	struct mbuf *control;
	int len;
	int error;

	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_rw = UIO_WRITE;
	auio.uio_offset = 0;
	auio.uio_resid = 0;
	iov = mp->msg_iov;

	/* Make sure we don't exceed max size... */
	for (i=0;i < mp->msg_iovlen;i++, iov++) {
		if (iov->iov_len > SSIZE_MAX ||
		    (auio.uio_resid += iov->iov_len) > SSIZE_MAX)
			return EINVAL;
	}
	if (mp->msg_name) {
		/* stick msg_name into an mbuf */
		to = m_get(MT_SONAME);
		to->m_len = mp->msg_namelen;
		memcpy(mtod(to, char*), mp->msg_name, mp->msg_namelen);
	} else
		to = NULL;
	
	if (mp->msg_control) {
		if (mp->msg_controllen < sizeof(struct cmsghdr)) {
			error = EINVAL;
			goto bad;
		}
		control = m_get(MT_CONTROL);
		control->m_len = mp->msg_controllen;
		memcpy(mtod(control, char*), mp->msg_control, mp->msg_controllen);
	} else
		control = NULL;

	len = auio.uio_resid;

	error = sosend(so, to, &auio, NULL, control, flags);
	if (error) {
		/* what went wrong! */
		if (auio.uio_resid != len && (/*error == ERESTART || */
		    error == EINTR || error == EWOULDBLOCK))
			error = 0; /* not really an error */
	}
	if (error == 0)
		*retsize = len - auio.uio_resid; /* tell them how much we did send */
bad:
	if (to)
		m_freem(to);

	return error;
}

int writeit(void *sp, struct iovec *iov, int flags)
{
	struct socket *so = (struct socket *)sp;
	struct uio auio;
	int len = iov->iov_len;
	int error;

	auio.uio_iov = iov;
	auio.uio_iovcnt = 1;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_rw = UIO_WRITE;
	auio.uio_offset = 0;
	auio.uio_resid = iov->iov_len;

	error = sosend(so, NULL, &auio, NULL, NULL, flags);

	if (error < 0)
		return error;
	return (len - auio.uio_resid);
}

int sosend(struct socket *so, struct mbuf *addr, struct uio *uio, struct mbuf *top,
	   struct mbuf *control, int flags)
{
	struct mbuf **mp;
	struct mbuf *m;
	int32 space;
	int32 len;
	uint64 resid;
	int clen = 0;
	int error, dontroute, mlen;
	int atomic = sosendallatonce(so) || top;

	if (uio)
		resid = uio->uio_resid;
	else
		resid = top->m_pkthdr.len;

	/* resid shouldn't be below 0 and also a flag of MSG_EOR on a 
	 * SOCK_STREAM isn't allowed (doesn't even make sense!)
	 */
	if (resid < 0 || (so->so_type == SOCK_STREAM && (flags & MSG_EOR))) {
		error = EINVAL;
		goto release;
	}

	dontroute = (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
		    (so->so_proto->pr_flags & PR_ATOMIC);

	if (control)
		clen = control->m_len;

#define snderr(errno)	{ error = errno; /* unlock */ goto release; } 
restart:
	if ((error = sblock(&so->so_snd, SBLOCKWAIT(flags))))
		goto out;
	
	/* Main Loop! We should loop here until resid == 0 */
	do { 
		if ((so->so_state & SS_CANTSENDMORE))
			snderr(EPIPE);
		if (so->so_error)
			snderr(so->so_error);
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED) {
				/* we need to be connected and we're not... */
				if ((so->so_state & SS_ISCONFIRMING) == 0 &&
				    !(resid == 0 && clen != 0))
					/* we're not even trying to connect and we
					 * have data to send, so it's an error!
					 * return ENOTCONN
					 */
					snderr(ENOTCONN);
			} else if (addr == NULL)
				/* UDP is a connectionless protocol, so it can work
				 * without being connected as long as we tell it where we
				 * want to send the data :)
				 */
				/* Doh! No address to send to (UDP) */
				snderr(EDESTADDRREQ);
		}
		space = sbspace(&so->so_snd);

		if (flags & MSG_OOB)
			space += 1024;

		if ((atomic && resid > so->so_snd.sb_hiwat) || clen > so->so_snd.sb_hiwat)
			snderr(EMSGSIZE);

		if (space < resid + clen && uio && 
		    (atomic || space < so->so_snd.sb_lowat || space < clen)) {
			if ((so->so_state & SS_NBIO)) { /* non blocking set */
				printf("so->so_state & SS_NBIO (%d)\n", so->so_state & SS_NBIO);
				snderr(EWOULDBLOCK);
			}
			/* free lock - we're waiting on send buffer space */
			sbunlock(&so->so_snd);
			error = sbwait(&so->so_snd);
			if (error)
				goto out;
			goto restart;
		}
		mp = &top;
		space -= clen;

		do {
			if (!uio) {
				/* data is actually just packaged as top. */
				resid = 0;
				if (flags & MSG_EOR)
					top->m_flags |= M_EOR;
			} else do {
				if (!top) {
					MGETHDR(m, MT_DATA);
					mlen = MHLEN;
					m->m_pkthdr.len = 0;
					m->m_pkthdr.rcvif = NULL;
				} else {
					MGET(m, MT_DATA);
					mlen = MLEN;
				}
				if (resid >= MINCLSIZE && space >= MCLBYTES) {
					MCLGET(m);
					if ((m->m_flags & M_EXT) == 0)
						/* didn't get a cluster */
						goto nopages;

					mlen = MCLBYTES;
					if (atomic && !top) {
						len = min(MCLBYTES - max_hdr, resid);
						m->m_data += max_hdr;
					} else
						len = min(MCLBYTES, resid);
					space -= len;
				} else {
nopages:
					len = min(min(mlen, resid), space);
					space -= len;
					/* leave room for headers if required */
					if (atomic && !top && len < mlen)
						MH_ALIGN(m, len);
				}

				error = uiomove(mtod(m, caddr_t), (int)len, uio);
				resid = uio->uio_resid;
				m->m_len = len;
				*mp = m;
				top->m_pkthdr.len += len;
				if (error)
					goto release;
				mp = &m->m_next;
				if (resid <= 0) {
					if (flags & MSG_EOR)
						/* we're the last record */
						top->m_flags |= M_EOR;
					break;
				}
			} while (space > 0 && atomic);

			if (dontroute)
				so->so_options |= SO_DONTROUTE;

			/* XXX - locking */
			error = (*so->so_proto->pr_userreq)(so, (flags & MSG_OOB) ? PRU_SENDOOB: PRU_SEND,
						        top, addr, control);

			/* XXX - unlock */
			if (dontroute)
				so->so_options &= ~SO_DONTROUTE;
			clen = 0;
			top = NULL;
			control = NULL;
			mp = &top;
			if (error)
				goto release;
		} while (resid && space > 0);
	} while (resid);

release:
	sbunlock(&so->so_snd);
out:
	if (top)
		m_freem(top);
	if (control)
		m_freem(control);
	return (error);
}

int readit(void *sp, struct iovec *iov, int *flags)
{
	struct socket *so = (struct socket *)sp;
	struct uio auio;
	int len = iov->iov_len;
	int error;
		
	auio.uio_iov = iov;
	auio.uio_iovcnt = 1;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_offset = 0;
	auio.uio_resid = iov->iov_len;

	error = soreceive(so, NULL, &auio, NULL, NULL, flags);
	if (error != 0)
		return error;
	return (len - auio.uio_resid);
}

int recvit(void *sp, struct msghdr *mp, caddr_t namelenp, int *retsize)
{
	struct socket *so = (struct socket*)sp;
	struct uio auio;
	struct iovec *iov;
	struct mbuf *control = NULL;
	struct mbuf *from = NULL;
	int error = 0, i, len = 0;

	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_offset = 0;
	auio.uio_resid = 0;
	iov = mp->msg_iov;

	for (i=0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len < 0)
			return EINVAL;
		if ((auio.uio_resid += iov->iov_len) < 0)
			return EINVAL;
	}
	len = auio.uio_resid;

	if ((error = soreceive(so, &from, &auio,
				NULL, mp->msg_control ? &control : NULL, 
				&mp->msg_flags)) != 0) {
		if (auio.uio_resid != len && (error == EINTR || error == EWOULDBLOCK))
			error = 0;
	}

	if (error)
		goto out;

	*retsize = len - auio.uio_resid;

	if (mp->msg_name) {
		len = mp->msg_namelen;
		
		if (len <= 0 || !from) {
			len = 0;
		} else {
			if (len > from->m_len)
				len = from->m_len;
			memcpy((caddr_t)mp->msg_name, mtod(from, caddr_t), len);
		}
		mp->msg_namelen = len;
		if (namelenp)
			memcpy(namelenp, (caddr_t)&len, sizeof(int));
	}

	/* XXX - add control handling */
out:
	if (from)
		m_freem(from);
	if (control)
		m_freem(control);

	return error;
}


int soreceive(struct socket *so, struct mbuf **paddr, struct uio *uio, struct mbuf**mp0,
		struct mbuf **controlp, int *flagsp)
{
	struct mbuf *m, **mp;
	int flags = 0;
	int len, error = 0, offset;
	struct mbuf *nextrecord;
	int moff, type = 0;
	int orig_resid = uio->uio_resid;
	struct protosw *pr = so->so_proto;

	mp = mp0;
	if (paddr)
		*paddr = NULL;
	if (controlp)
		*controlp = NULL;

	/* ensure we don't have MSG_EOR set */
	if (flagsp)
		flags = (*flagsp) & ~MSG_EOR;

	if (flags & MSG_OOB) {
		m = m_get(MT_DATA);
		error = (*pr->pr_userreq)(so, PRU_RCVOOB, m, (struct mbuf*)(flags & MSG_PEEK), NULL);
		if (error)
			goto bad;
		do {
			error = uiomove(mtod(m, caddr_t), (int) min(uio->uio_resid, m->m_len), uio);
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		if (m)
			m_freem(m);
		return error;
	}
	if (mp)
		*mp = NULL;
	if ((so->so_state & SS_ISCONFIRMING) && uio->uio_resid)
		(*pr->pr_userreq)(so, PRU_RCVD, NULL, NULL, NULL);
		
restart:
	if ((error = sblock(&so->so_rcv, SBLOCKWAIT(flags))))
		return error;
	m = so->so_rcv.sb_mb;
	/*
	 * If we have less data than requested, block awaiting more
	 * (subject to any timeout) if:
	 *   1. the current count is less than the low water mark,
	 *   2. MSG_WAITALL is set, and it is possible to do the entire
	 *	receive operation at once if we block (resid <= hiwat), or
	 *   3. MSG_DONTWAIT is not set.
	 * If MSG_WAITALL is set but resid is larger than the receive buffer,
	 * we have to do the receive in sections, and thus risk returning
	 * a short count if a timeout or signal occurs after we start.
	 */
	if (m == NULL || (((flags & MSG_DONTWAIT) == 0 &&
	    so->so_rcv.sb_cc < uio->uio_resid) &&
	    (so->so_rcv.sb_cc < so->so_rcv.sb_lowat ||
	    ((flags & MSG_WAITALL) && uio->uio_resid <= so->so_rcv.sb_hiwat)) &&
	    m->m_nextpkt == 0 && (pr->pr_flags & PR_ATOMIC) == 0)) {

		if (so->so_error) {
			if (m)
				goto dontblock;
			error = so->so_error;
			if ((flags & MSG_PEEK) == 0)
				so->so_error = 0;
			goto release;
		}
		if ((so->so_state & SS_CANTRCVMORE)) {
			if (m)
				goto dontblock;
			else
				goto release;
		}
		for (;m; m = m->m_next)
			if (m->m_type == MT_OOBDATA || (m->m_flags & M_EOR)) {
				m = so->so_rcv.sb_mb;
				goto dontblock;
			}
		if ((so->so_state & (SS_ISCONNECTED | SS_ISCONNECTING)) == 0 &&
			(so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			error = ENOTCONN;
			goto release;
		}
		if (uio->uio_resid == 0)
			goto release;
		if ((so->so_state & SS_NBIO) || (flags & MSG_DONTWAIT)) {
			error = EWOULDBLOCK;	
			goto release;
		}
		sbunlock(&so->so_rcv);
		error = sbwait(&so->so_rcv);
		if (error)
			return error;
		goto restart;
	}

dontblock:
	nextrecord = m->m_nextpkt;
	if (pr->pr_flags & PR_ADDR) {
		orig_resid = 0;
		if (flags & MSG_PEEK) {
			if (!paddr)
				*paddr = m_copym(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (paddr) {
				*paddr = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = NULL;
				m = so->so_rcv.sb_mb;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
		}
	}
	while (m && m->m_type == MT_CONTROL && error == 0) {
		if ((flags & MSG_PEEK)) {
			if (controlp)
				*controlp = m_copym(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (controlp) {
				/* XXX technically we should look at control rights here,
				 * but so far we have no notion of them...
				 */
				*controlp = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = NULL;
				m = so->so_rcv.sb_mb;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
		}
		if (controlp) {
			orig_resid = 0;
			controlp = &(*controlp)->m_next;
		}
	}
	if (m) {
		if ((flags & MSG_PEEK) == 0)
			m->m_nextpkt = nextrecord;
		type = m->m_type;
		if (type == MT_OOBDATA)
			flags |= MSG_OOB;
	}
	moff = 0;
	offset = 0;
	while (m && uio->uio_resid > 0 && error == 0) {
		if (m->m_type == MT_OOBDATA) {
			if (type != MT_OOBDATA)
				break;
		} else if (type == MT_OOBDATA) {	
			break;
		}
		so->so_state &= ~SS_RCVATMARK;
		len = uio->uio_resid;
		if (so->so_oobmark && len > so->so_oobmark - offset)
			len = so->so_oobmark - offset;
		if (len > m->m_len - moff)
			len = m->m_len - moff;
		if (!mp)
			error = uiomove(mtod(m, caddr_t) + moff, (int)len, uio);
		else
			uio->uio_resid -= len;
		if (len == m->m_len - moff) {
			if (m->m_flags & M_EOR)
				flags |= MSG_EOR;
			if (flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				nextrecord = m->m_nextpkt;
				sbfree(&so->so_rcv, m);
				if (mp) {
					*mp = m;
					mp = &m->m_next;
					so->so_rcv.sb_mb = m = m->m_next;
					*mp = NULL;
				} else {
					MFREE(m, so->so_rcv.sb_mb);
					m = so->so_rcv.sb_mb;
				}
				if (m)
					m->m_nextpkt = nextrecord;
			}
		} else {
			if ((flags & MSG_PEEK))
				moff += len;
			else {
				if (mp)
					*mp = m_copym(m, 0, len);
				m->m_data += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if (so->so_oobmark) {
			if ((flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					break;
				}
			} else {
				offset += len;
				if (offset == so->so_oobmark)
					break;
			}
		}
		if (flags & MSG_EOR)
			break;
		while (flags & MSG_WAITALL && m == NULL && uio->uio_resid > 0 &&
		       !sosendallatonce(so) && !nextrecord) {
			if (so->so_error || so->so_state & SS_CANTRCVMORE)
				break;
			error = sbwait(&so->so_rcv);
			if (error) {
				sbunlock(&so->so_rcv);
				return 0;
			}
			if ((m = so->so_rcv.sb_mb))
				nextrecord = m->m_nextpkt;
		}
	}

	if (m && pr->pr_flags & PR_ATOMIC) {
		flags |= MSG_TRUNC;
		if ((flags & MSG_PEEK) == 0) 
			sbdroprecord(&so->so_rcv);
	}
	if ((flags & MSG_PEEK) == 0) {
		if (!m)
			so->so_rcv.sb_mb = nextrecord;
		if (pr->pr_flags & PR_WANTRCVD && so->so_pcb)
			(*pr->pr_userreq)(so, PRU_RCVD, (struct mbuf*)flags, NULL, NULL);
	}
	if (orig_resid == uio->uio_resid && orig_resid &&
		(flags & MSG_EOR) == 0 && (so->so_state & SS_CANTRCVMORE) == 0) {
		sbunlock(&so->so_rcv);
		goto restart;
	}
	if (flagsp)
		*flagsp |= flags;

release:
	sbunlock(&so->so_rcv);
	return error;
}

int soo_ioctl(void *sp, int cmd, caddr_t data)
{
	struct socket *so = (struct socket*)sp;

	switch (cmd) {
		case FIONBIO:
			if (*(int*)data)
				so->so_state |= SS_NBIO;
			else
				so->so_state &= ~SS_NBIO;
			return 0;
		case FIONREAD:
			/* how many bytes do we have waiting... */
			*(int*)data = so->so_rcv.sb_cc;
			return 0;
		case SIOCATMARK:
			*(int*)data = (so->so_state & SS_RCVATMARK) != 0;
			return 0;
	}

	if (IOCGROUP(cmd) == 'i') {
		return ifioctl(so, cmd, data);
	}
	if (IOCGROUP(cmd) == 'r') {
		return EINVAL; /* EOPNOTSUPP */
	}	
	return (*so->so_proto->pr_userreq)(so, PRU_CONTROL, 
		(struct mbuf*)cmd, (struct mbuf*)data, NULL);
}


int soclose(void *sp)
{
	struct socket *so = (struct socket*)sp;
	int error = 0;

	/* we don't want any more events... */
	so->event_callback = NULL;
	so->event_callback_cookie = NULL;

	if (so->so_options & SO_ACCEPTCONN) {
		while (so->so_q0)
			(*so->so_proto->pr_userreq)(so, PRU_ABORT, NULL, NULL, NULL);
		while (so->so_q)
			(*so->so_proto->pr_userreq)(so, PRU_ABORT, NULL, NULL, NULL);
	}

	if (so->so_pcb == NULL)
		goto discard;

	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so);
			if (error)
				goto drop;
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO))
				goto drop;
			while (so->so_state & SS_ISCONNECTED)
				if ((error = nsleep(so->so_timeo, "lingering close", so->so_linger)))
					break;
		}
	}

drop:
	if (so->so_pcb) {
		int error2 = (*so->so_proto->pr_userreq)(so, PRU_DETACH, NULL, NULL, NULL);
		if (error2 == 0)
			error = error2;
	}

discard:
	if (so->so_state & SS_NOFDREF)
		printf("PANIC: soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	sofree(so);
	return error;
}

int sorflush(struct socket *so)
{
	struct sockbuf *sb = &so->so_rcv;
	struct sockbuf asb;

	sb->sb_flags |= SB_NOINTR;
	sblock(sb, M_WAITOK);
	socantrcvmore(so);
	sbunlock(sb);
	asb = *sb;
	memset(sb, 0, sizeof(*sb));
	
	sbrelease(&asb);
	return 0;
}

int sodisconnect(struct socket *so)
{
	int error;
	
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = so->so_proto->pr_userreq(so, PRU_DISCONNECT, 
		NULL, NULL, NULL);

bad:
	return error;
}

void sofree(struct socket *so)
{
	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;

	if (so->so_head) {
		if (!soqremque(so, 0) && !soqremque(so, 1)) {
			printf("PANIC: sofree: couldn't dq socket\n");
			return;
		}
		so->so_head = NULL;
	}
	sbrelease(&so->so_snd);
	sorflush(so);

	delete_sem(so->so_rcv.sb_pop);
	delete_sem(so->so_snd.sb_pop);
	delete_sem(so->so_timeo);
	delete_sem(so->so_rcv.sb_sleep);
	delete_sem(so->so_snd.sb_sleep);

	pool_put(spool, so);

	return;
}

int sosetopt(void *sp, int level, int optnum, const void *data, size_t datalen)
{
	struct socket *so = (struct socket*)sp;
	struct mbuf *m, *m0;
	int error = 0;

	m = m_get(MT_SOOPTS);
	if (!m)
		return ENOMEM;
	if (memcpy(mtod(m, void*), data, datalen) == NULL)
		return ENOMEM;
	m->m_len = datalen;
	m0 = m;
	
	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput)
			return (*so->so_proto->pr_ctloutput)(PRCO_SETOPT, so, level, optnum, &m0);
		error = ENOPROTOOPT;
	} else {
		switch (optnum) {
			case SO_LINGER:
				if (datalen != sizeof(struct linger)) {
					error = EINVAL;
					goto bad;
				}
				so->so_linger = mtod(m, struct linger*)->l_linger;
				/* fall thru... */
			case SO_DEBUG:
			case SO_KEEPALIVE:
			case SO_DONTROUTE:
			case SO_USELOOPBACK:
			case SO_BROADCAST:
			case SO_REUSEADDR:
			case SO_REUSEPORT:
			case SO_OOBINLINE:
				if (datalen < sizeof(int)) {
					error = EINVAL;
					goto bad;
				}
				if (*mtod(m, int*))
					so->so_options |= optnum;
				else
					so->so_options &= ~optnum;
				break;
			case SO_SNDBUF:
			case SO_RCVBUF:
			case SO_SNDLOWAT:
			case SO_RCVLOWAT:
				if (datalen < sizeof(int)) {
					error = EINVAL;
					goto bad;
				}
				switch (optnum) {
					case SO_SNDBUF:
					case SO_RCVBUF:
						if (sbreserve(optnum == SO_SNDBUF ? &so->so_snd : &so->so_rcv,
						              (uint32)*mtod(m, int32*)) == 0) {
							error = ENOBUFS;
							goto bad;
						}
						break;
					case SO_SNDLOWAT:
						so->so_snd.sb_lowat = *mtod(m, int*);
						break;
					case SO_RCVLOWAT:
						so->so_rcv.sb_lowat = *mtod(m, int*);
						break;
				}
				break;
			case SO_SNDTIMEO:
			case SO_RCVTIMEO: 
			/* bsd has the timeouts as int16, we're using int32... */
			{
				struct timeval *tv;
				int32 val;
				
				if (datalen < sizeof(*tv)) {
					error = EINVAL;
					goto bad;
				}
				tv = mtod(m, struct timeval *);
				val = tv->tv_sec * 1000000 + tv->tv_usec;
				switch (optnum) {
					case SO_SNDTIMEO:
						so->so_snd.sb_timeo = val;
						break;
					case SO_RCVTIMEO:
						so->so_rcv.sb_timeo = val;
						break;
				}
				break;
			}	
			default:
				error = ENOPROTOOPT;
		}
		if (error == 0 && so->so_proto && so->so_proto->pr_ctloutput) {
			(*so->so_proto->pr_ctloutput)(PRCO_SETOPT, so, level, optnum, &m0);
			m = NULL;
		}
	}
bad:
	if (m)
		m_free(m);
	
	return error;
}

int sogetopt(void *sp, int level, int optnum, void *data, size_t *datalen)
{
	struct socket *so = (struct socket*)sp;
	struct mbuf *m;
	
	m = m_get(MT_SOOPTS);
	if (memcpy(mtod(m, void*), data, *datalen) == NULL)
		return ENOMEM;
	if (*datalen < sizeof(int))
		return EINVAL;
	m->m_len = sizeof(int);

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput) {
			return (*so->so_proto->pr_ctloutput)(PRCO_GETOPT, so, level, optnum, &m);
		} else
			return ENOPROTOOPT;
	} else {
		switch(optnum) {
			case SO_LINGER:
				m->m_len = sizeof(struct linger);
				mtod(m, struct linger*)->l_onoff = so->so_options & SO_LINGER;
				mtod(m, struct linger*)->l_linger = so->so_linger;
				break;
			case SO_DEBUG:
			case SO_KEEPALIVE:
			case SO_DONTROUTE:
			case SO_USELOOPBACK:
			case SO_BROADCAST:
			case SO_REUSEADDR:
			case SO_REUSEPORT:
			case SO_OOBINLINE:
				*mtod(m, int*) = so->so_options & optnum;
				break;
			case SO_TYPE:
				*mtod(m, int*) = so->so_type;
				break;
			case SO_ERROR:
				*mtod(m, int*) = so->so_error;
				so->so_error = 0; /* cleared once read */
				break;
			case SO_SNDBUF:
				*mtod(m, int*) = so->so_snd.sb_hiwat;
				break;
			case SO_RCVBUF:
				*mtod(m, int*) = so->so_rcv.sb_hiwat;
				break;
			case SO_SNDLOWAT:
				*mtod(m, int*) = so->so_snd.sb_lowat;
				break;
			case SO_RCVLOWAT:
				*mtod(m, int*) = so->so_rcv.sb_lowat;
				break;
			case SO_SNDTIMEO:
			case SO_RCVTIMEO:
			{
				int32 val = (optnum == SO_SNDTIMEO ? so->so_snd.sb_timeo : so->so_rcv.sb_timeo);
				
				m->m_len = sizeof(struct timeval);
				mtod(m, struct timeval*)->tv_sec = val / 1000000;
				mtod(m, struct timeval*)->tv_usec = val % 1000000;
				break;
			}
			default:
				m_free(m);
				return ENOPROTOOPT;
		}
		if (m->m_len > *datalen)
			/* XXX - horrible fudge... */
			m->m_len = *datalen;
		memcpy(data, mtod(m, void *), m->m_len);
		*datalen = m->m_len;
		return 0;
	}
}


int set_socket_event_callback(void * sp, socket_event_callback cb, void * cookie, int event)
{
	struct socket *so = (struct socket *) sp;

	so->event_callback = cb;
	so->event_callback_cookie = cookie;
	if (cb) {
		if (event == 3)
			so->sel_ev |= SEL_EX;
		else
			so->sel_ev |= event;
	} else {
		if (event == 3)
			so->sel_ev &= ~SEL_EX;
		else
			so->sel_ev &= ~event;
	}	
	checkevent(so);	/* notify any event condition ASAP! */
	return B_OK;
}


static int checkevent(struct socket *so)
{
	if (!so || !so->event_callback)
		return B_OK;

	/* XXX - The checks here may seem a bit excessive, but we have a race
	 * condition here. If we're checking for soreadable() in one
	 * thread while we're closing in another, things will blow up as
	 * so->event_callback will have been set to NULL.
	 *
	 * ??? Maybe we should lock the socket for things like this?
	 */
	if (so && (so->sel_ev & SEL_READ) && soreadable(so)) {
		if (so->event_callback)
			so->event_callback(so, 1, so->event_callback_cookie);
		so->sel_ev &= ~SEL_READ;
	}
	if (so && (so->sel_ev & SEL_WRITE) && sowriteable(so)) {
		if (so->event_callback)
			so->event_callback(so, 2, so->event_callback_cookie);
		so->sel_ev &= ~SEL_WRITE;
	}
	if (so && (so->sel_ev & SEL_EX) && 
	    (so->so_oobmark || (so->so_state & SS_RCVATMARK))) {
		if (so->event_callback)
			so->event_callback(so, 3, so->event_callback_cookie);
		so->sel_ev &= ~SEL_EX;
	}
	
	return B_OK;
}

void sowakeup(struct socket *so, struct sockbuf *sb)
{
	if (!so || !sb)
		return;
		
	sb->sb_flags &= ~SB_SEL;
	if (sb->sb_flags & SB_WAIT) {
		sb->sb_flags &= ~SB_WAIT;
		/* release the lock here... */
		release_sem_etc(sb->sb_pop, 1, B_CAN_INTERRUPT);
	}
	checkevent(so);
}	

void soqinsque(struct socket *head, struct socket *so, int q)
{
	struct socket **prev;
	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;
		so->so_q0 = 0;
		for (prev = &(head->so_q0); *prev; )
			prev = &((*prev)->so_q0);
	} else {
		head->so_qlen++;
		so->so_q = NULL;
		for (prev = &(head->so_q); *prev; )
			prev = &((*prev)->so_q);
	}
	*prev = so;
}

int soqremque(struct socket *so, int q)
{
	struct socket *head, *prev, *next;

	head = so->so_head;
	prev = head;
	for (;;) {
		next = q ? prev->so_q : prev->so_q0;
		if (next == so)
			break;
		if (next == NULL)
			return 0;
		prev = next;
	}
	if (q == 0) {
		prev->so_q0 = next->so_q0;
		head->so_q0len--;
	} else {
		prev->so_q = next->so_q;
		head->so_qlen--;
	}
	next->so_q0 = next->so_q = 0;
	next->so_head = 0;
	return 1;
}

void sohasoutofband(struct socket *so)
{
	/* Should we signal the process with SIGURG??? */
	checkevent(so);
}

void socantsendmore(struct socket *so)
{
	so->so_state |= SS_CANTSENDMORE;
	sowwakeup(so);
}

void socantrcvmore(struct socket *so)
{
	so->so_state |= SS_CANTRCVMORE;
	sorwakeup(so);
}

void soisconnecting(struct socket *so)
{
	so->so_state &= ~(SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;
}

void soisconnected(struct socket *so)
{
	struct socket *head = so->so_head;

	so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING|SS_ISCONFIRMING);
	so->so_state |= SS_ISCONNECTED;
	if (head && soqremque(so, 0)) {
		soqinsque(head, so, 1);
		sorwakeup(head);
		wakeup(head->so_timeo);
	} else {
		wakeup(so->so_timeo);
		sorwakeup(so);
		sowwakeup(so);
	}
}

void soisdisconnecting(struct socket *so)
{
        so->so_state &= ~SS_ISCONNECTING;
        so->so_state |= (SS_ISDISCONNECTING|SS_CANTRCVMORE|SS_CANTSENDMORE);
        wakeup(so->so_timeo);
        sowwakeup(so);
        sorwakeup(so);
}

void soisdisconnected(struct socket *so)
{
	so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE|SS_ISDISCONNECTED);
	wakeup(so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

int nsleep(sem_id chan, char *msg, int timeo)
{
	status_t rv;
//	printf("nsleep: %s (%ld)\n", msg, chan);

	if (timeo > 0)
		rv = acquire_sem_etc(chan, 1, B_TIMEOUT|B_CAN_INTERRUPT, timeo);
	else
		rv = acquire_sem_etc(chan, 1, B_CAN_INTERRUPT, 0);

	if (rv == B_TIMED_OUT) {
		printf ("nsleep: EWOULDBLOCK\n");
		return EWOULDBLOCK;
	}
	if (rv == B_INTERRUPTED)
		return EINTR;
	return 0;
}

void wakeup(sem_id chan)
{
	/* we should release as many as are waiting...
	 * the number 100 is just something that shuld be large enough...
	 */
	release_sem_etc(chan, 100, B_CAN_INTERRUPT | B_DO_NOT_RESCHEDULE);
}

/* This file is too big - split it up!! */
int sogetsockname(void *sp, struct sockaddr *sa, int *alen)
{
	struct socket *so = (struct socket*)sp;
	struct mbuf *m = m_getclr(MT_SONAME);
	int len;
	int error;

	if (!m)
		return ENOBUFS;
	memcpy(&len, alen, sizeof(len));
	error = (*so->so_proto->pr_userreq)(so, PRU_SOCKADDR, NULL, m, NULL);
	if (error == 0) {
		if (len > m->m_len)
			len = m->m_len;
		memcpy(sa, mtod(m, caddr_t), len);
		memcpy(alen, &len, sizeof(len));
	}
	m_freem(m);
	return error;
}

int sogetpeername(void *sp, struct sockaddr *sa, int * alen)
{
        struct socket *so = (struct socket*)sp;
        struct mbuf *m = m_getclr(MT_SONAME);
        int len;
        int error;

        if (!m)
                return ENOBUFS;
	if ((so->so_state & (SS_ISCONNECTED|SS_ISCONFIRMING)) == 0)
		return ENOTCONN;
        memcpy(&len, alen, sizeof(len));
        error = (*so->so_proto->pr_userreq)(so, PRU_PEERADDR, NULL, m, NULL);
        if (error == 0) {
                if (len > m->m_len)
                        len = m->m_len;
                memcpy(sa, mtod(m, caddr_t), len);
                memcpy(alen, &len, sizeof(len));
        }
        m_freem(m);
        return error;
}

int soaccept(void *sp, void **nsp, void *data, int *alen)
{
	struct socket * so = (struct socket *)sp;
	int len;
	int error;
	struct mbuf *nam;
	
	if (data)
		memcpy(&len, alen, sizeof(len));
	if ((so->so_options & SO_ACCEPTCONN) == 0)
		return EINVAL;
	if ((so->so_state & SS_NBIO) && so->so_qlen == 0)
		return EWOULDBLOCK;
	while (so->so_qlen == 0 && so->so_error == 0) {
		if (so->so_state & SS_CANTRCVMORE) {
			so->so_error = ECONNABORTED;
			break;
		}
		if ((error = nsleep(so->so_timeo, "soaccept", 0)))
			return error;
	}
	if (so->so_error) {
		error = so->so_error;
		so->so_error = 0;
		return error;
	}

	{
		struct socket *aso = so->so_q;
		if (soqremque(aso, 1) == 0) {
			printf("PANIC: soaccept!\n");
			return ENOMEM;
		}
		so = aso;
	}
	nam = m_get(MT_SONAME);
	so->so_state &= ~SS_NOFDREF;
	error = (*so->so_proto->pr_userreq)(so, PRU_ACCEPT, NULL, nam, NULL);
	if (data) {
		if (len > nam->m_len)
			len = nam->m_len;
		if (memcpy(data, mtod(nam, caddr_t), len) == NULL)
			memcpy(alen, (caddr_t)&len, sizeof(len));
	}
	m_freem(nam);
	
	/* assign the socket to the cookie passed in! */
	*nsp = so;			
	return error;
}
