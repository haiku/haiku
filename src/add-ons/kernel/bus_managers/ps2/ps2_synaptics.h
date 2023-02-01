/*
 * Copyright 2008-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */
#ifndef PS2_SYNAPTICS_H
#define PS2_SYNAPTICS_H


#include <KernelExport.h>

#include "packet_buffer.h"
#include "ps2_service.h"


#define SYN_TOUCHPAD			0x47

// Synaptics modes (values of the "mode" field in synaptics_cookie)
#define SYN_MODE_ABSOLUTE		(1 << 7)
	// Absolute mode reports the absolute X/Y position of the finger,
	// instead of relative X/Y movement

#define SYN_MODE_W				(1 << 0)
	// Adds finger width (W) value in addition to absolute X/Y
#define SYN_MODE_EXTENDED_W		(1 << 2)
	// Supports tracking for multiple fingers
#define SYN_FOUR_BYTE_CHILD		(1 << 1)
	// Guest packets size for pass-through device
#define SYN_MODE_SLEEP			(1 << 3)
	// Low power sleep mode
#define SYN_MODE_PASSTHROUGH_ACPI			(1 << 4)
#define SYN_MODE_PASSTHROUGH_TRANSPARENT	(1 << 5)
#define SYN_MODE_HIGH_RATE					(1 << 6)
	// Use 80 packets per second instead of 40

// Synaptics Passthrough port
#define SYN_CHANGE_MODE			0x14
#define SYN_PASSTHROUGH_CMD		0x28


#define SYNAPTICS_HISTORY_SIZE	256

// no touchpad / left / right button pressed
#define IS_SYN_PT_PACKAGE(val) ((val[0] & 0xFC) == 0x84 \
	&& (val[3] & 0xCC) == 0xc4)


typedef struct {
	ps2_dev*			dev;

	sem_id				synaptics_sem;
struct packet_buffer*	synaptics_ring_buffer;
	size_t				packet_index;
	uint8				buffer[PS2_PACKET_SYNAPTICS];
	uint8				mode;
} synaptics_cookie;


status_t synaptics_pass_through_set_packet_size(ps2_dev *dev, uint8 size);
status_t passthrough_command(ps2_dev *dev, uint8 cmd, const uint8 *out,
	int out_count, uint8 *in, int in_count, bigtime_t timeout);
status_t probe_synaptics(ps2_dev *dev);

status_t synaptics_open(const char *name, uint32 flags, void **_cookie);
status_t synaptics_close(void *_cookie);
status_t synaptics_freecookie(void *_cookie);
status_t synaptics_ioctl(void *_cookie, uint32 op, void *buffer, size_t length);

int32 synaptics_handle_int(ps2_dev *dev);
void synaptics_disconnect(ps2_dev *dev);

extern device_hooks gSynapticsDeviceHooks;

#endif	// PS2_SYNAPTICS_H
