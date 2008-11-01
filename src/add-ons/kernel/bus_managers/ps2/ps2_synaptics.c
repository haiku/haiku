/*
 * Copyright 2008 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 synaptics touchpad
 *
 * Authors (in chronological order):
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */
 
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "ps2_service.h"
#include "ps2_synaptics.h"
#include "kb_mouse_driver.h"

const char* kSynapticsPath[4] = {
	"input/touchpad/ps2/synaptics_0",
	"input/touchpad/ps2/synaptics_1",
	"input/touchpad/ps2/synaptics_2",
	"input/touchpad/ps2/synaptics_3"
};

static touchpad_info gTouchpadInfo;
ps2_dev *gPassthroughDevice = &ps2_device[PS2_DEVICE_SYN_PASSTHROUGH];


void 
default_settings(touchpad_settings *set)
{
	memcpy(set, &kDefaultTouchpadSettings, sizeof(touchpad_settings));
}


status_t
synaptics_pt_set_packagesize(ps2_dev *dev, uint8 size)
{
	synaptics_cookie* syn_cookie = dev->parent_dev->cookie;	
	
	status_t status = ps2_dev_command(dev->parent_dev, PS2_CMD_DISABLE, NULL,
		0, NULL, 0);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot disable touchpad %s\n", dev->parent_dev->name);
		return B_ERROR;
	}
		
	syn_cookie->packet_index = 0;
		
	if (size == 4)
		syn_cookie->mode|= SYN_FOUR_BYTE_CHILD;
	else
		syn_cookie->mode&= ~SYN_FOUR_BYTE_CHILD;
	set_touchpad_mode(dev->parent_dev, syn_cookie->mode);
		
	status = ps2_dev_command(dev->parent_dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot enable touchpad %s\n", dev->parent_dev->name);
		return B_ERROR;
	}
	return status;
}


status_t 
send_touchpad_arg(ps2_dev *dev, uint8 arg)
{
	return send_touchpad_arg_timeout(dev, arg, 4000000);
}


status_t 
send_touchpad_arg_timeout(ps2_dev *dev, uint8 arg, bigtime_t timeout)
{
	int8 i;
	uint8 val[8];
	for (i = 0; i < 4; i++) {
		val[2*i] = (arg >> (6-2*i)) & 3;
		val[2*i + 1] = 0xE8;
	}
	return ps2_dev_command_timeout(dev, 0xE8, val, 7, NULL, 0, timeout);
}


status_t 
set_touchpad_mode(ps2_dev *dev, uint8 mode)
{
	uint8 sample_rate = SYN_CHANGE_MODE;
	send_touchpad_arg(dev, mode);
	return ps2_dev_command(dev, PS2_CMD_SET_SAMPLE_RATE, &sample_rate, 1,
		NULL, 0);
}


status_t
passthrough_command(ps2_dev *dev, uint8 cmd, const uint8 *out, int out_count,
	uint8 *in, int in_count, bigtime_t timeout)
{
	status_t status;
	uint8 pt_cmd = SYN_PASSTHROUGH_CMD;
	uint8 val;
	uint32 pt_in_count = (in_count + 1) * 6;
	uint8 pt_in[pt_in_count];
	int8 i;
	
	TRACE("SYNAPTICS: passthrough command 0x%x\n", cmd);
		
	status = ps2_dev_command(dev->parent_dev, PS2_CMD_DISABLE, NULL, 0,
		NULL, 0);
	if (status != B_OK)
		return status;
	
	for (i = -1; i < out_count; i++) {
		if (i == -1)
			val = cmd;
		else
			val = out[i];
		status = send_touchpad_arg_timeout(dev->parent_dev, val, timeout);
		if (status != B_OK)
			return status;
		if (i != out_count -1) {
			status = ps2_dev_command_timeout(dev->parent_dev,
				PS2_CMD_SET_SAMPLE_RATE, &pt_cmd, 1, NULL, 0, timeout);
			if (status != B_OK)
				return status;
		}
	}
	status = ps2_dev_command_timeout(dev->parent_dev, PS2_CMD_SET_SAMPLE_RATE,
		&pt_cmd, 1, pt_in, pt_in_count, timeout);
	if (status != B_OK)
		return status;
	
	for (i = 0; i < in_count + 1; i++) {
		uint8 *inPointer = &(pt_in[i * 6]);
		if (!IS_SYN_PT_PACKAGE(inPointer)) {
			TRACE("SYNAPTICS: not a pass throught package\n");
			return B_OK;
		}
		if (i == 0)
			continue;
		in[i - 1] = pt_in[i * 6 + 1];
	}
	
	status = ps2_dev_command(dev->parent_dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0);
	if (status != B_OK)
		return status;
	
	return B_OK;
}


