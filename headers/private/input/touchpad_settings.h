/*
 * Copyright 2001-2025, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TOUCHPAD_SETTINGS_H
#define TOUCHPAD_SETTINGS_H

#include <SupportDefs.h>


typedef struct {
	bool	scroll_twofinger;
	bool	scroll_twofinger_horizontal;
	float	scroll_rightrange;		// from 0 to 1
	float	scroll_bottomrange;		// from 0 to 1
	uint16	scroll_xstepsize;
	uint16	scroll_ystepsize;
	uint8	scroll_acceleration;	// from 0 to 20

	uint8	tapgesture_sensibility;	// 0 : no tapgesture
									// 20: very light tap is enough (default)
	uint16  padblocker_threshold;	//0 to 100

	int32  trackpad_speed;
	int32  trackpad_acceleration;

	bool	scroll_reverse;
	bool	scroll_twofinger_natural_scrolling;

	uint8	edge_motion;			// 0: disabled
									// or combined flags of:
									// 0x01: edge motion on move
									// 0x02: edge motion on tap drag
									// 0x04: edge motion on button click move
									// 0x08: edge motion on button click drag

	bool	software_button_areas;
} touchpad_settings;


const static touchpad_settings kDefaultTouchpadSettings = {
	true,
	true,
	0.15,
	0.15,
	7,
	10,
	10,
	20,
	30,
	65536,
	65536,
	false,
	true,
	0x02,
	false
};

#define TOUCHPAD_SETTINGS_FILE "Touchpad_settings"

#endif	/* TOUCHPAD_SETTINGS_H */
