#ifndef NET_TIMER_H
#define NET_TIMER_H
/* net_timer.h - a small and more or less inaccurate timer for net modules.
**		The registered hooks will be called in the thread of the timer.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
**
** This file may be used under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>


typedef void (*net_timer_hook)(void *);
typedef int32 net_timer_id;


extern status_t net_init_timer(void);
extern void net_shutdown_timer(void);

extern net_timer_id net_add_timer(net_timer_hook hook, void *data, bigtime_t interval);
extern status_t net_remove_timer(net_timer_id);


#endif	/* NET_TIMER_H */
