//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		GraphicsDefs.h
//	Author:			Frans van Nispen
//	Description:	BMessageFilter class creates objects that filter
//					in-coming BMessages.  
//------------------------------------------------------------------------------

#ifndef _GRAPHICS_DEFS_H
#define _GRAPHICS_DEFS_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


//------------------------------------------------------------------------------

typedef struct pattern {
		uint8		data[8];
} pattern;

extern _IMPEXP_BE const pattern B_SOLID_HIGH;
extern _IMPEXP_BE const pattern B_MIXED_COLORS;
extern _IMPEXP_BE const pattern B_SOLID_LOW;

//------------------------------------------------------------------------------

typedef struct rgb_color {
	uint8		red;
	uint8		green;
	uint8		blue;
	uint8		alpha;
} rgb_color;

//------------------------------------------------------------------------------

extern _IMPEXP_BE const rgb_color 	B_TRANSPARENT_COLOR;
extern _IMPEXP_BE const uint8		B_TRANSPARENT_MAGIC_CMAP8;
extern _IMPEXP_BE const uint16		B_TRANSPARENT_MAGIC_RGBA15;
extern _IMPEXP_BE const uint16		B_TRANSPARENT_MAGIC_RGBA15_BIG;
extern _IMPEXP_BE const uint32		B_TRANSPARENT_MAGIC_RGBA32;
extern _IMPEXP_BE const uint32		B_TRANSPARENT_MAGIC_RGBA32_BIG;

extern _IMPEXP_BE const uint8 		B_TRANSPARENT_8_BIT;
extern _IMPEXP_BE const rgb_color	B_TRANSPARENT_32_BIT;

//------------------------------------------------------------------------------

typedef struct color_map {
	int32				id;
	rgb_color			color_list[256];
	uint8				inversion_map[256];
	uint8				index_map[32768];
} color_map;

typedef struct overlay_rect_limits {
	uint16				horizontal_alignment;
	uint16				vertical_alignment;
	uint16				width_alignment;
	uint16				height_alignment;
	uint16				min_width;
	uint16				max_width;
	uint16				min_height;
	uint16				max_height;
	uint32				reserved[8];
} overlay_rect_limits;

typedef struct overlay_restrictions {
	overlay_rect_limits	source;
	overlay_rect_limits	destination;
	float				min_width_scale;
	float				max_width_scale;
	float				min_height_scale;
	float				max_height_scale;
	uint32				reserved[8];
} overlay_restrictions;

//------------------------------------------------------------------------------

struct screen_id { int32 id; };

extern _IMPEXP_BE const struct screen_id B_MAIN_SCREEN_ID;

//------------------------------------------------------------------------------