bool
edge_motion(mouse_movement *movement, touch_event *event, bool validStart)
{
	int32 xdelta = 0;
	int32 ydelta = 0;
	
	if (event->xPosition < SYN_AREA_START_X + SYN_EDGE_MOTION_WIDTH)
		xdelta = -SYN_EDGE_MOTION_SPEED;
	else if (event->xPosition > SYN_AREA_END_X - SYN_EDGE_MOTION_WIDTH)
		xdelta = SYN_EDGE_MOTION_SPEED;
	
	if (event->yPosition < SYN_AREA_START_Y + SYN_EDGE_MOTION_WIDTH)
		ydelta = -SYN_EDGE_MOTION_SPEED;
	else if (event->yPosition > SYN_AREA_END_Y - SYN_EDGE_MOTION_WIDTH)
		ydelta = SYN_EDGE_MOTION_SPEED;
		
	if (xdelta && validStart)
		movement->xdelta = xdelta;
	if (ydelta && validStart)
		movement->ydelta = ydelta;
	
	if ((xdelta || ydelta) && !validStart)
		return false;

	return true;
}


status_t
touchevent_to_movement(synaptics_cookie* cookie, touch_event *event,
	mouse_movement *movement)
{
	bool isSideScrollingV = false;
	bool isSideScrollingH = false;
	bool isInTouch = false;
	bool isStartOfMovement = false;
	float sens = 0;
	bigtime_t currentTime = system_time();
	touchpad_settings * settings = &(cookie->settings);
	
	if (!movement)
		return B_ERROR;
	
	movement->xdelta = 0;
	movement->ydelta = 0;
	movement->buttons = 0;
	movement->wheel_ydelta = 0;
	movement->wheel_xdelta = 0;
	movement->modifiers = 0;
	movement->clicks = 0;
	movement->timestamp = currentTime;
		
	if ((currentTime - cookie->tap_time) > SYN_TAP_TIMEOUT) {
		TRACE("SYNAPTICS: tap gesture timed out\n");
		cookie->tap_started = false;
		if (!cookie->double_click
			|| (currentTime - cookie->tap_time) > 2 * SYN_TAP_TIMEOUT) {
			cookie->tap_clicks = 0;
		}
	}
	
	if (event->buttons != 0) {
		cookie->tap_clicks = 0;
		cookie->tapdrag_started = false;
		cookie->tap_started = false;
		cookie->valid_edge_motion = false;
	}
	
	if (event->zPressure >= MIN_PRESSURE && event->zPressure < MAX_PRESSURE
		&& ((event->wValue >=4 && event->wValue <=7)
				|| event->wValue == 0 || event->wValue == 1)
		&& (event->xPosition != 0 || event->yPosition != 0)) {
		isInTouch = true;
	}
	
	if (isInTouch) {
		if ((SYN_AREA_END_X - SYN_AREA_WIDTH_X * settings->scroll_rightrange
			< event->xPosition && !cookie->movement_started
			&& settings->scroll_rightrange > 0.000001)
				|| settings->scroll_rightrange > 0.999999) {
			isSideScrollingV = true;
		}
		if ((SYN_AREA_START_Y + SYN_AREA_WIDTH_Y * settings->scroll_bottomrange
				> event->yPosition && !cookie->movement_started
				&& settings->scroll_bottomrange > 0.000001)
					|| settings->scroll_bottomrange > 0.999999) {
			isSideScrollingH = true;
		}
		if (isSideScrollingV || isSideScrollingH
			||(event->wValue == 0 && settings->scroll_twofinger)
			||(event->wValue == 1 && settings->scroll_multifinger)) {
			goto scrolling;
		} else {
			cookie->scrolling_started = false;
		}
		goto movement;
	} else {
		goto notouch;
	}

movement:
	TRACE("SYNAPTICS: movement event\n");
	if (!cookie->movement_started) {
		isStartOfMovement = true;
		cookie->movement_started = true;
		start_new_movment(&(cookie->movement_maker));
	}
				
	get_movement(&(cookie->movement_maker), event->xPosition, event->yPosition);
				
	movement->xdelta = cookie->movement_maker.xDelta;
	movement->ydelta = cookie->movement_maker.yDelta;
		
	// tap gesture
	cookie->tap_delta_x+= cookie->movement_maker.xDelta;
	cookie->tap_delta_y+= cookie->movement_maker.yDelta;
	
	if (cookie->tapdrag_started) {
		movement->buttons = 0x01;	// left button
		movement->clicks = 0;
		
		cookie->valid_edge_motion = edge_motion(movement, event,
			cookie->valid_edge_motion);
		TRACE("SYNAPTICS: tap drag\n");
	} else {
		TRACE("SYNAPTICS: movement set buttons\n");
		movement->buttons = event->buttons;
	}
	
	// use only a fraction of pressure range, the max pressure seems to be
	// to high
	sens = 20 * (event->zPressure - MIN_PRESSURE)
		/ (MAX_PRESSURE - MIN_PRESSURE - 100);
	if (!cookie->tap_started
		&& isStartOfMovement 
		&& settings->tapgesture_sensibility > (20 - sens)) {		
		TRACE("SYNAPTICS: tap started\n");
		cookie->tap_started = true;
		cookie->tap_time = system_time();
		cookie->tap_delta_x = 0;
		cookie->tap_delta_y = 0;
	}
			
	return B_OK;

scrolling:
	TRACE("SYNAPTICS: scroll event\n");
	cookie->tap_started = false;
	cookie->tap_clicks = 0;
	cookie->tapdrag_started = false;
	cookie->valid_edge_motion = false;
	if (!cookie->scrolling_started) {
		cookie->scrolling_started = true;
		start_new_movment(&(cookie->movement_maker));
	}
	get_scrolling(&(cookie->movement_maker), event->xPosition,
		event->yPosition);
	movement->wheel_ydelta = cookie->movement_maker.yDelta;
	movement->wheel_xdelta = cookie->movement_maker.xDelta;
	
	if (isSideScrollingV && !isSideScrollingH)
		movement->wheel_xdelta = 0;
	else if (isSideScrollingH && !isSideScrollingV)
		movement->wheel_ydelta = 0;
	return B_OK;
	
notouch:
	TRACE("SYNAPTICS: no touch event\n");
	cookie->scrolling_started = false;
	cookie->movement_started = false;
	movement->buttons = event->buttons;
	if (event->buttons)
		movement->clicks = 1;
	
	if (cookie->tapdrag_started
		&& (currentTime - cookie->tap_time) < SYN_TAP_TIMEOUT) {
		movement->buttons = 0x01;
		movement->clicks = 0;
	}
		
	// if the movement stopped switch off the dap trag when timeout is expired
	if ((currentTime - cookie->tap_time) > SYN_TAP_TIMEOUT) {
		cookie->tapdrag_started = false;
		cookie->valid_edge_motion = false;
		TRACE("SYNAPTICS: tap drag gesture timed out\n");
	}
	
	if (abs(cookie->tap_delta_x) > 15 || abs(cookie->tap_delta_y) > 15) {
		cookie->tap_started = false;
		cookie->tap_clicks = 0;
	}
		
	if (cookie->tap_started || cookie->double_click) {
		TRACE("SYNAPTICS: tap gesture\n");
		cookie->tap_clicks++;
		
		if (cookie->tap_clicks > 1) {
			TRACE("SYNAPTICS: empty click\n");
			movement->buttons = 0x00;
			movement->clicks = 0;
			cookie->tap_clicks = 0;
			cookie->double_click = true;
		} else {
			movement->buttons = 0x01;
			movement->clicks = 1;
			cookie->tap_started = false;
			cookie->tapdrag_started = true;
			cookie->double_click = false;
		}
	}
	
	return B_OK;
}


