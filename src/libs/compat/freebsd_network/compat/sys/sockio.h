/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_SOCKIO_H_
#define _FBSD_COMPAT_SYS_SOCKIO_H_


#include <posix/sys/sockio.h>

#include <sys/ioccom.h>


#define SIOCSIFCAP	SIOCSPACKETCAP

// TODO look whether those ioctls are already used by Haiku
// and add them to Haiku's sockio.h eventually
#define	SIOCGPRIVATE_0	_IOWR('i', 80, struct ifreq)	/* device private 0 */
#define	SIOCGPRIVATE_1	_IOWR('i', 81, struct ifreq)	/* device private 1 */

#define	SIOCSDRVSPEC	_IOW('i', 123, struct ifdrv)	/* set driver-specific
								  parameters */
#define	SIOCGDRVSPEC	_IOWR('i', 123, struct ifdrv)	/* get driver-specific
								  parameters */

#endif
