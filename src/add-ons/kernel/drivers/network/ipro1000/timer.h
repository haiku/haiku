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
#ifndef __TIMER_H
#define __TIMER_H

#include <KernelExport.h>

// Since the BeOS kernel timers are executed in interrupt context,
// a new timer has been created. The timers are executed in thread
// context, and are passed a cookie.

typedef int32 timer_id;

typedef void (*timer_function)(void *cookie);

// create_timer() can be called from interrupt context.
// Only up to 4 concurrent timers are supported.
// Flags can be one of B_ONE_SHOT_ABSOLUTE_TIMER,
// B_ONE_SHOT_RELATIVE_TIMER or B_PERIODIC_TIMER.

timer_id	create_timer(timer_function func, void *cookie, bigtime_t interval, uint32 flags);
status_t	delete_timer(timer_id id);


status_t	initialize_timer(void);
status_t	terminate_timer(void);

#endif
