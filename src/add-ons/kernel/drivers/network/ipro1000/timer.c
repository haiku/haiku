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
#include <Errors.h>
#include <OS.h>
#include <string.h>

#include "debug.h"
#include "timer.h"

#define MAX_TIMERS 4


struct timer_info
{
	timer_id 		id;
	timer_function	func;
	void *			cookie;
	bigtime_t		next_event;
	bigtime_t		interval;
	bool			periodic;
};


static struct timer_info	sTimerData[MAX_TIMERS];
static int					sTimerCount;
static timer_id				sTimerNextId;
static thread_id			sTimerThread;
static sem_id				sTimerSem;
static spinlock				sTimerSpinlock;


static int32
timer_thread(void *cookie)
{
	status_t status = 0;

	do {
		bigtime_t timeout;
		bigtime_t now;
		cpu_status cpu;
		timer_function func;
		void * cookie;
		int i;
		int index;

		cpu = disable_interrupts();
		acquire_spinlock(&sTimerSpinlock);

		now = system_time();
		cookie = 0;
		func = 0;
				
		// find timer with smallest event time
		index = -1;
		timeout = B_INFINITE_TIMEOUT;
		for (i = 0; i < sTimerCount; i++) {
			if (sTimerData[i].next_event < timeout) {
				timeout = sTimerData[i].next_event;
				index = i;
			}
		}
		
		if (timeout < now) {
			// timer is ready for execution, load func and cookie
			ASSERT(index >= 0 && index < sTimerCount);
			func = sTimerData[index].func;
			cookie = sTimerData[index].cookie;
			if (sTimerData[index].periodic) {
				// periodic timer is ready, update the entry
				sTimerData[index].next_event += sTimerData[index].interval;
			} else {
				// single shot timer is ready, delete the entry
				if (index != (sTimerCount - 1) && sTimerCount != 1) {
					memcpy(&sTimerData[index], &sTimerData[sTimerCount - 1], sizeof(struct timer_info));
				}
				sTimerCount--;
			}
		}

		release_spinlock(&sTimerSpinlock);
		restore_interrupts(cpu);
		
		// execute timer hook
		if (timeout < now) {
			ASSERT(func);
			func(cookie);
			continue;
		}

		status = acquire_sem_etc(sTimerSem, 1, B_ABSOLUTE_TIMEOUT, timeout);
	} while (status != B_BAD_SEM_ID);

	return 0;
}


timer_id
create_timer(timer_function func, void *cookie, bigtime_t interval, uint32 flags)
{
	cpu_status cpu;
	timer_id id;
	
	if (func == 0)
		return -1;
	
	// Attention: flags are not real flags, as B_PERIODIC_TIMER is 3

	cpu = disable_interrupts();
	acquire_spinlock(&sTimerSpinlock);
	
	if (sTimerCount < MAX_TIMERS) {
		id = sTimerNextId;
		sTimerData[sTimerCount].id = id;
		sTimerData[sTimerCount].func = func;
		sTimerData[sTimerCount].cookie = cookie;
		sTimerData[sTimerCount].next_event = (flags == B_ONE_SHOT_ABSOLUTE_TIMER) ? interval : system_time() + interval;
		sTimerData[sTimerCount].interval = interval;
		sTimerData[sTimerCount].periodic = flags == B_PERIODIC_TIMER;
		sTimerNextId++;
		sTimerCount++;
	} else {
		id = -1;
	}
	
	release_spinlock(&sTimerSpinlock);
	restore_interrupts(cpu);
	
	if (id != -1)
		release_sem_etc(sTimerSem, 1, B_DO_NOT_RESCHEDULE);

	return id;
}


status_t
delete_timer(timer_id id)
{
	cpu_status cpu;
	bool deleted;
	int i;

	deleted = false;
	
	cpu = disable_interrupts();
	acquire_spinlock(&sTimerSpinlock);
	
	for (i = 0; i < sTimerCount; i++) {
		if (sTimerData[i].id == id) {
			if (i != (sTimerCount - 1) && sTimerCount != 1) {
				memcpy(&sTimerData[i], &sTimerData[sTimerCount - 1], sizeof(struct timer_info));
			}
			sTimerCount--;
			deleted = true;
			break;
		}
	}
	
	release_spinlock(&sTimerSpinlock);
	restore_interrupts(cpu);

	if (!deleted)
		return B_ERROR;
		
	release_sem_etc(sTimerSem, 1, B_DO_NOT_RESCHEDULE);
	return B_OK;
}


status_t
initialize_timer(void)
{
	sTimerCount = 0;
	sTimerNextId = 1;
	sTimerSpinlock = 0;
	
	sTimerThread = spawn_kernel_thread(timer_thread, "ipro1000 timer", 80, 0);
	sTimerSem = create_sem(0, "ipro1000 timer");
	set_sem_owner(sTimerSem, B_SYSTEM_TEAM);
	
	if (sTimerSem < 0 || sTimerThread < 0) {
		delete_sem(sTimerSem);
		kill_thread(sTimerThread);
		return B_ERROR;
	}
	
	resume_thread(sTimerThread);
	return B_OK;
}


status_t
terminate_timer(void)
{
	status_t thread_return_value;

	delete_sem(sTimerSem);
	return wait_for_thread(sTimerThread, &thread_return_value);	
}

