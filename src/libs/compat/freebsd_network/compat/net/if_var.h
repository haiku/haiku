/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	From: @(#)if.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/net/if_var.h,v 1.98.2.6 2006/10/06 20:26:05 andre Exp $
 */

#ifndef	_FBSD_COMPAT_NET_IF_VAR_H_
#define	_FBSD_COMPAT_NET_IF_VAR_H_

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with three parameters:
 *	(*ifp->if_output)(ifp, m, dst, rt)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of an internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating an interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */

#ifdef __STDC__
/*
 * Forward structure declarations for function prototypes [sic].
 */
struct	mbuf;
struct	thread;
struct	rtentry;
struct	rt_addrinfo;
struct	socket;
struct	ether_header;
struct	carp_if;
struct	route;
#endif

#include <posix/net/if_dl.h>

#include <sys/queue.h>		/* get TAILQ macros */

#ifdef _KERNEL
#include <sys/mbuf.h>
#include <sys/eventhandler.h>
#endif /* _KERNEL */
#include <sys/counter.h>
#include <sys/lock.h>		/* XXX */
#include <sys/mutex.h>		/* XXX */
#include <sys/event.h>		/* XXX */
#include <sys/buf_ring.h>
#include <sys/_task.h>

#define	IF_DUNIT_NONE	-1

#include <altq/if_altq.h>

typedef enum {
	IFCOUNTER_IPACKETS = 0,
	IFCOUNTER_IERRORS,
	IFCOUNTER_OPACKETS,
	IFCOUNTER_OERRORS,
	IFCOUNTER_COLLISIONS,
	IFCOUNTER_IBYTES,
	IFCOUNTER_OBYTES,
	IFCOUNTER_IMCASTS,
	IFCOUNTER_OMCASTS,
	IFCOUNTER_IQDROPS,
	IFCOUNTER_OQDROPS,
	IFCOUNTER_NOPROTO,
	IFCOUNTERS /* Array size. */
} ift_counter;

TAILQ_HEAD(ifnethead, ifnet);	/* we use TAILQs so that the order of */
TAILQ_HEAD(ifaddrhead, ifaddr);	/* instantiation is preserved in the list */
TAILQ_HEAD(ifprefixhead, ifprefix);
TAILQ_HEAD(ifmultihead, ifmultiaddr);

typedef struct ifnet * if_t;

typedef	void (*if_start_fn_t)(if_t);
typedef	int (*if_ioctl_fn_t)(if_t, u_long, caddr_t);
typedef	void (*if_init_fn_t)(void *);
typedef void (*if_qflush_fn_t)(if_t);
typedef int (*if_transmit_fn_t)(if_t, struct mbuf *);
typedef	uint64_t (*if_get_counter_t)(if_t, ift_counter);

struct ifnet_hw_tsomax {
	u_int	tsomaxbytes;	/* TSO total burst length limit in bytes */
	u_int	tsomaxsegcount;	/* TSO maximum segment count */
	u_int	tsomaxsegsize;	/* TSO maximum segment size in bytes */
};

/* Interface encap request types */
typedef enum {
	IFENCAP_LL = 1			/* pre-calculate link-layer header */
} ife_type;

/*
 * The structure below allows to request various pre-calculated L2/L3 headers
 * for different media. Requests varies by type (rtype field).
 *
 * IFENCAP_LL type: pre-calculates link header based on address family
 *   and destination lladdr.
 *
 *   Input data fields:
 *     buf: pointer to destination buffer
 *     bufsize: buffer size
 *     flags: IFENCAP_FLAG_BROADCAST if destination is broadcast
 *     family: address family defined by AF_ constant.
 *     lladdr: pointer to link-layer address
 *     lladdr_len: length of link-layer address
 *     hdata: pointer to L3 header (optional, used for ARP requests).
 *   Output data fields:
 *     buf: encap data is stored here
 *     bufsize: resulting encap length is stored here
 *     lladdr_off: offset of link-layer address from encap hdr start
 *     hdata: L3 header may be altered if necessary
 */

struct if_encap_req {
	u_char		*buf;		/* Destination buffer (w) */
	size_t		bufsize;	/* size of provided buffer (r) */
	ife_type	rtype;		/* request type (r) */
	uint32_t	flags;		/* Request flags (r) */
	int		family;		/* Address family AF_* (r) */
	int		lladdr_off;	/* offset from header start (w) */
	int		lladdr_len;	/* lladdr length (r) */
	char		*lladdr;	/* link-level address pointer (r) */
	char		*hdata;		/* Upper layer header data (rw) */
};


/*
 * Structure defining a queue for a network interface.
 */
struct	ifqueue {
	struct	mbuf *ifq_head;
	struct	mbuf *ifq_tail;
	int	ifq_len;
	int	ifq_maxlen;
	int	ifq_drops;
	struct	mtx ifq_mtx;
};

struct device;

/*
 * Structure defining a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 */

struct ifnet {
	void	*if_softc;		/* pointer to driver state */
	void	*if_l2com;		/* pointer to protocol bits */
	TAILQ_ENTRY(ifnet) if_link; 	/* all struct ifnets are chained */
	char	if_xname[IFNAMSIZ];	/* external name (name + unit) */
	const char *if_dname;		/* driver name */
	int	if_dunit;		/* unit or IF_DUNIT_NONE */
	struct	ifaddrhead if_addrhead;	/* linked list of addresses per if */
		/*
		 * if_addrhead is the list of all addresses associated to
		 * an interface.
		 * Some code in the kernel assumes that first element
		 * of the list has type AF_LINK, and contains sockaddr_dl
		 * addresses which store the link-level address and the name
		 * of the interface.
		 * However, access to the AF_LINK address through this
		 * field is deprecated. Use ifaddr_byindex() instead.
		 */
	struct	knlist if_klist;	/* events attached to this if */
	int	if_pcount;		/* number of promiscuous listeners */
	struct	carp_if *if_carp;	/* carp interface structure */
	struct	bpf_if *if_bpf;		/* packet filter structure */
	u_short	if_index;		/* numeric abbreviation for this if  */
	short	if_timer;		/* time 'til if_watchdog called */
	struct  ifvlantrunk *if_vlantrunk; /* pointer to 802.1q data */
	int	if_flags;		/* up/down, broadcast, etc. */
	int	if_capabilities;	/* interface capabilities */
	int	if_capenable;		/* enabled features */
	void	*if_linkmib;		/* link-type-specific MIB data */
	size_t	if_linkmiblen;		/* length of above data */
	struct	if_data if_data;
	struct	ifmultihead if_multiaddrs; /* multicast addresses configured */
	int	if_amcount;		/* number of all-multicast requests */
	struct	ifaddr	*if_addr;	/* pointer to link-level address */
/* procedure handles */
	int	(*if_output)		/* output routine (enqueue) */
		(struct ifnet *, struct mbuf *, struct sockaddr *,
		     struct route *);
	void	(*if_input)		/* input routine (from h/w driver) */
		(struct ifnet *, struct mbuf *);
	void	(*if_start)		/* initiate output routine */
		(struct ifnet *);
	int	(*if_ioctl)		/* ioctl routine */
		(struct ifnet *, u_long, caddr_t);
	void	(*if_watchdog)		/* timer routine */
		(struct ifnet *);
	void	(*if_init)		/* Init routine */
		(void *);
	int	(*if_resolvemulti)	/* validate/resolve multicast */
		(struct ifnet *, struct sockaddr **, struct sockaddr *);
	int	(*if_transmit)		/* initiate output routine */
		(struct ifnet *, struct mbuf *);
	void	*if_spare1;		/* spare pointer 1 */
	void	*if_spare2;		/* spare pointer 2 */
	void	*if_spare3;		/* spare pointer 3 */
	int	if_drv_flags;		/* driver-managed status flags */
	u_int	if_spare_flags2;	/* spare flags 2 */
	struct  ifaltq if_snd;		/* output queue (includes altq) */
	const u_int8_t *if_broadcastaddr; /* linklevel broadcast bytestring */

	void	*if_bridge;		/* bridge glue */

	struct	lltable *lltables;	/* list of L3-L2 resolution tables */

	struct	label *if_label;	/* interface MAC label */

	/* these are only used by IPv6 */
	struct	ifprefixhead if_prefixhead; /* list of prefixes per if */
	void	*if_afdata[AF_MAX];
	int	if_afdata_initialized;
	struct	mtx if_afdata_mtx;
	struct	task if_starttask;	/* task for IFF_NEEDSGIANT */
	struct	task if_linktask;	/* task for link change events */
	struct	mtx if_addr_mtx;	/* mutex to protect address lists */

	if_qflush_fn_t	if_qflush;	/* flush any queue */
	if_get_counter_t if_get_counter; /* get counter values */
	int	(*if_requestencap)	/* make link header from request */
		(struct ifnet *, struct if_encap_req *);

	/*
	 * Network adapter TSO limits:
	 * ===========================
	 *
	 * If the "if_hw_tsomax" field is zero the maximum segment
	 * length limit does not apply. If the "if_hw_tsomaxsegcount"
	 * or the "if_hw_tsomaxsegsize" field is zero the TSO segment
	 * count limit does not apply. If all three fields are zero,
	 * there is no TSO limit.
	 *
	 * NOTE: The TSO limits should reflect the values used in the
	 * BUSDMA tag a network adapter is using to load a mbuf chain
	 * for transmission. The TCP/IP network stack will subtract
	 * space for all linklevel and protocol level headers and
	 * ensure that the full mbuf chain passed to the network
	 * adapter fits within the given limits.
	 */
	u_int	if_hw_tsomax;		/* TSO maximum size in bytes */
	u_int	if_hw_tsomaxsegcount;	/* TSO maximum segment count */
	u_int	if_hw_tsomaxsegsize;	/* TSO maximum segment size in bytes */

	/* Haiku additions */
	struct sockaddr_dl	if_lladdr;
	char				device_name[128];
	struct device		*root_device;
	struct ifqueue		receive_queue;
	sem_id				receive_sem;
	sem_id				link_state_sem;
	int32				open_count;
	int32				flags;

	/* WLAN specific additions */
	sem_id				scan_done_sem;
};

typedef void if_init_f_t(void *);

/*
 * XXX These aliases are terribly dangerous because they could apply
 * to anything.
 */
#define	if_mtu		if_data.ifi_mtu
#define	if_type		if_data.ifi_type
#define if_physical	if_data.ifi_physical
#define	if_addrlen	if_data.ifi_addrlen
#define	if_hdrlen	if_data.ifi_hdrlen
#define	if_metric	if_data.ifi_metric
#define	if_link_state	if_data.ifi_link_state
#define	if_baudrate	if_data.ifi_baudrate
#define	if_hwassist	if_data.ifi_hwassist
#define	if_ipackets	if_data.ifi_ipackets
#define	if_ierrors	if_data.ifi_ierrors
#define	if_opackets	if_data.ifi_opackets
#define	if_oerrors	if_data.ifi_oerrors
#define	if_collisions	if_data.ifi_collisions
#define	if_ibytes	if_data.ifi_ibytes
#define	if_obytes	if_data.ifi_obytes
#define	if_imcasts	if_data.ifi_imcasts
#define	if_omcasts	if_data.ifi_omcasts
#define	if_iqdrops	if_data.ifi_iqdrops
#define	if_oqdrops	if_data.ifi_oqdrops
#define	if_noproto	if_data.ifi_noproto
#define	if_lastchange	if_data.ifi_lastchange
#define if_recvquota	if_data.ifi_recvquota
#define	if_xmitquota	if_data.ifi_xmitquota
#define if_rawoutput(if, m, sa) if_output(if, m, sa, (struct route *)NULL)

/* for compatibility with other BSDs */
#define	if_addrlist	if_addrhead
#define	if_list		if_link

/*
 * Locks for address lists on the network interface.
 */
#define	IF_ADDR_LOCK_INIT(if)	mtx_init(&(if)->if_addr_mtx,		\
				    "if_addr_mtx", NULL, MTX_DEF)
