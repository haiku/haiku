
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
	MSG_REDRAW				= 'rdrw',

	// client messages
	MSG_BEGIN_UPDATE		= 'bgud',
	MSG_END_UPDATE			= 'edud',
	MSG_DRAWING_COMMAND		= 'draw',

	MSG_SHOW				= 'show',

	MSG_INVALIDATE_VIEW		= 'invl',
	MSG_DRAW_POLYGON		= 'drwp',
};

class UpdateSession {
 public:
									UpdateSession();
	virtual							~UpdateSession();

			void					Include(BRegion* additionalDirty);
			void					Exclude(BRegion* dirtyInNextSession);

	inline	BRegion&				DirtyRegion()
										{ return fDirtyRegion; }

			void					MoveBy(int32 x, int32 y);

			void					SetUsed(bool used);
	inline	bool					IsUsed() const
										{ return fInUse; }

			UpdateSession&			operator=(const UpdateSession& other);

 private:
			BRegion					fDirtyRegion;
			bool					fInUse;
};

class WindowLayer : public BLooper {
 public:
									WindowLayer(BRect frame, const char* name,
												DrawingEngine* drawingEngine,
												Desktop* desktop);
	virtual							~WindowLayer();

	virtual	void					MessageReceived(BMessage* message);
	virtual	bool					QuitRequested();

	inline	BRect					Frame() const
										{ return fFrame; }
			// setting and getting the "hard" clipping
			void					SetClipping(BRegion* stillAvailableOnScreen);
	inline	BRegion&				VisibleRegion()
										{ return fVisibleRegion; }
			BRegion&				VisibleContentRegion();

			void					GetFullRegion(BRegion* region) const;
			void					GetBorderRegion(BRegion* region);
			void					GetContentRegion(BRegion* region);

			void					SetFocus(bool focus);

			void					MoveBy(int32 x, int32 y);
			void					ResizeBy(int32 x, int32 y, BRegion* dirtyRegion);

			void					ScrollViewBy(ViewLayer* view, int32 dx, int32 dy);

			void					AddChild(ViewLayer* layer);

			ViewLayer*				ViewAt(const BPoint& where);

			void					SetHidden(bool hidden);
	inline	bool					IsHidden() const
										{ return fHidden; }

			void					MarkDirty(BRegion* regionOnScreen);
			void					MarkContentDirty(BRegion* regionOnScreen);
			void					InvalidateView(int32 token);

			void					ProcessDirtyRegion(BRegion* region);

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
			void					_DrawClientPolygon(int32 token, BPoint polygon[4]);
			void					_DrawBorder();

			// handling update sessions
			void					_MarkContentDirty(BRegion* contentDirtyRegion);
			void					_BeginUpdate();
			void					_EndUpdate();

			void					_UpdateContentRegion();

			BRect					fFrame;

			// the visible region is only recalculated from the
			// Desktop thread, when using it, Desktop::ReadLockClipping()
			// has to be called
			BRegion					fVisibleRegion;
			BRegion					fVisibleContentRegion;
			bool					fVisibleContentRegionValid;
			// our part of the "global" dirty region
			// it is calculated from the desktop thread,
			// but we can write to it when we read locked
			// the clipping, since it is local and the desktop
			// thread is blocked
			BRegion					fDirtyRegion;

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
			UpdateSession			fCurrentUpdateSession;
			UpdateSession			fPendingUpdateSession;
			// these two flags are supposed to ensure a sane
			// and consistent update session
			bool					fUpdateRequested;
			bool					fInUpdate;
};

#endif // WINDOW_LAYER_H
