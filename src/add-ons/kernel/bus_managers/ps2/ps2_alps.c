/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * The alps_model_info struct and all the hardware specs are taken from the
 * linux driver, thanks a lot!
 *
 * Authors:
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */


#include "ps2_alps.h"

#include <stdlib.h>
#include <string.h>

#include <keyboard_mouse_driver.h>

#include "ps2_service.h"


const char* kALPSPath[4] = {
	"input/touchpad/ps2/alps_0",
	"input/touchpad/ps2/alps_1",
	"input/touchpad/ps2/alps_2",
	"input/touchpad/ps2/alps_3"
};


enum button_ids
{
	kLeftButton = 0x01,
	kRightButton = 0x02	
};


typedef struct alps_model_info {
	uint8		id[3];
	uint8		firstByte;
	uint8		maskFirstByte;
	uint8		flags;
} alps_model_info;


#define ALPS_OLDPROTO           0x01	// old style input
#define ALPS_DUALPOINT          0x02	// touchpad has trackstick
#define ALPS_PASS               0x04    // device has a pass-through port

#define ALPS_WHEEL              0x08	// hardware wheel present
#define ALPS_FW_BK_1            0x10	// front & back buttons present
#define ALPS_FW_BK_2            0x20	// front & back buttons present
#define ALPS_FOUR_BUTTONS       0x40	// 4 direction button present
#define ALPS_PS2_INTERLEAVED    0x80	// 3-byte PS/2 packet interleaved with
										// 6-byte ALPS packet

static const struct alps_model_info gALPSModelInfos[] = {
//	{{0x32, 0x02, 0x14}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
		// Toshiba Salellite Pro M10 
//	{{0x33, 0x02, 0x0a}, 0x88, 0xf8, ALPS_OLDPROTO},
		// UMAX-530T
	{{0x53, 0x02, 0x0a}, 0xf8, 0xf8, 0},
	{{0x53, 0x02, 0x14}, 0xf8, 0xf8, 0},
	{{0x60, 0x03, 0xc8}, 0xf8, 0xf8, 0},
		// HP ze1115
	{{0x63, 0x02, 0x0a}, 0xf8, 0xf8, 0},
	{{0x63, 0x02, 0x14}, 0xf8, 0xf8, 0},
//	{{0x63, 0x02, 0x28}, 0xf8, 0xf8, ALPS_FW_BK_2},
		// Fujitsu Siemens S6010
//	{{0x63, 0x02, 0x3c}, 0x8f, 0x8f, ALPS_WHEEL},
		// Toshiba Satellite S2400-103
//	{{0x63, 0x02, 0x50}, 0xef, 0xef, ALPS_FW_BK_1},
		// NEC Versa L320
	{{0x63, 0x02, 0x64}, 0xf8, 0xf8, 0},
//	{{0x63, 0x03, 0xc8}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
		// Dell Latitude D800
	{{0x73, 0x00, 0x0a}, 0xf8, 0xf8, ALPS_DUALPOINT},
		// ThinkPad R61 8918-5QG, x301
	{{0x73, 0x02, 0x0a}, 0xf8, 0xf8, 0},
//	{{0x73, 0x02, 0x14}, 0xf8, 0xf8, ALPS_FW_BK_2},
		// Ahtec Laptop
//	{{0x20, 0x02, 0x0e}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
		// XXX
//	{{0x22, 0x02, 0x0a}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
//	{{0x22, 0x02, 0x14}, 0xff, 0xff, ALPS_PASS | ALPS_DUALPOINT},
		// Dell Latitude D600
//	{{0x62, 0x02, 0x14}, 0xcf, 0xcf,  ALPS_PASS | ALPS_DUALPOINT
//		| ALPS_PS2_INTERLEAVED},
		// Dell Latitude E5500, E6400, E6500, Precision M4400
//	{{0x73, 0x02, 0x50}, 0xcf, 0xcf, ALPS_FOUR_BUTTONS},
		// Dell Vostro 1400
//	{{0x52, 0x01, 0x14}, 0xff, 0xff, ALPS_PASS | ALPS_DUALPOINT
//		| ALPS_PS2_INTERLEAVED},
		// Toshiba Tecra A11-11L
	{{0, 0, 0}, 0, 0, 0}
};