#define	IF_ADDR_LOCK_DESTROY(if)	mtx_destroy(&(if)->if_addr_mtx)
#define	IF_ADDR_LOCK(if)	mtx_lock(&(if)->if_addr_mtx)
#define	IF_ADDR_UNLOCK(if)	mtx_unlock(&(if)->if_addr_mtx)
#define	IF_ADDR_LOCK_ASSERT(if)	mtx_assert(&(if)->if_addr_mtx, MA_OWNED)

void	if_addr_rlock(struct ifnet *ifp);	/* if_addrhead */
void	if_addr_runlock(struct ifnet *ifp);	/* if_addrhead */
void	if_maddr_rlock(struct ifnet *ifp);	/* if_multiaddrs */
void	if_maddr_runlock(struct ifnet *ifp);	/* if_multiaddrs */

/*
 * Output queues (ifp->if_snd) and slow device input queues (*ifp->if_slowq)
 * are queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splimp().
 */
#define IF_LOCK(ifq)		mtx_lock(&(ifq)->ifq_mtx)
#define IF_UNLOCK(ifq)		mtx_unlock(&(ifq)->ifq_mtx)
#define	IF_LOCK_ASSERT(ifq)	mtx_assert(&(ifq)->ifq_mtx, MA_OWNED)
#define	_IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define	_IF_DROP(ifq)		((ifq)->ifq_drops++)
#define	_IF_QLEN(ifq)		((ifq)->ifq_len)

