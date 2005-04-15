// DrawingMode.h

#ifndef DRAWING_MODE_H
#define DRAWING_MODE_H

#include <stdio.h>
#include <string.h>
#include <agg_basics.h>
#include <agg_color_rgba8.h>
#include <agg_rendering_buffer.h>

class PatternHandler;

// BLEND
//
// this macro assumes source alpha in range 0..255 and
// ignores dest alpha (is assumed to equal 255)
#define BLEND(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	(d1) = (((((s1) - (d1)) * (a)) + ((d1) << 8)) >> 8); \
	(d2) = (((((s2) - (d2)) * (a)) + ((d2) << 8)) >> 8); \
	(d3) = (((((s3) - (d3)) * (a)) + ((d3) << 8)) >> 8); \
	(da) = max_c((da), (a)); \
}

// BLEND16
//
// this macro assumes source alpha in range 0..65025 and
// ignores dest alpha (is assumed to equal 255)
#define BLEND16(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	(d1) = (((((s1) - (d1)) * (a)) + ((d1) << 16)) >> 16); \
	(d2) = (((((s2) - (d2)) * (a)) + ((d2) << 16)) >> 16); \
	(d3) = (((((s3) - (d3)) * (a)) + ((d3) << 16)) >> 16); \
	(da) = max_c((da), (a) >> 8); \
}

// BLEND_COMPOSITE
//
// this macro assumes source alpha in range 0..255 and
// composes the source color over a possibly semi-transparent background
#define BLEND_COMPOSITE(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	if ((da) == 255) { \
		BLEND(d1, d2, d3, da, s1, s2, s3, a); \
	} else { \
		uint8 alphaRest = 255 - (a); \
		uint32 alphaTemp = (65025 - alphaRest * (255 - (da))); \
		uint32 alphaDest = (da) * alphaRest; \
		uint32 alphaSrc = 255 * (a); \
		(d1) = ((d1) * alphaDest + (s2) * alphaSrc) / alphaTemp; \
		(d2) = ((d2) * alphaDest + (s2) * alphaSrc) / alphaTemp; \
		(d3) = ((d3) * alphaDest + (s3) * alphaSrc) / alphaTemp; \
		(da) = alphaTemp >> 8; \
	} \
}

// BLEND_COMPOSITE16
//
// this macro assumes source alpha in range 0..65025 and
// composes the source color over a possibly semi-transparent background
// TODO: implement a faster version
#define BLEND_COMPOSITE16(d1, d2, d3, da, s1, s2, s3, a) \
{ \
	BLEND_COMPOSITE(d1, d2, d3, da, s1, s2, s3, (a) >> 8); \
}

class DrawingMode {
 public:
	typedef agg::rgba8 color_type;

	// constructor
	DrawingMode()
		: fBuffer(NULL)
	{
	}

	// destructor
	virtual ~DrawingMode()
	{
	}

	// set_rendering_buffer
	void set_rendering_buffer(agg::rendering_buffer* buffer)
	{
		fBuffer = buffer;
	}

	// set_pattern_handler
	void set_pattern_handler(const PatternHandler* handler)
	{
		fPatternHandler = handler;
	}

	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover) = 0;

	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover) = 0;

	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover) = 0;

	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers) = 0;

	virtual	void blend_solid_vspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers) = 0;

	virtual	void blend_color_hspan(int x, int y, unsigned len, 
								   const color_type* colors, 
								   const uint8* covers,
								   uint8 cover) = 0;

	virtual	void blend_color_vspan(int x, int y, unsigned len, 
								   const color_type* colors, 
								   const uint8* covers,
								   uint8 cover) = 0;

protected:
	agg::rendering_buffer*	fBuffer;
	const PatternHandler*	fPatternHandler;
	

};

#endif // DRAWING_MODE_H

