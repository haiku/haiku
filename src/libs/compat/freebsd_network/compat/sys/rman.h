/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_RMAN_H_
#define _FBSD_COMPAT_SYS_RMAN_H_


#include <sys/bus.h>
#include <machine/resource.h>


#define RF_ACTIVE		0x0002
#define RF_SHAREABLE	0x0004
#define	RF_OPTIONAL		0x0080

struct resource {
	int					r_type;
	bus_space_tag_t		r_bustag;		/* bus_space tag */
	bus_space_handle_t	r_bushandle;	/* bus_space handle */
	area_id				r_mapped_area;
};

bus_space_handle_t rman_get_bushandle(struct resource *);
bus_space_tag_t rman_get_bustag(struct resource *);

#endif	/* _FBSD_COMPAT_SYS_RMAN_H_ */
