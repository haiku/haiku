/* Realtek RTL8169 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#include <KernelExport.h>

#ifdef DEBUG
	#define TRACE(a...) dprintf("rtl8169: " a)
#else
	#define TRACE(a...)
#endif

#define ERROR(a...) dprintf("rtl8169: " a)

#endif
