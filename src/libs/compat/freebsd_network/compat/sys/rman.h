/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_RMAN_H_
#define _FBSD_COMPAT_SYS_RMAN_H_


#include <machine/_bus.h>
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
uint32_t rman_make_alignment_flags(uint32_t);
u_long rman_get_start(struct resource *);

#endif	/* _FBSD_COMPAT_SYS_RMAN_H_ */
