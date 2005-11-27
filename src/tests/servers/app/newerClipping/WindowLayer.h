
#ifndef WINDOW_LAYER_H
#define WINDOW_LAYER_H

#include <List.h>
#include <Looper.h>
#include <Region.h>
#include <String.h>

#include "ViewLayer.h"

class ClientLooper;
class Desktop;
class DrawingEngine;

enum {
	MSG_REDRAW			= 'rdrw',

	MSG_BEGIN_UPDATE	= 'bgud',
	MSG_END_UPDATE		= 'edud',
	MSG_DRAWING_COMMAND	= 'draw',
};

class UpdateSession {
 public:
									UpdateSession(const BRegion& dirtyRegion);
	virtual							~UpdateSession();

			void					Include(BRegion* additionalDirty);
			void					Exclude(BRegion* dirtyInNextSession);

	inline	BRegion&				DirtyRegion()
										{ return fDirtyRegion; }

			void					MoveBy(int32 x, int32 y);

 private:
			BRegion					fDirtyRegion;
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
			void					GetBorderRegion(BRegion* region);
			void					GetContentRegion(BRegion* region);

			void					SetFocus(bool focus);

			void					MoveBy(int32 x, int32 y);
			void					ResizeBy(int32 x, int32 y, BRegion* dirtyRegion);

			void					AddChild(ViewLayer* layer);

	inline	bool					IsHidden() const
										{ return fHidden; }
			void					Hide();
			void					Show();

			void					MarkDirty(BRegion* regionOnScreen);
			void					MarkContentDirty(BRegion* regionOnScreen);

			DrawingEngine*			GetDrawingEngine() const
										{ return fDrawingEngine; }

//			BRegion					DirtyRegion();

 private:
			void					_DrawContents(ViewLayer* layer = NULL);
			void					_DrawClient(int32 token);
			void					_DrawBorder();

			void					_MarkContentDirty(BRegion* contentDirtyRegion);
			void					_BeginUpdate();
			void					_EndUpdate();


			BRect					fFrame;
			// the visible region is only recalculated from the
			// Desktop thread, when using it, Desktop::LockClipping()
			// has to be called
			BRegion					fVisibleRegion;
			BRegion					fVisibleContentRegion;

			// caching local regions
			BRegion					fBorderRegion;
			bool					fBorderRegionValid;
			BRegion					fContentRegion;
			bool					fContentRegionValid;
			BRegion					fEffectiveDrawingRegion;
			bool					fEffectiveDrawingRegionValid;

			bool					fFocus;

			ViewLayer*				fTopLayer;

			bool					fHidden;

			DrawingEngine*			fDrawingEngine;
			Desktop*				fDesktop;

			BList					fTokenViewMap;

			ClientLooper*			fClient;
			UpdateSession*			fCurrentUpdateSession;
			UpdateSession*			fPendingUpdateSession;
			bool					fUpdateRequested;
			bool					fInUpdate;
};

#endif // WINDOW_LAYER_H
