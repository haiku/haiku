// DrawingModeFactory.h

#ifndef DRAWING_MODE_FACTORY_H
#define DRAWING_MODE_FACTORY_H

#include <GraphicsDefs.h>

#include "DrawingMode.h"

class DrawingModeFactory {
 public:
								DrawingModeFactory() {}
	virtual						~DrawingModeFactory() {}

	static	agg::DrawingMode*		DrawingModeFor(drawing_mode mode);
};

#endif // DRAWING_MODE_FACTORY_H