typedef enum
{
	B_NO_COLOR_SPACE =	0x0000,	//* byte in memory order, high bit first
	
	// linear color space (little endian is the default)
	B_RGB32 = 			0x0008,	//* B[7:0]  G[7:0]  R[7:0]  -[7:0]
	B_RGBA32 = 			0x2008,	// B[7:0]  G[7:0]  R[7:0]  A[7:0]
	B_RGB24 = 			0x0003,	// B[7:0]  G[7:0]  R[7:0]
	B_RGB16 = 			0x0005,	// G[2:0],B[4:0]  R[4:0],G[5:3]
	B_RGB15 = 			0x0010,	// G[2:0],B[4:0]  	   -[0],R[4:0],G[4:3]
	B_RGBA15 = 			0x2010,	// G[2:0],B[4:0]  	   A[0],R[4:0],G[4:3]
	B_CMAP8 = 			0x0004,	// D[7:0]
	B_GRAY8 = 			0x0002,	// Y[7:0]
	B_GRAY1 = 			0x0001,	// Y0[0],Y1[0],Y2[0],Y3[0],Y4[0],Y5[0],Y6[0],Y7[0]

	// big endian version, when the encoding is not endianess independant
	B_RGB32_BIG =		0x1008,	// -[7:0]  R[7:0]  G[7:0]  B[7:0]
	B_RGBA32_BIG = 		0x3008,	// A[7:0]  R[7:0]  G[7:0]  B[7:0]
	B_RGB24_BIG = 		0x1003,	// R[7:0]  G[7:0]  B[7:0]
	B_RGB16_BIG = 		0x1005,	// R[4:0],G[5:3]  G[2:0],B[4:0]
	B_RGB15_BIG = 		0x1010,	// -[0],R[4:0],G[4:3]  G[2:0],B[4:0]
	B_RGBA15_BIG = 		0x3010,	// A[0],R[4:0],G[4:3]  G[2:0],B[4:0]

	// little-endian declarations, for completness
	B_RGB32_LITTLE = 	B_RGB32,
	B_RGBA32_LITTLE =	B_RGBA32,
	B_RGB24_LITTLE =	B_RGB24,
	B_RGB16_LITTLE =	B_RGB16,
	B_RGB15_LITTLE =	B_RGB15,
	B_RGBA15_LITTLE =	B_RGBA15,

	// non linear color space -- note that these are here for exchange purposes;
	// a BBitmap or BView may not necessarily support all these color spaces.

	// Loss/Saturation points are Y 16-235 (absoulte); Cb/Cr 16-240 (center 128)

	B_YCbCr422 = 		0x4000,	// Y0[7:0]  Cb0[7:0]  Y1[7:0]  Cr0[7:0]  Y2[7:0]...
								// Cb2[7:0]  Y3[7:0]  Cr2[7:0]
	B_YCbCr411 = 		0x4001,	// Cb0[7:0]  Y0[7:0]  Cr0[7:0]  Y1[7:0]  Cb4[7:0]...
								// Y2[7:0]  Cr4[7:0]  Y3[7:0]  Y4[7:0]  Y5[7:0]...
								// Y6[7:0]  Y7[7:0]	
	B_YCbCr444 = 		0x4003,	// Y0[7:0]  Cb0[7:0]  Cr0[7:0]
	B_YCbCr420 = 		0x4004,	// Non-interlaced only, Cb0  Y0  Y1  Cb2 Y2  Y3
								// on even scan lines,  Cr0  Y0  Y1  Cr2 Y2  Y3
								// on odd scan lines

	// Extrema points are
	//		Y 0 - 207 (absolute)
	//		U -91 - 91 (offset 128)
	//		V -127 - 127 (offset 128)
	// note that YUV byte order is different from YCbCr
	// USE YCbCr, not YUV, when that's what you mean!
	B_YUV422 =			0x4020, // U0[7:0]  Y0[7:0]   V0[7:0]  Y1[7:0] ...
								// U2[7:0]  Y2[7:0]   V2[7:0]  Y3[7:0]
	B_YUV411 =			0x4021, // U0[7:0]  Y0[7:0]  Y1[7:0]  V0[7:0]  Y2[7:0]  Y3[7:0]
								// U4[7:0]  Y4[7:0]  Y5[7:0]  V4[7:0]  Y6[7:0]  Y7[7:0]
	B_YUV444 =			0x4023,	// U0[7:0]  Y0[7:0]  V0[7:0]  U1[7:0]  Y1[7:0]  V1[7:0]
	B_YUV420 = 			0x4024,	// Non-interlaced only, U0  Y0  Y1  U2 Y2  Y3
								// on even scan lines,  V0  Y0  Y1  V2 Y2  Y3
								// on odd scan lines
	B_YUV9 = 			0x402C,	// planar?	410?
	B_YUV12 = 			0x402D,	// planar?	420?

	B_UVL24 =			0x4030,	// U0[7:0] V0[7:0] L0[7:0] ...
	B_UVL32 =			0x4031,	// U0[7:0] V0[7:0] L0[7:0] X0[7:0]...
	B_UVLA32 =			0x6031,	// U0[7:0] V0[7:0] L0[7:0] A0[7:0]...

	B_LAB24 =			0x4032,	// L0[7:0] a0[7:0] b0[7:0] ...  (a is not alpha!)
	B_LAB32 =			0x4033,	// L0[7:0] a0[7:0] b0[7:0] X0[7:0] ... (b is not alpha!)
	B_LABA32 =			0x6033,	// L0[7:0] a0[7:0] b0[7:0] A0[7:0] ... (A is alpha)

	// red is at hue = 0

	B_HSI24 =			0x4040,	// H[7:0]  S[7:0]  I[7:0]
	B_HSI32 =			0x4041,	// H[7:0]  S[7:0]  I[7:0]  X[7:0]
	B_HSIA32 =			0x6041,	// H[7:0]  S[7:0]  I[7:0]  A[7:0]

	B_HSV24 =			0x4042,	// H[7:0]  S[7:0]  V[7:0]
	B_HSV32 =			0x4043,	// H[7:0]  S[7:0]  V[7:0]  X[7:0]
	B_HSVA32 =			0x6043,	// H[7:0]  S[7:0]  V[7:0]  A[7:0]

	B_HLS24 =			0x4044,	// H[7:0]  L[7:0]  S[7:0]
	B_HLS32 =			0x4045,	// H[7:0]  L[7:0]  S[7:0]  X[7:0]
	B_HLSA32 =			0x6045,	// H[7:0]  L[7:0]  S[7:0]  A[7:0]

	B_CMY24 =			0xC001,	// C[7:0]  M[7:0]  Y[7:0]  			No gray removal done
	B_CMY32 =			0xC002,	// C[7:0]  M[7:0]  Y[7:0]  X[7:0]	No gray removal done
	B_CMYA32 =			0xE002,	// C[7:0]  M[7:0]  Y[7:0]  A[7:0]	No gray removal done
	B_CMYK32 =			0xC003,	// C[7:0]  M[7:0]  Y[7:0]  K[7:0]

	// compatibility declarations
	B_MONOCHROME_1_BIT = 	B_GRAY1,
	B_GRAYSCALE_8_BIT =		B_GRAY8,
	B_COLOR_8_BIT =			B_CMAP8,
	B_RGB_32_BIT =			B_RGB32,
	B_RGB_16_BIT =			B_RGB15,
	B_BIG_RGB_32_BIT =		B_RGB32_BIG,
	B_BIG_RGB_16_BIT =		B_RGB15_BIG
} color_space;


