/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * global definitions for the Painter frame work, mainly types for the
 * AGG pipelines
 *
 */

#ifndef DEFINES_H
#define DEFINES_H

#include <agg_rasterizer_outline.h>
#include <agg_rasterizer_outline_aa.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_renderer_outline_aa.h>
#include <agg_renderer_primitives.h>
#include <agg_renderer_scanline.h>
#include <agg_scanline_bin.h>
#include <agg_scanline_p.h>
#include <agg_scanline_u.h>
#include <agg_span_allocator.h>
#include <agg_span_gradient.h>
#include <agg_span_interpolator_linear.h>
#include <agg_rendering_buffer.h>

#include "agg_rasterizer_scanline_aa_subpix.h"
#include "agg_renderer_region.h"
#include "agg_renderer_scanline_subpix.h"
#include "agg_scanline_p_subpix.h"
#include "agg_scanline_p_subpix_avrg_filtering.h"
#include "agg_scanline_u_subpix.h"
#include "agg_scanline_u_subpix_avrg_filtering.h"

#include "GlobalSubpixelSettings.h"
#include "PixelFormat.h"


#define ALIASED_DRAWING 0

	typedef PixelFormat											pixfmt;
	typedef agg::renderer_region<pixfmt>						renderer_base;

#if ALIASED_DRAWING
	typedef agg::renderer_primitives<renderer_base>				outline_renderer_type;
	typedef agg::rasterizer_outline<outline_renderer_type>		outline_rasterizer_type;

	typedef agg::scanline_bin									scanline_unpacked_type;
	typedef agg::scanline_bin									scanline_packed_type;
	typedef agg::renderer_scanline_bin_solid<renderer_base>		renderer_type;
#else
	typedef agg::renderer_outline_aa<renderer_base>				outline_renderer_type;
	typedef agg::rasterizer_outline_aa<outline_renderer_type>	outline_rasterizer_type;

	typedef agg::scanline_u8									scanline_unpacked_type;
	typedef agg::scanline_p8									scanline_packed_type;
#ifdef AVERAGE_BASED_SUBPIXEL_FILTERING
	typedef agg::scanline_p8_subpix_avrg_filtering				scanline_packed_subpix_type;
	typedef agg::scanline_u8_subpix_avrg_filtering				scanline_unpacked_subpix_type;
#else
	typedef agg::scanline_p8_subpix								scanline_packed_subpix_type;
	typedef agg::scanline_u8_subpix								scanline_unpacked_subpix_type;
#endif
	typedef agg::renderer_scanline_aa_solid<renderer_base>		renderer_type;
#endif // !ALIASED_DRAWING

	typedef agg::renderer_scanline_bin_solid<renderer_base>		renderer_bin_type;
	typedef agg::renderer_scanline_subpix_solid<renderer_base>  renderer_subpix_type;

	typedef agg::rasterizer_scanline_aa<>						rasterizer_type;
	typedef agg::rasterizer_scanline_aa_subpix<>				rasterizer_subpix_type;

#endif // DEFINES_H


