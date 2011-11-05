/*
 * Copyright 2001-2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Brecht Machiels <brecht@mos6581.org>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef WINDOW_H
#define WINDOW_H


#include "RegionPool.h"
#include "ServerWindow.h"
#include "View.h"
#include "WindowList.h"

#include <ObjectList.h>
#include <Referenceable.h>
#include <Region.h>
#include <String.h>


class Window;


typedef	BObjectList<Window>	StackWindows;


class WindowStack : public BReferenceable {
public:
								WindowStack(::Decorator* decorator);
								~WindowStack();

			void				SetDecorator(::Decorator* decorator);
			::Decorator*		Decorator();

	const	StackWindows&		WindowList() const { return fWindowList; }
	const	StackWindows&		LayerOrder() const { return fWindowLayerOrder; }

			Window*				TopLayerWindow() const;

			int32				CountWindows();
			Window*				WindowAt(int32 index);
			bool				AddWindow(Window* window,
									int32 position = -1);
			bool				RemoveWindow(Window* window);

			bool				MoveToTopLayer(Window* window);
			bool				Move(int32 from, int32 to);
private:
			::Decorator*		fDecorator;

			StackWindows		fWindowList;
			StackWindows		fWindowLayerOrder;
};


namespace BPrivate {
	class PortLink;
};

class ClickTarget;
class ClientLooper;
class Decorator;
class Desktop;
class DrawingEngine;
class EventDispatcher;
class Screen;
class WindowBehaviour;
class WorkspacesView;

// TODO: move this into a proper place
#define AS_REDRAW 'rdrw'

enum {
	UPDATE_REQUEST		= 0x01,
	UPDATE_EXPOSE		= 0x02,
};


class Window {
public:
								Window(const BRect& frame, const char *name,
									window_look look, window_feel feel,
									uint32 flags, uint32 workspaces,
									::ServerWindow* window,
									DrawingEngine* drawingEngine);
	virtual						~Window();

			status_t			InitCheck() const;

			BRect				Frame() const { return fFrame; }
			const char*			Title() const { return fTitle.String(); }

			window_anchor&		Anchor(int32 index);
			Window*				NextWindow(int32 index) const;
			Window*				PreviousWindow(int32 index) const;

			::Desktop*			Desktop() const { return fDesktop; }
			::Decorator*		Decorator() const;
			::ServerWindow*		ServerWindow() const { return fWindow; }
			::EventTarget&		EventTarget() const
									{ return fWindow->EventTarget(); }

			bool				ReloadDecor();

			void				SetScreen(const ::Screen* screen);
			const ::Screen*		Screen() const;

			// setting and getting the "hard" clipping, you need to have
			// WriteLock()ed the clipping!
			void				SetClipping(BRegion* stillAvailableOnScreen);
			// you need to have ReadLock()ed the clipping!
	inline	BRegion&			VisibleRegion() { return fVisibleRegion; }
			BRegion&			VisibleContentRegion();

			// TODO: not protected by a lock, but noone should need this anyways
			// make private? when used inside Window, it has the ReadLock()
			void				GetFullRegion(BRegion* region);
			void				GetBorderRegion(BRegion* region);
			void				GetContentRegion(BRegion* region);

			void				MoveBy(int32 x, int32 y, bool moveStack = true);
			void				ResizeBy(int32 x, int32 y,
									BRegion* dirtyRegion,
									bool resizeStack = true);

			void				ScrollViewBy(View* view, int32 dx, int32 dy);

			void				SetTopView(View* topView);
			View*				TopView() const { return fTopView; }
			View*				ViewAt(const BPoint& where);

	virtual	bool				IsOffscreenWindow() const { return false; }

			void				GetEffectiveDrawingRegion(View* view,
									BRegion& region);
			bool				DrawingRegionChanged(View* view) const;

			// generic version, used by the Desktop
			void				ProcessDirtyRegion(BRegion& regionOnScreen);
			void				RedrawDirtyRegion();

			// can be used from inside classes that don't
			// need to know about Desktop (first version uses Desktop)
			void				MarkDirty(BRegion& regionOnScreen);
			// these versions do not use the Desktop
			void				MarkContentDirty(BRegion& regionOnScreen);
			void				MarkContentDirtyAsync(BRegion& regionOnScreen);
			// shortcut for invalidating just one view
			void				InvalidateView(View* view, BRegion& viewRegion);

			void				DisableUpdateRequests();
			void				EnableUpdateRequests();

			void				BeginUpdate(BPrivate::PortLink& link);
			void				EndUpdate();
			bool				InUpdate() const
									{ return fInUpdate; }

			bool				NeedsUpdate() const
									{ return fUpdateRequested; }

			DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }

			// managing a region pool
			::RegionPool*		RegionPool()
									{ return &fRegionPool; }
	inline	BRegion*			GetRegion()
									{ return fRegionPool.GetRegion(); }
	inline	BRegion*			GetRegion(const BRegion& copy)
									{ return fRegionPool.GetRegion(copy); }
	inline	void				RecycleRegion(BRegion* region)
									{ fRegionPool.Recycle(region); }

			void				CopyContents(BRegion* region,
									int32 xOffset, int32 yOffset);

			void				MouseDown(BMessage* message, BPoint where,
									const ClickTarget& lastClickTarget,
									int32& clickCount,
									ClickTarget& _clickTarget);
			void				MouseUp(BMessage* message, BPoint where,
									int32* _viewToken);
			void				MouseMoved(BMessage* message, BPoint where,
									int32* _viewToken, bool isLatestMouseMoved,
									bool isFake);

			void				ModifiersChanged(int32 modifiers);

			// some hooks to inform the client window
			// TODO: move this to ServerWindow maybe?
			void				WorkspaceActivated(int32 index, bool active);
			void				WorkspacesChanged(uint32 oldWorkspaces,
									uint32 newWorkspaces);
			void				Activated(bool active);

			// changing some properties
			void				SetTitle(const char* name, BRegion& dirty);

			void				SetFocus(bool focus);
			bool				IsFocus() const { return fIsFocus; }

			void				SetHidden(bool hidden);
	inline	bool				IsHidden() const { return fHidden; }

			void				SetMinimized(bool minimized);
	inline	bool				IsMinimized() const { return fMinimized; }

			void				SetCurrentWorkspace(int32 index)
									{ fCurrentWorkspace = index; }
			int32				CurrentWorkspace() const
									{ return fCurrentWorkspace; }
			bool				IsVisible() const;

			bool				IsDragging() const;
			bool				IsResizing() const;

			void				SetSizeLimits(int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);

			void				GetSizeLimits(int32* minWidth, int32* maxWidth,
									int32* minHeight, int32* maxHeight) const;

								// 0.0 -> left .... 1.0 -> right
			bool				SetTabLocation(float location, bool isShifting,
									BRegion& dirty);
			float				TabLocation() const;

			bool				SetDecoratorSettings(const BMessage& settings,
													 BRegion& dirty);
			bool				GetDecoratorSettings(BMessage* settings);

			void				HighlightDecorator(bool active);

			void				FontsChanged(BRegion* updateRegion);

			void				SetLook(window_look look,
									BRegion* updateRegion);
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

			bool				HasModal() const;

			Window*				Frontmost(Window* first = NULL,
									int32 workspace = -1);
			Window*				Backmost(Window* first = NULL,
									int32 workspace = -1);

			bool				AddToSubset(Window* window);
			void				RemoveFromSubset(Window* window);
			bool				HasInSubset(const Window* window) const;
			bool				SameSubset(Window* window);
			uint32				SubsetWorkspaces() const;
			bool				InSubsetWorkspace(int32 index) const;

			bool				HasWorkspacesViews() const
									{ return fWorkspacesViewCount != 0; }
			void				AddWorkspacesView()
									{ fWorkspacesViewCount++; }
			void				RemoveWorkspacesView()
									{ fWorkspacesViewCount--; }
			void				FindWorkspacesViews(
									BObjectList<WorkspacesView>& list) const;

	static	bool				IsValidLook(window_look look);
	static	bool				IsValidFeel(window_feel feel);
	static	bool				IsModalFeel(window_feel feel);
	static	bool				IsFloatingFeel(window_feel feel);

	static	uint32				ValidWindowFlags();
	static	uint32				ValidWindowFlags(window_feel feel);

			// Window stack methods.
			WindowStack*		GetWindowStack();

			bool				DetachFromWindowStack(
									bool ownStackNeeded = true);
			bool				AddWindowToStack(Window* window);
			Window*				StackedWindowAt(const BPoint& where);
			Window*				TopLayerStackWindow();

			int32				PositionInStack() const;
			bool				MoveToTopStackLayer();
			bool				MoveToStackPosition(int32 index,
									bool isMoving);
protected:
			void				_ShiftPartOfRegion(BRegion* region,
									BRegion* regionToShift, int32 xOffset,
									int32 yOffset);

			// different types of drawing
			void				_TriggerContentRedraw(BRegion& dirty);
			void				_DrawBorder();

			// handling update sessions
			void				_TransferToUpdateSession(
									BRegion* contentDirtyRegion);
			void				_SendUpdateMessage();

			void				_UpdateContentRegion();

			void				_ObeySizeLimits();
			void				_PropagatePosition();

			BString				fTitle;
			// TODO: no fp rects anywhere
			BRect				fFrame;
			const ::Screen*		fScreen;

			window_anchor		fAnchor[kListCount];

			// the visible region is only recalculated from the
			// Desktop thread, when using it, Desktop::ReadLockClipping()
			// has to be called

			BRegion				fVisibleRegion;
			BRegion				fVisibleContentRegion;
			// our part of the "global" dirty region
			// it is calculated from the desktop thread,
			// but we can write to it when we read locked
			// the clipping, since it is local and the desktop
			// thread is blocked
			BRegion				fDirtyRegion;
			uint32				fDirtyCause;

			// caching local regions
			BRegion				fContentRegion;
			BRegion				fEffectiveDrawingRegion;

			bool				fVisibleContentRegionValid : 1;
			bool				fContentRegionValid : 1;
			bool				fEffectiveDrawingRegionValid : 1;

			::RegionPool		fRegionPool;

			BObjectList<Window> fSubsets;

			WindowBehaviour*	fWindowBehaviour;
			View*				fTopView;
			::ServerWindow*		fWindow;
			DrawingEngine*		fDrawingEngine;
			::Desktop*			fDesktop;

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

				void				AddCause(uint8 cause);
		inline	bool				IsExpose() const
										{ return fCause & UPDATE_EXPOSE; }
		inline	bool				IsRequest() const
										{ return fCause & UPDATE_REQUEST; }

	private:
				BRegion				fDirtyRegion;
				bool				fInUse;
				uint8				fCause;
	};

			UpdateSession		fUpdateSessions[2];
			UpdateSession*		fCurrentUpdateSession;
			UpdateSession*		fPendingUpdateSession;
			// these two flags are supposed to ensure a sane
			// and consistent update session
			bool				fUpdateRequested : 1;
			bool				fInUpdate : 1;
			bool				fUpdatesEnabled : 1;

			bool				fHidden : 1;
			bool				fMinimized : 1;
			bool				fIsFocus : 1;

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

			int32				fWorkspacesViewCount;

		friend class DecorManager;

private:
			WindowStack*		_InitWindowStack();

			BReference<WindowStack>		fCurrentStack;
};


#endif // WINDOW_H
