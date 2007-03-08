/*
 * Copyright 2005-2007 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 bus manager
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */
#ifndef __PS2_DEV_H
#define __PS2_DEV_H

struct ps2_dev;
typedef struct ps2_dev ps2_dev;

#include "ps2_common.h"

typedef struct
{
	bigtime_t		time;
	uint8			data;
	bool			error;
} data_history;

struct ps2_dev
{
	const char *	name;
	bool			active;
	uint32			flags;
	uint8           idx;
	sem_id			result_sem;
	uint8 *			result_buf;
	int				result_buf_idx;
	int				result_buf_cnt;
	void *			cookie;
	data_history	history[2];

// functions
	void          (*disconnect)(ps2_dev *);
	int32		  (*handle_int)(ps2_dev *);
};

#define PS2_DEVICE_COUNT 5

extern ps2_dev ps2_device[PS2_DEVICE_COUNT];

#define PS2_DEVICE_MOUSE 0
#define PS2_DEVICE_KEYB  4

#define PS2_FLAG_KEYB		(1<<0)
#define PS2_FLAG_OPEN		(1<<1)
#define PS2_FLAG_ENABLED	(1<<2)
#define PS2_FLAG_CMD		(1<<3)
#define PS2_FLAG_ACK		(1<<4)
#define PS2_FLAG_NACK		(1<<5)
#define PS2_FLAG_GETID		(1<<6)

status_t	ps2_dev_init(void);
void		ps2_dev_exit(void);

status_t	ps2_dev_command(ps2_dev *dev, uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count);


void		ps2_dev_publish(ps2_dev *dev);
void		ps2_dev_unpublish(ps2_dev *dev);

int32		ps2_dev_handle_int(ps2_dev *dev);

#endif
