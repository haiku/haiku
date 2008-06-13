#ifndef BITMAP_DRAWING_ENGINE_H
#define BITMAP_DRAWING_ENGINE_H

#include "DrawingEngine.h"
#include <Region.h>

class BitmapHWInterface;
class UtilityBitmap;

class BitmapDrawingEngine : public DrawingEngine {
public:
								BitmapDrawingEngine();
virtual							~BitmapDrawingEngine();

			status_t			SetSize(int32 newWidth, int32 newHeight);
			UtilityBitmap *		ExportToBitmap(int32 width, int32 height,
									color_space space);

private:
			BitmapHWInterface *	fHWInterface;
			UtilityBitmap *		fBitmap;
			BRegion				fClipping;
};

#endif // BITMAP_DRAWING_ENGINE_H
