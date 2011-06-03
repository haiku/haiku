/*
 * Copyright 2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef _JOYSTICK_PRIVATE_H
#define _JOYSTICK_PRIVATE_H

#include <SupportDefs.h>

typedef struct _variable_joystick {
	uint32		axis_count;
	uint32		hat_count;
	uint32		button_blocks;

	// these pointers all point into the data section
	bigtime_t *	timestamp;
	uint32 *	buttons;
	int16 *		axes;
	uint8 *		hats;

	size_t		data_size;
	uint8 *		data;
} variable_joystick;

#endif // _JOYSTICK_PRIVATE_H
