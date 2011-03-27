#ifndef ALPS_H
#define ALPS_H

#include <KernelExport.h>

#include <touchpad_settings.h>

#include "movement_maker.h"
#include "packet_buffer.h"
#include "ps2_dev.h"


typedef struct {
	ps2_dev*			dev;

	sem_id				sem;
	packet_buffer*		ring_buffer;
	size_t				packet_index;
	uint8				packet_buffer[PS2_PACKET_ALPS];
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

	bigtime_t			click_last_time;
	bigtime_t			click_speed;
	int32				click_count;
	uint32				buttons_state;

	touchpad_settings	settings;
} alps_cookie;


status_t probe_alps(ps2_dev *dev);

status_t alps_open(const char *name, uint32 flags, void **_cookie);
status_t alps_close(void *_cookie);
status_t alps_freecookie(void *_cookie);
status_t alps_ioctl(void *_cookie, uint32 op, void *buffer, size_t length);

int32 alps_handle_int(ps2_dev *dev);
void alps_disconnect(ps2_dev *dev);

device_hooks gALPSDeviceHooks;


#endif /* ALPS_H */