static alps_model_info* sFoundModel = NULL;


#define PS2_MOUSE_CMD_SET_SCALE11		0xe6
#define PS2_MOUSE_CMD_SET_SCALE21		0xe7
#define PS2_MOUSE_CMD_SET_RES			0xe8
#define PS2_MOUSE_CMD_GET_INFO			0xe9
#define PS2_MOUSE_CMD_SET_STREAM  		0xea
#define PS2_MOUSE_CMD_SET_POLL			0xf0
#define PS2_MOUSE_CMD_SET_RATE			0xf3


// touchpad proportions
#define SPEED_FACTOR		4.5
#define EDGE_MOTION_WIDTH	10 
#define EDGE_MOTION_SPEED	(5 * SPEED_FACTOR)
// increase the touchpad size a little bit
#define AREA_START_X		50
#define AREA_END_X			985
#define AREA_WIDTH_X		(AREA_END_X - AREA_START_X)
#define AREA_START_Y		45
#define AREA_END_Y			735
#define AREA_WIDTH_Y		(AREA_END_Y - AREA_START_Y)

#define TAP_TIMEOUT			200000

#define MIN_PRESSURE		15
#define MAX_PRESSURE		115

#define ALPS_HISTORY_SIZE	256


typedef struct {
	uint8		buttons;
	uint32		xPosition;
	uint32		yPosition;
	uint8		zPressure;
	// absolut mode
	bool		finger;
	// absolut w mode
	uint8		wValue;
} touch_event;


static bool
edge_motion(mouse_movement *movement, touch_event *event, bool validStart)
{
	int32 xdelta = 0;
	int32 ydelta = 0;

	if (event->xPosition < AREA_START_X + EDGE_MOTION_WIDTH)
		xdelta = -EDGE_MOTION_SPEED;
	else if (event->xPosition > AREA_END_X - EDGE_MOTION_WIDTH)
		xdelta = EDGE_MOTION_SPEED;

	if (event->yPosition < AREA_START_Y + EDGE_MOTION_WIDTH)
		ydelta = -EDGE_MOTION_SPEED;
	else if (event->yPosition > AREA_END_Y - EDGE_MOTION_WIDTH)
		ydelta = EDGE_MOTION_SPEED;

	if (xdelta && validStart)
		movement->xdelta = xdelta;
	if (ydelta && validStart)
		movement->ydelta = ydelta;

	if ((xdelta || ydelta) && !validStart)
		return false;

	return true;
}


/*!	If a button has been clicked (movement->buttons must be set accordingly),
	this function updates the click_count of the \a cookie, as well as the
	\a movement's clicks field.
	Also, it sets the cookie's button state from movement->buttons.
*/
static inline void
update_buttons(alps_cookie *cookie, mouse_movement *movement)
{
	// set click count correctly according to double click timeout
	if (movement->buttons != 0 && cookie->buttons_state == 0) {
		if (cookie->click_last_time + cookie->click_speed > movement->timestamp)
			cookie->click_count++;
		else
			cookie->click_count = 1;

		cookie->click_last_time = movement->timestamp;
	}

	if (movement->buttons != 0)
		movement->clicks = cookie->click_count;

	cookie->buttons_state = movement->buttons;
}