status_t 
get_synaptics_movment(synaptics_cookie* cookie, mouse_movement *movement)
{
	status_t status;
	touch_event event;
	uint8 event_buffer[PS2_MAX_PACKET_SIZE];
	uint8 wValue0, wValue1, wValue2, wValue3, wValue;
	uint32 val32;
	uint32 xTwelfBit, yTwelfBit;
	
	status = acquire_sem_etc(cookie->synaptics_sem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (!cookie->dev->active) {
		TRACE("SYNAPTICS: read_event: Error device no longer active\n");
		return B_ERROR;
	}
	
	if (packet_buffer_read(cookie->synaptics_ring_buffer, event_buffer,
			cookie->dev->packet_size) != cookie->dev->packet_size) {
		TRACE("SYNAPTICS: error copying buffer\n");
		return B_ERROR;
	}
	 		 	
	event.buttons = event_buffer[0] & 3;
 	event.zPressure = event_buffer[2];
 	
 	if (gTouchpadInfo.capExtended) {
 		wValue0 = event_buffer[3] >> 2 & 1;
	 	wValue1 = event_buffer[0] >> 2 & 1;
 		wValue2 = event_buffer[0] >> 4 & 1;
	 	wValue3 = event_buffer[0] >> 5 & 1;
 	 	
 		wValue = wValue0;
 		wValue = wValue | (wValue1 << 1);
 		wValue = wValue | (wValue2 << 2);
 		wValue = wValue | (wValue3 << 3);
 	
	 	event.wValue = wValue;
	 	event.gesture = false;
 	} else {
 		bool finger = event_buffer[0] >> 5 & 1;
 		if (finger) {
 			// finger with normal width
 			event.wValue = 4;
 		}
 		event.gesture = event_buffer[0] >> 2 & 1;
 	}
 	 	
 	event.xPosition = event_buffer[4];
 	event.yPosition = event_buffer[5];
 	
 	val32 = event_buffer[1] & 0x0F;
 	event.xPosition+= val32 << 8;
 	val32 = event_buffer[1] >> 4 & 0x0F;
 	event.yPosition+= val32 << 8;
 	
 	xTwelfBit = event_buffer[3] >> 4 & 1;
 	event.xPosition+= xTwelfBit << 12;
 	yTwelfBit = event_buffer[3] >> 5 & 1;
 	event.yPosition+= yTwelfBit << 12;
 	
 	status = touchevent_to_movement(cookie, &event, movement);
 		
	return B_OK;
}


void
query_capability(ps2_dev *dev)
{
	uint8 val[3];
	send_touchpad_arg(dev, 0x02);
	ps2_dev_command(dev, 0xE9, NULL, 0, val, 3);
	
	gTouchpadInfo.capExtended = val[0] >> 7 & 1;
	TRACE("SYNAPTICS: extended mode %2x\n", val[0] >> 7 & 1);
	TRACE("SYNAPTICS: sleep mode %2x\n", val[2] >> 4 & 1);
	gTouchpadInfo.capSleep = val[2] >> 4 & 1;
	TRACE("SYNAPTICS: four buttons %2x\n", val[2] >> 3 & 1);
	gTouchpadInfo.capFourButtons = val[2] >> 3 & 1;
	TRACE("SYNAPTICS: multi finger %2x\n", val[2] >> 1 & 1);
	gTouchpadInfo.capMultiFinger = val[2] >> 1 & 1;
	TRACE("SYNAPTICS: palm detection %2x\n", val[2] & 1);
	gTouchpadInfo.capPalmDetection = val[2] & 1;
	TRACE("SYNAPTICS: pass through %2x\n", val[2] >> 7 & 1);
	gTouchpadInfo.capPassThrough = val[2] >> 7 & 1;
}


status_t
probe_synaptics(ps2_dev *dev)
{
	uint8 val[3];
	uint8 deviceId;
	TRACE("SYNAPTICS: probe\n");
	
	send_touchpad_arg(dev, 0x00);
	ps2_dev_command(dev, 0xE9, NULL, 0, val, 3);
	
	gTouchpadInfo.minorVersion = val[0];
	deviceId = val[1];
	if (deviceId != SYN_TOUCHPAD) {
		TRACE("SYNAPTICS: not found\n");
		return B_ERROR;
	}
		
	TRACE("SYNAPTICS: Touchpad found id:l %2x\n", deviceId);
	gTouchpadInfo.majorVersion = val[2] & 0x0F;
	TRACE("SYNAPTICS: version %d.%d\n", gTouchpadInfo.majorVersion,
		gTouchpadInfo.minorVersion);
	// version >= 4.0?
	if (gTouchpadInfo.minorVersion <= 2
			&& gTouchpadInfo.majorVersion <= 3) {
		TRACE("SYNAPTICS: too old touchpad not supported\n");
		return B_ERROR;
	}
	dev->name = kSynapticsPath[dev->idx];
	return B_OK;
}


status_t 
synaptics_open(const char *name, uint32 flags, void **_cookie)
{
	status_t status;
	synaptics_cookie* cookie;
	ps2_dev *dev;
	int i;
	
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
		
	cookie = (synaptics_cookie*)malloc(sizeof(synaptics_cookie));	
	if (cookie == NULL)
		goto err1;
		
	*_cookie = cookie;
	memset(cookie, 0, sizeof(synaptics_cookie));
	
	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &synaptics_disconnect;
	dev->handle_int = &synaptics_handle_int;
	
	default_settings(&(cookie->settings));
	
	cookie->movement_maker.speed = 1;
	cookie->movement_maker.scrolling_xStep = cookie->settings.scroll_xstepsize;
	cookie->movement_maker.scrolling_yStep = cookie->settings.scroll_ystepsize;
	cookie->movement_maker.scroll_acceleration
		= cookie->settings.scroll_acceleration;
	cookie->movement_started = false;
	cookie->scrolling_started = false;
	cookie->tap_started = false;
	cookie->double_click = false;
	cookie->valid_edge_motion = false;
	
	dev->packet_size = PS2_PACKET_SYNAPTICS;
	
	cookie->synaptics_ring_buffer
		= create_packet_buffer(SYNAPTICS_HISTORY_SIZE * dev->packet_size);
	if (cookie->synaptics_ring_buffer == NULL) {
		TRACE("ps2: can't allocate mouse actions buffer\n");
		goto err2;
	}
	
	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	cookie->synaptics_sem = create_sem(0, "ps2_synaptics_sem");
	if (cookie->synaptics_sem < 0) {
		TRACE("SYNAPTICS: failed creating semaphore!\n");
		goto err3;
	}
	query_capability(dev);
	
	// create pass through dev
	if (gTouchpadInfo.capPassThrough) {
		TRACE("SYNAPTICS: pass through detected\n");
		gPassthroughDevice->parent_dev = dev;
		gPassthroughDevice->idx = dev->idx;
		ps2_service_notify_device_added(gPassthroughDevice);
	}
	
	// Set Mode
	if (gTouchpadInfo.capExtended)
		cookie->mode = SYN_ABSOLUTE_W_MODE;
	else
		cookie->mode = SYN_ABSOLUTE_MODE;

	status = set_touchpad_mode(dev, cookie->mode);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot set mode %s\n", name);
		goto err4;
	}	
	
	status = ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot enable touchpad %s\n", name);
		goto err4;
	}
	
	atomic_or(&dev->flags, PS2_FLAG_ENABLED);
				
	TRACE("SYNAPTICS: open %s success\n", name);
	return B_OK;
	