#define	_IF_ENQUEUE(ifq, m) do { 				\
	(m)->m_nextpkt = NULL;					\
	if ((ifq)->ifq_tail == NULL) 				\
		(ifq)->ifq_head = m; 				\
	else 							\
		(ifq)->ifq_tail->m_nextpkt = m; 		\
	(ifq)->ifq_tail = m; 					\
	(ifq)->ifq_len++; 					\
} while (0)

#define IF_ENQUEUE(ifq, m) do {					\
	IF_LOCK(ifq); 						\
	_IF_ENQUEUE(ifq, m); 					\
	IF_UNLOCK(ifq); 					\
} while (0)

#define	_IF_PREPEND(ifq, m) do {				\
	(m)->m_nextpkt = (ifq)->ifq_head; 			\
	if ((ifq)->ifq_tail == NULL) 				\
		(ifq)->ifq_tail = (m); 				\
	(ifq)->ifq_head = (m); 					\
	(ifq)->ifq_len++; 					\
} while (0)

#define IF_PREPEND(ifq, m) do {		 			\
	IF_LOCK(ifq); 						\
	_IF_PREPEND(ifq, m); 					\
	IF_UNLOCK(ifq); 					\
} while (0)

#define	_IF_DEQUEUE(ifq, m) do { 				\
	(m) = (ifq)->ifq_head; 					\
	if (m) { 						\
		if (((ifq)->ifq_head = (m)->m_nextpkt) == NULL)	\
			(ifq)->ifq_tail = NULL; 		\
		(m)->m_nextpkt = NULL; 				\
		(ifq)->ifq_len--; 				\
	} 							\
} while (0)

