/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_TIME_H_
#define _FBSD_COMPAT_SYS_TIME_H_


#include <posix/sys/time.h>

#include <sys/_timeval.h>
#include <sys/types.h>


#define time_uptime (system_time() / 1000000)


int	ppsratecheck(struct timeval*, int*, int);

#endif /* _FBSD_COMPAT_SYS_TIME_H_ */