static inline void
no_touch_to_movement(alps_cookie *cookie, touch_event *event,
	mouse_movement *movement)
{
	uint32 buttons = event->buttons;

	TRACE("ALPS: no touch event\n");

	cookie->scrolling_started = false;
	cookie->movement_started = false;

	if (cookie->tapdrag_started
		&& (movement->timestamp - cookie->tap_time) < TAP_TIMEOUT) {
		buttons = 0x01;
	}

	// if the movement stopped switch off the tap drag when timeout is expired
	if ((movement->timestamp - cookie->tap_time) > TAP_TIMEOUT) {
		cookie->tapdrag_started = false;
		cookie->valid_edge_motion = false;
		TRACE("ALPS: tap drag gesture timed out\n");
	}

	if (abs(cookie->tap_delta_x) > 15 || abs(cookie->tap_delta_y) > 15) {
		cookie->tap_started = false;
		cookie->tap_clicks = 0;
	}

	if (cookie->tap_started || cookie->double_click) {
		TRACE("ALPS: tap gesture\n");
		cookie->tap_clicks++;

		if (cookie->tap_clicks > 1) {
			TRACE("ALPS: empty click\n");
			buttons = 0x00;
			cookie->tap_clicks = 0;
			cookie->double_click = true;
		} else {
			buttons = 0x01;
			cookie->tap_started = false;
			cookie->tapdrag_started = true;
			cookie->double_click = false;
		}
	}

	movement->buttons = buttons;
	update_buttons(cookie, movement);
}


static inline void
move_to_movement(alps_cookie *cookie, touch_event *event,
	mouse_movement *movement)
{
	touchpad_settings *settings = &cookie->settings;
	bool isStartOfMovement = false;
	float pressure = 0;

	TRACE("ALPS: movement event\n");
	if (!cookie->movement_started) {
		isStartOfMovement = true;
		cookie->movement_started = true;
		start_new_movment(&cookie->movement_maker);
	}

	get_movement(&cookie->movement_maker, event->xPosition, event->yPosition);

	movement->xdelta = cookie->movement_maker.xDelta;
	movement->ydelta = cookie->movement_maker.yDelta;

	// tap gesture
	cookie->tap_delta_x += cookie->movement_maker.xDelta;
	cookie->tap_delta_y += cookie->movement_maker.yDelta;

	if (cookie->tapdrag_started) {
		movement->buttons = kLeftButton;
		movement->clicks = 0;

		cookie->valid_edge_motion = edge_motion(movement, event,
			cookie->valid_edge_motion);
		TRACE("ALPS: tap drag\n");
	} else {
		TRACE("ALPS: movement set buttons\n");
		movement->buttons = event->buttons;
	}

	// use only a fraction of pressure range, the max pressure seems to be
	// to high
	pressure = 20 * (event->zPressure - MIN_PRESSURE)
		/ (MAX_PRESSURE - MIN_PRESSURE - 40);
	if (!cookie->tap_started
		&& isStartOfMovement
		&& settings->tapgesture_sensibility > 0.
		&& settings->tapgesture_sensibility > (20 - pressure)) {
		TRACE("ALPS: tap started\n");
		cookie->tap_started = true;
		cookie->tap_time = system_time();
		cookie->tap_delta_x = 0;
		cookie->tap_delta_y = 0;
	}

	update_buttons(cookie, movement);
}


