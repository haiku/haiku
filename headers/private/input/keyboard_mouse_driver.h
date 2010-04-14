/*
 * Copyright 2002-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEYBOARD_MOUSE_DRIVER_H
#define _KEYBOARD_MOUSE_DRIVER_H


#include <SupportDefs.h>
#include <Drivers.h>


#define KEY_Scroll      0x0f
#define KEY_Pause       0x10
#define KEY_Num         0x22
#define KEY_CapsLock    0x3b
#define KEY_ShiftL      0x4b
#define KEY_ShiftR      0x56
#define KEY_ControlL    0x5c
#define KEY_CmdL        0x5d
#define KEY_AltL        0x5d
#define KEY_CmdR        0x5f
#define KEY_AltR        0x5f
#define KEY_ControlR    0x60
#define KEY_OptL        0x66
#define KEY_WinL        0x66
#define KEY_OptR        0x67
#define KEY_WinR        0x67
#define KEY_Menu        0x68
#define KEY_NumEqual    0x6a
#define KEY_Power       0x6b
#define KEY_SysRq       0x7e
#define KEY_Break       0x7f
#define KEY_Spacebar	0x5e

#define KB_DEFAULT_CONTROL_ALT_DEL_TIMEOUT 4000000

enum {
	KB_READ = B_DEVICE_OP_CODES_END,
	KB_GET_KEYBOARD_ID,
	KB_SET_LEDS,
	KB_SET_KEY_REPEATING,
	KB_SET_KEY_NONREPEATING,
	KB_SET_KEY_REPEAT_RATE,
	KB_GET_KEY_REPEAT_RATE,
	KB_SET_KEY_REPEAT_DELAY,
	KB_GET_KEY_REPEAT_DELAY,
	KB_SET_CONTROL_ALT_DEL_TIMEOUT,
	KB_RESERVED_1,
	KB_CANCEL_CONTROL_ALT_DEL,
	KB_DELAY_CONTROL_ALT_DEL,
	KB_SET_DEBUG_READER,

	MS_READ = B_DEVICE_OP_CODES_END + 100,
	MS_NUM_EVENTS,
	MS_GET_ACCEL,
	MS_SET_ACCEL,
	MS_GET_TYPE,
	MS_SET_TYPE,
	MS_GET_MAP,
	MS_SET_MAP,
	MS_GET_CLICKSPEED,
	MS_SET_CLICKSPEED,
	MS_NUM_SERIAL_MICE,
	MS_IS_TOUCHPAD,
	MS_SET_TOUCHPAD_SETTINGS,
	
	IIC_WRITE = B_DEVICE_OP_CODES_END + 200, 
	RESTART_SYSTEM,
	SHUTDOWN_SYSTEM
};


typedef struct {
	bigtime_t	timestamp;
	uint32		keycode;
	bool		is_keydown;
} raw_key_info;


typedef struct {
	bool		num_lock;
	bool		caps_lock;
	bool		scroll_lock;
} led_info;


typedef struct {
	int32		cookie;
	uint32		buttons;
	int32		xdelta;
	int32		ydelta;
	int32		clicks;
	int32		modifiers;
	bigtime_t	timestamp;
	int32		wheel_ydelta;
	int32		wheel_xdelta;
} mouse_movement;


typedef struct {
	uint32		buttons;
	float		xpos;
	float		ypos;
	bool		has_contact;
	float		pressure;
	int32		clicks;
	bool		eraser;
	bigtime_t	timestamp;
	int32		wheel_ydelta;
	int32		wheel_xdelta;
	float		tilt_x;
	float		tilt_y;
} tablet_movement;


#define B_ONE_FINGER	0x01
#define B_TWO_FINGER	0x02
#define B_MULTI_FINGER	0x04
#define B_PEN			0x08


typedef struct {
	uint8		buttons;
	uint32		xPosition;
	uint32		yPosition;
	uint8		zPressure;
	uint8		fingers;
	bool		gesture;
	uint8		fingerWidth;
	// 1 - 4	normal width
	// 5 - 11	very wide finger or palm
	// 12		maximum reportable width; extrem wide contact
} touchpad_movement;


#endif	// _KB_MOUSE_DRIVER_H
