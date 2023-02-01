/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_NET_IF_H_
#define _OBSD_COMPAT_NET_IF_H_


#include_next <net/if.h>


#define LINK_STATE_IS_UP(_s)	\
		((_s) >= LINK_STATE_UP || (_s) == LINK_STATE_UNKNOWN)

#define IFF_RUNNING		IFF_DRV_RUNNING


#endif	/* _OBSD_COMPAT_NET_IF_H_ */
