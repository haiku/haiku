/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:  DarkWyrm <bpmagic@columbus.rr.com>
 *			Adi Oanca <adioanca@gmail.com>
 *			Stephan Aßmus <superstippi@gmx.de>
 *			Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WINDOW_LAYER_H
#define WINDOW_LAYER_H


#include "Decorator.h"
#include "ViewLayer.h"
#include "ServerWindow.h"
#include "WindowList.h"

#include <ObjectList.h>
#include <Region.h>
#include <String.h>


class ClientLooper;
class Decorator;
class Desktop;
class DrawingEngine;
class EventDispatcher;
class WindowLayer;

// TODO: move this into a proper place
#define AS_REDRAW 'rdrw'

class WindowLayer {
 public:
								WindowLayer(const BRect& frame,
											const char *name, window_look look,
											window_feel feel, uint32 flags,
											uint32 workspaces,
											::ServerWindow* window,
											DrawingEngine* drawingEngine);
	virtual						~WindowLayer();

			BRect				Frame() const { return fFrame; }
			const char*			Title() const { return fTitle.String(); }

			window_anchor&		Anchor(int32 index);
			WindowLayer*		NextWindow(int32 index);
			WindowLayer*		PreviousWindow(int32 index);

			::Desktop*			Desktop() const { return fDesktop; }
			::Decorator*		Decorator() const { return fDecorator; }
			::ServerWindow*		ServerWindow() const { return fWindow; }
			::EventTarget&		EventTarget() const { return fWindow->EventTarget(); }

			// for convenience, handles window not attached to Desktop
			bool				ReadLockWindows();
			void				ReadUnlockWindows();

			// setting and getting the "hard" clipping, you need to have
			// WriteLock()ed the clipping!
			void				SetClipping(BRegion* stillAvailableOnScreen);
			// you need to have ReadLock()ed the clipping!
	inline	BRegion&			VisibleRegion() { return fVisibleRegion; }
			BRegion&			VisibleContentRegion();

			// TODO: not protected by a lock, but noone should need this anyways
			// make private? when used inside WindowLayer, it has the ReadLock()
			void				GetFullRegion(BRegion* region);
			void				GetBorderRegion(BRegion* region);
			void				GetContentRegion(BRegion* region);

			void				MoveBy(int32 x, int32 y);
			void				ResizeBy(int32 x, int32 y, BRegion* dirtyRegion);

			void				ScrollViewBy(ViewLayer* view, int32 dx, int32 dy);

			void				SetTopLayer(ViewLayer* topLayer);
			ViewLayer*			TopLayer() const { return fTopLayer; }
				// TODO: only used for WorkspacesLayer, can go away if we do
				//	this differently one day

			ViewLayer*			ViewAt(const BPoint& where);

	virtual	bool				IsOffscreenWindow() const { return false; }

			void				GetEffectiveDrawingRegion(ViewLayer* layer, BRegion& region);
			bool				DrawingRegionChanged(ViewLayer* layer) const;

			// generic version, used by the Desktop
			void				ProcessDirtyRegion(BRegion& regionOnScreen);
			void				RedrawDirtyRegion();

			// can be used from inside classes that don't
			// need to know about Desktop (first version uses Desktop)
			void				MarkDirty(BRegion& regionOnScreen);
			// this version does not use the Desktop
			void				MarkContentDirty(BRegion& regionOnScreen);
			// shortcut for invalidating just one view
			void				InvalidateView(ViewLayer* view, BRegion& layerRegion);

			void				EnableUpdateRequests();
			void				DisableUpdateRequests();

			void				BeginUpdate();
			void				EndUpdate();
			bool				InUpdate() const
									{ return fInUpdate; }

			bool				NeedsUpdate() const
									{ return fUpdateRequested; }

			DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }

			void				CopyContents(BRegion* region,
											 int32 xOffset, int32 yOffset);

			void				MouseDown(BMessage* message, BPoint where, int32* _viewToken);
			void				MouseUp(BMessage* message, BPoint where, int32* _viewToken);
			void				MouseMoved(BMessage* message, BPoint where, int32* _viewToken,
									bool isLatestMouseMoved);

			// some hooks to inform the client window
			// TODO: move this to ServerWindow maybe?
			void				WorkspaceActivated(int32 index, bool active);
			void				WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces);
			void				Activated(bool active);

			// changing some properties
			void				SetTitle(const char* name, BRegion& dirty);

			void				SetFocus(bool focus);
			bool				IsFocus() const { return fIsFocus; }

			void				SetHidden(bool hidden);
	inline	bool				IsHidden() const { return fHidden; }

			void				SetCurrentWorkspace(int32 index)
									{ fCurrentWorkspace = index; }
			bool				IsVisible() const;

			// TODO: make this int32 stuff
			void				SetSizeLimits(int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);

			void				GetSizeLimits(int32* minWidth, int32* maxWidth,
									int32* minHeight, int32* maxHeight) const;

								// 0.0 -> left .... 1.0 -> right
			void				SetTabLocation(float location);
			float				TabLocation() const;

			void				HighlightDecorator(bool active);

			void				SetLook(window_look look, BRegion* updateRegion);
			void				SetFeel(window_feel feel);
			void				SetFlags(uint32 flags, BRegion* updateRegion);

			window_look			Look() const { return fLook; }
			window_feel			Feel() const { return fFeel; }
			uint32				Flags() const { return fFlags; }

			// window manager stuff
			uint32				Workspaces() const { return fWorkspaces; }
			void				SetWorkspaces(uint32 workspaces)
									{ fWorkspaces = workspaces; }
			bool				InWorkspace(int32 index) const;

			bool				SupportsFront();

			bool				IsModal() const;
			bool				IsFloating() const;
			bool				IsNormal() const;

			WindowLayer*		Frontmost(WindowLayer* first = NULL, int32 workspace = -1);
			WindowLayer*		Backmost(WindowLayer* first = NULL, int32 workspace = -1);

			bool				AddToSubset(WindowLayer* window);
			void				RemoveFromSubset(WindowLayer* window);
			bool				HasInSubset(WindowLayer* window);
			bool				SameSubset(WindowLayer* window);
			uint32				SubsetWorkspaces() const;

	static bool					IsValidLook(window_look look);
	static bool					IsValidFeel(window_feel feel);
	static bool					IsModalFeel(window_feel feel);
	static bool					IsFloatingFeel(window_feel feel);

	static uint32				ValidWindowFlags();
	static uint32				ValidWindowFlags(window_feel feel);

 protected:
 	friend class Desktop;
 		// TODO: for now (list management)

			void				_ShiftPartOfRegion(BRegion* region, BRegion* regionToShift,
												   int32 xOffset, int32 yOffset);

			// different types of drawing
			void				_TriggerContentRedraw(BRegion& dirty);
			void				_DrawBorder();

			// handling update sessions
			void				_TransferToUpdateSession(BRegion* contentDirtyRegion);
			void				_SendUpdateMessage();

			void				_UpdateContentRegion();

			click_type			_ActionFor(const BMessage* msg) const;

			void				_ObeySizeLimits();
			void				_PropagatePosition();

			BString				fTitle;	
			// TODO: no fp rects anywhere
			BRect				fFrame;

			window_anchor		fAnchor[kListCount];

			// the visible region is only recalculated from the
			// Desktop thread, when using it, Desktop::ReadLockClipping()
			// has to be called
	
			BRegion				fVisibleRegion;
			BRegion				fVisibleContentRegion;
			bool				fVisibleContentRegionValid;
			// our part of the "global" dirty region
			// it is calculated from the desktop thread,
			// but we can write to it when we read locked
			// the clipping, since it is local and the desktop
			// thread is blocked
			BRegion				fDirtyRegion;

			// caching local regions
			BRegion				fBorderRegion;
			bool				fBorderRegionValid;
			BRegion				fContentRegion;
			bool				fContentRegionValid;
			BRegion				fEffectiveDrawingRegion;
			bool				fEffectiveDrawingRegionValid;

			BObjectList<WindowLayer> fSubsets;

// TODO: remove those some day (let the decorator handle that stuff)
			bool				fIsClosing;
			bool				fIsMinimizing;
			bool				fIsZooming;
			bool				fIsResizing;
			bool				fIsSlidingTab;
			bool				fIsDragging;

			::Decorator*		fDecorator;
			ViewLayer*			fTopLayer;
			::ServerWindow*		fWindow;
			DrawingEngine*		fDrawingEngine;
			::Desktop*			fDesktop;

			BPoint				fLastMousePosition;

			// The synchronization, which client drawing commands
			// belong to the redraw of which dirty region is handled
			// through an UpdateSession. When the client has
			// been informed that it should redraw stuff, then
			// this is the current update session. All new
			// redraw requests from the Desktop will go
			// into the pending update session.
	class UpdateSession {
	 public:
									UpdateSession();
		virtual						~UpdateSession();
	
				void				Include(BRegion* additionalDirty);
				void				Exclude(BRegion* dirtyInNextSession);
	
		inline	BRegion&			DirtyRegion()
										{ return fDirtyRegion; }
	
				void				MoveBy(int32 x, int32 y);
	
				void				SetUsed(bool used);
		inline	bool				IsUsed() const
										{ return fInUse; }
	
				UpdateSession&		operator=(const UpdateSession& other);
	
	 private:
				BRegion				fDirtyRegion;
				bool				fInUse;
	};

			BRegion				fDecoratorRegion;

			UpdateSession		fCurrentUpdateSession;
			UpdateSession		fPendingUpdateSession;
			// these two flags are supposed to ensure a sane
			// and consistent update session
			bool				fUpdateRequested;
			bool				fInUpdate;

			bool				fHidden;
			bool				fIsFocus;

			window_look			fLook;
			window_feel			fFeel;
			uint32				fOriginalFlags;
			uint32				fFlags;
			uint32				fWorkspaces;
			int32				fCurrentWorkspace;

			int32				fMinWidth;
			int32				fMaxWidth;
			int32				fMinHeight;
			int32				fMaxHeight;

	mutable	bool				fReadLocked;
};

#endif // WINDOW_LAYER_H
