/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_NET_IF_VAR_H_
#define _OBSD_COMPAT_NET_IF_VAR_H_


#include_next <net/if_var.h>


static inline int
if_input_openbsd(if_t ifp, struct mbuf_list* ml)
{
	if (ml_empty(ml))
		return 0;

	struct mbuf* mb = ml->ml_head, *next = NULL;
	int status = 0;
	while (mb != NULL) {
		// if_input takes only the first packet, it ignores mb->m_nextpkt.
		next = mb->m_nextpkt;
		int status = if_input(ifp, mb);
		if (status != 0)
			break;

		mb = next;
		next = NULL;
	}

	if (next != NULL)
		m_freem(next);
	ml_init(ml);
	return status;
}
#define if_input if_input_openbsd


static int
ifq_enqueue(struct ifaltq *ifq, struct mbuf *m)
{
	IF_ENQUEUE(ifq, m);
	return 0;
}

static struct mbuf*
ifq_dequeue(struct ifaltq *ifq)
{
	struct mbuf* m = NULL;
	IF_DEQUEUE(ifq, m);
	return m;
}


#endif	/* _OBSD_COMPAT_NET_IF_VAR_H_ */
