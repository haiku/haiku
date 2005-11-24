
#ifndef WINDOW_LAYER_H
#define WINDOW_LAYER_H

#include <Looper.h>
#include <Region.h>
#include <String.h>

#include "ViewLayer.h"

class Desktop;
class DrawingEngine;

enum {
	MSG_REDRAW		= 'rdrw',
};

class WindowLayer : public BLooper {
 public:
									WindowLayer(BRect frame, const char* name,
												DrawingEngine* drawingEngine,
												Desktop* desktop);
	virtual							~WindowLayer();

	virtual	void					MessageReceived(BMessage* message);

	inline	BRect					Frame() const
										{ return fFrame; }
			void					SetClipping(BRegion* stillAvailableOnScreen);
	inline	BRegion&				VisibleRegion()
										{ return fVisibleRegion; }
			void					GetFullRegion(BRegion* region) const;
			void					GetBorderRegion(BRegion* region) const;

			void					MoveBy(int32 x, int32 y);
			void					ResizeBy(int32 x, int32 y, BRegion* dirtyRegion);

			void					AddChild(ViewLayer* layer);

			void					MarkDirty(BRegion* regionOnScreen);

 private:
			void					_DrawContents(ViewLayer* layer = NULL);
			void					_DrawBorder();


			BRect					fFrame;
			// the visible region is only recalculated from the
			// Desktop thread, when using it, Desktop::LockClipping()
			// has to be called
			BRegion					fVisibleRegion;

			rgb_color				fBorderColor;

			ViewLayer*				fTopLayer;

			DrawingEngine*			fDrawingEngine;
			Desktop*				fDesktop;
};

#endif // WINDOW_LAYER_H
