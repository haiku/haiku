/*
	
	ChartRender.h
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _CHART_RENDER_
#define _CHART_RENDER_

/* This header file has been designed to encapsulate ALL declarations
   related to basic drawing and animation in the application. The idea
   was to reduce the most processor-intesinve part of the application
   to its minimal "C-like" core, independent of the Be headers, so that
   the correspondant source code (in ChartRender.c) can be compiled
   with another compiler and just link with the rest of the projects.
   
   That allow you to use advanced compiler generating faster code for
   the critical part of the demo. The drawback is that such manipulation
   is difficult and should be reserved for really critical code.
   This is really useful only on intel. */

/* this is used for a non Be-environment */
#if 0

/* this is a copy of the part of GraphicsDefs.h we really use. */
typedef enum {
	B_RGB32 = 			0x0008,		
	B_RGBA32 = 			0x2008,
	B_RGB16 = 			0x0005,
	B_RGB15 = 			0x0010,
	B_RGBA15 = 			0x2010,
	B_CMAP8 = 			0x0004,
	B_RGB32_BIG =		0x1008,
	B_RGBA32_BIG = 		0x3008,
	B_RGB16_BIG = 		0x1005,
	B_RGB15_BIG = 		0x1010,
	B_RGBA15_BIG = 		0x3010
} color_space;

/* this is a copy of the part of SupportDefs.h we really use */
typedef	signed char				int8;
typedef unsigned char			uint8;
typedef	short					int16;
typedef unsigned short			uint16;
typedef	long					int32;
typedef unsigned long			uint32;
typedef unsigned char			bool;
#define false	0
#define true	1

/* this is a copy of the part of DirectWindow.h we really use */
typedef struct {
	int32		left;
	int32		top;
	int32		right;
	int32		bottom;
} clipping_rect;	

/* The default rounding mode on intel processor is round to
   closest. With the intel compiler, it's possible to ask for
   the floating to integer conversion to be done that way and
   not the C-standard way (round to floor). This constant is
   used to compensate for the change of conversion algorythm.
   Using the CPU native mode is faster than the C-standard
   mode. Another crazy optimisation you shouldn't care too
   much about (except if you're a crazy geek). */ 
#ifdef __i386__
#define ROUNDING	0.0
#else
#define ROUNDING	0.5
#endif

/* This represent the standard Be-environment */
#else

/* We just include the Be headers */
#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <DirectWindow.h>

/* Rounding is always done in C-standard mode */
#define ROUNDING	0.5

#endif

/* Count of different light level available for a star. */
#define		LEVEL_COUNT			32
/* Used to mark the last drawing offset of a star as unused
   (the star was invisible last time we tried drawing it) */
#define		INVALID				0x10000000
/* Clipping is done in 2 pass. A pixel clipping handle the
   real window visibility clipping. A geometric clipping
   sort out all stars that are clearly out of the pyramid
   of vision of the full-window. As star are bigger than
   just a pixel, we need a error margin to compensate for
   that, so that we don't clipped out star that would be
   barely visible on the side. */
#define		BORDER_CLIPPING		0.01

/* the three drawing row-format. */
#define		PIXEL_1_BYTE		0
#define		PIXEL_2_BYTES		1
#define		PIXEL_4_BYTES		2


