/* timer.c - a small and more or less inaccurate timer for net modules.
**		The registered hooks will be called in the thread of the timer.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
**
** This file may be used under the terms of the OpenBeOS License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <KernelExport.h>
#include <OS.h>

#include "net_stack.h"
#include "timer.h"

struct net_timer {
	struct net_timer	*next;
	net_timer_func		func;
	void				*func_cookie;
	bigtime_t			period;
	bigtime_t			until;
	bool				pending;
};

struct timers_queue {
	struct net_timer	*first;
	sem_id				lock;
	sem_id				wait;
	int32				counter;
	volatile int32		in_use;
};

static struct timers_queue 	g_timers;
static thread_id			g_timers_thread = -1;

static int32 timers_thread(void *data);

//	#pragma mark [Start/Stop Service functions]


// --------------------------------------------------
status_t start_timers_service(void)
{
	memset(&g_timers, 0, sizeof(g_timers));

	g_timers.lock = create_sem(1, "net_timers lock");
	if (g_timers.lock < B_OK)
		return B_ERROR;

	g_timers.wait = create_sem(0,"net_timers wait");
	if (g_timers.wait < B_OK)
		return B_ERROR;

#ifdef _KERNEL_MODE
	set_sem_owner(g_timers.lock, B_SYSTEM_TEAM);
	set_sem_owner(g_timers.wait, B_SYSTEM_TEAM);
#endif

	g_timers_thread = spawn_kernel_thread(timers_thread, "timers lone runner", B_URGENT_DISPLAY_PRIORITY, &g_timers);
	if (g_timers_thread < B_OK)
		return g_timers_thread;

	puts("net timers service started.");

	return resume_thread(g_timers_thread);
}


// --------------------------------------------------
status_t stop_timers_service(void)
{
	net_timer *nt, *next;
	int32 tries = 20;
	status_t status;

	delete_sem(g_timers.wait);
	delete_sem(g_timers.lock);
	g_timers.wait = -1;
	g_timers.lock = -1;

	wait_for_thread(g_timers_thread, &status);

	// make sure the structure isn't used anymore
	while (g_timers.in_use != 0 && tries-- > 0)
		snooze(1000);

	// free the remaining timer entries
	for (nt = g_timers.first; nt; nt = next) {
		next = nt->next;
		free(nt);
	}

	puts("net timers service stopped.");
		
	return B_OK;
}


// #pragma mark [Public functions]

// --------------------------------------------------
net_timer * new_net_timer(void)
{
	return NULL;
}


// --------------------------------------------------
status_t delete_net_timer(net_timer *nt)
{
	return B_ERROR;
}


// --------------------------------------------------
status_t start_net_timer(net_timer *nt, net_timer_func func, void *cookie, bigtime_t period)
{
	return B_ERROR;
}


// --------------------------------------------------
status_t cancel_net_timer(net_timer *nt)
{
	return B_ERROR;
}


// --------------------------------------------------
status_t net_timer_appointment(net_timer *nt, bigtime_t *period, bigtime_t *when)
{
	return 0;
}

// #pragma mark -

// --------------------------------------------------
static int32 timers_thread(void *data)
{
	struct timers_queue *timers = (struct timers_queue *) data;
	status_t status = B_OK;

	do {
		bigtime_t timeout = B_INFINITE_TIMEOUT;
		net_timer *nt;

		// get access to the info structure
		if (status == B_TIMED_OUT || status == B_OK) {
			if (acquire_sem(timers->lock) == B_OK) {
				for (nt = timers->first; nt; nt = nt->next) {
					// new entry?
					if (nt->until == -1)
						nt->until = system_time() + nt->period;

					// execute timer?
					if (nt->until < system_time()) {
						nt->until += nt->period;
						nt->func(nt, nt->func_cookie);
					}
					
					// calculate new timeout
					if (nt->until < timeout)
						timeout = nt->until;
				}
	
				release_sem(timers->lock);
			}
		}
		
		status = acquire_sem_etc(timers->wait, 1, B_ABSOLUTE_TIMEOUT, timeout);
		// the wait sem normally can't be acquired, so we
		// have to look at the status value the call returns:
		//
		// B_OK - someone wanted to notify us
		// B_TIMED_OUT - look for timers to be executed
		// B_BAD_SEM_ID - our sem got deleted

	} while (status != B_BAD_SEM_ID);
	
	return 0;
}

#if 0



net_timer_id
net_add_timer(net_timer_hook hook,void *data,bigtime_t interval)
{
	struct timer_entry *te;
	status_t status;
	
	if (interval < 100)
		return B_BAD_VALUE;

	atomic_add(&gTimerInfo.ti_inUse,1);

	// get access to the timer info structure
	status = acquire_sem(gTimerInfo.ti_lock);
	if (status < B_OK) {
		atomic_add(&gTimerInfo.ti_inUse,-1);
		return status;
	}

	te = (struct timer_entry *)malloc(sizeof(struct timer_entry));
	if (te == NULL) {
		atomic_add(&gTimerInfo.ti_inUse,-1);
		release_sem(gTimerInfo.ti_lock);
		return B_NO_MEMORY;
	}

	te->te_hook = hook;
	te->te_data = data;
	te->te_interval = interval;
	te->te_until = -1;
	te->te_id = ++gTimerInfo.ti_counter;

	// add the new entry
	te->te_next = gTimerInfo.ti_first;
	gTimerInfo.ti_first = te;

	atomic_add(&gTimerInfo.ti_inUse,-1);
	release_sem(gTimerInfo.ti_lock);
	
	// notify timer about the change
	release_sem(gTimerInfo.ti_wait);

	return te->te_id;
}


status_t
net_remove_timer(net_timer_id id)
{
	struct timer_entry *te,*last;
	status_t status;

	if (id <= B_OK)
		return B_BAD_VALUE;

	atomic_add(&gTimerInfo.ti_inUse,1);

	// get access to the timer info structure
	status = acquire_sem(gTimerInfo.ti_lock);
	if (status < B_OK) {
		atomic_add(&gTimerInfo.ti_inUse,-1);
		return status;
	}

	// search the list for the right timer
	
	// little hack that relies on ti_first being on the same position
	// in the structure as te_next
	last = (struct timer_entry *)&gTimerInfo;
	for (te = gTimerInfo.ti_first;te;te = te->te_next) {
		if (te->te_id == id) {
			last->te_next = te->te_next;
			free(te);
			break;
		}
		last = te;
	}
	atomic_add(&gTimerInfo.ti_inUse,-1);
	release_sem(gTimerInfo.ti_lock);

	if (te == NULL)
		return B_ENTRY_NOT_FOUND;

	// notify timer about the change
	release_sem(gTimerInfo.ti_wait);

	return B_OK;
}

#endif