err4:
	delete_sem(cookie->synaptics_sem);
err3:
	delete_packet_buffer(cookie->synaptics_ring_buffer);
err2:
	free(cookie);
err1:
	atomic_and(&dev->flags, ~PS2_FLAG_OPEN);
	
	TRACE("SYNAPTICS: synaptics_open %s failed\n", name);
	return B_ERROR;
}


status_t
synaptics_close(void *_cookie)
{
	synaptics_cookie *cookie = _cookie;

	ps2_dev_command_timeout(cookie->dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0,
		150000);

	delete_packet_buffer(cookie->synaptics_ring_buffer);
	delete_sem(cookie->synaptics_sem);

	atomic_and(&cookie->dev->flags, ~PS2_FLAG_OPEN);
	atomic_and(&cookie->dev->flags, ~PS2_FLAG_ENABLED);
	
	if (gTouchpadInfo.capPassThrough)
		ps2_service_notify_device_removed(gPassthroughDevice);
	
	TRACE("SYNAPTICS: close %s done\n", cookie->dev->name);
	return B_OK;
}


status_t
synaptics_freecookie(void *_cookie)
{
	free(_cookie);
	return B_OK;
}


static status_t
synaptics_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
synaptics_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


status_t
synaptics_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	synaptics_cookie *cookie = _cookie;
	mouse_movement movement;
	status_t status;
	
	switch (op) {
		case MS_READ:
			TRACE("SYNAPTICS: MS_READ get event\n");
			if ((status = get_synaptics_movment(cookie, &movement)) != B_OK)
				return status;
			return user_memcpy(buffer, &movement, sizeof(movement));
		case MS_IS_TOUCHPAD:
			TRACE("SYNAPTICS: MS_IS_TOUCHPAD\n");	
			return B_OK;
		case MS_SET_TOUCHPAD_SETTINGS:
			TRACE("SYNAPTICS: MS_SET_TOUCHPAD_SETTINGS");
			user_memcpy(&(cookie->settings), buffer, sizeof(touchpad_settings));
			cookie->movement_maker.scrolling_xStep
				= cookie->settings.scroll_xstepsize;
			cookie->movement_maker.scrolling_yStep
				= cookie->settings.scroll_ystepsize;
			cookie->movement_maker.scroll_acceleration
				= cookie->settings.scroll_acceleration;
			return B_OK;
		default:
			TRACE("SYNAPTICS: unknown opcode: %ld\n", op);
			return B_BAD_VALUE;
	}
}


