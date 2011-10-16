/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
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

	uint32* Data() { return &opcode; }
};

class QueueCommands {
	public:
		QueueCommands(ring_buffer &ring);
		~QueueCommands();

		void Put(struct command &command, size_t size);
		void PutFlush();
		void PutWaitFor(uint32 event);
		void PutOverlayFlip(uint32 code, bool updateCoefficients);

		void MakeSpace(uint32 size);
		void Write(uint32 data);

	private:
		ring_buffer	&fRingBuffer;
};


// commands

struct xy_command : command {
	uint16	dest_bytes_per_row;
	uint8	raster_operation;
	uint8	mode;
	uint16	dest_left;
	uint16	dest_top;
	uint16	dest_right;
	uint16	dest_bottom;
	uint32	dest_base;

	xy_command(uint32 command, uint16 rop)
	{
		opcode = command;
		switch (gInfo->shared_info->bits_per_pixel) {
			case 8:
				mode = COMMAND_MODE_CMAP8;
				break;
			case 15:
				mode = COMMAND_MODE_RGB15;
				break;
			case 16:
				mode = COMMAND_MODE_RGB16;
				break;
			case 32:
				mode = COMMAND_MODE_RGB32;
				opcode |= COMMAND_BLIT_RGBA;
				break;
		}

		dest_bytes_per_row = gInfo->shared_info->bytes_per_row;
		dest_base = gInfo->shared_info->frame_buffer_offset;
		raster_operation = rop;
	}
};

struct xy_source_blit_command : xy_command {
	uint16	source_left;
	uint16	source_top;
	uint16	source_bytes_per_row;
	uint16	reserved;
	uint32	source_base;

	xy_source_blit_command()
		: xy_command(XY_COMMAND_SOURCE_BLIT, 0xcc)
	{
		source_bytes_per_row = dest_bytes_per_row;
		source_base = dest_base;
		reserved = 0;
	}
};

struct xy_color_blit_command : xy_command {
	uint32	color;

	xy_color_blit_command(bool invert)
		: xy_command(XY_COMMAND_COLOR_BLIT, invert ? 0x55 : 0xf0)
	{
	}
};

struct xy_setup_mono_pattern_command : xy_command {
	uint32	background_color;
	uint32	foreground_color;
	uint64	pattern;

	xy_setup_mono_pattern_command()
		: xy_command(XY_COMMAND_SETUP_MONO_PATTERN, 0xf0)
	{
		mode |= COMMAND_MODE_SOLID_PATTERN;

		// this defines the clipping window (but clipping is disabled)
		dest_left = 0;
		dest_top = 0;
		dest_right = 0;
		dest_bottom = 0;
	}
};

struct xy_scanline_blit_command : command {
	uint16	dest_left;
	uint16	dest_top;
	uint16	dest_right;
	uint16	dest_bottom;

	xy_scanline_blit_command()
	{
		opcode = XY_COMMAND_SCANLINE_BLIT;
	}
};

#endif	// COMMANDS_H
