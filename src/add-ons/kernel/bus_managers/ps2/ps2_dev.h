/*
 * Copyright 2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */
#ifndef __PS2_DEV_H
#define __PS2_DEV_H

#include "ps2_common.h"

typedef struct
{
	const char *	name;
	bool			active;
	uint32			flags;
	uint8           idx;
	sem_id			result_sem;
	uint8 *			result_buf;
	int				result_buf_idx;
	int				result_buf_cnt;
} ps2_dev;

extern ps2_dev ps2_device[5];

#define PS2_DEVICE_MOUSE 0
#define PS2_DEVICE_KEYB  4

#define PS2_FLAG_KEYB    1

status_t	ps2_dev_init(void);
void		ps2_dev_exit(void);

status_t	ps2_dev_command(ps2_dev *dev, uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count);


void		ps2_dev_publish(ps2_dev *dev);
void		ps2_dev_unpublish(ps2_dev *dev);

int32		ps2_dev_handle_int(ps2_dev *dev, uint8 data);

#endif
