/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
 */


#ifndef _UTIL_H_
#define _UTIL_H_

area_id alloc_mem(physical_entry *phy, addr_t *log, size_t size,
	const char *name);
cpu_status lock(void);
void unlock(cpu_status status);

#endif
