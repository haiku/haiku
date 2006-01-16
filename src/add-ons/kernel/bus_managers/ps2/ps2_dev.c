/*
 * Copyright 2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */

#include <fs/devfs.h>
#include "ps2_dev.h"
#include "ps2_service.h"

#define PS2_DEV_COUNT 5

ps2_dev ps2_device[PS2_DEV_COUNT] =
{
	{ .name = "input/mouse/ps2/0",   .active = false, .result_sem = -1 },
	{ .name = "input/mouse/ps2/1",   .active = false, .result_sem = -1 },
	{ .name = "input/mouse/ps2/2",   .active = false, .result_sem = -1 },
	{ .name = "input/mouse/ps2/3",   .active = false, .result_sem = -1 },
	{ .name = "input/keyboard/at/0", .active = false, .result_sem = -1, .flags = PS2_FLAG_KEYB }
};


status_t
ps2_dev_init(void)
{
	int i;
	for (i = 0; i < PS2_DEV_COUNT; i++) {
		ps2_dev *dev = &ps2_device[i];
		dev->result_sem = create_sem(0, "ps2 result");
		if (dev->result_sem < 0)
			goto err;
	}
	return B_OK;
err:
	ps2_dev_exit();
	return B_ERROR;
}


void
ps2_dev_exit(void)
{
	int i;
	for (i = 0; i < PS2_DEV_COUNT; i++) {
		ps2_dev *dev = &ps2_device[i];
		if (dev->result_sem >= 0) {
			delete_sem(dev->result_sem);
			dev->result_sem = -1;
		}
	}
}


void
ps2_dev_publish(ps2_dev *dev)
{
	status_t status;
	
	if (dev->active)
		return;

	TRACE(("ps2_dev_publish %s\n", dev->name));

	dev->active = true;
	
	status = devfs_publish_device(dev->name, NULL, 
		(dev->flags & PS2_FLAG_KEYB) ? &sKeyboardDeviceHooks : &sMouseDeviceHooks);

	TRACE(("devfs_publish_device %s, status = 0x%08x\n", dev->name, status));
}


void
ps2_dev_unpublish(ps2_dev *dev)
{
	if (!dev->active)
		return;

	TRACE(("ps2_dev_unpublish %s\n", dev->name));
		
	dev->active = false;
}


int32
ps2_dev_handle_int(ps2_dev *dev, uint8 data)
{
	if (!dev->active) {
		ps2_service_handle_device_added(dev);
	}

	if (dev->result_buf_cnt) {
		dev->result_buf[dev->result_buf_idx] = data;
		dev->result_buf_idx++;
		dev->result_buf_cnt--;
		if (dev->result_buf_cnt == 0) {
			release_sem_etc(dev->result_sem, 1, B_DO_NOT_RESCHEDULE);
			return B_INVOKE_SCHEDULER;
		}
		return B_HANDLED_INTERRUPT;
	}

	if (!dev->active) {
		return B_HANDLED_INTERRUPT;
	}
	
	// temporary hack...
	if (dev->flags & PS2_FLAG_KEYB)
		return keyboard_handle_int(data);
	else
		return mouse_handle_int(data);
}

