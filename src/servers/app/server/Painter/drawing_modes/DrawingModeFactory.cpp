// DrawingModeFactory.h


#include "DrawingModeAdd.h"
#include "DrawingModeCopy.h"
#include "DrawingModeInvert.h"
#include "DrawingModeOver.h"

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

		default:
			return new agg::DrawingModeBGRA32Copy();
	}
}