/*!	Checks if this is a scrolling event or not, and also actually does the
	scrolling work if it is.

	\return \c true if this was a scrolling event, \c false if not.
*/
static inline bool
check_scrolling_to_movement(alps_cookie *cookie, touch_event *event,
	mouse_movement *movement)
{
	touchpad_settings *settings = &cookie->settings;
	bool isSideScrollingV = false;
	bool isSideScrollingH = false;

	// if a button is pressed don't allow to scroll, we likely be in a drag
	// action
	if (cookie->buttons_state != 0)
		return false;

	if ((AREA_END_X - AREA_WIDTH_X * settings->scroll_rightrange
			< event->xPosition && !cookie->movement_started
		&& settings->scroll_rightrange > 0.000001)
			|| settings->scroll_rightrange > 0.999999) {
		isSideScrollingV = true;
	}
	if ((AREA_START_Y + AREA_WIDTH_Y * settings->scroll_bottomrange
				> event->yPosition && !cookie->movement_started
			&& settings->scroll_bottomrange > 0.000001)
				|| settings->scroll_bottomrange > 0.999999) {
		isSideScrollingH = true;
	}
	if ((event->wValue == 0 || event->wValue == 1)
		&& settings->scroll_twofinger) {
		// two finger scrolling is enabled
		isSideScrollingV = true;
		isSideScrollingH = settings->scroll_twofinger_horizontal;
	}

	if (!isSideScrollingV && !isSideScrollingH) {
		cookie->scrolling_started = false;
		return false;
	}

	TRACE("ALPS: scroll event\n");

	cookie->tap_started = false;
	cookie->tap_clicks = 0;
	cookie->tapdrag_started = false;
	cookie->valid_edge_motion = false;
	if (!cookie->scrolling_started) {
		cookie->scrolling_started = true;
		start_new_movment(&cookie->movement_maker);
	}
	get_scrolling(&cookie->movement_maker, event->xPosition,
		event->yPosition);
	movement->wheel_ydelta = cookie->movement_maker.yDelta;
	movement->wheel_xdelta = cookie->movement_maker.xDelta;

	if (isSideScrollingV && !isSideScrollingH)
		movement->wheel_xdelta = 0;
	else if (isSideScrollingH && !isSideScrollingV)
		movement->wheel_ydelta = 0;

	cookie->buttons_state = movement->buttons;

	return true;
}


static status_t
event_to_movement(alps_cookie *cookie, touch_event *event,
	mouse_movement *movement)
{
	if (!movement)
		return B_ERROR;

	movement->xdelta = 0;
	movement->ydelta = 0;
	movement->buttons = 0;
	movement->wheel_ydelta = 0;
	movement->wheel_xdelta = 0;
	movement->modifiers = 0;
	movement->clicks = 0;
	movement->timestamp = system_time();

	if ((movement->timestamp - cookie->tap_time) > TAP_TIMEOUT) {
		TRACE("ALPS: tap gesture timed out\n");
		cookie->tap_started = false;
		if (!cookie->double_click
			|| (movement->timestamp - cookie->tap_time) > 2 * TAP_TIMEOUT) {
			cookie->tap_clicks = 0;
		}
	}

	if (event->buttons & kLeftButton) {
		cookie->tap_clicks = 0;
		cookie->tapdrag_started = false;
		cookie->tap_started = false;
		cookie->valid_edge_motion = false;
	}

	if (event->zPressure >= MIN_PRESSURE && event->zPressure < MAX_PRESSURE
		&& ((event->wValue >=4 && event->wValue <=7)
			|| event->wValue == 0 || event->wValue == 1)
		&& (event->xPosition != 0 || event->yPosition != 0)) {
		// The touch pad is in touch with at least one finger
		if (!check_scrolling_to_movement(cookie, event, movement))
			move_to_movement(cookie, event, movement);
	} else
		no_touch_to_movement(cookie, event, movement);

	return B_OK;
}


