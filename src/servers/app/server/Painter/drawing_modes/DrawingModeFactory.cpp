// DrawingModeFactory.h

#include <stdio.h>

#include "DrawingModeAdd.h"
#include "DrawingModeAlphaCC.h"
#include "DrawingModeAlphaCO.h"
#include "DrawingModeAlphaPC.h"
#include "DrawingModeAlphaPO.h"
#include "DrawingModeBlend.h"
#include "DrawingModeCopy.h"
#include "DrawingModeCopySolid.h"
#include "DrawingModeErase.h"
#include "DrawingModeInvert.h"
#include "DrawingModeMax.h"
#include "DrawingModeMin.h"
#include "DrawingModeOver.h"
#include "DrawingModeSelect.h"
#include "DrawingModeSubtract.h"

#include "DrawingModeFactory.h"

// DrawingModeFor
agg::DrawingMode*
DrawingModeFactory::DrawingModeFor(drawing_mode mode,
								   source_alpha alphaSrcMode,
								   alpha_function alphaFncMode,
								   bool solid)
{
	switch (mode) {
		// these drawing modes discard source pixels
		// which have the current low color
		case B_OP_OVER:
			return new agg::DrawingModeBGRA32Over();
			break;
		case B_OP_ERASE:
			return new agg::DrawingModeBGRA32Erase();
			break;
		case B_OP_INVERT:
			return new agg::DrawingModeBGRA32Invert();
			break;
		case B_OP_SELECT:
			return new agg::DrawingModeBGRA32Select();
			break;

		// in these drawing modes, the current high
		// and low color are treated equally
		case B_OP_COPY:
			if (solid) {
printf("DrawingModeBGRA32CopySolid()\n");
				return new agg::DrawingModeBGRA32CopySolid();
			} else
				return new agg::DrawingModeBGRA32Copy();
			break;
		case B_OP_ADD:
			return new agg::DrawingModeBGRA32Add();
			break;
		case B_OP_SUBTRACT:
			return new agg::DrawingModeBGRA32Subtract();
			break;
		case B_OP_BLEND:
			return new agg::DrawingModeBGRA32Blend();
			break;
		case B_OP_MIN:
			return new agg::DrawingModeBGRA32Min();
			break;
		case B_OP_MAX:
			return new agg::DrawingModeBGRA32Max();
			break;

		// this drawing mode is the only one considering
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
					return new agg::DrawingModeBGRA32AlphaCO();
				} else if (alphaFncMode == B_ALPHA_COMPOSITE) {
					return new agg::DrawingModeBGRA32AlphaCC();
				}
			} else if (alphaSrcMode == B_PIXEL_ALPHA){
				if (alphaFncMode == B_ALPHA_OVERLAY) {
					return new agg::DrawingModeBGRA32AlphaPO();
				} else if (alphaFncMode == B_ALPHA_COMPOSITE) {
					return new agg::DrawingModeBGRA32AlphaPC();
				}
			}
			break;

		default:
fprintf(stderr, "DrawingModeFactory::DrawingModeFor() - drawing_mode not implemented\n");
			return new agg::DrawingModeBGRA32Copy();
	}
	return NULL;
}

