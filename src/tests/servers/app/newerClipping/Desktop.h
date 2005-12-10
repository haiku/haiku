
#ifndef DESKTOP_H
#define DESKTOP_H

#include <List.h>
#include <Region.h>
#include <View.h>
#include <Window.h>

#define SHOW_GLOBAL_DIRTY_REGION 0
#define SHOW_WINDOW_CONTENT_DIRTY_REGION 0

#define MULTI_LOCKER 1

#if MULTI_LOCKER
#  include "MultiLocker.h"
#else
#  include <Locker.h>
#endif

class DrawingEngine;
class DrawView;
class WindowLayer;
class ViewLayer;

enum {
	MSG_ADD_WINDOW		= 'addw',
	MSG_DRAW			= 'draw',

	MSG_MARK_CLEAN		= 'mcln',

	MSG_QUIT			= 'quit',
};

class Desktop : public BLooper {
 public:
								Desktop(DrawView* drawView,
										DrawingEngine* engine);
	virtual						~Desktop();

			// functions for the DrawView
			void				MouseDown(BPoint where, uint32 buttons,
										  int32 clicks); 
			void				MouseUp(BPoint where); 
			void				MouseMoved(BPoint where, uint32 code,
										   const BMessage* dragMessage);

	virtual	void				MessageReceived(BMessage* message);

			void				SetMasterClipping(BRegion* clipping);
			void				SetOffset(int32 x, int32 y);

			bool				AddWindow(WindowLayer* window);
			bool				RemoveWindow(WindowLayer* window);
			int32				IndexOf(WindowLayer* window) const;
			int32				CountWindows() const;
			bool				HasWindow(WindowLayer* window) const;

			WindowLayer*		WindowAt(int32 index) const;
			WindowLayer*		WindowAtFast(int32 index) const;
			WindowLayer*		WindowAt(const BPoint& where) const;
			WindowLayer*		TopWindow() const;
			WindowLayer*		BottomWindow() const;

			// doing something with the windows
			void				MoveWindowBy(WindowLayer* window, int32 x, int32 y);
			void				ResizeWindowBy(WindowLayer* window, int32 x, int32 y);

			void				ShowWindow(WindowLayer* window);
			void				HideWindow(WindowLayer* window);
			void				SetWindowHidden(WindowLayer* window, bool hidden);

			void				BringToFront(WindowLayer* window);
			void				SendToBack(WindowLayer* window);

			void				SetFocusWindow(WindowLayer* window);

#if MULTI_LOCKER
			bool				ReadLockClipping() { return fClippingLock.ReadLock(); }
			void				ReadUnlockClipping() { fClippingLock.ReadUnlock(); }

			bool				LockClipping() { return fClippingLock.WriteLock(); }
			void				UnlockClipping() { fClippingLock.WriteUnlock(); }
#else // BLocker
			bool				ReadLockClipping() { return fClippingLock.Lock(); }
			void				ReadUnlockClipping() { fClippingLock.Unlock(); }

			bool				LockClipping() { return fClippingLock.Lock(); }
			void				UnlockClipping() { fClippingLock.Unlock(); }
#endif

			void				MarkDirty(BRegion* region);

			DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }

			BRegion&			BackgroundRegion()
									{ return fBackgroundRegion; }

			void				WindowDied(WindowLayer* window);

private:
			void				_RebuildClippingForAllWindows(BRegion* stillAvailableOnScreen);
			void				_TriggerWindowRedrawing(BRegion* newDirtyRegion);
			void				_SetBackground(BRegion* background);

			bool				fTracking;
			BPoint				fLastMousePos;
			WindowLayer*		fClickedWindow;
			ViewLayer*			fScrollingView;
			bool				fResizing;
			bigtime_t			fClickTime;
			bool				fIs2ndButton;

#if MULTI_LOCKER
			MultiLocker			fClippingLock;
#else
			BLocker				fClippingLock;
#endif
			BRegion				fBackgroundRegion;

			BRegion				fMasterClipping;
			int32				fXOffset;
			int32				fYOffset;

			DrawView*			fDrawView;
			DrawingEngine*		fDrawingEngine;

			BList				fWindows;

			bool				fFocusFollowsMouse;
			WindowLayer*		fFocusWindow;
};

#endif // DESKTOP_H