int32 
synaptics_handle_int(ps2_dev *dev)
{
	synaptics_cookie *cookie = dev->cookie;
	uint8 val;
	
	val = cookie->dev->history[0].data;
		
	if ((cookie->packet_index == 0 || cookie->packet_index == 3) && val & 8) {
		INFO("SYNAPTICS: bad mouse data, trying resync\n");
		cookie->packet_index = 0;
		goto unhandled;
	}
	if (cookie->packet_index == 0 && val >> 6 != 0x02) {
	 	TRACE("SYNAPTICS: first package begins not with bit 1, 0\n");
 		goto unhandled;
 	}
 	if (cookie->packet_index == 3 && val >> 6 != 0x03) {
	 	TRACE("SYNAPTICS: third package begins not with bit 1, 1\n");
	 	cookie->packet_index = 0;
 		goto unhandled;
 	}
 	cookie->packet_buffer[cookie->packet_index] = val;
	
	cookie->packet_index++;
	if (cookie->packet_index >= 6) {
		cookie->packet_index = 0;
		
		// check if package is a pass through package if true pass it
		// too the pass through interrupt handle
		if (gPassthroughDevice->active 
			&& gPassthroughDevice->handle_int != NULL
			&& IS_SYN_PT_PACKAGE(cookie->packet_buffer)) {
			status_t status;
			
			gPassthroughDevice->history[0].data = cookie->packet_buffer[1];
			gPassthroughDevice->handle_int(gPassthroughDevice);
			gPassthroughDevice->history[0].data = cookie->packet_buffer[4];
			gPassthroughDevice->handle_int(gPassthroughDevice);
			gPassthroughDevice->history[0].data = cookie->packet_buffer[5];
			status = gPassthroughDevice->handle_int(gPassthroughDevice);
			
			if (cookie->dev->packet_size == 4) {
				gPassthroughDevice->history[0].data = cookie->packet_buffer[2];
				status = gPassthroughDevice->handle_int(gPassthroughDevice);
			}
			return status;		
		}
	
		if (packet_buffer_write(cookie->synaptics_ring_buffer,
				cookie->packet_buffer, cookie->dev->packet_size)
			!= cookie->dev->packet_size) {
			// buffer is full, drop new data
			return B_HANDLED_INTERRUPT;
		}
		release_sem_etc(cookie->synaptics_sem, 1, B_DO_NOT_RESCHEDULE);
		
		return B_INVOKE_SCHEDULER;
	}
	
	return B_HANDLED_INTERRUPT;
unhandled:
	return B_UNHANDLED_INTERRUPT;
}


void
synaptics_disconnect(ps2_dev *dev)
{
	synaptics_cookie *cookie = dev->cookie;
	// the mouse device might not be opened at this point
	INFO("SYNAPTICS: synaptics_disconnect %s\n", dev->name);
	if (dev->flags & PS2_FLAG_OPEN)
		release_sem(cookie->synaptics_sem);
}


device_hooks gSynapticsDeviceHooks = {
	synaptics_open,
	synaptics_close,
	synaptics_freecookie,
	synaptics_ioctl,
	synaptics_read,
	synaptics_write,
};