#define IF_DEQUEUE(ifq, m) do { 				\
	IF_LOCK(ifq); 						\
	_IF_DEQUEUE(ifq, m); 					\
	IF_UNLOCK(ifq); 					\
} while (0)

#define	_IF_POLL(ifq, m)	((m) = (ifq)->ifq_head)
#define	IF_POLL(ifq, m)		_IF_POLL(ifq, m)

#define _IF_DRAIN(ifq) do { 					\
	struct mbuf *m; 					\
	for (;;) { 						\
		_IF_DEQUEUE(ifq, m); 				\
		if (m == NULL) 					\
			break; 					\
		m_freem(m); 					\
	} 							\
} while (0)

#define IF_DRAIN(ifq) do {					\
	IF_LOCK(ifq);						\
	_IF_DRAIN(ifq);						\
	IF_UNLOCK(ifq);						\
} while(0)

#ifdef _KERNEL
/* interface address change event */
typedef void (*ifaddr_event_handler_t)(void *, struct ifnet *);
EVENTHANDLER_DECLARE(ifaddr_event, ifaddr_event_handler_t);
/* new interface arrival event */
typedef void (*ifnet_arrival_event_handler_t)(void *, struct ifnet *);
EVENTHANDLER_DECLARE(ifnet_arrival_event, ifnet_arrival_event_handler_t);
/* interface departure event */
typedef void (*ifnet_departure_event_handler_t)(void *, struct ifnet *);
EVENTHANDLER_DECLARE(ifnet_departure_event, ifnet_departure_event_handler_t);

#define	IF_AFDATA_LOCK_INIT(ifp)	\
    mtx_init(&(ifp)->if_afdata_mtx, "if_afdata", NULL, MTX_DEF)
#define	IF_AFDATA_LOCK(ifp)	mtx_lock(&(ifp)->if_afdata_mtx)
#define	IF_AFDATA_TRYLOCK(ifp)	mtx_trylock(&(ifp)->if_afdata_mtx)
#define	IF_AFDATA_UNLOCK(ifp)	mtx_unlock(&(ifp)->if_afdata_mtx)
#define	IF_AFDATA_DESTROY(ifp)	mtx_destroy(&(ifp)->if_afdata_mtx)

#define	IFF_LOCKGIANT(ifp) do {						\
	if ((ifp)->if_flags & IFF_NEEDSGIANT)				\
		mtx_lock(&Giant);					\
} while (0)

#define	IFF_UNLOCKGIANT(ifp) do {					\
	if ((ifp)->if_flags & IFF_NEEDSGIANT)				\
		mtx_unlock(&Giant);					\
} while (0)

int	if_handoff(struct ifqueue *ifq, struct mbuf *m, struct ifnet *ifp,
	    int adjust);
#define	IF_HANDOFF(ifq, m, ifp)			\
	if_handoff((struct ifqueue *)ifq, m, ifp, 0)
#define	IF_HANDOFF_ADJ(ifq, m, ifp, adj)	\
	if_handoff((struct ifqueue *)ifq, m, ifp, adj)

void	if_start(struct ifnet *);

#define	IFQ_ENQUEUE(ifq, m, err)					\
do {									\
	IF_LOCK(ifq);							\
	if (ALTQ_IS_ENABLED(ifq))					\
		ALTQ_ENQUEUE(ifq, m, NULL, err);			\
	else {								\
		if (_IF_QFULL(ifq)) {					\
			m_freem(m);					\
			(err) = ENOBUFS;				\
		} else {						\
			_IF_ENQUEUE(ifq, m);				\
			(err) = 0;					\
		}							\
	}								\
	if (err)							\
		(ifq)->ifq_drops++;					\
	IF_UNLOCK(ifq);							\
} while (0)

#define	IFQ_DEQUEUE_NOLOCK(ifq, m)					\
do {									\
	if (TBR_IS_ENABLED(ifq))					\
		(m) = tbr_dequeue_ptr(ifq, ALTDQ_REMOVE);		\
	else if (ALTQ_IS_ENABLED(ifq))					\
		ALTQ_DEQUEUE(ifq, m);					\
	else								\
		_IF_DEQUEUE(ifq, m);					\
} while (0)

#define	IFQ_DEQUEUE(ifq, m)						\
do {									\
	IF_LOCK(ifq);							\
	IFQ_DEQUEUE_NOLOCK(ifq, m);					\
	IF_UNLOCK(ifq);							\
} while (0)

#define	IFQ_POLL_NOLOCK(ifq, m)						\
do {									\
	if (TBR_IS_ENABLED(ifq))					\
		(m) = tbr_dequeue_ptr(ifq, ALTDQ_POLL);			\
	else if (ALTQ_IS_ENABLED(ifq))					\
		ALTQ_POLL(ifq, m);					\
	else								\
		_IF_POLL(ifq, m);					\
} while (0)

