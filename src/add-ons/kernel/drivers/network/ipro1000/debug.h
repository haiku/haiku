/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#include <KernelExport.h>

#ifdef DEBUG

	#define TRACE(a...) dprintf("ipro1000: " a)

	/* used by if_em.h */
	#define DEBUG_INIT  1
	#define DEBUG_IOCTL 1
	#define DEBUG_HW    1
	#define DBG_STATS	1

#else

	#define TRACE(a...)

	/* used by if_em.h */
	#define DEBUG_INIT  0
	#define DEBUG_IOCTL 0
	#define DEBUG_HW    0
	#undef DBG_STATS

#endif

#define ERROR(a...) dprintf("ipro1000: ERROR " a)
#define PRINT(a...) dprintf("ipro1000: " a)

#endif
