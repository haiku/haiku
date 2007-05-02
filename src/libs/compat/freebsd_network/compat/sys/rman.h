#ifndef _FBSD_COMPAT_SYS_RMAN_H_
#define _FBSD_COMPAT_SYS_RMAN_H_

#include <sys/bus.h>

#define RF_ACTIVE		0x0002
#define RF_SHAREABLE	0x0004

bus_space_handle_t rman_get_bushandle(struct resource *);
bus_space_tag_t rman_get_bustag(struct resource *);

#endif
