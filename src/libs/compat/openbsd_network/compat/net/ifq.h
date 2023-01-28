/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_NET_IFQ_H_
#define _OBSD_COMPAT_NET_IFQ_H_


#define ifq_purge(IFQ)				IFQ_PURGE(IFQ)
#define ifq_set_maxlen(IFQ, LEN)	IFQ_SET_MAXLEN(IFQ, LEN)

#define ifq_is_oactive(IFQ) ((if_getdrvflags(ifp) & IFF_DRV_OACTIVE) != 0)
#define ifq_set_oactive(IFQ) if_setdrvflagbits(ifp, IFF_DRV_OACTIVE, 0)
#define ifq_clr_oactive(IFQ) if_setdrvflagbits(ifp, 0, IFF_DRV_OACTIVE)


static void
ifq_serialize(struct ifaltq* ifq, struct task* t)
{
	task_add(systq, t);
}


static void
ifq_barrier(struct ifaltq* ifq)
{
	taskqueue_drain_all(taskqueue_fast);
}


#endif	/* _OBSD_COMPAT_NET_IFQ_H_ */
