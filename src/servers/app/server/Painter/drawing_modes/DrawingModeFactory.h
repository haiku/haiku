// DrawingModeFactory.h

#ifndef DRAWING_MODE_FACTORY_H
#define DRAWING_MODE_FACTORY_H

#include <GraphicsDefs.h>

#include "DrawingMode.h"

class DrawingModeFactory {
 public:
								DrawingModeFactory() {}
	virtual						~DrawingModeFactory() {}

	static	DrawingMode*		DrawingModeFor(drawing_mode mode,
											   source_alpha alphaSrcMode,
											   alpha_function alphaFncMode,
											   bool solid = false);
};

#endif // DRAWING_MODE_FACTORY_H

