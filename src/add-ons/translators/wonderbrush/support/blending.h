/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef BLENDING_H
#define BLENDING_H

#include <SupportDefs.h>

// faster bit error version:
//#define INT_MULT(a, b, t)			((t) = (a) * (b), ((((t) >> 8) + (t)) >> 8))
// correct version
#define INT_MULT(a, b, t)			((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

#define INT_LERP(p, q, a, t)		((p) + INT_MULT(a, ((q) - (p)), t))

#define INT_PRELERP(p, q, a, t)		((p) + (q) - INT_MULT(a, p, t))



#define GAMMA_BLEND 1

#if GAMMA_BLEND

extern uint16* kGammaTable;
extern uint8* kInverseGammaTable;

void init_gamma_blending();
void uninit_gamma_blending();

// blend
inline void
blend_gamma(uint16 b1, uint16 b2, uint16 b3, uint8 ba,	// bottom components
			uint16 t1, uint16 t2, uint16 t3, uint8 ta,	// top components
			uint8* d1, uint8* d2, uint8* d3, uint8* da)	// dest components
{
	if (ba == 255) {
		uint32 destAlpha = 255 - ta;
		*d1 = kInverseGammaTable[(b1 * destAlpha + t1 * ta) / 255];
		*d2 = kInverseGammaTable[(b2 * destAlpha + t2 * ta) / 255];
		*d3 = kInverseGammaTable[(b3 * destAlpha + t3 * ta) / 255];
		*da = 255;
	} else {
		uint8 alphaRest = 255 - ta;
		register uint32 alphaTemp = (65025 - alphaRest * (255 - ba));
		uint32 alphaDest = ba * alphaRest;
		uint32 alphaSrc = 255 * ta;
		*d1 = kInverseGammaTable[(b1 * alphaDest + t1 * alphaSrc) / alphaTemp];
		*d2 = kInverseGammaTable[(b2 * alphaDest + t2 * alphaSrc) / alphaTemp];
		*d3 = kInverseGammaTable[(b3 * alphaDest + t3 * alphaSrc) / alphaTemp];
		*da = alphaTemp / 255;
	}
}

// blend
inline void
blend(uint8 b1, uint8 b2, uint8 b3, uint8 ba,		// bottom components
	  uint8 t1, uint8 t2, uint8 t3, uint8 ta,		// top components
	  uint8* d1, uint8* d2, uint8* d3, uint8* da)	// dest components
{
	// convert to linear rgb
	uint16 gt1 = kGammaTable[t1];
	uint16 gt2 = kGammaTable[t2];
	uint16 gt3 = kGammaTable[t3];

	uint16 gb1 = kGammaTable[b1];
	uint16 gb2 = kGammaTable[b2];
	uint16 gb3 = kGammaTable[b3];

	blend_gamma(gb1, gb2, gb3, ba,
				gt1, gt2, gt3, ta,
				d1, d2, d3, da);
}

// convert_to_gamma
//
// converted value will be gamma corrected in the range [0...2550]
// and can be passed on to the other functions that take uint16 components
uint16
convert_to_gamma(uint8 value);

// blend_colors_copy
inline void
blend_colors_copy(uint8* bottom, uint8 alpha, uint8* dest,
				  uint8 c1, uint8 c2, uint8 c3,
				  uint16 gc1, uint16 gc2, uint16 gc3)
{
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			dest[0] = c1;
			dest[1] = c2;
			dest[2] = c3;
			dest[3] = alpha;
		} else {
			// only bottom components need to be gamma corrected
			uint16 gb1 = kGammaTable[bottom[0]];
			uint16 gb2 = kGammaTable[bottom[1]];
			uint16 gb3 = kGammaTable[bottom[2]];

			blend_gamma(gb1, gb2, gb3, bottom[3],
						gc1, gc2, gc3, alpha,
						&dest[0], &dest[1], &dest[2], &dest[3]);
		}
	} else {
		*((uint32*)dest) = *((uint32*)bottom);
	}
}