#define	IFQ_POLL(ifq, m)						\
do {									\
	IF_LOCK(ifq);							\
	IFQ_POLL_NOLOCK(ifq, m);					\
	IF_UNLOCK(ifq);							\
} while (0)

#define	IFQ_PURGE_NOLOCK(ifq)						\
do {									\
	if (ALTQ_IS_ENABLED(ifq)) {					\
		ALTQ_PURGE(ifq);					\
	} else								\
		_IF_DRAIN(ifq);						\
} while (0)

#define	IFQ_PURGE(ifq)							\
do {									\
	IF_LOCK(ifq);							\
	IFQ_PURGE_NOLOCK(ifq);						\
	IF_UNLOCK(ifq);							\
} while (0)

#define	IFQ_SET_READY(ifq)						\
	do { ((ifq)->altq_flags |= ALTQF_READY); } while (0)

#define	IFQ_LOCK(ifq)			IF_LOCK(ifq)
#define	IFQ_UNLOCK(ifq)			IF_UNLOCK(ifq)
#define	IFQ_LOCK_ASSERT(ifq)		IF_LOCK_ASSERT(ifq)
#define	IFQ_IS_EMPTY(ifq)		((ifq)->ifq_len == 0)
#define	IFQ_INC_LEN(ifq)		((ifq)->ifq_len++)
#define	IFQ_DEC_LEN(ifq)		(--(ifq)->ifq_len)
#define	IFQ_INC_DROPS(ifq)		((ifq)->ifq_drops++)
#define	IFQ_SET_MAXLEN(ifq, len)	((ifq)->ifq_maxlen = (len))

/*
 * The IFF_DRV_OACTIVE test should really occur in the device driver, not in
 * the handoff logic, as that flag is locked by the device driver.
 */
#define	IFQ_HANDOFF_ADJ(ifp, m, adj, err)				\
do {									\
	int len;							\
	short mflags;							\
									\
	len = (m)->m_pkthdr.len;					\
	mflags = (m)->m_flags;						\
	IFQ_ENQUEUE(&(ifp)->if_snd, m, err);				\
	if ((err) == 0) {						\
		(ifp)->if_obytes += len + (adj);			\
		if (mflags & M_MCAST)					\
			(ifp)->if_omcasts++;				\
		if (((ifp)->if_drv_flags & IFF_DRV_OACTIVE) == 0)	\
			if_start(ifp);					\
	}								\
} while (0)

#define	IFQ_HANDOFF(ifp, m, err)					\
	IFQ_HANDOFF_ADJ(ifp, m, 0, err)

#define	IFQ_DRV_DEQUEUE(ifq, m)						\
do {									\
	(m) = (ifq)->ifq_drv_head;					\
	if (m) {							\
		if (((ifq)->ifq_drv_head = (m)->m_nextpkt) == NULL)	\
			(ifq)->ifq_drv_tail = NULL;			\
		(m)->m_nextpkt = NULL;					\
		(ifq)->ifq_drv_len--;					\
	} else {							\
		IFQ_LOCK(ifq);						\
		IFQ_DEQUEUE_NOLOCK(ifq, m);				\
		while ((ifq)->ifq_drv_len < (ifq)->ifq_drv_maxlen) {	\
			struct mbuf *m0;				\
			IFQ_DEQUEUE_NOLOCK(ifq, m0);			\
			if (m0 == NULL)					\
				break;					\
			m0->m_nextpkt = NULL;				\
			if ((ifq)->ifq_drv_tail == NULL)		\
				(ifq)->ifq_drv_head = m0;		\
			else						\
				(ifq)->ifq_drv_tail->m_nextpkt = m0;	\
			(ifq)->ifq_drv_tail = m0;			\
			(ifq)->ifq_drv_len++;				\
		}							\
		IFQ_UNLOCK(ifq);					\
	}								\
} while (0)

#define	IFQ_DRV_PREPEND(ifq, m)						\
do {									\
	(m)->m_nextpkt = (ifq)->ifq_drv_head;				\
	if ((ifq)->ifq_drv_tail == NULL)				\
		(ifq)->ifq_drv_tail = (m);				\
	(ifq)->ifq_drv_head = (m);					\
	(ifq)->ifq_drv_len++;						\
} while (0)

#define	IFQ_DRV_IS_EMPTY(ifq)						\
	(((ifq)->ifq_drv_len == 0) && ((ifq)->ifq_len == 0))

#define	IFQ_DRV_PURGE(ifq)						\
do {									\
	struct mbuf *m, *n = (ifq)->ifq_drv_head;			\
	while((m = n) != NULL) {					\
		n = m->m_nextpkt;					\
		m_freem(m);						\
	}								\
	(ifq)->ifq_drv_head = (ifq)->ifq_drv_tail = NULL;		\
	(ifq)->ifq_drv_len = 0;						\
	IFQ_PURGE(ifq);							\
} while (0)

/*
 * 72 was chosen below because it is the size of a TCP/IP
 * header (40) + the minimum mss (32).
 */
#define	IF_MINMTU	72
#define	IF_MAXMTU	65535

