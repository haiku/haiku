/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
 *
 * A class implementing the AGG "pixel format" interface which maintains
 * a PatternHandler and pointers to blending functions implementing the
 * different BeOS "drawing_modes".
 *
 */

#include "PixelFormat.h"

#include <stdio.h>

#include "DrawingModeAdd.h"
#include "DrawingModeAlphaCC.h"
#include "DrawingModeAlphaCO.h"
#include "DrawingModeAlphaCOSolid.h"
#include "DrawingModeAlphaPC.h"
#include "DrawingModeAlphaPCSolid.h"
#include "DrawingModeAlphaPO.h"
#include "DrawingModeAlphaPOSolid.h"
#include "DrawingModeBlend.h"
#include "DrawingModeCopy.h"
#include "DrawingModeCopySolid.h"
#include "DrawingModeErase.h"
#include "DrawingModeInvert.h"
#include "DrawingModeMin.h"
#include "DrawingModeMax.h"
#include "DrawingModeOver.h"
#include "DrawingModeOverSolid.h"
#include "DrawingModeSelect.h"
#include "DrawingModeSubtract.h"

#include "DrawingModeAddSUBPIX.h"
#include "DrawingModeAlphaCCSUBPIX.h"
#include "DrawingModeAlphaCOSUBPIX.h"
#include "DrawingModeAlphaCOSolidSUBPIX.h"
#include "DrawingModeAlphaPCSUBPIX.h"
#include "DrawingModeAlphaPOSUBPIX.h"
#include "DrawingModeAlphaPOSolidSUBPIX.h"
#include "DrawingModeBlendSUBPIX.h"
#include "DrawingModeCopySUBPIX.h"
#include "DrawingModeCopySolidSUBPIX.h"
#include "DrawingModeEraseSUBPIX.h"
#include "DrawingModeInvertSUBPIX.h"
#include "DrawingModeMinSUBPIX.h"
#include "DrawingModeMaxSUBPIX.h"
#include "DrawingModeOverSUBPIX.h"
#include "DrawingModeOverSolidSUBPIX.h"
#include "DrawingModeSelectSUBPIX.h"
#include "DrawingModeSubtractSUBPIX.h"

#include "PatternHandler.h"

// blend_pixel_empty
void
blend_pixel_empty(int x, int y, const color_type& c, uint8 cover,
				  agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_pixel_empty()\n");
}

// blend_hline_empty
void
blend_hline_empty(int x, int y, unsigned len,
				  const color_type& c, uint8 cover,
				  agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_hline_empty()\n");
}

// blend_vline_empty
void
blend_vline_empty(int x, int y, unsigned len,
				  const color_type& c, uint8 cover,
				  agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_vline_empty()\n");
}

// blend_solid_hspan_empty
void
blend_solid_hspan_empty(int x, int y, unsigned len,
						const color_type& c, const uint8* covers,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_solid_hspan_empty()\n");
}

// blend_solid_hspan_subpix_empty
void
blend_solid_hspan_empty_subpix(int x, int y, unsigned len,
						const color_type& c, const uint8* covers,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_solid_hspan_empty_subpix()\n");
}

// blend_solid_vspan_empty
void
blend_solid_vspan_empty(int x, int y,
						unsigned len, const color_type& c,
						const uint8* covers,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_solid_vspan_empty()\n");
}

// blend_color_hspan_empty
void
blend_color_hspan_empty(int x, int y, unsigned len,
						const color_type* colors,
						const uint8* covers, uint8 cover,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_color_hspan_empty()\n");
}

// blend_color_vspan_empty
void
blend_color_vspan_empty(int x, int y, unsigned len,
						const color_type* colors,
						const uint8* covers, uint8 cover,
						agg_buffer* buffer, const PatternHandler* pattern)
{
	printf("blend_color_vspan_empty()\n");
}

// #pragma mark -

// constructor
PixelFormat::PixelFormat(agg::rendering_buffer& rb,
						 const PatternHandler* handler)
	: fBuffer(&rb),
	  fPatternHandler(handler),

	  fBlendPixel(blend_pixel_empty),
	  fBlendHLine(blend_hline_empty),
	  fBlendVLine(blend_vline_empty),
	  fBlendSolidHSpan(blend_solid_hspan_empty),
	  fBlendSolidHSpanSubpix(blend_solid_hspan_empty_subpix),
	  fBlendSolidVSpan(blend_solid_vspan_empty),
	  fBlendColorHSpan(blend_color_hspan_empty),
	  fBlendColorVSpan(blend_color_vspan_empty)
{
}

// destructor
PixelFormat::~PixelFormat()
{
}

