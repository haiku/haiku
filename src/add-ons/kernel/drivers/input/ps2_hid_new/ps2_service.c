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

#define PS2_SERVICE_INTERVAL 200000


void
ps2_service_handle_device_added(ps2_dev *dev)
{
	TRACE(("ps2_service_handle_device_added %s\n", dev->name));

}


void
ps2_service_handle_device_removed(ps2_dev *dev)
{
	TRACE(("ps2_service_handle_device_removed %s\n", dev->name));
}


static status_t
ps2_service_probe_device(ps2_dev *dev)
{
	uint8 d;
	status_t res;

	TRACE(("ps2_service_probe_device %s\n", dev->name));

	if (dev->flags & PS2_FLAG_KEYB) {

		res = ps2_command(0xae, NULL, 0, NULL, 0);
		dprintf("KBD enable: res 0x%08x\n", res);

		res = ps2_command(0xab, NULL, 0, &d, 1);
		dprintf("KBD test: res 0x%08x, d 0x%02x\n", res, d);

	} else {

		res = ps2_command(0xa8, NULL, 0, NULL, 0);
		dprintf("AUX enable: res 0x%08x\n", res);

		res = ps2_command(0xa9, NULL, 0, &d, 1);
		dprintf("AUX test: res 0x%08x, d 0x%02x\n", res, d);
	}
	
	return B_OK;
}


static int32
ps2_service_thread(void *arg)
{
	for (;;) {
		status_t status;
		status = acquire_sem_etc(sServiceSem, 1, B_RELATIVE_TIMEOUT, PS2_SERVICE_INTERVAL);
		if (sServiceTerminate)
			break;
		if (status == B_OK) {
			// process service commands
		
		} else if (status == B_TIMED_OUT) {
			// do periodic processing
		
		} else {
			dprintf("ps2_service_thread: Error, status 0x%08x, terminating\n", status);
			break;
		}
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
