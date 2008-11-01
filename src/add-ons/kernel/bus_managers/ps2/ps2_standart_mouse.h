/*
 * Copyright 2001-2008 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 mouse device driver
 *
 * Authors (in chronological order):
 * 		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
 *		Clemens Zeidler	<czeidler@gmx.de>
 */

#ifndef __PS2_STANDART_standart_mouse_H
#define __PS2_STANDART_standart_mouse_H

#include <Drivers.h>

#include "packet_buffer.h"

#define standart_mouse_HISTORY_SIZE	256
	// we record that many mouse packets before we start to drop them

#define F_pointing_dev_TYPE_STANDARD			0x1
#define F_pointing_dev_TYPE_INTELLIMOUSE		0x2

typedef struct
{
	ps2_dev *		dev;
	
	sem_id			standart_mouse_sem;
	packet_buffer *	standart_mouse_buffer;
	bigtime_t		click_last_time;
	bigtime_t		click_speed;
	int				click_count;
	int				buttons_state;
	int				flags;
	size_t			packet_index;
	uint8			packet_buffer[PS2_MAX_PACKET_SIZE];

} standart_mouse_cookie;


status_t probe_standart_mouse(ps2_dev *dev);

status_t standart_mouse_open(const char *name, uint32 flags, void **_cookie);
status_t standart_mouse_close(void *_cookie);
status_t standart_mouse_freecookie(void *_cookie);
status_t standart_mouse_ioctl(void *_cookie, uint32 op, void *buffer, size_t length);
	
int32 standart_mouse_handle_int(ps2_dev *dev);
void standart_mouse_disconnect(ps2_dev *dev);

device_hooks gStandartMouseDeviceHooks; 


#endif

