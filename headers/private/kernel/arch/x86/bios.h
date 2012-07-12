/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_x86_BIOS_H
#define ARCH_x86_BIOS_H


#include <SupportDefs.h>


#ifndef __x86_64__

#define BIOS32_PCI_SERVICE	'ICP$'

struct bios32_service {
	uint32	base;
	uint32	size;
	uint32	offset;
};


#ifdef __cplusplus
extern "C" {
#endif

status_t get_bios32_service(uint32 identifier, struct bios32_service *service);
status_t bios_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* __x86_64__ */

#endif	/* ARCH_x86_BIOS_H */
