// DrawingModeFactory.h


#include "DrawingModeAdd.h"
#include "DrawingModeBlend.h"
#include "DrawingModeCopy.h"
#include "DrawingModeInvert.h"
#include "DrawingModeMax.h"
#include "DrawingModeMin.h"
#include "DrawingModeOver.h"
#include "DrawingModeSubtract.h"

#include "DrawingModeFactory.h"

// DrawingModeFor
agg::DrawingMode*
DrawingModeFactory::DrawingModeFor(drawing_mode mode)
{
	switch (mode) {
		case B_OP_INVERT:
			return new agg::DrawingModeBGRA32Invert();
			break;
		case B_OP_COPY:
			return new agg::DrawingModeBGRA32Copy();
			break;
		case B_OP_OVER:
			return new agg::DrawingModeBGRA32Over();
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

		default:
			return new agg::DrawingModeBGRA32Copy();
	}
}