#ifdef __cplusplus
extern "C" {
#endif

/* This is the generic definition of a drawing buffer. This
   can describe both an offscreen bitmap or a directwindow
   frame-buffer. That the layer that allows us to abstract
   our real drawing buffer and make our drawing code indepen-
   dant of its real target, so that it's trivial to switch
   from Bitmap mode to DirectWindow mode. */
typedef struct {
	/* base address of the buffer. */
	void				*bits;
	/* count of bytes between the begining of 2 lines. */
	int32				bytes_per_row;
	/* count of bytes per pixel. */
	int32				bytes_per_pixel;
	/* row-format of the buffer (PIXEL_1_BYTE, PIXEL_2_BYTES
	   or PIXEL_4_BYTES. */
	int32				depth_mode;
	/* buffer dimensions. */
	int32				buffer_width;
	int32				buffer_height;
	/* color_space of the buffer. */
	color_space			depth;
	/* 7x8x32 bits words representing the row value of the 7
	   possible star color, at 8 different light levels. If
	   the pixel encoding doesn't use 4 bytes, the color
	   value is repeated as many as necessary to fill the
	   32 bits. */
	uint32				colors[7][8];
	/* same color value, for the background color of the
	   buffer. */
	uint32				back_color;
	/* clipping of the buffer, in the standard DirectWindow
	   format. */
	uint32				clip_list_count;
	clipping_rect		clip_bounds;
	clipping_rect		clip_list[64];
	/* base address of the buffer offset by the delta offset
	   of the 32 different bits used to render the different
	   size of stars. */
	void				*pattern_bits[32];
} buffer;

/* this strcuture is used to represent a star. */
typedef struct {
	/* position of the star in a [0-1]x[0-1]x[0-1] cube */
	float		x;
	float		y;
	float		z;
	/* size coefficient. Some star are bigger than others */
	float		size;
	/* color profile of the star (between the 7 existing colors */
	uint8		color_type;
	/* lighting level */
	uint8		level;
	/* pattern level. Used to design which representation of star
	   will be used, depending of the ligthing level and the half-
	   pixel alignment. */
	uint16		pattern_level;
	/* screen coordinate, relative to the center */
	int16		h;
	int16		v;
	/* if the star was drawn at the previous frame, this contains
	   the drawing offset of the reference pixel in the buffer.
	   Then last_draw_pattern contains the mask of bits really
	   draw in the star pixel pattern (32 pixels max).
	   If the star wasn't draw, last_draw_offset contains INVALID */
	int32		last_draw_offset;
	int32		last_draw_pattern;
} star;

/* struct defining a collection of star. Include a array of stars,
   the count of currently used members of the array (the array can
   be bigger and only partialy used, and the previous value of that
   count (call erase_count) that define which stars were drawn
   before, and so need to be erase for this frame. */
typedef struct {
	star		*list;
	int32		count;
	int32		erase_count;
} star_packet;

/* this struct define the full geometry of the camera looking at
   the star field. */
typedef struct {
    /* position of the camera in the extended [0-2]x[0-2]x[0-2]
       cube. */
	float   	x;
	float   	y;
	float   	z;
	/* those values define the limit below which stars should be
	   virtually duplicate in extended space. That allows to move
	   the [0-1]x[0-1]x[0-1] star cube (as a cycling torus on the
	   3 axis), to any position inside the [0-2]x[0-2]x[0-2] cube.
	   The pyramid of vision is designed to be always included
	   inside a [0-1]x[0-1]x[0-1]. So all stars are defined in
	   such a small cube, then the cube will cycle using those
	   3 variables (for example, cutx = 0.3 will virtually cut
	   the [0-0.3]x[0-1]x[0-1] of the cube and duplicate it at
	   [1.0-1.3]x[0-1]x[0-1], creating a [0.3-1.3]x[0-1]x[0-1]
	   star field. Doing the same thing on the 3 axis, allows
	   to cycle the starfield wherever it's needed to fully
	   include the pyramid of vision. That way, the camera
	   always look at a properly positionned 1x1x1 starfield... */
	float   	cutx;
	float   	cuty;
	float   	cutz;
	/* rotation matrix of the camera */
	float 		m[3][3];
	/* offset of the center of the camera vision axis in the window */
	float		offset_h, offset_v;
	/* min and max ratio x/z or y/z defining the pyramid of vision
	   used for the first quick clipping. */
	float		xz_min, xz_max, yz_min, yz_max;
	/* min and max visibility threshold used for the front and rear
	   clipping, and the square factor used in the lighting formula */
	float		z_min, z_max, z_max_square;
	/* zoom factor of the camera, basically how large (in pixel) a
	   object of size 1.0 at a depth of 1.0 would look like */
	float		zoom_factor;
} geometry;

/* offset (horizontal and vertical) of the 32 pixels used to
   represent different sizes of stars. */
int8	pattern_dh[32];
int8	pattern_dv[32];

/* the public API of th animation module */
	/* first time init */
void		InitPatterns();
	/* used to move/draw/erase a colection of star in specific buffer */
void		RefreshStarPacket(buffer *buf, star_packet *sp, geometry *geo);
	/* used to update the visibility status of a collection of stars
	   after the clipping changed. */
void		RefreshClipping(buffer *buf, star_packet *sp);

#ifdef __cplusplus
};
#endif

#endif