// SetDrawingMode
void
PixelFormat::SetDrawingMode(drawing_mode mode, source_alpha alphaSrcMode,
							alpha_function alphaFncMode, bool text)
{
	switch (mode) {
		// These drawing modes discard source pixels
		// which have the current low color.
		case B_OP_OVER:
			if (fPatternHandler->IsSolid()) {
				fBlendPixel = blend_pixel_over_solid;
				fBlendHLine = blend_hline_over_solid;
				fBlendSolidHSpan = blend_solid_hspan_over_solid;
				fBlendSolidVSpan = blend_solid_vspan_over_solid;
				fBlendSolidHSpanSubpix = blend_solid_hspan_over_solid_subpix;
			} else {
				fBlendPixel = blend_pixel_over;
				fBlendHLine = blend_hline_over;
				fBlendSolidHSpanSubpix = blend_solid_hspan_over_subpix;
				fBlendSolidHSpan = blend_solid_hspan_over;
				fBlendSolidVSpan = blend_solid_vspan_over;
			}
			fBlendColorHSpan = blend_color_hspan_over;
			break;
		case B_OP_ERASE:
			fBlendPixel = blend_pixel_erase;
			fBlendHLine = blend_hline_erase;
			fBlendSolidHSpanSubpix = blend_solid_hspan_erase_subpix;
			fBlendSolidHSpan = blend_solid_hspan_erase;
			fBlendSolidVSpan = blend_solid_vspan_erase;
			fBlendColorHSpan = blend_color_hspan_erase;
			break;
		case B_OP_INVERT:
			fBlendPixel = blend_pixel_invert;
			fBlendHLine = blend_hline_invert;
			fBlendSolidHSpanSubpix = blend_solid_hspan_invert_subpix;
			fBlendSolidHSpan = blend_solid_hspan_invert;
			fBlendSolidVSpan = blend_solid_vspan_invert;
			fBlendColorHSpan = blend_color_hspan_invert;
			break;
		case B_OP_SELECT:
			fBlendPixel = blend_pixel_select;
			fBlendHLine = blend_hline_select;
			fBlendSolidHSpanSubpix = blend_solid_hspan_select_subpix;
			fBlendSolidHSpan = blend_solid_hspan_select;
			fBlendSolidVSpan = blend_solid_vspan_select;
			fBlendColorHSpan = blend_color_hspan_select;
			break;

		// In these drawing modes, the current high
		// and low color are treated equally.
		case B_OP_COPY:
			if (fPatternHandler->IsSolid()) {
				fBlendPixel = blend_pixel_copy_solid;
				fBlendHLine = blend_hline_copy_solid;
				fBlendSolidHSpanSubpix = blend_solid_hspan_copy_solid_subpix;
				fBlendSolidHSpan = blend_solid_hspan_copy_solid;
				fBlendSolidVSpan = blend_solid_vspan_copy_solid;
				fBlendColorHSpan = blend_color_hspan_copy_solid;
			} else {
				fBlendPixel = blend_pixel_copy;
				fBlendHLine = blend_hline_copy;
				fBlendSolidHSpanSubpix = blend_solid_hspan_copy_subpix;
				fBlendSolidHSpan = blend_solid_hspan_copy;
				fBlendSolidVSpan = blend_solid_vspan_copy;
				fBlendColorHSpan = blend_color_hspan_copy;
			}
			break;
		case B_OP_ADD:
			fBlendPixel = blend_pixel_add;
			fBlendHLine = blend_hline_add;
			fBlendSolidHSpanSubpix = blend_solid_hspan_add_subpix;
			fBlendSolidHSpan = blend_solid_hspan_add;
			fBlendSolidVSpan = blend_solid_vspan_add;
			fBlendColorHSpan = blend_color_hspan_add;
			break;
		case B_OP_SUBTRACT:
			fBlendPixel = blend_pixel_subtract;
			fBlendHLine = blend_hline_subtract;
			fBlendSolidHSpanSubpix = blend_solid_hspan_subtract_subpix;
			fBlendSolidHSpan = blend_solid_hspan_subtract;
			fBlendSolidVSpan = blend_solid_vspan_subtract;
			fBlendColorHSpan = blend_color_hspan_subtract;
			break;
		case B_OP_BLEND:
			fBlendPixel = blend_pixel_blend;
			fBlendHLine = blend_hline_blend;
			fBlendSolidHSpanSubpix = blend_solid_hspan_blend_subpix;
			fBlendSolidHSpan = blend_solid_hspan_blend;
			fBlendSolidVSpan = blend_solid_vspan_blend;
			fBlendColorHSpan = blend_color_hspan_blend;
			break;
		case B_OP_MIN:
			fBlendPixel = blend_pixel_min;
			fBlendHLine = blend_hline_min;
			fBlendSolidHSpanSubpix = blend_solid_hspan_min_subpix;
			fBlendSolidHSpan = blend_solid_hspan_min;
			fBlendSolidVSpan = blend_solid_vspan_min;
			fBlendColorHSpan = blend_color_hspan_min;
			break;
		case B_OP_MAX:
			fBlendPixel = blend_pixel_max;
			fBlendHLine = blend_hline_max;
			fBlendSolidHSpanSubpix = blend_solid_hspan_max_subpix;
			fBlendSolidHSpan = blend_solid_hspan_max;
			fBlendSolidVSpan = blend_solid_vspan_max;
			fBlendColorHSpan = blend_color_hspan_max;
			break;

		// This drawing mode is the only one considering
		// alpha at all. In B_CONSTANT_ALPHA, the alpha
		// value from the current high color is used for
		// all computations. In B_PIXEL_ALPHA, the alpha
		// is considered at every source pixel.
		// To simplify the implementation, four separate
		// DrawingMode classes are used to cover the
		// four possible combinations of alpha enabled drawing.
		case B_OP_ALPHA:
			if (alphaSrcMode == B_CONSTANT_ALPHA) {
				if (alphaFncMode == B_ALPHA_OVERLAY) {
					if (fPatternHandler->IsSolid()) {
						fBlendPixel = blend_pixel_alpha_co_solid;
						fBlendHLine = blend_hline_alpha_co_solid;
						fBlendSolidHSpanSubpix = blend_solid_hspan_alpha_co_solid_subpix;
						fBlendSolidHSpan = blend_solid_hspan_alpha_co_solid;
						fBlendSolidVSpan = blend_solid_vspan_alpha_co_solid;
					} else {
						fBlendPixel = blend_pixel_alpha_co;
						fBlendHLine = blend_hline_alpha_co;
						fBlendSolidHSpanSubpix = blend_solid_hspan_alpha_co_subpix;
						fBlendSolidHSpan = blend_solid_hspan_alpha_co;
						fBlendSolidVSpan = blend_solid_vspan_alpha_co;
					}
					fBlendColorHSpan = blend_color_hspan_alpha_co;
				} else if (alphaFncMode == B_ALPHA_COMPOSITE) {
					fBlendPixel = blend_pixel_alpha_cc;
					fBlendHLine = blend_hline_alpha_cc;
					fBlendSolidHSpanSubpix = blend_solid_hspan_alpha_cc_subpix;
					fBlendSolidHSpan = blend_solid_hspan_alpha_cc;
					fBlendSolidVSpan = blend_solid_vspan_alpha_cc;
					fBlendColorHSpan = blend_color_hspan_alpha_cc;
				}
			} else if (alphaSrcMode == B_PIXEL_ALPHA){
				if (alphaFncMode == B_ALPHA_OVERLAY) {
					if (fPatternHandler->IsSolid()) {
						fBlendPixel = blend_pixel_alpha_po_solid;
						fBlendHLine = blend_hline_alpha_po_solid;
						fBlendSolidHSpanSubpix = blend_solid_hspan_alpha_po_solid_subpix;
						fBlendSolidHSpan = blend_solid_hspan_alpha_po_solid;
						fBlendSolidVSpan = blend_solid_vspan_alpha_po_solid;
					} else {
						fBlendPixel = blend_pixel_alpha_po;
						fBlendHLine = blend_hline_alpha_po;
						fBlendSolidHSpanSubpix = blend_solid_hspan_alpha_po_subpix;
						fBlendSolidHSpan = blend_solid_hspan_alpha_po;
						fBlendSolidVSpan = blend_solid_vspan_alpha_po;
					}
					fBlendColorHSpan = blend_color_hspan_alpha_po;
				} else if (alphaFncMode == B_ALPHA_COMPOSITE) {
					if (fPatternHandler->IsSolid()) {
						fBlendPixel = blend_pixel_alpha_pc_solid;
						fBlendHLine = blend_hline_alpha_pc_solid;
						fBlendSolidHSpanSubpix = blend_solid_hspan_alpha_pc_subpix;
						fBlendSolidHSpan = blend_solid_hspan_alpha_pc_solid;
						fBlendSolidVSpan = blend_solid_vspan_alpha_pc_solid;
						fBlendColorHSpan = blend_color_hspan_alpha_pc_solid;
					} else {
						fBlendPixel = blend_pixel_alpha_pc;
						fBlendHLine = blend_hline_alpha_pc;
						fBlendSolidHSpanSubpix = blend_solid_hspan_alpha_pc_subpix;
						fBlendSolidHSpan = blend_solid_hspan_alpha_pc;
						fBlendSolidVSpan = blend_solid_vspan_alpha_pc;
						fBlendColorHSpan = blend_color_hspan_alpha_pc;
					}
				}
			}
			break;

		default:
fprintf(stderr, "PixelFormat::SetDrawingMode() - drawing_mode not implemented\n");
//			return fDrawingModeBGRA32Copy;
			break;
	}
}