#endif /* _KERNEL */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 *
 * NOTE: a 'struct ifaddr' is always at the beginning of a larger
 * chunk of malloc'ed memory, where we store the three addresses
 * (ifa_addr, ifa_dstaddr and ifa_netmask) referenced here.
 */
struct ifaddr {
	struct	sockaddr *ifa_addr;	/* address of interface */
	struct	sockaddr *ifa_dstaddr;	/* other end of p-to-p link */
#define	ifa_broadaddr	ifa_dstaddr	/* broadcast address interface */
	struct	sockaddr *ifa_netmask;	/* used to determine subnet */
	struct	if_data if_data;	/* not all members are meaningful */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	TAILQ_ENTRY(ifaddr) ifa_link;	/* queue macro glue */
	void	(*ifa_rtrequest)	/* check or clean routes (+ or -)'d */
		(int, struct rtentry *, struct rt_addrinfo *);
	u_short	ifa_flags;		/* mostly rt_flags for cloning */
	u_int	ifa_refcnt;		/* references to this structure */
	int	ifa_metric;		/* cost of going out this interface */
	int (*ifa_claim_addr)		/* check if an addr goes to this if */
		(struct ifaddr *, struct sockaddr *);
	struct mtx ifa_mtx;
};
#define	IFA_ROUTE	RTF_UP		/* route installed */

/* for compatibility with other BSDs */
#define	ifa_list	ifa_link


struct ifaddr *	ifa_alloc(size_t size, int flags);
void	ifa_free(struct ifaddr *ifa);
void	ifa_ref(struct ifaddr *ifa);


#define	IFA_LOCK_INIT(ifa)	\
    mtx_init(&(ifa)->ifa_mtx, "ifaddr", NULL, MTX_DEF)
#define	IFA_LOCK(ifa)		mtx_lock(&(ifa)->ifa_mtx)
#define	IFA_UNLOCK(ifa)		mtx_unlock(&(ifa)->ifa_mtx)
#define	IFA_DESTROY(ifa)	mtx_destroy(&(ifa)->ifa_mtx)

/*
 * The prefix structure contains information about one prefix
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when a prefix or an address is set,
 * and are linked together so all prefixes for an interface can be located.
 */
struct ifprefix {
	struct	sockaddr *ifpr_prefix;	/* prefix of interface */
	struct	ifnet *ifpr_ifp;	/* back-pointer to interface */
	TAILQ_ENTRY(ifprefix) ifpr_list; /* queue macro glue */
	u_char	ifpr_plen;		/* prefix length in bits */
	u_char	ifpr_type;		/* protocol dependent prefix type */
};

/*
 * Multicast address structure.  This is analogous to the ifaddr
 * structure except that it keeps track of multicast addresses.
 * Also, the reference count here is a count of requests for this
 * address, not a count of pointers to this structure.
 */
struct ifmultiaddr {
	TAILQ_ENTRY(ifmultiaddr) ifma_link; /* queue macro glue */
	struct	sockaddr *ifma_addr; 	/* address this membership is for */
	struct	sockaddr *ifma_lladdr;	/* link-layer translation, if any */
	struct	ifnet *ifma_ifp;	/* back-pointer to interface */
	u_int	ifma_refcount;		/* reference count */
	void	*ifma_protospec;	/* protocol-specific state, if any */

	/* haiku additions, save a allocation -hugo */
	struct sockaddr_dl ifma_addr_storage;
};

#ifdef _KERNEL
#define	IFA_LOCK(ifa)		mtx_lock(&(ifa)->ifa_mtx)
#define	IFA_UNLOCK(ifa)		mtx_unlock(&(ifa)->ifa_mtx)

extern	struct rw_lock ifnet_rwlock;
#define	IFNET_LOCK_INIT()		rw_lock_init(&ifnet_rwlock, "ifnet rwlock")
#define	IFNET_WLOCK()			rw_lock_write_lock(&ifnet_rwlock)
#define	IFNET_WUNLOCK()			rw_lock_write_unlock(&ifnet_rwlock)
#define	IFNET_RLOCK()			rw_lock_read_lock(&ifnet_rwlock)
#define	IFNET_RLOCK_NOSLEEP()	rw_lock_read_lock(&ifnet_rwlock)
#define	IFNET_RUNLOCK()			rw_lock_read_unlock(&ifnet_rwlock)
#define	IFNET_RUNLOCK_NOSLEEP()	rw_lock_read_unlock(&ifnet_rwlock)

struct ifnet	*ifnet_byindex(u_short idx);
struct ifnet	*ifnet_byindex_locked(u_short idx);

extern	struct ifnethead ifnet;
extern	int ifqmaxlen;
extern	struct ifnet *loif;	/* first loopback interface */
extern	int if_index;

