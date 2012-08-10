/*
 * Copyright 2009, Colin Günther. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_MACHINE__BUS_H_
#define _FBSD_COMPAT_MACHINE__BUS_H_


#ifdef B_HAIKU_64_BIT


typedef uint64_t bus_addr_t;
typedef uint64_t bus_size_t;

typedef uint64_t bus_space_tag_t;
typedef uint64_t bus_space_handle_t;


#else


typedef uint32_t bus_addr_t;
typedef uint32_t bus_size_t;

typedef int bus_space_tag_t;
typedef unsigned int bus_space_handle_t;


#endif

#endif /* _FBSD_COMPAT_MACHINE__BUS_H_ */
