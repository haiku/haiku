/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_MBUF_H_
#define _OBSD_COMPAT_SYS_MBUF_H_

#include <sys/systm.h>

/* FreeBSD KASSERT */
#undef KASSERT
#define KASSERT KASSERT_FREEBSD

#include_next <sys/mbuf.h>

/* back to OpenBSD KASSERT */
#undef KASSERT
#define KASSERT KASSERT_OPENBSD

#include <sys/mutex.h>


#define ph_cookie PH_loc.ptr

#define M_DATABUF(m)	M_START(m)
#define M_READONLY(m)	(!M_WRITABLE(m))

#define MAXMCLBYTES MJUM16BYTES

#define MCLGETL m_cljget

struct mbuf_list {
	struct mbuf		*ml_head;
	struct mbuf		*ml_tail;
	u_int			ml_len;
};

#define MBUF_LIST_INITIALIZER() { NULL, NULL, 0 }
#define	ml_len(_ml)		((_ml)->ml_len)
#define	ml_empty(_ml)		((_ml)->ml_len == 0)

struct mbuf_queue {
	struct mutex		mq_mtx;
	struct mbuf_list	mq_list;
	u_int			mq_maxlen;
	u_int			mq_drops;
};


/* FreeBSD methods not compatible with their OpenBSD counterparts */
#define m_defrag(mbuf, how) __m_defrag_unimplemented()


#include "mbuf-obsd.h"


#endif	/* _OBSD_COMPAT_SYS_MBUF_H_ */
