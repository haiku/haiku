/*
 * Copyright 2009-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */


#include <malloc.h>
#include <string.h>

#include <keyboard_mouse_driver.h>

#include "ps2_trackpoint.h"


//#define TRACE_PS2_TRACKPOINT
#ifdef TRACE_PS2_TRACKPOINT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


const char* kTrackpointPath[4] = {
	"input/mouse/ps2/ibm_trackpoint_0",
	"input/mouse/ps2/ibm_trackpoint_1",
	"input/mouse/ps2/ibm_trackpoint_2",
	"input/mouse/ps2/ibm_trackpoint_3"
};


status_t
probe_trackpoint(ps2_dev* dev)
{
	uint8 val[2];

	TRACE("TRACKPOINT: probe\n");
	ps2_dev_command(dev, 0xE1, NULL, 0, val, 2);

	if (val[0] != 0x01) {
		TRACE("TRACKPOINT: not found\n");
		return B_ERROR;
	}
	dev->name = kTrackpointPath[dev->idx];
	dev->packet_size = 3;
	TRACE("TRACKPOINT: version 0x%x found\n", val[1]);

	return B_OK;
}
