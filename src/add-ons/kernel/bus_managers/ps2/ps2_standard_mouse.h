/*
 * Copyright 2001-2010 Haiku, Inc.
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
#ifndef __PS2_STANDARD_MOUSE_H
#define __PS2_STANDARD_MOUSE_H


#include <Drivers.h>

#include "packet_buffer.h"


#define MOUSE_HISTORY_SIZE				256
	// we record that many mouse packets before we start to drop them

#define F_MOUSE_TYPE_STANDARD			0x1
#define F_MOUSE_TYPE_INTELLIMOUSE		0x2

typedef struct {
	ps2_dev*			dev;

	sem_id				standard_mouse_sem;
struct packet_buffer*	standard_mouse_buffer;
	bigtime_t			click_last_time;
	bigtime_t			click_speed;
	int					click_count;
	int					buttons_state;
	int					flags;
	size_t				packet_index;
	uint8				buffer[PS2_MAX_PACKET_SIZE];
} standard_mouse_cookie;


status_t probe_standard_mouse(ps2_dev* dev);

status_t standard_mouse_open(const char* name, uint32 flags, void** _cookie);
status_t standard_mouse_close(void* _cookie);
status_t standard_mouse_freecookie(void* _cookie);
status_t standard_mouse_ioctl(void* _cookie, uint32 op, void* buffer,
	size_t length);

int32 standard_mouse_handle_int(ps2_dev* dev);
void standard_mouse_disconnect(ps2_dev* dev);

extern device_hooks gStandardMouseDeviceHooks;


#endif	/* __PS2_STANDARD_MOUSE_H */
