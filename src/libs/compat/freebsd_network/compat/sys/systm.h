/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_SYSTM_H_
#define _FBSD_COMPAT_SYS_SYSTM_H_

#include <stdint.h>

#include <sys/callout.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_media.h>

#define DELAY(n) \
	do {				\
		if (n < 1000)	\
			spin(n);	\
		else			\
			snooze(n);	\
	} while (0)

#endif	/* _FBSD_COMPAT_SYS_SYSTM_H_ */
