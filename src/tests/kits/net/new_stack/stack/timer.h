/* timer.h
 * private definitions for network timers support
 */

#ifndef OBOS_NET_STACK_TIMER_H
#define OBOS_NET_STACK_TIMER_H

/* timer.h - a small and more or less inaccurate timer for net modules.
**		The registered hooks will be called in the thread of the timer.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
**
** This file may be used under the terms of the OpenBeOS License.
*/

#include <SupportDefs.h>

#include "net_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

extern status_t		start_timers_service(void);
extern status_t		stop_timers_service(void);

extern net_timer *	new_net_timer(void);
extern status_t 	delete_net_timer(net_timer *nt);
extern status_t		start_net_timer(net_timer *nt, net_timer_func hook, void *cookie, bigtime_t period);
extern status_t		cancel_net_timer(net_timer *nt);
extern status_t		net_timer_appointment(net_timer *nt, bigtime_t *period, bigtime_t *when);

#ifdef __cplusplus
}
#endif

#endif	/* OBOS_NET_STACK_TIMER_H */
