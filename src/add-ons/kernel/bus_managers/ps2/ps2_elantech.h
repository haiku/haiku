/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * The elantech_model_info struct and all the hardware specs are taken from the
 * linux driver, thanks a lot!
 *
 * Authors:
 *		Jérôme Duval <korli@users.berlios.de>
 */
#ifndef _PS2_ELANTECH_H
#define _PS2_ELANTECH_H


#include <KernelExport.h>

#include <touchpad_settings.h>

#include "movement_maker.h"
#include "packet_buffer.h"
#include "ps2_dev.h"


typedef struct {
			ps2_dev*			dev;

			sem_id				sem;
	struct	packet_buffer*		ring_buffer;
			size_t				packet_index;

			TouchpadMovement	movementMaker;

			touchpad_settings	settings;
			uint32				version;
			uint32				fwVersion;

			uint32				x;
			uint32				y;
			uint32				fingers;


			uint8				buffer[PS2_PACKET_ELANTECH];
			uint8				capabilities[3];

			uint8				previousZ;

			uint8				mode;
			bool				crcEnabled;


	status_t (*send_command)(ps2_dev* dev, uint8 cmd, uint8 *in, int in_count);
} elantech_cookie;


status_t probe_elantech(ps2_dev *dev);

status_t elantech_open(const char *name, uint32 flags, void **_cookie);
status_t elantech_close(void *_cookie);
status_t elantech_freecookie(void *_cookie);
status_t elantech_ioctl(void *_cookie, uint32 op, void *buffer, size_t length);

int32 elantech_handle_int(ps2_dev *dev);
void elantech_disconnect(ps2_dev *dev);

extern device_hooks gElantechDeviceHooks;


#endif /* _PS2_ELANTECH_H */
