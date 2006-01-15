/*
 * Copyright 2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */

#include "ps2_service.h"

static sem_id			sServiceSem;
static thread_id		sServiceThread;
static volatile bool	sServiceTerminate;


static int32
ps2_service_thread(void *arg)
{
	for (;;) {
		while (B_INTERRUPTED == acquire_sem(sServiceSem))
			;
		if (sServiceTerminate)
			break;
		
	}
	return 0;
}


status_t
ps2_service_init(void)
{
	sServiceSem = create_sem(0, "ps2 service");
	if (sServiceSem < B_OK)
		goto err1;
	sServiceThread = spawn_kernel_thread(ps2_service_thread, "ps2 service", 20, NULL);
	if (sServiceThread < B_OK)
		goto err2;
	sServiceTerminate = false;
	resume_thread(sServiceThread);
	return B_OK;
	
err2:
	delete_sem(sServiceSem);
err1:
	return B_ERROR;
}


void
ps2_service_exit(void)
{
	sServiceTerminate = true;
	release_sem(sServiceSem);
	wait_for_thread(sServiceThread, NULL);
	delete_sem(sServiceSem);
}
