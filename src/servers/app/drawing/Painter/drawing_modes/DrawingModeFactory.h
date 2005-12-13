/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Manages retrieving/instantiating of the correct DrawingMode class for
 * a given drawing_mode constant.
 *
 */

#ifndef DRAWING_MODE_FACTORY_H
#define DRAWING_MODE_FACTORY_H

#include <GraphicsDefs.h>

#include "DrawingMode.h"

class DrawingModeFactory {
 public:
								DrawingModeFactory();
	virtual						~DrawingModeFactory();

			DrawingMode*		DrawingModeFor(drawing_mode mode,
											   source_alpha alphaSrcMode,
											   alpha_function alphaFncMode,
											   bool solid = false);
 private:
			DrawingMode*		fDrawingModeBGRA32Over;
			DrawingMode*		fDrawingModeBGRA32OverSolid;
			DrawingMode*		fDrawingModeBGRA32Erase;
			DrawingMode*		fDrawingModeBGRA32Invert;
			DrawingMode*		fDrawingModeBGRA32Select;
			DrawingMode*		fDrawingModeBGRA32CopySolid;
			DrawingMode*		fDrawingModeBGRA32Copy;
			DrawingMode*		fDrawingModeBGRA32Add;
			DrawingMode*		fDrawingModeBGRA32Subtract;
			DrawingMode*		fDrawingModeBGRA32Blend;
			DrawingMode*		fDrawingModeBGRA32Min;
			DrawingMode*		fDrawingModeBGRA32Max;
			DrawingMode*		fDrawingModeBGRA32AlphaCO;
			DrawingMode*		fDrawingModeBGRA32AlphaCC;
			DrawingMode*		fDrawingModeBGRA32AlphaPO;
			DrawingMode*		fDrawingModeBGRA32AlphaPC;
};

#endif // DRAWING_MODE_FACTORY_H

