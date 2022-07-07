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


static int
m_dup_pkthdr_openbsd(struct mbuf* to, const struct mbuf* from, int how)
{
	return !m_dup_pkthdr(to, from, how);
}
#define m_dup_pkthdr m_dup_pkthdr_openbsd

static int
m_tag_copy_chain_openbsd(struct mbuf* to, const struct mbuf* from, int how)
{
	return !m_tag_copy_chain(to, from, how);
}
#define m_tag_copy_chain m_tag_copy_chain_openbsd


/* FreeBSD methods not compatible with their OpenBSD counterparts */
#define m_defrag(mbuf, how) __m_defrag_unimplemented()


#include "mbuf-obsd.h"


#endif	/* _OBSD_COMPAT_SYS_MBUF_H_ */