/* Data taken from linux driver:
ALPS absolute Mode - new format
byte 0:  1    ?    ?    ?    1    ?    ?    ?
byte 1:  0   x6   x5   x4   x3   x2   x1   x0
byte 2:  0  x10   x9   x8   x7    ?  fin  ges
byte 3:  0   y9   y8   y7    1    M    R    L
byte 4:  0   y6   y5   y4   y3   y2   y1   y0
byte 5:  0   z6   z5   z4   z3   z2   z1   z0
*/
// debug 
static int minX = 50000;
static int minY = 50000;
static int maxX = 0;
static int maxY = 0;
static int maxZ = 0;
// debug end
static status_t
get_alps_movment(alps_cookie *cookie, mouse_movement *movement)
{
	status_t status;
	touch_event event;
	uint8 event_buffer[PS2_PACKET_ALPS];

	status = acquire_sem_etc(cookie->sem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (!cookie->dev->active) {
		TRACE("ALPS: read_event: Error device no longer active\n");
		return B_ERROR;
	}

	if (packet_buffer_read(cookie->ring_buffer, event_buffer,
			cookie->dev->packet_size) != cookie->dev->packet_size) {
		TRACE("ALPS: error copying buffer\n");
		return B_ERROR;
	}

	event.buttons = event_buffer[3] & 3;
 	event.zPressure = event_buffer[5];

	// finger on touchpad
	if (event_buffer[2] & 0x2) {
		// finger with normal width
		event.wValue = 4;
	} else {
		event.wValue = 3;
	}

	// tabgesture
	if (event_buffer[2] & 0x1) {
		event.zPressure = 80;
		event.wValue = 4;
	}

 	event.xPosition = event_buffer[1] | ((event_buffer[2] & 0x78) << 4);
 	event.yPosition = event_buffer[4] | ((event_buffer[3] & 0x70) << 3);

// debug 
if (event.xPosition < minX)
	minX = event.xPosition;
if (event.yPosition < minY)
	minY = event.yPosition;
if (event.xPosition > maxX)
	maxX = event.xPosition;
if (event.yPosition > maxY)
	maxY = event.yPosition;
if (event.zPressure > maxZ)
	maxZ = event.zPressure;
dprintf("x %i %i, y %i %i, z %i %i, fin %i ges %i\n", minX, maxX, minY, maxY,
	maxZ, event.zPressure, event_buffer[2] & 0x2, event_buffer[2] & 0x1);
// debug end

	event.yPosition = AREA_END_Y - event.yPosition;
 	status = event_to_movement(cookie, &event, movement);

	// check for trackpoint even (z pressure 127)
	if (sFoundModel->flags & ALPS_DUALPOINT && event.zPressure == 127) {
		movement->xdelta = event.xPosition > 383 ? event.xPosition - 768
			: event.xPosition;
		event.yPosition = AREA_END_Y - event.yPosition;
		movement->ydelta = event.yPosition > 255
			? event.yPosition - 512 : event.yPosition;
		event.yPosition = AREA_END_Y - event.yPosition;
		movement->wheel_xdelta = 0;
		movement->wheel_ydelta = 0;
	}
	return status;
}


static void
default_settings(touchpad_settings *set)
{
	memcpy(set, &kDefaultTouchpadSettings, sizeof(touchpad_settings));
}


status_t
probe_alps(ps2_dev* dev)
{
	int i;
	uint8 val[3];
	TRACE("ALPS: probe\n");
return B_ERROR;
	val[0] = 0;
	if (ps2_dev_command(dev, PS2_MOUSE_CMD_SET_RES, val, 1, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_MOUSE_CMD_SET_SCALE11, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_MOUSE_CMD_SET_SCALE11, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_MOUSE_CMD_SET_SCALE11, NULL, 0, NULL, 0)
			!= B_OK)
		return B_ERROR;

	if (ps2_dev_command(dev, PS2_MOUSE_CMD_GET_INFO, NULL, 0, val, 3)
		!= B_OK)
		return B_ERROR;

	if (val[0] != 0 || val[1] != 0 || (val[2] != 10 && val[2] != 100))
		return B_ERROR;

	val[0] = 0;
	if (ps2_dev_command(dev, PS2_MOUSE_CMD_SET_RES, val, 1, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_MOUSE_CMD_SET_SCALE21, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_MOUSE_CMD_SET_SCALE21, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_MOUSE_CMD_SET_SCALE21, NULL, 0, NULL, 0)
			!= B_OK)
		return B_ERROR;

	if (ps2_dev_command(dev, PS2_MOUSE_CMD_GET_INFO, NULL, 0, val, 3)
		!= B_OK)
		return B_ERROR;

	for (i = 0; ; i++) {
		const alps_model_info* info = &gALPSModelInfos[i];
		if (info->id[0] == 0) {
			INFO("ALPS not supported: %2.2x %2.2x %2.2x\n", val[0], val[1],
				val[2]);
			return B_ERROR;
		}

		if (info->id[0] == val[0] && info->id[1] == val[1]
			&& info->id[2] == val[2]) {
			sFoundModel = (alps_model_info*)info;
			INFO("ALPS found: %2.2x %2.2x %2.2x\n", val[0], val[1], val[2]);
			break;
		}
	}

	dev->name = kALPSPath[dev->idx];
	dev->packet_size = -1;

	return B_OK;
}


status_t
alps_open(const char *name, uint32 flags, void **_cookie)
{
	alps_cookie *cookie;
	ps2_dev *dev;
	int i;
	uint8 val[3];
	uint8 arg;

	for (dev = NULL, i = 0; i < PS2_DEVICE_COUNT; i++) {
		if (0 == strcmp(ps2_device[i].name, name)) {
			dev = &ps2_device[i];
			break;
		}
	}

	if (dev == NULL) {
		TRACE("ps2: dev = NULL\n");
		return B_ERROR;
	}

	if (atomic_or(&dev->flags, PS2_FLAG_OPEN) & PS2_FLAG_OPEN)
		return B_BUSY;

	cookie = (alps_cookie*)malloc(sizeof(alps_cookie));
	if (cookie == NULL)
		goto err1;

	*_cookie = cookie;
	memset(cookie, 0, sizeof(alps_cookie));

	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &alps_disconnect;
	dev->handle_int = &alps_handle_int;

	default_settings(&cookie->settings);

	cookie->movement_maker.speed = 1 * SPEED_FACTOR;
	cookie->movement_maker.scrolling_xStep = cookie->settings.scroll_xstepsize;
	cookie->movement_maker.scrolling_yStep = cookie->settings.scroll_ystepsize;
	cookie->movement_maker.scroll_acceleration
		= cookie->settings.scroll_acceleration;
	cookie->movement_started = false;
	cookie->scrolling_started = false;
	cookie->tap_started = false;
	cookie->double_click = false;
	cookie->valid_edge_motion = false;

	dev->packet_size = PS2_PACKET_ALPS;

	cookie->ring_buffer
		= create_packet_buffer(ALPS_HISTORY_SIZE * dev->packet_size);
	if (cookie->ring_buffer == NULL) {
		TRACE("ALPS: can't allocate mouse actions buffer\n");
		goto err2;
	}
	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	cookie->sem = create_sem(0, "ps2_alps_sem");
	if (cookie->sem < 0) {
		TRACE("ALPS: failed creating semaphore!\n");
		goto err3;
	}

	// switch tap mode off
	arg = 0x00;
	if (ps2_dev_command(dev, PS2_MOUSE_CMD_GET_INFO, NULL, 0, val, 3) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_MOUSE_CMD_SET_RES, &arg, 1, NULL, 0) != B_OK)
		goto err4;

	// init the alps device to absolut mode
	if (ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0) != B_OK)
		goto err4;

	if (ps2_dev_command(dev, PS2_MOUSE_CMD_SET_STREAM, NULL, 0, NULL, 0) != B_OK)
		goto err4;

	if (ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0) != B_OK)
		goto err4;

	atomic_or(&dev->flags, PS2_FLAG_ENABLED);

	TRACE("ALPS: open %s success\n", name);
	return B_OK;

