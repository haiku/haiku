/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <KernelExport.h>

area_id alloc_mem(void **phy, void **log, size_t size, const char *name);

cpu_status lock(void);
void unlock(cpu_status status);

#endif