int	if_addmulti(struct ifnet *, struct sockaddr *, struct ifmultiaddr **);
int	if_allmulti(struct ifnet *, int);
struct	ifnet* if_alloc(u_char);
void	if_attach(struct ifnet *);
int	if_delmulti(struct ifnet *, struct sockaddr *);
void	if_detach(struct ifnet *);
void	if_purgeaddrs(struct ifnet *);
void    if_delallmulti(struct ifnet *);
void	if_purgemaddrs(struct ifnet *);
void	if_down(struct ifnet *);
void	if_free(struct ifnet *);
void	if_free_type(struct ifnet *, u_char);
void	if_initname(struct ifnet *, const char *, int);
void	if_link_state_change(struct ifnet *, int);
int	if_printf(struct ifnet *, const char *, ...) __printflike(2, 3);
int	if_setlladdr(struct ifnet *, const u_char *, int);
void	if_up(struct ifnet *);
/*void	ifinit(void);*/ /* declared in systm.h for main() */
int	ifioctl(struct socket *, u_long, caddr_t, struct thread *);
int	ifpromisc(struct ifnet *, int);
struct	ifnet *ifunit(const char *);

struct	ifaddr *ifa_ifwithaddr(struct sockaddr *);
struct	ifaddr *ifa_ifwithbroadaddr(struct sockaddr *);
struct	ifaddr *ifa_ifwithdstaddr(struct sockaddr *);
struct	ifaddr *ifa_ifwithnet(struct sockaddr *);
struct	ifaddr *ifa_ifwithroute(int, struct sockaddr *, struct sockaddr *);
struct	ifaddr *ifaof_ifpforaddr(struct sockaddr *, struct ifnet *);

int	if_simloop(struct ifnet *ifp, struct mbuf *m, int af, int hlen);

typedef	void *if_com_alloc_t(u_char type, struct ifnet *ifp);
typedef	void if_com_free_t(void *com, u_char type);
void	if_register_com_alloc(u_char type, if_com_alloc_t *a, if_com_free_t *f);
void	if_deregister_com_alloc(u_char type);
void	if_data_copy(struct ifnet *, struct if_data *);
uint64_t if_get_counter_default(struct ifnet *, ift_counter);
void	if_inc_counter(struct ifnet *, ift_counter, int64_t);

#define IF_LLADDR(ifp)							\
    LLADDR((struct sockaddr_dl *)((ifp)->if_addr->ifa_addr))

uint64_t if_setbaudrate(if_t ifp, uint64_t baudrate);
uint64_t if_getbaudrate(if_t ifp);
int if_setcapabilities(if_t ifp, int capabilities);
int if_setcapabilitiesbit(if_t ifp, int setbit, int clearbit);
int if_getcapabilities(if_t ifp);
int if_togglecapenable(if_t ifp, int togglecap);
int if_setcapenable(if_t ifp, int capenable);
int if_setcapenablebit(if_t ifp, int setcap, int clearcap);
int if_getcapenable(if_t ifp);
const char *if_getdname(if_t ifp);
int if_setdev(if_t ifp, void *dev);
int if_setdrvflagbits(if_t ifp, int if_setflags, int clear_flags);
int if_getdrvflags(if_t ifp);
int if_setdrvflags(if_t ifp, int flags);
int if_clearhwassist(if_t ifp);
int if_sethwassistbits(if_t ifp, int toset, int toclear);
int if_sethwassist(if_t ifp, int hwassist_bit);
int if_gethwassist(if_t ifp);
int if_setsoftc(if_t ifp, void *softc);
void *if_getsoftc(if_t ifp);
int if_setflags(if_t ifp, int flags);
int if_gethwaddr(if_t ifp, struct ifreq *);
int if_setmtu(if_t ifp, int mtu);
int if_getmtu(if_t ifp);
int if_getmtu_family(if_t ifp, int family);
int if_setflagbits(if_t ifp, int set, int clear);
int if_getflags(if_t ifp);
int if_sendq_empty(if_t ifp);
int if_setsendqready(if_t ifp);
int if_setsendqlen(if_t ifp, int tx_desc_count);
int if_input(if_t ifp, struct mbuf* sendmp);
int if_sendq_prepend(if_t ifp, struct mbuf *m);
struct mbuf *if_dequeue(if_t ifp);
int if_setifheaderlen(if_t ifp, int len);
void if_setrcvif(struct mbuf *m, if_t ifp);

void if_setvtag(struct mbuf *m, u_int16_t tag);
u_int16_t if_getvtag(struct mbuf *m);
int if_vlantrunkinuse(if_t ifp);
caddr_t if_getlladdr(if_t ifp);
void *if_gethandle(u_char);
void if_bpfmtap(if_t ifp, struct mbuf *m);
void if_etherbpfmtap(if_t ifp, struct mbuf *m);
void if_vlancap(if_t ifp);

int if_setupmultiaddr(if_t ifp, void *mta, int *cnt, int max);
int if_multiaddr_array(if_t ifp, void *mta, int *cnt, int max);
int if_multiaddr_count(if_t ifp, int max);

/* Functions */
void if_setinitfn(if_t ifp, void (*)(void *));
void if_setioctlfn(if_t ifp, int (*)(if_t, u_long, caddr_t));
void if_setstartfn(if_t ifp, void (*)(if_t));
void if_settransmitfn(if_t ifp, if_transmit_fn_t);
void if_setqflushfn(if_t ifp, if_qflush_fn_t);
void if_setgetcounterfn(if_t ifp, if_get_counter_t);

