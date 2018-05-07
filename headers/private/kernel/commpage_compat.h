/*
 * Copyright 2018 Haiku Inc. All Rights Reserved.
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMMPAGE_COMPAT_H
#define _KERNEL_COMMPAGE_COMPAT_H


#include <image.h>
#include <SupportDefs.h>

#define COMMPAGE_COMPAT
#include <commpage_defs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t	commpage_compat_init(void);
status_t	commpage_compat_init_post_cpus(void);
void*		allocate_commpage_compat_entry(int entry, size_t size);
addr_t		fill_commpage_compat_entry(int entry, const void* copyFrom,
				size_t size);
image_id	get_commpage_compat_image();
area_id		clone_commpage_compat_area(team_id team, void** address);

// implemented in the architecture specific part
status_t	arch_commpage_compat_init(void);
status_t	arch_commpage_compat_init_post_cpus(void);


#ifdef __cplusplus
}      // extern "C"
#endif


#endif /* _KERNEL_COMMPAGE_COMPAT_H */
