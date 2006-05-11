/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef COMMANDS_H
#define COMMANDS_H


#include "accelerant.h"


struct command {
	uint32	opcode;

	uint32 *Data() { return &opcode; }
};

class QueueCommands {
	public:
		QueueCommands(ring_buffer &ring);
		~QueueCommands();

		void Put(struct command &command, size_t size);
		void PutFlush();
		void PutWaitFor(uint32 event);
		void PutOverlayFlip(uint32 code, bool updateCoefficients);

	private:
		void _MakeSpace(uint32 size);
		void _Write(uint32 data);

		ring_buffer	&fRingBuffer;
};


// commands

struct blit_command : command {
	uint16	dest_bytes_per_row;
	uint8	raster_operation;
	uint8	flags;
	uint16	dest_left;
	uint16	dest_top;
	uint16	dest_right;
	uint16	dest_bottom;
	uint32	dest_base;
	uint16	source_left;
	uint16	source_top;
	uint16	source_bytes_per_row;
	uint16	reserved;
	uint32	source_base;

	blit_command()
	{
		opcode = COMMAND_BLIT;
		switch (gInfo->shared_info->bits_per_pixel) {
			case 8:
				flags = 0;
				break;
			case 15:
				flags = 2;
				break;
			case 16:
				flags = 1;
				break;
			case 32:
				flags = 3;
				opcode |= COMMAND_BLIT_RGBA;
				break;
		}
		dest_base = source_base = gInfo->shared_info->frame_buffer_offset;
		dest_bytes_per_row = source_bytes_per_row = gInfo->shared_info->bytes_per_row;
		reserved = 0;
		raster_operation = 0xcc;
	}
};

#endif	// COMMANDS_H
