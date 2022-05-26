#ifndef BITMAP_DRAWING_ENGINE_H
#define BITMAP_DRAWING_ENGINE_H

#include "DrawingEngine.h"

#include <AutoDeleter.h>
#include <Referenceable.h>
#include <Region.h>

class BitmapHWInterface;
class UtilityBitmap;

class BitmapDrawingEngine : public DrawingEngine {
public:
								BitmapDrawingEngine(
									color_space colorSpace = B_RGB32);
virtual							~BitmapDrawingEngine();

#if DEBUG
	virtual	bool				IsParallelAccessLocked() const;
#endif
	virtual	bool				IsExclusiveAccessLocked() const;

			status_t			SetSize(int32 newWidth, int32 newHeight);
			UtilityBitmap*		ExportToBitmap(int32 width, int32 height,
									color_space space);

private:
			color_space			fColorSpace;
			ObjectDeleter<BitmapHWInterface>
								fHWInterface;
			BReference<UtilityBitmap>
								fBitmap;
			BRegion				fClipping;
};

#endif // BITMAP_DRAWING_ENGINE_H
