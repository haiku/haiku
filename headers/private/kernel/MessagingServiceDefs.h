/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// kernel - userland interface definitions

#ifndef MESSAGING_SERVICE_DEFS_H
#define MESSAGING_SERVICE_DEFS_H

#include <OS.h>

#include <messaging.h>

enum {
	MESSAGING_COMMAND_SEND_MESSAGE	= 0,
};

struct messaging_area_header {
	vint32	lock_counter;
	int32	size;				// set to 0, when area is discarded
	area_id	kernel_area;
	area_id	next_kernel_area;
	int32	command_count;
	int32	first_command;
	int32	last_command;
};

struct messaging_command {
	int32	next_command;
	uint32	command;
	int32	size;			// == sizeof(messaging_command) + dataSize
	char	data[0];
};

struct messaging_command_send_message {
	int32				message_size;
	int32				target_count;
	messaging_target	targets[0];	// [target_count]
//	char				message[message_size];
};

#endif	// MESSAGING_SERVICE_DEFS_H
