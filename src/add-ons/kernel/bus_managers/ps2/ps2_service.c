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
#include "packet_buffer.h"

static sem_id			sServiceSem;
static thread_id		sServiceThread;
static volatile bool	sServiceTerminate;
static packet_buffer *  sServiceCmdBuffer;

typedef struct
{
	uint32		id;
	ps2_dev *	dev;
} ps2_service_cmd;

enum
{
	PS2_SERVICE_NOTIFY_DEVICE_ADDED = 1,
	PS2_SERVICE_NOTIFY_DEVICE_REMOVED,
};

void
ps2_service_notify_device_added(ps2_dev *dev)
{
	ps2_service_cmd cmd;

	TRACE(("ps2_service_notify_device_added %s\n", dev->name));
	
	cmd.id = PS2_SERVICE_NOTIFY_DEVICE_ADDED;
	cmd.dev = dev;
	
	packet_buffer_write(sServiceCmdBuffer, (const uint8 *)&cmd, sizeof(cmd));
	release_sem_etc(sServiceSem, 1, B_DO_NOT_RESCHEDULE);

	TRACE(("ps2_service_notify_device_added done\n"));
}


void
ps2_service_notify_device_removed(ps2_dev *dev)
{
	ps2_service_cmd cmd;

	TRACE(("ps2_service_notify_device_removed %s\n", dev->name));

	cmd.id = PS2_SERVICE_NOTIFY_DEVICE_REMOVED;
	cmd.dev = dev;
	
	packet_buffer_write(sServiceCmdBuffer, (const uint8 *)&cmd, sizeof(cmd));
	release_sem_etc(sServiceSem, 1, B_DO_NOT_RESCHEDULE);

	TRACE(("ps2_service_notify_device_removed done\n"));
}


static int32
ps2_service_thread(void *arg)
{
	TRACE(("ps2_service_thread started\n"));

	for (;;) {
		status_t status;
		status = acquire_sem_etc(sServiceSem, 1, B_CAN_INTERRUPT, 0);
		if (sServiceTerminate)
			break;
		if (status == B_OK) {

			// process service commands
			ps2_service_cmd cmd;
			packet_buffer_read(sServiceCmdBuffer, (uint8 *)&cmd, sizeof(cmd));
			switch (cmd.id) {
				case PS2_SERVICE_NOTIFY_DEVICE_ADDED:
					TRACE(("PS2_SERVICE_NOTIFY_DEVICE_ADDED %s\n", cmd.dev->name));
					ps2_dev_publish(cmd.dev);
					break;
					
				case PS2_SERVICE_NOTIFY_DEVICE_REMOVED:
					TRACE(("PS2_SERVICE_NOTIFY_DEVICE_REMOVED %s\n", cmd.dev->name));
					ps2_dev_unpublish(cmd.dev);
					break;

				default:
					TRACE(("PS2_SERVICE: unknown id %lu\n", cmd.id));
					break;
			}
		} else {
			dprintf("ps2_service_thread: Error, status 0x%08lx, terminating\n", status);
			break;
		}
	}
	return 0;
}


status_t
ps2_service_init(void)
{
	TRACE(("ps2_service_init\n"));
	sServiceCmdBuffer = create_packet_buffer(sizeof(ps2_service_cmd) * 50);
	if (sServiceCmdBuffer == NULL)
		goto err1;
	sServiceSem = create_sem(0, "ps2 service");
	if (sServiceSem < B_OK)
		goto err2;
	sServiceThread = spawn_kernel_thread(ps2_service_thread, "ps2 service", 20, NULL);
	if (sServiceThread < B_OK)
		goto err3;
	sServiceTerminate = false;
	resume_thread(sServiceThread);
	TRACE(("ps2_service_init done\n"));
	return B_OK;
	
err3:
	delete_sem(sServiceSem);
err2:
	delete_packet_buffer(sServiceCmdBuffer);
err1:
	TRACE(("ps2_service_init failed\n"));
	return B_ERROR;
}


void
ps2_service_exit(void)
{
	TRACE(("ps2_service_exit\n"));
	sServiceTerminate = true;
	release_sem(sServiceSem);
	wait_for_thread(sServiceThread, NULL);
	delete_sem(sServiceSem);
	delete_packet_buffer(sServiceCmdBuffer);
}
