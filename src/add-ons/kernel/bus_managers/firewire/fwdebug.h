/* Kernel driver for firewire 
 *
 * Copyright (C) 2007 JiSheng Zhang <jszhang3@gmail.com>. All rights reserved 
 * Distributed under the terms of the MIT license.
 */

#ifndef __FWDEBUG_H
#define __FWDEBUG_H

#include <KernelExport.h>
#define DEBUG 1

#ifdef DEBUG
	#define TRACE(a...) dprintf("firewire: " a)
#else
	#define TRACE(a...)
#endif

#define ERROR(a...) dprintf("firewire: ERROR " a)

#endif
