/*
 * Copyright 2006-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "vga.h"
#include "driver.h"

#include <vga.h>

#include <KernelExport.h>


status_t
vga_set_indexed_colors(uint8 first, uint8 *colors, uint16 count)
{
	// If we don't actually have an ISA bus, bail.
	if (gISA == NULL)
		return B_BAD_ADDRESS;

	if (first + count > 256)
		count = 256 - first;

	gISA->write_io_8(VGA_COLOR_WRITE_MODE, first);

	// write VGA palette
	for (int32 i = first; i < count; i++) {
		uint8 color[3];
		if (user_memcpy(color, &colors[i * 3], 3) < B_OK)
			return B_BAD_ADDRESS;

		// VGA (usually) has only 6 bits per gun
		gISA->write_io_8(VGA_COLOR_DATA, color[0] >> 2);
		gISA->write_io_8(VGA_COLOR_DATA, color[1] >> 2);
		gISA->write_io_8(VGA_COLOR_DATA, color[2] >> 2);
	}
	return B_OK;
}


status_t
vga_planar_blit(vesa_shared_info *info, uint8 *src, int32 srcBPR,
	int32 left, int32 top, int32 right, int32 bottom)
{
	// If we don't actually have an ISA bus, bail.
	if (gISA == NULL)
		return B_BAD_ADDRESS;

	// If we don't actually have a frame_buffer, bail.
	if (info->frame_buffer == NULL)
		return B_BAD_ADDRESS;

	int32 dstBPR = info->bytes_per_row;
	uint8 *dst = info->frame_buffer + top * dstBPR + left / 8;

	// TODO: this is awfully slow...
	// TODO: assumes BGR order
	for (int32 y = top; y <= bottom; y++) {
		for (int32 plane = 0; plane < 4; plane++) {
			// select the plane we intend to write to and read from
			gISA->write_io_16(VGA_SEQUENCER_INDEX, (1 << (plane + 8)) | 0x02);
			gISA->write_io_16(VGA_GRAPHICS_INDEX, (plane << 8) | 0x04);

			uint8* srcHandle = src;
			uint8* dstHandle = dst;
			uint8 current8 = dstHandle[0];
				// we store 8 pixels before writing them back

			int32 x = left;
			for (; x <= right; x++) {
				uint8 rgba[4];
				if (user_memcpy(rgba, srcHandle, 4) < B_OK)
					return B_BAD_ADDRESS;
				uint8 pixel = (308 * rgba[2] + 600 * rgba[1]
					+ 116 * rgba[0]) / 16384;
				srcHandle += 4;

				if (pixel & (1 << plane))
					current8 |= 0x80 >> (x & 7);
				else
					current8 &= ~(0x80 >> (x & 7));

				if ((x & 7) == 7) {
					// last pixel in 8 pixel group
					dstHandle[0] = current8;
					dstHandle++;
					current8 = dstHandle[0];
				}
			}

			if (x & 7) {
				// last pixel has not been written yet
				dstHandle[0] = current8;
			}
		}
		dst += dstBPR;
		src += srcBPR;
	}
	return B_OK;
}

