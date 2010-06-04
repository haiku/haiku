/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2008, Philippe Saint-Pierre <stpere@gmail.com>
 * Distributed under the terms of the MIT License.
 */


#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

void
uncompress_24bit_RLE(const uint8 compressed[], uint8 *uncompressed)
{
	uint32 cursorUncompressed = 0;
	uint32 cursorCompressed = 0;
	uint8 count = 0;
	uint8 item = 0;
	int i = 0;
	for (uint8 c = 0; c < 3; c++) {
		// for Red channel, then Green, then finally Blue...
		cursorUncompressed = c;
		while (compressed[cursorCompressed]) {
			// at the end of the channel there is a terminating 0,
			// so the loop will end... (ref: generate_boot_screen.cpp)
			count = compressed[cursorCompressed++];
			if (count < 128) {
				// regular run, repeat "item" "count" times...
				item = compressed[cursorCompressed++];
				for (i = count - 1; i >= 0; --i) {
					uncompressed[cursorUncompressed] = item;
					cursorUncompressed += 3;
				}
			} else {
				// enumeration, just write the next "count" items as is...
				count = count - 128;
				for (i = count - 1; i >= 0; --i) {
					uncompressed[cursorUncompressed]
						= compressed[cursorCompressed++];
					cursorUncompressed += 3;
				}
			}
		}
		// the current position of compressed[cursor] is the end of channel,
		// we skip it...
		cursorCompressed++;
	}
}

void
uncompress_8bit_RLE(const uint8 compressed[], uint8 *uncompressed)
{
	uint32 cursorUncompressed = 0;
	uint32 cursorCompressed = 0;
	uint8 count = 0;
	uint8 item = 0;
	int i = 0;

	while (compressed[cursorCompressed]) {
		// at the end of the channel there is a terminating 0,
		// so the loop will end... (ref: generate_boot_screen.cpp)
		count = compressed[cursorCompressed++];
		if (count < 128) {
			// regular run, repeat "item" "count" times...
			item = compressed[cursorCompressed++];
			for (i = count - 1; i >= 0; --i) {
				uncompressed[cursorUncompressed] = item;
				cursorUncompressed++;
			}
		} else {
			// enumeration, just write the next "count" items as is...
			count = count - 128;
			for (i = count - 1; i >= 0; --i) {
				uncompressed[cursorUncompressed]
					= compressed[cursorCompressed++];
				cursorUncompressed++;
			}
		}
	}
}


