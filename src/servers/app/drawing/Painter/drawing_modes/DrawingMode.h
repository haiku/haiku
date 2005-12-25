/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Base class for different drawing modes.
 *
 */

#ifndef DRAWING_MODE_H
#define DRAWING_MODE_H

#include "PatternHandler.h"
#include "PixelFormat.h"

class PatternHandler;

typedef PixelFormat::color_type		color_type;
typedef PixelFormat::agg_buffer		agg_buffer;

union pixel32 {
	uint32	data32;
	uint8	data8[4];
};

// BLEND
//
// This macro assumes source alpha in range 0..255 and
// ignores dest alpha (is assumed to equal 255).
// TODO: We need the assignment of alpha only when drawing into bitmaps!
#define BLEND(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = (((((b) - _p.data8[0]) * (a)) + (_p.data8[0] << 8)) >> 8); \
	d[1] = (((((g) - _p.data8[1]) * (a)) + (_p.data8[1] << 8)) >> 8); \
	d[2] = (((((r) - _p.data8[2]) * (a)) + (_p.data8[2] << 8)) >> 8); \
	d[3] = 255; \
}

// BLEND_FROM
//
// This macro assumes source alpha in range 0..255 and
// ignores dest alpha (is assumed to equal 255).
// It uses two colors for the blending (f and s) and writes
// the result into a third color (d).
// TODO: We need the assignment of alpha only when drawing into bitmaps!
#define BLEND_FROM(d, r1, g1, b1, r2, g2, b2, a) \
{ \
	d[0] = (((((b2) - (b1)) * (a)) + ((b1) << 8)) >> 8); \
	d[1] = (((((g2) - (g1)) * (a)) + ((g1) << 8)) >> 8); \
	d[2] = (((((r2) - (r1)) * (a)) + ((r1) << 8)) >> 8); \
	d[3] = 255; \
}

// BLEND16
//
// This macro assumes source alpha in range 0..65025 and
// ignores dest alpha (is assumed to equal 255).
// TODO: We need the assignment of alpha only when drawing into bitmaps!
// BLEND16
#define BLEND16(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = (((((b) - _p.data8[0]) * (a)) + (_p.data8[0] << 16)) >> 16); \
	d[1] = (((((g) - _p.data8[1]) * (a)) + (_p.data8[1] << 16)) >> 16); \
	d[2] = (((((r) - _p.data8[2]) * (a)) + (_p.data8[2] << 16)) >> 16); \
	d[3] = 255; \
}



// BLEND_COMPOSITE
//
// This macro assumes source alpha in range 0..255 and
// composes the source color over a possibly semi-transparent background.
#define BLEND_COMPOSITE(d, r, g, b, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	if (_p.data8[3] == 255) { \
		BLEND(d, r, g, b, a); \
	} else { \
		uint8 alphaRest = 255 - (a); \
		uint32 alphaTemp = (65025 - alphaRest * (255 - _p.data8[3])); \
		uint32 alphaDest = _p.data8[3] * alphaRest; \
		uint32 alphaSrc = 255 * (a); \
		d[0] = (_p.data8[0] * alphaDest + (b) * alphaSrc) / alphaTemp; \
		d[1] = (_p.data8[1] * alphaDest + (g) * alphaSrc) / alphaTemp; \
		d[2] = (_p.data8[2] * alphaDest + (r) * alphaSrc) / alphaTemp; \
		d[3] = alphaTemp >> 8; \
	} \
}

// BLEND_COMPOSITE16
//
// This macro assumes source alpha in range 0..65025 and
// composes the source color over a possibly semi-transparent background.
// TODO: implement a faster version
#define BLEND_COMPOSITE16(d, r, g, b, a) \
{ \
	BLEND_COMPOSITE(d, r, g, b, (a) >> 8); \
}


#endif // DRAWING_MODE_H