// blend_colors
inline void
blend_colors(uint8* bottom, uint8 alpha,
			 uint8 c1, uint8 c2, uint8 c3,
			 uint16 gc1, uint16 gc2, uint16 gc3)
{
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			bottom[0] = c1;
			bottom[1] = c2;
			bottom[2] = c3;
			bottom[3] = alpha;
		} else {
			// only bottom components need to be gamma corrected
			uint16 gb1 = kGammaTable[bottom[0]];
			uint16 gb2 = kGammaTable[bottom[1]];
			uint16 gb3 = kGammaTable[bottom[2]];

			blend_gamma(gb1, gb2, gb3, bottom[3],
						gc1, gc2, gc3, alpha,
						&bottom[0], &bottom[1], &bottom[2], &bottom[3]);
		}
	}
}

// blend_colors_copy
inline void
blend_colors_copy(uint8* bottom, uint8 alpha, uint8* dest,
				  uint8 c1, uint8 c2, uint8 c3)
{
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			dest[0] = c1;
			dest[1] = c2;
			dest[2] = c3;
			dest[3] = alpha;
		} else {
			blend(bottom[0], bottom[1], bottom[2], bottom[3],
				  c1, c2, c3, alpha,
				  &dest[0], &dest[1], &dest[2], &dest[3]);
		}
	} else {
		*((uint32*)dest) = *((uint32*)bottom);
	}
}

// blend_colors
inline void
blend_colors(uint8* bottom, uint8 alpha, uint8 c1, uint8 c2, uint8 c3)
{
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			bottom[0] = c1;
			bottom[1] = c2;
			bottom[2] = c3;
			bottom[3] = alpha;
		} else {
			blend(bottom[0], bottom[1], bottom[2], bottom[3],
				  c1, c2, c3, alpha,
				  &bottom[0], &bottom[1], &bottom[2], &bottom[3]);
		}
	}
}

// blend_colors
inline void
blend_colors(uint8* bottom, uint8* source, uint8 alphaOverride)
{
	uint8 alpha = (source[3] * alphaOverride) / 255;
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			bottom[0] = source[0];
			bottom[1] = source[1];
			bottom[2] = source[2];
			bottom[3] = alpha;
		} else {
			blend(bottom[0], bottom[1], bottom[2], bottom[3],
				  source[0], source[1], source[2], alpha,
				  &bottom[0], &bottom[1], &bottom[2], &bottom[3]);
		}
	}
}

// blend_colors
inline void
blend_colors(uint8* bottom, uint8* source)
{
	if (source[3] > 0) {
		if (bottom[3] == 0 || source[3] == 255) {
			bottom[0] = source[0];
			bottom[1] = source[1];
			bottom[2] = source[2];
			bottom[3] = source[3];
		} else {
			blend(bottom[0], bottom[1], bottom[2], bottom[3],
				  source[0], source[1], source[2], source[3],
				  &bottom[0], &bottom[1], &bottom[2], &bottom[3]);
		}
	}
}

// blend_colors_copy
inline void
blend_colors_copy(uint8* dest, uint8* bottom, uint8* top)
{
	if (bottom[3] == 0 || top[3] == 255) {
		dest[0] = top[0];
		dest[1] = top[1];
		dest[2] = top[2];
		dest[3] = top[3];
	} else {
			blend(bottom[0], bottom[1], bottom[2], bottom[3],
				  top[0], top[1], top[2], top[3],
				  &dest[0], &dest[1], &dest[2], &dest[3]);
	}
}

