// agg_renderer_types.h

#ifndef DEFINES_H
#define DEFINES_H

#include <agg_rasterizer_outline.h>
#include <agg_rasterizer_outline_aa.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_renderer_mclip.h>
#include <agg_renderer_outline_aa.h>
#include <agg_renderer_primitives.h>
#include <agg_renderer_scanline.h>
#include <agg_scanline_bin.h>
//#include <agg_scanline_p.h>
#include <agg_scanline_u.h>
#include <agg_rendering_buffer.h>

//#include "_for_reference_.h"
#include "forwarding_pixfmt.h"

#define ALIASED_DRAWING 0

//	typedef agg::pixfmt_bgra32									pixfmt;
	typedef forwarding_pixel_format<agg::order_bgra32>			pixfmt;
	typedef agg::renderer_mclip<pixfmt>							renderer_base;

#if ALIASED_DRAWING
	typedef agg::renderer_primitives<renderer_base>				outline_renderer_type;
	typedef agg::rasterizer_outline<outline_renderer_type>		outline_rasterizer_type;

	typedef agg::scanline_bin									scanline_type;
	typedef agg::rasterizer_scanline_aa<>						rasterizer_type;
	typedef agg::renderer_scanline_bin_solid<renderer_base>		renderer_type;
#else
	typedef agg::renderer_outline_aa<renderer_base>				outline_renderer_type;
	typedef agg::rasterizer_outline_aa<outline_renderer_type>	outline_rasterizer_type;

	typedef agg::scanline_u8									scanline_type;
	typedef agg::rasterizer_scanline_aa<>						rasterizer_type;
	typedef agg::renderer_scanline_aa_solid<renderer_base>		renderer_type;
#endif

	typedef agg::renderer_scanline_aa_solid<renderer_base>		font_renderer_solid_type;
	typedef agg::renderer_scanline_bin_solid<renderer_base>		font_renderer_bin_type;


#endif // DEFINES_H