// Find out whether a specific color space is supported by BBitmaps.
// Support_flags will be set to what kinds of support are available.
// If support_flags is set to 0, false will be returned.
enum {
	B_VIEWS_SUPPORT_DRAW_BITMAP = 0x1,
	B_BITMAPS_SUPPORT_ATTACHED_VIEWS = 0x2
};
_IMPEXP_BE bool bitmaps_support_space(color_space space, uint32 * support_flags);

//------------------------------------------------------------------------------
// "pixel_chunk" is the native increment from one pixel starting on an integral byte
// to the next. "row_alignment" is the native alignment for pixel scanline starts.
// "pixels_per_chunk" is the number of pixels in a pixel_chunk. For instance, B_GRAY1
// sets pixel_chunk to 1, row_alignment to 4 and pixels_per_chunk to 8, whereas
// B_RGB24 sets pixel_chunk to 3, row_alignment to 4 and pixels_per_chunk to 1.
//------------------------------------------------------------------------------
_IMPEXP_BE status_t get_pixel_size_for(color_space space, size_t * pixel_chunk, 
	size_t * row_alignment, size_t * pixels_per_chunk);


enum buffer_orientation {
	B_BUFFER_TOP_TO_BOTTOM,
	B_BUFFER_BOTTOM_TO_TOP
};

enum buffer_layout { 
	B_BUFFER_NONINTERLEAVED = 1
};
		      
//------------------------------------------------------------------------------

enum drawing_mode {
	B_OP_COPY,
	B_OP_OVER,
	B_OP_ERASE,
	B_OP_INVERT,
	B_OP_ADD,
	B_OP_SUBTRACT,
	B_OP_BLEND,
	B_OP_MIN,
	B_OP_MAX,
	B_OP_SELECT,
	B_OP_ALPHA
};

enum source_alpha {
	B_PIXEL_ALPHA=0,
	B_CONSTANT_ALPHA
};

enum alpha_function {
	B_ALPHA_OVERLAY=0,
	B_ALPHA_COMPOSITE
};

enum {
	B_8_BIT_640x480    = 0x00000001,
	B_8_BIT_800x600    = 0x00000002,
	B_8_BIT_1024x768   = 0x00000004,
	B_8_BIT_1280x1024  = 0x00000008,
	B_8_BIT_1600x1200  = 0x00000010,
	B_16_BIT_640x480   = 0x00000020,
	B_16_BIT_800x600   = 0x00000040,
	B_16_BIT_1024x768  = 0x00000080,
	B_16_BIT_1280x1024 = 0x00000100,
	B_16_BIT_1600x1200 = 0x00000200,
	B_32_BIT_640x480   = 0x00000400,
	B_32_BIT_800x600   = 0x00000800,
	B_32_BIT_1024x768  = 0x00001000,
	B_32_BIT_1280x1024 = 0x00002000,
	B_32_BIT_1600x1200 = 0x00004000,
    B_8_BIT_1152x900   = 0x00008000,
    B_16_BIT_1152x900  = 0x00010000,
    B_32_BIT_1152x900  = 0x00020000,
	B_15_BIT_640x480   = 0x00040000,
	B_15_BIT_800x600   = 0x00080000,
	B_15_BIT_1024x768  = 0x00100000,
	B_15_BIT_1280x1024 = 0x00200000,
	B_15_BIT_1600x1200 = 0x00400000,
    B_15_BIT_1152x900  = 0x00800000,
	
	// do not use B_FAKE_DEVICE--it will go away!
	B_FAKE_DEVICE	   = 0x40000000,
	B_8_BIT_640x400	   = (int)0x80000000
};

#endif	// _GRAPHICSDEFS_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

