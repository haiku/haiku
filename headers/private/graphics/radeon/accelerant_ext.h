/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon accelerant
		
	additional accelerant interface definitions
*/


#ifndef _ACCELERANT_EXT_H
#define _ACCELERANT_EXT_H


// additional timing flags for GetMode/SetMode
enum {
	RADEON_MODE_STANDARD = 0 << 16,
	RADEON_MODE_MIRROR = 1 << 16,
	RADEON_MODE_CLONE = 2 << 16,
	RADEON_MODE_COMBINE = 3 << 16,
	
	RADEON_MODE_MASK = 7 << 16,
	
	RADEON_MODE_DISPLAYS_SWAPPED = 1 << 20,
	
	// used internally
	RADEON_MODE_POSITION_HORIZONTAL = 0 << 21,
	RADEON_MODE_POSITION_VERTICAL = 1 << 21,
	RADEON_MODE_POSITION_MASK = 1 << 21,
	
	RADEON_MODE_MULTIMON_REQUEST = 1 << 25,
	RADEON_MODE_MULTIMON_REPLY = 1 << 26
};

// operation codes tunneled via ProposeDisplayMode
enum {
	ms_swap,
	ms_overlay_port
} multi_mon_settings;


#endif
