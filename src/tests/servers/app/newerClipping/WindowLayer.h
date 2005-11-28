
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

			void					CopyContents(BRegion* region,
												 int32 xOffset, int32 yOffset);

 private:
			void					_ShiftPartOfRegion(BRegion* region, BRegion* regionToShift,
													   int32 xOffset, int32 yOffset);

			// different types of drawing
			void					_TriggerContentRedraw();
			void					_DrawClient(int32 token);
			void					_DrawBorder();

			// handling update sessions
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

			// for mapping drawing requests from the client
			// to the local view pointers
			BList					fTokenViewMap;

			// the client looper, which will do asynchronous
			// drawing (the window will clear the content
			// and the client is supposed to draw onto
			// the cleared regions)
			ClientLooper*			fClient;
			// The synchronization, which client drawing commands
			// belong to the redraw of which region is handled
			// through an UpdateSession. When the client has
			// been informed that it should redraw stuff, then
			// this is the current update session. All new
			// redraw requests from the root layer will go
			// into the pending update session.
			UpdateSession*			fCurrentUpdateSession;
			UpdateSession*			fPendingUpdateSession;
			// these two flags are supposed to ensure a sane
			// and consistent update session
			bool					fUpdateRequested;
			bool					fInUpdate;
};

#endif // WINDOW_LAYER_H
