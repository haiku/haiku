/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Manages retrieving/instantiating of the correct DrawingMode class for
 * a given drawing_mode constant.
 *
 */

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
#include "DrawingModeOverSolid.h"
#include "DrawingModeSelect.h"
#include "DrawingModeSubtract.h"

#include "DrawingModeFactory.h"

// constructor
DrawingModeFactory::DrawingModeFactory()
	: fDrawingModeBGRA32Over(new DrawingModeBGRA32Over()),
	  fDrawingModeBGRA32OverSolid(new DrawingModeBGRA32OverSolid()),
	  fDrawingModeBGRA32Erase(new DrawingModeBGRA32Erase()),
	  fDrawingModeBGRA32Invert(new DrawingModeBGRA32Invert()),
	  fDrawingModeBGRA32Select(new DrawingModeBGRA32Select()),
	  fDrawingModeBGRA32CopySolid(new DrawingModeBGRA32CopySolid()),
	  fDrawingModeBGRA32Copy(new DrawingModeBGRA32Copy()),
	  fDrawingModeBGRA32Add(new DrawingModeBGRA32Add()),
	  fDrawingModeBGRA32Subtract(new DrawingModeBGRA32Subtract()),
	  fDrawingModeBGRA32Blend(new DrawingModeBGRA32Blend()),
	  fDrawingModeBGRA32Min(new DrawingModeBGRA32Min()),
	  fDrawingModeBGRA32Max(new DrawingModeBGRA32Max()),
	  fDrawingModeBGRA32AlphaCO(new DrawingModeBGRA32AlphaCO()),
	  fDrawingModeBGRA32AlphaCC(new DrawingModeBGRA32AlphaCC()),
	  fDrawingModeBGRA32AlphaPO(new DrawingModeBGRA32AlphaPO()),
	  fDrawingModeBGRA32AlphaPC(new DrawingModeBGRA32AlphaPC())
{
}

// destructor
DrawingModeFactory::~DrawingModeFactory()
{
	delete fDrawingModeBGRA32Over;
	delete fDrawingModeBGRA32OverSolid;
	delete fDrawingModeBGRA32Erase;
	delete fDrawingModeBGRA32Invert;
	delete fDrawingModeBGRA32Select;
	delete fDrawingModeBGRA32CopySolid;
	delete fDrawingModeBGRA32Copy;
	delete fDrawingModeBGRA32Add;
	delete fDrawingModeBGRA32Subtract;
	delete fDrawingModeBGRA32Blend;
	delete fDrawingModeBGRA32Min;
	delete fDrawingModeBGRA32Max;
	delete fDrawingModeBGRA32AlphaCO;
	delete fDrawingModeBGRA32AlphaCC;
	delete fDrawingModeBGRA32AlphaPO;
	delete fDrawingModeBGRA32AlphaPC;
}

// DrawingModeFor
DrawingMode*
DrawingModeFactory::DrawingModeFor(drawing_mode mode,
								   source_alpha alphaSrcMode,
								   alpha_function alphaFncMode,
								   bool solid)
{
	switch (mode) {
		// these drawing modes discard source pixels
		// which have the current low color
		case B_OP_OVER:
			if (solid)
				return fDrawingModeBGRA32OverSolid;
			else
				return fDrawingModeBGRA32Over;
			break;
		case B_OP_ERASE:
			return fDrawingModeBGRA32Erase;
			break;
		case B_OP_INVERT:
			return fDrawingModeBGRA32Invert;
			break;
		case B_OP_SELECT:
			return fDrawingModeBGRA32Select;
			break;

		// in these drawing modes, the current high
		// and low color are treated equally
		case B_OP_COPY:
			if (solid)
				return fDrawingModeBGRA32CopySolid;
			else
				return fDrawingModeBGRA32Copy;
			break;
		case B_OP_ADD:
			return fDrawingModeBGRA32Add;
			break;
		case B_OP_SUBTRACT:
			return fDrawingModeBGRA32Subtract;
			break;
		case B_OP_BLEND:
			return fDrawingModeBGRA32Blend;
			break;
		case B_OP_MIN:
			return fDrawingModeBGRA32Min;
			break;
		case B_OP_MAX:
			return fDrawingModeBGRA32Max;
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
					return fDrawingModeBGRA32AlphaCO;
				} else if (alphaFncMode == B_ALPHA_COMPOSITE) {
					return fDrawingModeBGRA32AlphaCC;
				}
			} else if (alphaSrcMode == B_PIXEL_ALPHA){
				if (alphaFncMode == B_ALPHA_OVERLAY) {
					return fDrawingModeBGRA32AlphaPO;
				} else if (alphaFncMode == B_ALPHA_COMPOSITE) {
					return fDrawingModeBGRA32AlphaPC;
				}
			}
			break;

		default:
fprintf(stderr, "DrawingModeFactory::DrawingModeFor() - drawing_mode not implemented\n");
			return fDrawingModeBGRA32Copy;
	}
	return NULL;
}