// blend_pixels
inline void
blend_pixels(uint8* bottom, uint8* top, uint8 alpha)
{
	if (alpha > 0) {
		if (alpha == 255) {
			bottom[0] = top[0];
			bottom[1] = top[1];
			bottom[2] = top[2];
			bottom[3] = top[3];
		} else {
			// convert to linear rgb
			uint16 t1 = kGammaTable[top[0]];
			uint16 t2 = kGammaTable[top[1]];
			uint16 t3 = kGammaTable[top[2]];
			uint16 b1 = kGammaTable[bottom[0]];
			uint16 b2 = kGammaTable[bottom[1]];
			uint16 b3 = kGammaTable[bottom[2]];

			uint8 mergeAlpha = bottom[3] ? (top[3] * alpha) / 255 : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			bottom[0] = kInverseGammaTable[(b1 * invAlpha + t1 * mergeAlpha) / 255];
			bottom[1] = kInverseGammaTable[(b2 * invAlpha + t2 * mergeAlpha) / 255];
			bottom[2] = kInverseGammaTable[(b3 * invAlpha + t3 * mergeAlpha) / 255];
			bottom[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	}
}

// blend_pixels_copy
inline void
blend_pixels_copy(uint8* bottom, uint8* top, uint8* dest, uint8 alpha)
{
	if (alpha > 0) {	
		if (alpha == 255) {
			dest[0] = top[0];
			dest[1] = top[1];
			dest[2] = top[2];
			dest[3] = top[3];
		} else {
			// convert to linear rgb
			uint16 t1 = kGammaTable[top[0]];
			uint16 t2 = kGammaTable[top[1]];
			uint16 t3 = kGammaTable[top[2]];
			uint16 b1 = kGammaTable[bottom[0]];
			uint16 b2 = kGammaTable[bottom[1]];
			uint16 b3 = kGammaTable[bottom[2]];

			uint8 mergeAlpha = bottom[3] ? (top[3] * alpha) / 255 : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			dest[0] = kInverseGammaTable[(b1 * invAlpha + t1 * mergeAlpha) / 255];
			dest[1] = kInverseGammaTable[(b2 * invAlpha + t2 * mergeAlpha) / 255];
			dest[2] = kInverseGammaTable[(b3 * invAlpha + t3 * mergeAlpha) / 255];
			dest[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	} else {
		dest[0] = bottom[0];
		dest[1] = bottom[1];
		dest[2] = bottom[2];
		dest[3] = bottom[3];
	}
}

// blend_pixels_overlay
inline void
blend_pixels_overlay(uint8* bottom, uint8* top, uint8 alphaOverride)
{
	uint8 alpha = (top[3] * alphaOverride) / 255;
	if (alpha > 0) {
		if (alpha == 255) {
			bottom[0] = top[0];
			bottom[1] = top[1];
			bottom[2] = top[2];
			bottom[3] = top[3];
		} else {
			// convert to linear rgb
			uint16 t1 = kGammaTable[top[0]];
			uint16 t2 = kGammaTable[top[1]];
			uint16 t3 = kGammaTable[top[2]];
			uint16 b1 = kGammaTable[bottom[0]];
			uint16 b2 = kGammaTable[bottom[1]];
			uint16 b3 = kGammaTable[bottom[2]];

			uint8 mergeAlpha = bottom[3] ? alpha : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			bottom[0] = kInverseGammaTable[(b1 * invAlpha + t1 * mergeAlpha) / 255];
			bottom[1] = kInverseGammaTable[(b2 * invAlpha + t2 * mergeAlpha) / 255];
			bottom[2] = kInverseGammaTable[(b3 * invAlpha + t3 * mergeAlpha) / 255];
			bottom[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	}
}

// blend_pixels_overlay_copy
inline void
blend_pixels_overlay_copy(uint8* bottom, uint8* top, uint8* dest, uint8 alphaOverride)
{
	uint8 alpha = (top[3] * alphaOverride) / 255;
	if (alpha > 0) {
		if (alpha == 255) {
			dest[0] = top[0];
			dest[1] = top[1];
			dest[2] = top[2];
			dest[3] = top[3];
		} else {
			// convert to linear rgb
			uint16 t1 = kGammaTable[top[0]];
			uint16 t2 = kGammaTable[top[1]];
			uint16 t3 = kGammaTable[top[2]];
			uint16 b1 = kGammaTable[bottom[0]];
			uint16 b2 = kGammaTable[bottom[1]];
			uint16 b3 = kGammaTable[bottom[2]];

			uint8 mergeAlpha = bottom[3] ? alpha : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			dest[0] = kInverseGammaTable[(b1 * invAlpha + t1 * mergeAlpha) / 255];
			dest[1] = kInverseGammaTable[(b2 * invAlpha + t2 * mergeAlpha) / 255];
			dest[2] = kInverseGammaTable[(b3 * invAlpha + t3 * mergeAlpha) / 255];
			dest[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	} else {
		dest[0] = bottom[0];
		dest[1] = bottom[1];
		dest[2] = bottom[2];
		dest[3] = bottom[3];
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
#else // GAMMA_BLEND

// blend_colors_copy
inline void
blend_colors_copy(uint8* bottom, uint8 alpha, uint8* dest,
				  uint8 c1, uint8 c2, uint8 c3)
{
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			dest[0] = c1;
			dest[1] = c2;
			dest[2] = c3;
			dest[3] = alpha;
		} else {
			if (bottom[3] == 255) {
				uint32 destAlpha = 255 - alpha;
				dest[0] = (uint8)((bottom[0] * destAlpha + c1 * alpha) / 255);
				dest[1] = (uint8)((bottom[1] * destAlpha + c2 * alpha) / 255);
				dest[2] = (uint8)((bottom[2] * destAlpha + c3 * alpha) / 255);
				dest[3] = 255;
			} else {
				uint8 alphaRest = 255 - alpha;
				uint32 alphaTemp = (65025 - alphaRest * (255 - bottom[3]));
				uint32 alphaDest = bottom[3] * alphaRest;
				uint32 alphaSrc = 255 * alpha;
				dest[0] = (bottom[0] * alphaDest + c1 * alphaSrc) / alphaTemp;
				dest[1] = (bottom[1] * alphaDest + c2 * alphaSrc) / alphaTemp;
				dest[2] = (bottom[2] * alphaDest + c3 * alphaSrc) / alphaTemp;
				dest[3] = alphaTemp / 255;
			}
		}
	} else {
		*((uint32*)dest) = *((uint32*)bottom);
	}
}

// blend_colors
inline void
blend_colors(uint8* bottom, uint8 alpha, uint8 c1, uint8 c2, uint8 c3)
{
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			bottom[0] = c1;
			bottom[1] = c2;
			bottom[2] = c3;
			bottom[3] = alpha;
		} else {
			if (bottom[3] == 255) {
				uint32 destAlpha = 255 - alpha;
				bottom[0] = (uint8)((bottom[0] * destAlpha + c1 * alpha) / 255);
				bottom[1] = (uint8)((bottom[1] * destAlpha + c2 * alpha) / 255);
				bottom[2] = (uint8)((bottom[2] * destAlpha + c3 * alpha) / 255);
			} else {
				uint8 alphaRest = 255 - alpha;
				uint32 alphaTemp = (65025 - alphaRest * (255 - bottom[3]));
				uint32 alphaDest = bottom[3] * alphaRest;
				uint32 alphaSrc = 255 * alpha;
				bottom[0] = (bottom[0] * alphaDest + c1 * alphaSrc) / alphaTemp;
				bottom[1] = (bottom[1] * alphaDest + c2 * alphaSrc) / alphaTemp;
				bottom[2] = (bottom[2] * alphaDest + c3 * alphaSrc) / alphaTemp;
				bottom[3] = alphaTemp / 255;
			}
		}
	}
}

// blend_colors
inline void
blend_colors(uint8* bottom, uint8* source, uint8 alphaOverride)
{
	uint8 alpha = (source[3] * alphaOverride) / 255;
	if (alpha > 0) {
		if (bottom[3] == 0 || alpha == 255) {
			bottom[0] = source[0];
			bottom[1] = source[1];
			bottom[2] = source[2];
			bottom[3] = alpha;
		} else {
			if (bottom[3] == 255) {
				uint32 destAlpha = 255 - alpha;
				bottom[0] = (uint8)((bottom[0] * destAlpha + source[0] * alpha) / 255);
				bottom[1] = (uint8)((bottom[1] * destAlpha + source[1] * alpha) / 255);
				bottom[2] = (uint8)((bottom[2] * destAlpha + source[2] * alpha) / 255);
			} else {
				uint8 alphaRest = 255 - alpha;
				uint32 alphaTemp = (65025 - alphaRest * (255 - bottom[3]));
				uint32 alphaDest = bottom[3] * alphaRest;
				uint32 alphaSrc = 255 * alpha;
				bottom[0] = (bottom[0] * alphaDest + source[0] * alphaSrc) / alphaTemp;
				bottom[1] = (bottom[1] * alphaDest + source[1] * alphaSrc) / alphaTemp;
				bottom[2] = (bottom[2] * alphaDest + source[2] * alphaSrc) / alphaTemp;
				bottom[3] = alphaTemp / 255;
			}
		}
	}
}

// blend_colors
inline void
blend_colors(uint8* bottom, uint8* source)
{
	if (source[3] > 0) {
		if (bottom[3] == 0 || source[3] == 255) {
			bottom[0] = source[0];
			bottom[1] = source[1];
			bottom[2] = source[2];
			bottom[3] = source[3];
		} else {
			if (bottom[3] == 255) {
				uint32 destAlpha = 255 - source[3];
				bottom[0] = (uint8)((bottom[0] * destAlpha + source[0] * source[3]) / 255);
				bottom[1] = (uint8)((bottom[1] * destAlpha + source[1] * source[3]) / 255);
				bottom[2] = (uint8)((bottom[2] * destAlpha + source[2] * source[3]) / 255);
			} else {
				uint8 alphaRest = 255 - source[3];
				uint32 alphaTemp = (65025 - alphaRest * (255 - bottom[3]));
				uint32 alphaDest = bottom[3] * alphaRest;
				uint32 alphaSrc = 255 * source[3];
				bottom[0] = (bottom[0] * alphaDest + source[0] * alphaSrc) / alphaTemp;
				bottom[1] = (bottom[1] * alphaDest + source[1] * alphaSrc) / alphaTemp;
				bottom[2] = (bottom[2] * alphaDest + source[2] * alphaSrc) / alphaTemp;
				bottom[3] = alphaTemp / 255;
			}
		}
	}
}

// blend_colors_copy
inline void
blend_colors_copy(uint8* dest, uint8* bottom, uint8* top)
{
	if (bottom[3] == 0 || top[3] == 255) {
		dest[0] = top[0];
		dest[1] = top[1];
		dest[2] = top[2];
		dest[3] = top[3];
	} else {
		if (bottom[3] == 255) {
			uint32 destAlpha = 255 - top[3];
			dest[0] = (uint8)((bottom[0] * destAlpha + top[0] * top[3]) / 255);
			dest[1] = (uint8)((bottom[1] * destAlpha + top[1] * top[3]) / 255);
			dest[2] = (uint8)((bottom[2] * destAlpha + top[2] * top[3]) / 255);
			dest[3] = 255;
		} else {
			uint8 alphaRest = 255 - top[3];
			uint32 alphaTemp = (65025 - alphaRest * (255 - bottom[3]));
			uint32 alphaDest = bottom[3] * alphaRest;
			uint32 alphaSrc = 255 * top[3];
			dest[0] = (bottom[0] * alphaDest + top[0] * alphaSrc) / alphaTemp;
			dest[1] = (bottom[1] * alphaDest + top[1] * alphaSrc) / alphaTemp;
			dest[2] = (bottom[2] * alphaDest + top[2] * alphaSrc) / alphaTemp;
			dest[3] = alphaTemp / 255;
		}
	}
}

// blend_pixels
inline void
blend_pixels(uint8* bottom, uint8* top, uint8 alpha)
{
	if (alpha > 0) {
		if (alpha == 255) {
			bottom[0] = top[0];
			bottom[1] = top[1];
			bottom[2] = top[2];
			bottom[3] = top[3];
		} else {
			uint8 mergeAlpha = bottom[3] ? (top[3] * alpha) / 255 : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			bottom[0] = (bottom[0] * invAlpha + top[0] * mergeAlpha) / 255;
			bottom[1] = (bottom[1] * invAlpha + top[1] * mergeAlpha) / 255;
			bottom[2] = (bottom[2] * invAlpha + top[2] * mergeAlpha) / 255;
			bottom[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	}
}

// blend_pixels_copy
inline void
blend_pixels_copy(uint8* bottom, uint8* top, uint8* dest, uint8 alpha)
{
	if (alpha > 0) {
		if (alpha == 255) {
			dest[0] = top[0];
			dest[1] = top[1];
			dest[2] = top[2];
			dest[3] = top[3];
		} else {
			uint8 mergeAlpha = bottom[3] ? (top[3] * alpha) / 255 : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			dest[0] = (bottom[0] * invAlpha + top[0] * mergeAlpha) / 255;
			dest[1] = (bottom[1] * invAlpha + top[1] * mergeAlpha) / 255;
			dest[2] = (bottom[2] * invAlpha + top[2] * mergeAlpha) / 255;
			dest[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	} else {
		dest[0] = bottom[0];
		dest[1] = bottom[1];
		dest[2] = bottom[2];
		dest[3] = bottom[3];
	}
}

// blend_pixels_overlay
inline void
blend_pixels_overlay(uint8* bottom, uint8* top, uint8 alphaOverride)
{
	uint8 alpha = (top[3] * alphaOverride) / 255;
	if (alpha > 0) {
		if (alpha == 255) {
			bottom[0] = top[0];
			bottom[1] = top[1];
			bottom[2] = top[2];
			bottom[3] = top[3];
		} else {
			uint8 mergeAlpha = bottom[3] ? alpha : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			bottom[0] = (bottom[0] * invAlpha + top[0] * mergeAlpha) / 255;
			bottom[1] = (bottom[1] * invAlpha + top[1] * mergeAlpha) / 255;
			bottom[2] = (bottom[2] * invAlpha + top[2] * mergeAlpha) / 255;
			bottom[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	}
}

// blend_pixels_overlay_copy
inline void
blend_pixels_overlay_copy(uint8* bottom, uint8* top, uint8* dest, uint8 alphaOverride)
{
	uint8 alpha = (top[3] * alphaOverride) / 255;
	if (alpha > 0) {
		if (alpha == 255) {
			dest[0] = top[0];
			dest[1] = top[1];
			dest[2] = top[2];
			dest[3] = top[3];
		} else {
			uint8 mergeAlpha = bottom[3] ? alpha : 255;
			uint8 invAlpha = 255 - mergeAlpha;
			dest[0] = (bottom[0] * invAlpha + top[0] * mergeAlpha) / 255;
			dest[1] = (bottom[1] * invAlpha + top[1] * mergeAlpha) / 255;
			dest[2] = (bottom[2] * invAlpha + top[2] * mergeAlpha) / 255;
			dest[3] = (bottom[3] * (255 - alpha) + top[3] * alpha) / 255;
		}
	} else {
		dest[0] = bottom[0];
		dest[1] = bottom[1];
		dest[2] = bottom[2];
		dest[3] = bottom[3];
	}
}

#endif	// GAMMA_BLEND

#endif // BLENDING_H
