/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMMPAGE_H
#define _KERNEL_COMMPAGE_H

#include <image.h>
#include <SupportDefs.h>

#include <commpage_defs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t	commpage_init(void);
status_t	commpage_init_post_cpus(void);
void*		allocate_commpage_entry(int entry, size_t size);
addr_t		fill_commpage_entry(int entry, const void* copyFrom, size_t size);
image_id	get_commpage_image();
area_id		clone_commpage_area(team_id team, void** address);

// implemented in the architecture specific part
status_t	arch_commpage_init(void);
status_t	arch_commpage_init_post_cpus(void);

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	/* _KERNEL_COMMPAGE_H */
