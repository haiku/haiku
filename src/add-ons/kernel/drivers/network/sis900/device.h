#ifndef DEVICE_H
#define DEVICE_H

#include <Drivers.h>

#define ETHER_TRANSMIT_TIMEOUT ((bigtime_t)5000000)  /* five seconds */

/* The published PCI bus interface device hooks */
extern device_hooks gDeviceHooks;

#endif	/* DEVICE_H */
