/* ++++++++++
	FILE:	joystick_driver.h
	REVS:	$Revision: 1.1 $
	NAME:	herold
	DATE:	Tue Jun  4 14:57:25 PDT 1996
+++++ */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _JOYSTICK_DRIVER_H
#define _JOYSTICK_DRIVER_H

#include <SupportDefs.h>
#include <Drivers.h>
#include <module.h>

typedef struct _joystick {
	bigtime_t	timestamp;
	uint32		horizontal;
	uint32		vertical;
	bool		button1;
	bool		button2;
} joystick;

/* maximum number of axes on one controller (pads count as 2 axes each) */
#define MAX_AXES 12
/* maximum number of hats on one controller -- PADS SHOULD BE RETURNED AS AXES! */
#define MAX_HATS 8
/* maximum number of buttons on one controller */
#define MAX_BUTTONS 32
/* maximum number of controllers on one port */
#define MAX_STICKS 4

typedef struct _extended_joystick {
	bigtime_t	timestamp;		/* system_time when it was read */
	uint32		buttons;		/* lsb to msb, 1 == on */
	int16		axes[MAX_AXES];	/* -32768 to 32767, X, Y, Z, U, V, W */
	uint8		hats[MAX_HATS];	/* 0 through 8 (1 == N, 3 == E, 5 == S, 7 == W) */
} extended_joystick;

#define MAX_CONFIG_SIZE 100

enum {	/* flags for joystick module info */
	js_flag_force_feedback = 0x1,
	js_flag_force_feedback_directional = 0x2
};

typedef struct _joystick_module_info {
	char			module_name[64];
	char			device_name[64];
	int16			num_axes;
	int16			num_buttons;
	int16			num_hats;
	uint16			_reserved[7];
	uint32			flags;
	uint16			num_sticks;
	int16			config_size;
	char			device_config[MAX_CONFIG_SIZE];	/* device specific */
} joystick_module_info;

/* Note that joystick_module is something used by the game port driver */
/* to talk to digital joysticks; if you're writing a sound card driver */
/* and want to add support for a /dev/joystick device, use the generic_gameport */
/* module. */

typedef struct _joystick_module {
	module_info minfo;
	/** "configure" might change the "info" if it auto-detects a device */
	int (*configure)(int port, joystick_module_info * info, size_t size, void ** out_cookie);
	/** "read" actual data from device into "data" */
	int (*read)(void * cookie, int port, extended_joystick * data, size_t size);
	/** "crumble" the cookie (deallocate) when done */
	int (*crumble)(void * cookie, int port);
	/** "force" tells the joystick to exert force on the same axes as input for the specified duration */
	int (*force)(void * cookie, int port, bigtime_t duration, extended_joystick * force, size_t size);
	int _reserved_;
} joystick_module;

/** Doing force feedback means writing an extended_joystick to the device with force values.
    The "timestamp" should be the duration of the feedback. Successive writes will be queued
    by the device module. */
enum { /* Joystick driver ioctl() opcodes */
    B_JOYSTICK_GET_SPEED_COMPENSATION = B_JOYSTICK_DRIVER_BASE,
                                            /* arg -> ptr to int32 */
    B_JOYSTICK_SET_SPEED_COMPENSATION,      /* arg -> ptr to int32 */
    B_JOYSTICK_GET_MAX_LATENCY,             /* arg -> ptr to long long */
    B_JOYSTICK_SET_MAX_LATENCY,             /* arg -> ptr to long long */
    B_JOYSTICK_SET_DEVICE_MODULE,			/* arg -> ptr to joystick_module; also enters enhanced mode */
    B_JOYSTICK_GET_DEVICE_MODULE,           /* arg -> ptr to joystick_module */
	B_JOYSTICK_SET_RAW_MODE					/* arg -> ptr to bool (true or false) */
};

/* Speed compensation is not generally necessary, because the joystick */
/* driver is measuring using real time, not just # cycles. "0" means the */
/* default, center value. + typically returns higher values; - returns lower */
/* A typical range might be from -10 to +10, but it varies by driver */

/* Lower latency will make for more overhead in reading the joystick */
/* ideally, you set this value to just short of how long it takes you */
/* to calculate and render a frame. 30 fps -> latency 33000 */


typedef struct _generic_gameport_module {
	module_info minfo;
	status_t (*create_device)(int port, void ** out_storage);
	status_t (*delete_device)(void * storage);
	status_t (*open_hook)(void * storage, uint32 flags, void ** out_cookie);
	status_t (*close_hook)(void * cookie);
	status_t (*free_hook)(void * cookie);
	status_t (*control_hook)(void * cookie, uint32 op, void * data, size_t len);
	status_t (*read_hook)(void * cookie, off_t pos, void * data, size_t * len);
	status_t (*write_hook)(void * cookie, off_t pos, const void * data, size_t * len);
	int	_reserved_;
} generic_gameport_module;


#endif
