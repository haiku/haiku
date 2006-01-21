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
	{ .name = "input/mouse/ps2/0",   .active = false, .idx = 0, .result_sem = -1 },
	{ .name = "input/mouse/ps2/1",   .active = false, .idx = 1, .result_sem = -1 },
	{ .name = "input/mouse/ps2/2",   .active = false, .idx = 2, .result_sem = -1 },
	{ .name = "input/mouse/ps2/3",   .active = false, .idx = 3, .result_sem = -1 },
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
	uint32 flags;
	
	flags = atomic_get(&dev->flags);
	
	if (flags & PS2_FLAG_CMD) {
		if ((flags & (PS2_FLAG_ACK | PS2_FLAG_NACK)) == 0) {
			atomic_or(&dev->flags, data == 0xfa ? PS2_FLAG_ACK : PS2_FLAG_NACK);
		} else if (dev->result_buf_cnt) {
			dev->result_buf[dev->result_buf_idx] = data;
			dev->result_buf_idx++;
			dev->result_buf_cnt--;
			if (dev->result_buf_cnt == 0) {
				release_sem_etc(dev->result_sem, 1, B_DO_NOT_RESCHEDULE);
				return B_INVOKE_SCHEDULER;
			}
		} else {
			dprintf("ps2_dev_handle_int can't handle\n");
		}
		return B_HANDLED_INTERRUPT;
	}
	
	if (!dev->active) {
		ps2_service_handle_device_added(dev);
		dprintf("not active, data dropped\n");
		return B_HANDLED_INTERRUPT;
	}
	
	if ((flags & PS2_FLAG_ENABLED) == 0) {
		dprintf("not enabled, data dropped\n");
		return B_HANDLED_INTERRUPT;
	}
			
	// temporary hack...
	if (flags & PS2_FLAG_KEYB)
		return keyboard_handle_int(data);
	else
		return mouse_handle_int(dev, data);
}


status_t
ps2_dev_command(ps2_dev *dev, uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count)
{
	status_t res = B_OK;
	int i, count;
	
	dprintf("ps2_dev_command %02x, %d out, in %d, dev %s\n", cmd, out_count, in_count, dev->name);
	
	dev->flags |= PS2_FLAG_CMD;
	dev->flags &= ~(PS2_FLAG_ACK | PS2_FLAG_NACK);

	dev->result_buf_cnt = in_count;
	dev->result_buf_idx = 0;
	dev->result_buf = in;

	for (i = -1; res == B_OK && i < out_count; i++) {

		if (!(dev->flags & PS2_FLAG_KEYB)) {
			uint8 prefix_cmd;
			if (gMultiplexingActive)
				prefix_cmd = 0x90 + dev->idx;
			else
				prefix_cmd = 0xd4;
			res = ps2_wait_write();
			if (res == B_OK)
				ps2_write_ctrl(prefix_cmd);
		}

		res = ps2_wait_write();
		if (res == B_OK) {
			ps2_write_data(i == -1 ? cmd : out[i]);
		}
	}

	if (dev->flags & PS2_FLAG_ACK) {
		dprintf("ps2_dev_command got ACK\n");
	}

	if (dev->flags & PS2_FLAG_NACK) {
		dev->flags &= ~PS2_FLAG_CMD;
		dprintf("ps2_dev_command got NACK\n");
		return B_ERROR;
	}
	
	if (res != B_OK) {
		dprintf("ps2_dev_command send failed\n");
	}

	if (in_count != 0) {
		dprintf("ps2_dev_command waiting for result\n");
	
		res = acquire_sem_etc(dev->result_sem, 1, B_RELATIVE_TIMEOUT, 4000000);

		count = in_count - dev->result_buf_cnt;
		dev->result_buf_cnt = 0;
	
		dprintf("ps2_dev_command res %08x, in %d\n", res, count);

		for (i = 0; i < count; i++)
			dprintf("ps2_dev_command data %02x\n", in[i]);
	}

	dev->flags &= ~PS2_FLAG_CMD;

	return res;
}

