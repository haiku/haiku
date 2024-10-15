/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2008, Philippe Saint-Pierre <stpere@gmail.com>
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GENERIC_BLITTER_H
#define GENERIC_BLITTER_H


#include <SupportDefs.h>


struct BlitParameters {
	const uint8* from;
	uint32 fromWidth;
	uint16 fromLeft, fromTop;
	uint16 fromRight, fromBottom;

	uint8* to;
	uint32 toBytesPerRow;
	uint16 toLeft, toTop;
};


static void
blit8(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft);
	uint8* start = (uint8*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 1 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = src[0];
			dst++;
			src++;
		}

		data += params.fromWidth;
		start = (uint8*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit15(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint16* start = (uint16*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 2 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = ((src[2] >> 3) << 10)
				| ((src[1] >> 3) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint16*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit16(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint16* start = (uint16*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 2 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = ((src[2] >> 3) << 11)
				| ((src[1] >> 2) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint16*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit24(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint8* start = (uint8*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 3 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint8*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit32(const BlitParameters& params)
{
	const uint8* data = params.from;
	data += (params.fromWidth * params.fromTop + params.fromLeft) * 3;
	uint32* start = (uint32*)(params.to
		+ params.toBytesPerRow * params.toTop
		+ 4 * params.toLeft);

	for (int32 y = params.fromTop; y < params.fromBottom; y++) {
		const uint8* src = data;
		uint32* dst = start;
		for (int32 x = params.fromLeft; x < params.fromRight; x++) {
			dst[0] = (src[2] << 16) | (src[1] << 8) | (src[0]);
			dst++;
			src += 3;
		}

		data += params.fromWidth * 3;
		start = (uint32*)((addr_t)start + params.toBytesPerRow);
	}
}


static void
blit(const BlitParameters& params, int32 depth)
{
	switch (depth) {
		case 8:
			blit8(params);
			return;
		case 15:
			blit15(params);
			return;
		case 16:
			blit16(params);
			return;
		case 24:
			blit24(params);
			return;
		case 32:
			blit32(params);
			return;
	}
}


#endif	/* GENERIC_BLITTER_H */