err4:
	delete_sem(cookie->sem);
err3:
	delete_packet_buffer(cookie->ring_buffer);
err2:
	free(cookie);
err1:
	atomic_and(&dev->flags, ~PS2_FLAG_OPEN);

	TRACE("ALPS: open %s failed\n", name);
	return B_ERROR;
}


status_t
alps_close(void *_cookie)
{
	status_t status;
	alps_cookie *cookie = _cookie;

	ps2_dev_command_timeout(cookie->dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0,
		150000);

	delete_packet_buffer(cookie->ring_buffer);
	delete_sem(cookie->sem);

	atomic_and(&cookie->dev->flags, ~PS2_FLAG_OPEN);
	atomic_and(&cookie->dev->flags, ~PS2_FLAG_ENABLED);

	// Reset the touchpad so it generate standard ps2 packets instead of
	// extended ones. If not, BeOS is confused with such packets when rebooting
	// without a complete shutdown.
	status = ps2_reset_mouse(cookie->dev);
	if (status != B_OK) {
		INFO("ps2: reset failed\n");
		return B_ERROR;
	}

	TRACE("ALPS: close %s done\n", cookie->dev->name);
	return B_OK;
}


status_t
alps_freecookie(void *_cookie)
{
	free(_cookie);
	return B_OK;
}


