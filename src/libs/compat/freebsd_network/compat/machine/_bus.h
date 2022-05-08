/*
 * Copyright 2009, Colin Günther. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_MACHINE__BUS_H_
#define _FBSD_COMPAT_MACHINE__BUS_H_


#include <SupportDefs.h>


typedef phys_addr_t bus_addr_t;


typedef int bus_space_tag_t;
enum {
	BUS_SPACE_TAG_INVALID = 0,

	BUS_SPACE_TAG_IO,
	BUS_SPACE_TAG_MEM,
	BUS_SPACE_TAG_IRQ,
	BUS_SPACE_TAG_MSI,
};


#ifdef B_HAIKU_64_BIT

typedef uint64_t bus_size_t;
typedef uint64_t bus_space_handle_t;

#else

typedef uint32_t bus_size_t;
typedef unsigned int bus_space_handle_t;

#if defined(__HAIKU_ARCH_PHYSICAL_64_BIT) && defined(__i386__)
#define PAE 1
#endif

#endif


#endif /* _FBSD_COMPAT_MACHINE__BUS_H_ */
