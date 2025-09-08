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
	struct mbuf* m;
	while ((m = ml_dequeue(ml)) != NULL) {
		if_input(ifp, m);
	}

	return 0;
}
#define if_input if_input_openbsd


static int
ifq_enqueue(struct ifaltq *ifq, struct mbuf *m)
{
	IF_ENQUEUE(ifq, m);
	return 0;
}


static struct mbuf*
ifq_dequeue_openbsd(if_t ifp, struct ifaltq *ifq)
{
	struct mbuf* m = NULL;
	IF_DEQUEUE(ifq, m);
	if (m != NULL && ifq == &ifp->if_snd) {
		// OpenBSD drivers don't increment the OPACKETS counter directly,
		// so we do it here.
		if_inc_counter(ifp, IFCOUNTER_OPACKETS, 1);
	}
	return m;
}
#define ifq_dequeue(IFQ) ifq_dequeue_openbsd(ifp, IFQ)


#ifndef __cplusplus
#include "if_var-obsd.h"
#endif


#endif	/* _OBSD_COMPAT_NET_IF_VAR_H_ */
