/*
 * Copyright 2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */

#include "ps2_dev.h"
#include "ps2_service.h"

ps2_dev ps2_device[5] =
{
	{ .name = "input/mouse/ps2/0",   .active = false },
	{ .name = "input/mouse/ps2/1",   .active = false },
	{ .name = "input/mouse/ps2/2",   .active = false },
	{ .name = "input/mouse/ps2/3",   .active = false },
	{ .name = "input/keyboard/at/0", .active = false, .flags = PS2_FLAG_KEYB }
};


void
ps2_dev_publish(ps2_dev *dev)
{
	status_t status;
	
	if (dev->active)
		return;

	TRACE(("ps2_dev_publish %s\n", dev->name));

	dev->result_sem = create_sem(0, "ps2 result");
	dev->result_buf = NULL;
	dev->result_buf_idx = 0;
	dev->result_buf_cnt = 0;
	dev->flags &= PS2_FLAG_KEYB;

	dev->active = true;
	
	status = devfs_publish_device(dev->name, NULL, 
		(dev->flags & PS2_FLAG_KEYB) ? sKeyboardDeviceHooks : sMouseDeviceHooks);

	TRACE(("devfs_publish_device %s, status = 0x%08x\n", dev->name, status));
}


void
ps2_dev_unpublish(ps2_dev *dev)
{
	if (!dev->active)
		return;

	TRACE(("ps2_dev_unpublish %s\n", dev->name));
		
	dev->active = false;
		
	delete_sem(dev->result_sem);
}


int32
ps2_dev_handle_int(ps2_dev *dev, uint8 data)
{
	if (!dev->active) {
		ps2_service_handle_device_added(dev);
		return B_HANDLED_INTERRUPT;
	}
	if (dev->result_buf_cnt) {
		dev->result_buf[dev->result_buf_idx] = data;
		dev->result_buf_idx++;
		dev->result_buf_cnt--;
		if (dev->result_buf_cnt == 0) {
			release_sem_etc(dev->result_sem, 0, B_DO_NOT_RESCHEDULE);
			return B_INVOKE_SCHEDULER;
		}
		return B_HANDLED_INTERRUPT;
	}
	
	// temporary hack...
	if (dev->flags & PS2_FLAG_KEYB)
		return keyboard_handle_int(data);
	else
		return mouse_handle_int(data);
}
