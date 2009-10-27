/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "device.h"


#define CONVERT_HZ_TO_USECS(hertz) (1000000LL / (hertz))
#define FREEBSD_CLOCK_FREQUENCY_IN_HZ 1000


int ticks;
static sem_id sHardClockSem;
static thread_id sHardClockThread;


/*!
 * Implementation of FreeBSD's hardclock timer.
 *
 * Note: We are not using the FreeBSD variable hz as the invocation frequency
 * as it is the case in FreeBSD's hardclock function. This is due to lower
 * system load. The hz (see compat/sys/kernel.h) variable in the compat layer is
 * set to 1000000 Hz, whereas it is usually set to 1000 Hz for FreeBSD.
 */
static status_t
hard_clock_thread(void* data)
{
	status_t status = B_OK;
	const bigtime_t duration
		= CONVERT_HZ_TO_USECS(FREEBSD_CLOCK_FREQUENCY_IN_HZ);

	do {
		bigtime_t timeout = system_time() + duration;
		status = acquire_sem_etc(sHardClockSem, 1, B_ABSOLUTE_TIMEOUT, timeout);

		if (system_time() >= timeout) {
			atomic_add((vint32*)&ticks, 1);
		}
	} while (status != B_BAD_SEM_ID);

	return status;
}


status_t
init_hard_clock()
{
	status_t status = B_OK;

	sHardClockSem = create_sem(0, "hard clock wait");
	if (sHardClockSem < B_OK) {
		status = sHardClockSem;
		goto error1;
	}

	sHardClockThread = spawn_kernel_thread(hard_clock_thread, "hard clock",
		B_NORMAL_PRIORITY, NULL);
	if (sHardClockThread < B_OK) {
		status = sHardClockThread;
		goto error2;
	}

	return resume_thread(sHardClockThread);

error2:
	delete_sem(sHardClockSem);
error1:
	return status;
}


void
uninit_hard_clock()
{
	status_t status;

	delete_sem(sHardClockSem);
	wait_for_thread(sHardClockThread, &status);
}
