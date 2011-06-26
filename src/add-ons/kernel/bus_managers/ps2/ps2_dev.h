/*
 * Copyright 2005-2010 Haiku, Inc.
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


typedef struct {
	bigtime_t		time;
	uint8			data;
	bool			error;
} data_history;

struct ps2_dev {
	const char *	name;
	bool			active;
	uint8           idx;
	sem_id			result_sem;
	vint32			flags;
	uint8 *			result_buf;
	int				result_buf_idx;
	int				result_buf_cnt;
	void *			cookie;
	data_history	history[2];
	ps2_dev *		parent_dev;
	size_t			packet_size;

// functions
	void          (*disconnect)(ps2_dev *);
	int32		  (*handle_int)(ps2_dev *);
	status_t	  (*command)(ps2_dev *dev, uint8 cmd, const uint8 *out,
					int out_count, uint8 *in, int in_count, bigtime_t timeout);
};

#define PS2_DEVICE_COUNT 6

extern ps2_dev ps2_device[PS2_DEVICE_COUNT];

#define PS2_DEVICE_MOUSE 0
#define PS2_DEVICE_SYN_PASSTHROUGH 4
#define PS2_DEVICE_KEYB  5

#define PS2_FLAG_KEYB		(1 << 0)
#define PS2_FLAG_OPEN		(1 << 1)
#define PS2_FLAG_ENABLED	(1 << 2)
#define PS2_FLAG_CMD		(1 << 3)
#define PS2_FLAG_ACK		(1 << 4)
#define PS2_FLAG_NACK		(1 << 5)
#define PS2_FLAG_GETID		(1 << 6)
#define PS2_FLAG_RESEND		(1 << 7)


#ifdef __cplusplus
extern "C" {
#endif

void 		ps2_dev_send(ps2_dev *dev, uint8 data);

status_t	ps2_dev_detect_pointing(ps2_dev *dev, device_hooks **hooks);

status_t	ps2_dev_init(void);
void		ps2_dev_exit(void);

status_t	standard_command_timeout(ps2_dev *dev, uint8 cmd, const uint8 *out,
				int out_count, uint8 *in, int in_count, bigtime_t timeout);

status_t	ps2_dev_command(ps2_dev *dev, uint8 cmd, const uint8 *out,
				int out_count, uint8 *in, int in_count);
status_t	ps2_dev_command_timeout(ps2_dev *dev, uint8 cmd, const uint8 *out,
				int out_count, uint8 *in, int in_count, bigtime_t timeout);

status_t	ps2_reset_mouse(ps2_dev *dev);

void		ps2_dev_publish(ps2_dev *dev);
void		ps2_dev_unpublish(ps2_dev *dev);

int32		ps2_dev_handle_int(ps2_dev *dev);

#ifdef __cplusplus
}
#endif


#endif	/* __PS2_DEV_H */
