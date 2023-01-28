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
	int status = 0;
	while ((m = ml_dequeue(ml)) != NULL) {
		status = if_input(ifp, m);
		if (status != 0)
			break;
	}

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


#ifndef __cplusplus
#include "if_var-obsd.h"
#endif


#endif	/* _OBSD_COMPAT_NET_IF_VAR_H_ */
