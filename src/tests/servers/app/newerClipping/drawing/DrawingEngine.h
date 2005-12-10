
#ifndef DRAWING_ENGINE_H
#define DRAWING_ENGINE_H

#include <GraphicsDefs.h>
#include <Region.h>

class AccelerantHWInterface;
class DirectWindowBuffer;

class DrawingEngine {
 public:
								DrawingEngine(AccelerantHWInterface* interface,
											  DirectWindowBuffer* buffer);
	virtual						~DrawingEngine();

			bool				Lock();
			void				Unlock();

			void				ConstrainClipping(BRegion* region);

			bool				StraightLine(BPoint a, BPoint b, const rgb_color& c);
			void				StrokeLine(BPoint a, BPoint b, const rgb_color& color);
			void				StrokeRect(BRect r, const rgb_color& color);

			void				FillRegion(BRegion *region, const rgb_color& color);

			void				DrawString(const char* string, BPoint baseLine,
										   const rgb_color& color);

			void				CopyRegion(BRegion *region, int32 xOffset, int32 yOffset);

 private:
	AccelerantHWInterface*		fHWInterface;
	DirectWindowBuffer*			fBuffer;
	BRegion						fCurrentClipping;
};


#endif // DRAWING_ENGINE_H