status_t
alps_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	alps_cookie *cookie = _cookie;
	mouse_movement movement;
	status_t status;

	switch (op) {
		case MS_READ:
			TRACE("ALPS: MS_READ get event\n");
			if ((status = get_alps_movment(cookie, &movement)) != B_OK)
				return status;
			return user_memcpy(buffer, &movement, sizeof(movement));

		case MS_IS_TOUCHPAD:
			TRACE("ALPS: MS_IS_TOUCHPAD\n");
			return B_OK;

		case MS_SET_TOUCHPAD_SETTINGS:
			TRACE("ALPS: MS_SET_TOUCHPAD_SETTINGS");
			user_memcpy(&cookie->settings, buffer, sizeof(touchpad_settings));
			cookie->movement_maker.scrolling_xStep
				= cookie->settings.scroll_xstepsize;
			cookie->movement_maker.scrolling_yStep
				= cookie->settings.scroll_ystepsize;
			cookie->movement_maker.scroll_acceleration
				= cookie->settings.scroll_acceleration;
			return B_OK;

		case MS_SET_CLICKSPEED:
			TRACE("ALPS: ioctl MS_SETCLICK (set click speed)\n");
			return user_memcpy(&cookie->click_speed, buffer, sizeof(bigtime_t));

		default:
			TRACE("ALPS: unknown opcode: %ld\n", op);
			return B_BAD_VALUE;
	}
}


static status_t
alps_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
alps_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


int32
alps_handle_int(ps2_dev *dev)
{
	alps_cookie *cookie = dev->cookie;
	uint8 val;

	val = cookie->dev->history[0].data;
//dprintf("%i %i %i %i %i %i %i %i\n", (val >> 7) & 0x1, (val >> 6) & 0x1, (val >> 5) & 0x1, (val >> 4) & 0x1, (val >> 3) & 0x1,
//	(val >> 2) & 0x1,  (val >> 1) & 0x1,  (val >> 0) & 0x1);
	if (cookie->packet_index == 0
		&& (val & sFoundModel->maskFirstByte) != sFoundModel->firstByte) {
		INFO("ALPS: bad header, trying resync\n");
		cookie->packet_index = 0;
		return B_UNHANDLED_INTERRUPT;
	}

	// data packages starting with a 0
	if (cookie->packet_index > 1 && (val & 0x80)) {
		INFO("ALPS: bad package data, trying resync\n");
		cookie->packet_index = 0;
		return B_UNHANDLED_INTERRUPT;
	}

 	cookie->packet_buffer[cookie->packet_index] = val;

	cookie->packet_index++;
	if (cookie->packet_index >= 6) {
		cookie->packet_index = 0;

		if (packet_buffer_write(cookie->ring_buffer,
				cookie->packet_buffer, cookie->dev->packet_size)
			!= cookie->dev->packet_size) {
			// buffer is full, drop new data
			return B_HANDLED_INTERRUPT;
		}
		release_sem_etc(cookie->sem, 1, B_DO_NOT_RESCHEDULE);

		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}


void
alps_disconnect(ps2_dev *dev)
{
	alps_cookie *cookie = dev->cookie;
	// the mouse device might not be opened at this point
	INFO("ALPS: alps_disconnect %s\n", dev->name);
	if ((dev->flags & PS2_FLAG_OPEN) != 0)
		release_sem(cookie->sem);
}


device_hooks gALPSDeviceHooks = {
	alps_open,
	alps_close,
	alps_freecookie,
	alps_ioctl,
	alps_read,
	alps_write,
};

