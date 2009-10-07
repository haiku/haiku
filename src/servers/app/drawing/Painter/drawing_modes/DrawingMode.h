/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Base class for different drawing modes.
 *
 */

#ifndef DRAWING_MODE_H
#define DRAWING_MODE_H

#include "drawing_support.h"

#include "PatternHandler.h"
#include "PixelFormat.h"

class PatternHandler;

typedef PixelFormat::color_type		color_type;
typedef PixelFormat::agg_buffer		agg_buffer;

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


#define BLEND_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = (((((b) - _p.data8[0]) * (a1)) + (_p.data8[0] << 8)) >> 8); \
	d[1] = (((((g) - _p.data8[1]) * (a2)) + (_p.data8[1] << 8)) >> 8); \
	d[2] = (((((r) - _p.data8[2]) * (a3)) + (_p.data8[2] << 8)) >> 8); \
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

#define BLEND_FROM_SUBPIX(d, r1, g1, b1, r2, g2, b2, a1, a2, a3) \
{ \
	d[0] = (((((b2) - (b1)) * (a1)) + ((b1) << 8)) >> 8); \
	d[1] = (((((g2) - (g1)) * (a2)) + ((g1) << 8)) >> 8); \
	d[2] = (((((r2) - (r1)) * (a3)) + ((r1) << 8)) >> 8); \
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

// BLEND16_SUBPIX
#define BLEND16_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = (((((b) - _p.data8[0]) * (a1)) + (_p.data8[0] << 16)) >> 16); \
	d[1] = (((((g) - _p.data8[1]) * (a2)) + (_p.data8[1] << 16)) >> 16); \
	d[2] = (((((r) - _p.data8[2]) * (a3)) + (_p.data8[2] << 16)) >> 16); \
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
		d[0] = (((((b) - _p.data8[0]) * (a)) + (_p.data8[0] << 8)) >> 8); \
		d[1] = (((((g) - _p.data8[1]) * (a)) + (_p.data8[1] << 8)) >> 8); \
		d[2] = (((((r) - _p.data8[2]) * (a)) + (_p.data8[2] << 8)) >> 8); \
		d[3] = 255; \
	} else { \
		if (_p.data8[3] == 0) { \
			d[0] = (b); \
			d[1] = (g); \
			d[2] = (r); \
			d[3] = (a); \
		} else { \
			uint8 alphaRest = 255 - (a); \
			uint32 alphaTemp = (65025 - alphaRest * (255 - _p.data8[3])); \
			uint32 alphaDest = _p.data8[3] * alphaRest; \
			uint32 alphaSrc = 255 * (a); \
			d[0] = (_p.data8[0] * alphaDest + (b) * alphaSrc) / alphaTemp; \
			d[1] = (_p.data8[1] * alphaDest + (g) * alphaSrc) / alphaTemp; \
			d[2] = (_p.data8[2] * alphaDest + (r) * alphaSrc) / alphaTemp; \
			d[3] = alphaTemp / 255; \
		} \
	} \
}

// BLEND_COMPOSITE_SUBPIX
#define BLEND_COMPOSITE_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	if (_p.data8[3] == 255) { \
		d[0] = (((((b) - _p.data8[0]) * (a1)) + (_p.data8[0] << 8)) >> 8); \
		d[1] = (((((g) - _p.data8[1]) * (a2)) + (_p.data8[1] << 8)) >> 8); \
		d[2] = (((((r) - _p.data8[2]) * (a3)) + (_p.data8[2] << 8)) >> 8); \
		d[3] = 255; \
	} else { \
		if (_p.data8[3] == 0) { \
			d[0] = (b); \
			d[1] = (g); \
			d[2] = (r); \
			d[3] = (a1 + a2 + a3)/3; \
		} else { \
			uint8 alphaRest1 = 255 - (a1); \
			uint8 alphaRest2 = 255 - (a2); \
			uint8 alphaRest3 = 255 - (a3); \
			uint32 alphaTemp1 = (65025 - alphaRest1 * (255 - _p.data8[3])); \
			uint32 alphaTemp2 = (65025 - alphaRest2 * (255 - _p.data8[3])); \
			uint32 alphaTemp3 = (65025 - alphaRest3 * (255 - _p.data8[3])); \
			uint32 alphaDest1 = _p.data8[3] * alphaRest1; \
			uint32 alphaDest2 = _p.data8[3] * alphaRest2; \
			uint32 alphaDest3 = _p.data8[3] * alphaRest3; \
			uint32 alphaSrc1 = 255 * (a1); \
			uint32 alphaSrc2 = 255 * (a2); \
			uint32 alphaSrc3 = 255 * (a3); \
			d[0] = (_p.data8[0] * alphaDest1 + (b) * alphaSrc1) / alphaTemp1; \
			d[1] = (_p.data8[1] * alphaDest2 + (g) * alphaSrc2) / alphaTemp2; \
			d[2] = (_p.data8[2] * alphaDest3 + (r) * alphaSrc3) / alphaTemp3; \
			d[3] = (alphaTemp1 + alphaTemp2 + alphaTemp3)/765; \
		} \
	} \
}

// BLEND_COMPOSITE16
//
// This macro assumes source alpha in range 0..65025 and
// composes the source color over a possibly semi-transparent background.
// TODO: implement a faster version
#define BLEND_COMPOSITE16(d, r, g, b, a) \
{ \
	uint16 _a = (a) / 255; \
	BLEND_COMPOSITE(d, r, g, b, _a); \
}

// BLEND_COMPOSITE16_SUBPIX
#define BLEND_COMPOSITE16_SUBPIX(d, r, g, b, a1, a2, a3) \
{ \
	uint16 _a1 = (a1) / 255; \
	uint16 _a2 = (a2) / 255; \
	uint16 _a3 = (a3) / 255; \
	BLEND_COMPOSITE_SUBPIX(d, r, g, b, _a1, _a2, _a3); \
}

static inline
uint8
brightness_for(uint8 red, uint8 green, uint8 blue)
{
	// brightness = 0.301 * red + 0.586 * green + 0.113 * blue
	// we use for performance reasons:
	// brightness = (308 * red + 600 * green + 116 * blue) / 1024
	return uint8((308 * red + 600 * green + 116 * blue) / 1024);
}

#endif // DRAWING_MODE_H