/* accessors for struct ifreq */
static inline void*
ifr_data_get_ptr(void* ifrp)
{
	struct ifreq* ifr = (struct ifreq *)ifrp;
	return ifr->ifr_data;
}

#ifdef DEVICE_POLLING
enum poll_cmd {	POLL_ONLY, POLL_AND_CHECK_STATUS };

typedef	void poll_handler_t(struct ifnet *ifp, enum poll_cmd cmd, int count);
int    ether_poll_register(poll_handler_t *h, struct ifnet *ifp);
int    ether_poll_deregister(struct ifnet *ifp);
#endif /* DEVICE_POLLING */

static __inline int
drbr_enqueue(struct ifnet *ifp, struct buf_ring *br, struct mbuf *m)
{
	int error = 0;

#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd)) {
		IFQ_ENQUEUE(&ifp->if_snd, m, error);
		if (error)
			if_inc_counter((ifp), IFCOUNTER_OQDROPS, 1);
		return (error);
	}
#endif
	error = buf_ring_enqueue(br, m);
	if (error)
		m_freem(m);

	return (error);
}

static __inline void
drbr_putback(struct ifnet *ifp, struct buf_ring *br, struct mbuf *_new)
{
	/*
	 * The top of the list needs to be swapped
	 * for this one.
	 */
#ifdef ALTQ
	if (ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd)) {
		/*
		 * Peek in altq case dequeued it
		 * so put it back.
		 */
		IFQ_DRV_PREPEND(&ifp->if_snd, _new);
		return;
	}
#endif
	buf_ring_putback_sc(br, _new);
}

static __inline struct mbuf *
drbr_peek(struct ifnet *ifp, struct buf_ring *br)
{
#ifdef ALTQ
	struct mbuf *m;
	if (ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd)) {
		/*
		 * Pull it off like a dequeue
		 * since drbr_advance() does nothing
		 * for altq and drbr_putback() will
		 * use the old prepend function.
		 */
		IFQ_DEQUEUE(&ifp->if_snd, m);
		return (m);
	}
#endif
	return (struct mbuf*)buf_ring_peek_clear_sc(br);
}

static __inline void
drbr_flush(struct ifnet *ifp, struct buf_ring *br)
{
	struct mbuf *m;

#ifdef ALTQ
	if (ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd))
		IFQ_PURGE(&ifp->if_snd);
#endif
	while ((m = (struct mbuf*)buf_ring_dequeue_sc(br)) != NULL)
		m_freem(m);
}

static __inline void
drbr_free(struct buf_ring *br, struct malloc_type *type)
{

	drbr_flush(NULL, br);
	buf_ring_free(br, type);
}

static __inline struct mbuf *
drbr_dequeue(struct ifnet *ifp, struct buf_ring *br)
{
#ifdef ALTQ
	struct mbuf *m;

	if (ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd)) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		return (m);
	}
#endif
	return (struct mbuf*)buf_ring_dequeue_sc(br);
}

static __inline void
drbr_advance(struct ifnet *ifp, struct buf_ring *br)
{
#ifdef ALTQ
	/* Nothing to do here since peek dequeues in altq case */
	if (ifp != NULL && ALTQ_IS_ENABLED(&ifp->if_snd))
		return;
#endif
	return (buf_ring_advance_sc(br));
}


static __inline struct mbuf *
drbr_dequeue_cond(struct ifnet *ifp, struct buf_ring *br,
    int (*func) (struct mbuf *, void *), void *arg)
{
	struct mbuf *m;
#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd)) {
		IFQ_LOCK(&ifp->if_snd);
		IFQ_POLL_NOLOCK(&ifp->if_snd, m);
		if (m != NULL && func(m, arg) == 0) {
			IFQ_UNLOCK(&ifp->if_snd);
			return (NULL);
		}
		IFQ_DEQUEUE_NOLOCK(&ifp->if_snd, m);
		IFQ_UNLOCK(&ifp->if_snd);
		return (m);
	}
#endif
	m = (struct mbuf*)buf_ring_peek(br);
	if (m == NULL || func(m, arg) == 0)
		return (NULL);

	return (struct mbuf*)buf_ring_dequeue_sc(br);
}

static __inline int
drbr_empty(struct ifnet *ifp, struct buf_ring *br)
{
#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		return (IFQ_IS_EMPTY(&ifp->if_snd));
#endif
	return (buf_ring_empty(br));
}

static __inline int
drbr_needs_enqueue(struct ifnet *ifp, struct buf_ring *br)
{
#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		return (1);
#endif
	return (!buf_ring_empty(br));
}

static __inline int
drbr_inuse(struct ifnet *ifp, struct buf_ring *br)
{
#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		return (ifp->if_snd.ifq_len);
#endif
	return (buf_ring_count(br));
}

#endif /* _KERNEL */

#endif /* _FBSD_COMPAT_NET_IF_VAR_H_ */
