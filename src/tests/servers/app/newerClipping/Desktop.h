
#ifndef DESKTOP_H
#define DESKTOP_H

#include <List.h>
#include <Locker.h>
#include <Region.h>
#include <View.h>
#include <Window.h>

#include "DrawingEngine.h"
#include "MultiLocker.h"

class WindowLayer;

enum {
	MSG_ADD_WINDOW		= 'addw',
	MSG_DRAW			= 'draw',
};

#define MULTI_LOCKER 0
#define SHOW_GLOBAL_DIRTY_REGION 0
#define SHOW_WINDOW_CONTENT_DIRTY_REGION 0

class Desktop : public BLooper {
 public:
								Desktop(DrawView* drawView);
	virtual						~Desktop();

			// functions for the DrawView
			void				Draw(BRect updateRect);

			void				MouseDown(BPoint where, uint32 buttons); 
			void				MouseUp(BPoint where); 
			void				MouseMoved(BPoint where, uint32 code,
										   const BMessage* dragMessage);

	virtual	void				MessageReceived(BMessage* message);

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

			void				BringToFront(WindowLayer* window);
			void				SendToBack(WindowLayer* window);

			void				SetFocusWindow(WindowLayer* window);

#if MULTI_LOCKER
#  if 0
			bool				ReadLockClipping() { return fClippingLock.ReadLock(); }
			void				ReadUnlockClipping() { fClippingLock.ReadUnlock(); }
#  else
			bool				ReadLockClipping() { return fClippingLock.WriteLock(); }
			void				ReadUnlockClipping() { fClippingLock.WriteUnlock(); }
#  endif
#else
			bool				ReadLockClipping() { return fClippingLock.Lock(); }
			bool				ReadLockClippingWithTimeout() { return fClippingLock.LockWithTimeout(10000) >= B_OK; }
			void				ReadUnlockClipping() { fClippingLock.Unlock(); }
#endif

			bool				LockClipping() { return fClippingLock.Lock(); }
			void				UnlockClipping() { fClippingLock.Unlock(); }

			void				MarkDirty(BRegion* region);
			void				MarkClean(BRegion* region);
			BRegion*			DirtyRegion()
									{ return &fDirtyRegion; }

			DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }

			BRegion&			BackgroundRegion()
									{ return fBackgroundRegion; }

private:
			void				_RebuildClippingForAllWindows(BRegion* stillAvailableOnScreen);
			void				_TriggerWindowRedrawing(BRegion* newDirtyRegion);
			void				_SetBackground(BRegion* background);

			bool				fTracking;
			BPoint				fLastMousePos;
			WindowLayer*		fClickedWindow;
			bool				fResizing;
			bigtime_t			fClickTime;
			bool				fIs2ndButton;

#if MULTI_LOCKER
			MultiLocker			fClippingLock;
#else
			BLocker				fClippingLock;
#endif
			BRegion				fDirtyRegion;
			BRegion				fBackgroundRegion;

			DrawView*			fDrawView;
			DrawingEngine*		fDrawingEngine;

			BList				fWindows;

			bool				fFocusFollowsMouse;
			WindowLayer*		fFocusWindow;
};

#endif // DESKTOP_H

