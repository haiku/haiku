/*
 * Copyright 2001-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KB_MOUSE_SETTINGS_H
#define _KB_MOUSE_SETTINGS_H


#include <InterfaceDefs.h>


typedef struct {
	bigtime_t	key_repeat_delay;
	int32		key_repeat_rate;
} kb_settings;

#define kb_default_key_repeat_delay 500000
#define kb_default_key_repeat_rate	250

#define kb_settings_file "Keyboard_settings"

typedef struct {
	bool		enabled;
	int32		accel_factor;
	int32		speed;
} mouse_accel;

typedef struct {
	int32		type;
	mouse_map	map;
	mouse_accel	accel;
	bigtime_t	click_speed;
} mouse_settings;

#define mouse_settings_file "Mouse_settings"

#endif	/* _KB_MOUSE_SETTINGS_H */
