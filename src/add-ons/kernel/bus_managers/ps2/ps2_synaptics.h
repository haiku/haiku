#ifndef SYNAPTICS_H
#define SYNAPTICS_H

#include <KernelExport.h>

#include "kb_mouse_driver.h"
#include "movement_maker.h"
#include "packet_buffer.h"
#include "touchpad_settings.h"


#define SYN_TOUCHPAD			0x47
// Synaptics modes
#define SYN_ABSOLUTE_MODE		0x80
// Absolute plus w mode
#define SYN_ABSOLUTE_W_MODE		0x81
#define SYN_FOUR_BYTE_CHILD		(1 << 1)
// Low power sleep mode
#define SYN_SLEEP_MODE			0x0C
// Synaptics Passthrough port
#define SYN_CHANGE_MODE			0x14
#define SYN_PASSTHROUGH_CMD		0x28


// synaptics touchpad proportions
#define SYN_EDGE_MOTION_WIDTH	50
#define SYN_EDGE_MOTION_SPEED	5
#define SYN_AREA_OFFSET			40	// increase the touchpad size a little bit
#define SYN_AREA_START_X		(1472 - SYN_AREA_OFFSET)
#define SYN_AREA_END_X			(5472 + SYN_AREA_OFFSET)
#define SYN_AREA_WIDTH_X		(SYN_AREA_END_X - SYN_AREA_START_X)
#define SYN_AREA_START_Y		(1408 - SYN_AREA_OFFSET)
#define SYN_AREA_END_Y			(4448 + SYN_AREA_OFFSET)
#define SYN_AREA_WIDTH_Y		(SYN_AREA_END_Y - SYN_AREA_START_Y)

#define SYN_TAP_TIMEOUT			200000

#define MIN_PRESSURE			30
#define MAX_PRESSURE			200

#define SYNAPTICS_HISTORY_SIZE	256

// no touchpad / left / right button pressed
#define IS_SYN_PT_PACKAGE(val) ((val[0] & 0xFC) == 0x84 \
	&& (val[3] & 0xCC) == 0xc4)


typedef struct {
	uint8 majorVersion;
	uint8 minorVersion;

	bool capExtended;
	bool capSleep;
	bool capFourButtons;
	bool capMultiFinger;
	bool capPalmDetection;
	bool capPassThrough;

} touchpad_info;


typedef struct {
	uint8		buttons;
	uint32		xPosition;
	uint32		yPosition;
	uint8		zPressure;
	// absolut mode
	bool		finger;
	bool		gesture;
	// absolut w mode
	uint8		wValue;
} touch_event;


typedef struct
{
	ps2_dev *			dev;

	sem_id				synaptics_sem;
	packet_buffer 	*	synaptics_ring_buffer;
	size_t				packet_index;
	uint8				packet_buffer[PS2_PACKET_SYNAPTICS];
	uint8				mode;

	movement_maker		movement_maker;
	bool				movement_started;
	bool				scrolling_started;
	bool				tap_started;
	bigtime_t			tap_time;
	int32				tap_delta_x;
	int32				tap_delta_y;
	int32				tap_clicks;
	bool				tapdrag_started;
	bool				valid_edge_motion;
	bool				double_click;

	touchpad_settings	settings;
} synaptics_cookie;


void default_settings(touchpad_settings *settings);

status_t synaptics_pt_set_packagesize(ps2_dev *dev, uint8 size);
status_t send_touchpad_arg(ps2_dev *dev, uint8 arg);
status_t send_touchpad_arg_timeout(ps2_dev *dev, uint8 arg, bigtime_t timeout);
status_t set_touchpad_mode(ps2_dev *dev, uint8 mode);
status_t passthrough_command(ps2_dev *dev, uint8 cmd, const uint8 *out,
	int out_count, uint8 *in, int in_count, bigtime_t timeout);

bool edge_motion(mouse_movement *movement, touch_event *event, bool validStart);
status_t touchevent_to_movement(synaptics_cookie* cookie, touch_event *event,
	mouse_movement *movement);
status_t get_synaptics_movment(synaptics_cookie* cookie,
	mouse_movement *movement);
void query_capability(ps2_dev *dev);
status_t probe_synaptics(ps2_dev *dev);

status_t synaptics_open(const char *name, uint32 flags, void **_cookie);
status_t synaptics_close(void *_cookie);
status_t synaptics_freecookie(void *_cookie);
status_t synaptics_ioctl(void *_cookie, uint32 op, void *buffer, size_t length);

int32 synaptics_handle_int(ps2_dev *dev);
void synaptics_disconnect(ps2_dev *dev);

device_hooks gSynapticsDeviceHooks;

#endif
