//
// kb_mouse_driver.h
//


#ifndef _KB_MOUSE_DRIVER_H
#define _KB_MOUSE_DRIVER_H

#include <SupportDefs.h>
#include <Drivers.h>

#ifdef __cplusplus
extern "C" {
#endif



// Be key numbers for various cool keys

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

#define KB_DEFAULT_CONTROL_ALT_DEL_TIMEOUT 4000000


// ioctl codes

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
	KB_RESERVED_1,						// was KB_ACKNOWLEDGE_CONTROL_ALT_DEL,
	KB_CANCEL_CONTROL_ALT_DEL,
	KB_DELAY_CONTROL_ALT_DEL,
	
	MS_READ = B_DEVICE_OP_CODES_END + 100,
	MS_NUM_EVENTS,
	MS_GETA,
	MS_SETA,
	MS_GETTYPE,
	MS_SETTYPE,
	MS_GETMAP,
	MS_SETMAP,
	MS_GETCLICK,
	MS_SETCLICK,
	MS_NUM_SERIAL_MICE,
	
	IIC_WRITE = B_DEVICE_OP_CODES_END + 200, 
	RESTART_SYSTEM,
	SHUTDOWN_SYSTEM
};


// keyboard settings info, as kept in settings file


typedef struct {
	bigtime_t       key_repeat_delay;
	int32           key_repeat_rate;
} kb_settings;

#define kb_settings_file "Keyboard_settings"


// structure passed to KB_READ

typedef struct {                        // USB, ADB keyboards
	bigtime_t       timestamp;
	uint32          be_keycode;
	bool            is_keydown;
} raw_key_info;

typedef struct {                        // AT keyboards
	bigtime_t       timestamp;
	uint8           scancode;       // high bit set for extended scancodes
	bool            is_keydown;
} at_kbd_io;


// structure passed to KB_SET_LEDS

typedef struct {
	bool    num_lock;
	bool    caps_lock;
	bool    scroll_lock;
} led_info;



// mouse settings info

typedef enum {
	MOUSE_1_BUTTON = 1,
	MOUSE_2_BUTTON,
	MOUSE_3_BUTTON
} mouse_type;

typedef struct {
	int32           left;
	int32           right;
	int32           middle;
} map_mouse;

typedef struct {
	bool    enabled;        // Acceleration on / off
	int32   accel_factor;   // accel factor: 256 = step by 1, 128 = step by 1/2
	int32   speed;          // speed accelerator (1=1X, 2 = 2x)...
} mouse_accel;

typedef struct {
	mouse_type      type;
	map_mouse       map;
	mouse_accel     accel;
	bigtime_t       click_speed;
} mouse_settings;

#define mouse_settings_file "Mouse_settings"

typedef struct {
	int             serial_cookie;
	int             buttons;
	int             xdelta;
	int             ydelta;
	int32           clicks;
	int32           modifiers;
	bigtime_t       time;
	int             wheel_delta;
} mouse_pos;


// On the Mac, the I-squared C bus is controlled by the Cuda microcontroller,
// which also runs ADB.  Since we have not yet partitioned the driver into
// a separate Cuda driver and a kb/mouse driver, the iic is controlled with
// control calls to the kb_mouse driver.  Yuck.

typedef struct {
	char            device;
	char            reg;
	char            value;
} iic_write;

#ifdef __cplusplus
}
#endid

#endif