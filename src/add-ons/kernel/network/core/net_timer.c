/* net_timer.h - a small and more or less inaccurate timer for net modules.
**		The registered hooks will be called in the thread of the timer.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
**
** This file may be used under the terms of the OpenBeOS License.
*/

#include <OS.h>
#include <malloc.h>

#ifdef _KERNEL_
#include <KernelExport.h>
#endif

#include "net_timer.h"



struct timer_entry {
	struct timer_entry	*te_next;
	net_timer_hook		te_hook;
	void				*te_data;
	bigtime_t			te_interval;
	bigtime_t			te_until;
	net_timer_id		te_id;
};

struct timer_info {
	struct timer_entry	*ti_first;

	sem_id				ti_lock;
	sem_id				ti_wait;
	int32				ti_counter;
	volatile int32		ti_inUse;
};

int32 net_timer(void *_data);

struct timer_info gTimerInfo;

status_t
net_init_timer(void)
{
	thread_id thread;

	memset(&gTimerInfo,0,sizeof(struct timer_info));

	gTimerInfo.ti_lock = create_sem(1,"net timer lock");
	if (gTimerInfo.ti_lock < B_OK)
		return B_ERROR;

	gTimerInfo.ti_wait = create_sem(0,"net timer wait");
	if (gTimerInfo.ti_wait < B_OK)
		return B_ERROR;

#ifdef _KERNEL_
	set_sem_owner(gTimerInfo.ti_lock, B_SYSTEM_TEAM);
	set_sem_owner(gTimerInfo.ti_wait, B_SYSTEM_TEAM);
	
	thread = spawn_kernel_thread(net_timer,"net timer",B_NORMAL_PRIORITY,&gTimerInfo);
#else
	thread = spawn_thread(net_timer,"net timer",B_NORMAL_PRIORITY,&gTimerInfo);
#endif
	if (thread < B_OK)
		return thread;

	return resume_thread(thread);
}


void
net_shutdown_timer(void)
{
	struct timer_entry *te,*next;
	int32 tries = 20;

	delete_sem(gTimerInfo.ti_wait);
	delete_sem(gTimerInfo.ti_lock);
	gTimerInfo.ti_wait = -1;
	gTimerInfo.ti_lock = -1;

	// make sure the structure isn't used anymore
	while (gTimerInfo.ti_inUse != 0 && tries-- > 0)
		snooze(1000);

	// free the remaining timer entries
	for (te = gTimerInfo.ti_first;te;te = next) {
		next = te->te_next;
		free(te);
	}
}


int32
net_timer(void *_data)
{
	struct timer_info *timer = (struct timer_info *)_data;
	status_t status = B_OK;

	do {
		bigtime_t timeout = B_INFINITE_TIMEOUT;
		struct timer_entry *te;

		// get access to the info structure
		if (status == B_TIMED_OUT || status == B_OK) {
			if (acquire_sem(timer->ti_lock) == B_OK) {
				for (te = timer->ti_first;te;te = te->te_next) {
					// new entry?
					if (te->te_until == -1)
						te->te_until = system_time() + te->te_interval;

					// execute timer?
					if (te->te_until < system_time()) {
						te->te_until += te->te_interval;
						te->te_hook(te->te_data);
					}
					
					// calculate new timeout
					if (te->te_until < timeout)
						timeout = te->te_until;
				}
	
				release_sem(timer->ti_lock);
			}
		}
		
		status = acquire_sem_etc(timer->ti_wait,1,B_ABSOLUTE_TIMEOUT,timeout);
		// the wait sem normally can't be acquired, so we
		// have to look at the status value the call returns:
		//
		// B_OK - someone wanted to notify us
		// B_TIMED_OUT - look for timers to be executed
		// B_BAD_SEM_ID - our sem got deleted

	} while (status != B_BAD_SEM_ID);
	
	return 0;
}


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

