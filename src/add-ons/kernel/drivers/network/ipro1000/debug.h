/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#include <KernelExport.h>

#define DEBUG

#ifdef DEBUG
	#define TRACE(a...) dprintf("ipro1000: " a)
#else
	#define TRACE(a...)
#endif

#define ERROR(a...) dprintf("ipro1000: " a)

#endif
