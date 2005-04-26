/* Device hooks for SiS 900 networking
 *
 * Copyright © 2001-2005 pinc Software. All Rights Reserved.
 */
#ifndef DEVICE_H
#define DEVICE_H

#include <Drivers.h>

#define ETHER_TRANSMIT_TIMEOUT ((bigtime_t)5000000)
	/* five seconds */

/* The published PCI bus interface device hooks */
extern device_hooks gDeviceHooks;

#endif	/* DEVICE_H */
