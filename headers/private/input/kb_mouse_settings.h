//
// kb_mouse_settings.h
//


#ifndef _KB_MOUSE_SETTINGS_H
#define _KB_MOUSE_SETTINGS_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif


// keyboard settings info, as kept in settings file


typedef struct {
	bigtime_t       key_repeat_delay;
	int32           key_repeat_rate;
} kb_settings;

#define kb_settings_file "Keyboard_settings"

// mouse settings info

typedef struct {
	bool    enabled;        // Acceleration on / off
	int32   accel_factor;   // accel factor: 256 = step by 1, 128 = step by 1/2
	int32   speed;          // speed accelerator (1=1X, 2 = 2x)...
} mouse_accel;

typedef struct {
	int32		type;
	mouse_map	map;
	mouse_accel     accel;
	bigtime_t       click_speed;
} mouse_settings;

#define mouse_settings_file "Mouse_settings"

#ifdef __cplusplus
}
#endif

#endif
