/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 */
#ifndef DESKTOP_H
#define DESKTOP_H


#include "CursorManager.h"
#include "DesktopSettings.h"
#include "EventDispatcher.h"
#include "MessageLooper.h"
#include "Screen.h"
#include "ScreenManager.h"
#include "ServerCursor.h"
#include "VirtualScreen.h"
#include "WindowList.h"
#include "Workspace.h"
#include "WorkspacePrivate.h"

#include <ObjectList.h>

#include <Autolock.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Menu.h>
#include <Region.h>
#include <Window.h>

#define USE_MULTI_LOCKER 1

#if USE_MULTI_LOCKER
#  include "MultiLocker.h"
#else
#  include <Locker.h>
#endif

class BMessage;

class DrawingEngine;
class HWInterface;
class ServerApp;
class Window;
class WorkspacesView;
struct server_read_only_memory;

namespace BPrivate {
	class LinkSender;
};

class Desktop : public MessageLooper, public ScreenOwner {
	public:
								Desktop(uid_t userID);
		virtual					~Desktop();

		status_t				Init();

		uid_t					UserID() const { return fUserID; }
		virtual port_id			MessagePort() const { return fMessagePort; }
		area_id					SharedReadOnlyArea() const
									{ return fSharedReadOnlyArea; }

		::EventDispatcher&		EventDispatcher() { return fEventDispatcher; }

		void					BroadcastToAllApps(int32 code);

		// Screen and drawing related methods

		Screen*					ScreenAt(int32 index) const
									{ return fActiveScreen; }
		Screen*					ActiveScreen() const
									{ return fActiveScreen; }

		CursorManager&			GetCursorManager() { return fCursorManager; }

		void					SetCursor(ServerCursor* cursor);
		ServerCursorReference	Cursor() const;
		void					SetLastMouseState(const BPoint& position,
									int32 buttons, Window* windowUnderMouse);
									// for use by the mouse filter only
									// both mouse position calls require
									// the Desktop object to be locked
									// already
		void					GetLastMouseState(BPoint* position,
									int32* buttons) const;
									// for use by ServerWindow

		void					ScreenChanged(Screen* screen, bool makeDefault);

		void					ScreenRemoved(Screen* screen) {}
		void					ScreenAdded(Screen* screen) {}
		bool					ReleaseScreen(Screen* screen) { return false; }

		const ::VirtualScreen&	VirtualScreen() const { return fVirtualScreen; }
		DrawingEngine*			GetDrawingEngine() const
									{ return fVirtualScreen.DrawingEngine(); }
		::HWInterface*			HWInterface() const
									{ return fVirtualScreen.HWInterface(); }

		// Workspace methods

		void					SetWorkspaceAsync(int32 index);
		void					SetWorkspace(int32 index);
		int32					CurrentWorkspace()
									{ return fCurrentWorkspace; }
		Workspace::Private&		WorkspaceAt(int32 index)
									{ return fWorkspaces[index]; }
		status_t				SetWorkspacesCount(int32 newCount);

		// Window methods

		void					ActivateWindow(Window* window);
		void					SendWindowBehind(Window* window,
									Window* behindOf = NULL);

		void					ShowWindow(Window* window);
		void					HideWindow(Window* window);

		void					MoveWindowBy(Window* window, float x, float y,
									int32 workspace = -1);
		void					ResizeWindowBy(Window* window, float x,
									float y);
		bool					SetWindowTabLocation(Window* window,
									float location);
		bool					SetWindowDecoratorSettings(Window* window,
									const BMessage& settings);

		void					SetWindowWorkspaces(Window* window,
									uint32 workspaces);

		void					AddWindow(Window* window);
		void					RemoveWindow(Window* window);

		bool					AddWindowToSubset(Window* subset,
									Window* window);
		void					RemoveWindowFromSubset(Window* subset,
									Window* window);

		void					SetWindowLook(Window* window, window_look look);
		void					SetWindowFeel(Window* window, window_feel feel);
		void					SetWindowFlags(Window* window, uint32 flags);
		void					SetWindowTitle(Window* window,
									const char* title);

		Window*					FocusWindow() const { return fFocus; }
		Window*					FrontWindow() const { return fFront; }
		Window*					BackWindow() const { return fBack; }

		Window*					WindowAt(BPoint where);

		Window*					MouseEventWindow() const
									{ return fMouseEventWindow; }
		void					SetMouseEventWindow(Window* window);

		void					SetViewUnderMouse(const Window* window,
									int32 viewToken);
		int32					ViewUnderMouse(const Window* window);

		void					SetFocusWindow(Window* window = NULL);
		EventTarget*			KeyboardEventTarget();
		void					SetFocusLocked(const Window* window);

		Window*					FindWindowByClientToken(int32 token,
									team_id teamID);
		EventTarget*			FindTarget(BMessenger& messenger);

#if USE_MULTI_LOCKER
		bool					LockSingleWindow()
									{ return fWindowLock.ReadLock(); }
		void					UnlockSingleWindow()
									{ fWindowLock.ReadUnlock(); }

		bool					LockAllWindows()
									{ return fWindowLock.WriteLock(); }
		void					UnlockAllWindows()
									{ fWindowLock.WriteUnlock(); }
#else // USE_MULTI_LOCKER
		bool					LockSingleWindow()
									{ return fWindowLock.Lock(); }
		void					UnlockSingleWindow()
									{ fWindowLock.Unlock(); }

		bool					LockAllWindows()
									{ return fWindowLock.Lock(); }
		void					UnlockAllWindows()
									{ fWindowLock.Unlock(); }
#endif // USE_MULTI_LOCKER

		void					MarkDirty(BRegion& region);
		void					Redraw();

		BRegion&				BackgroundRegion()
									{ return fBackgroundRegion; }

		void					RedrawBackground();
		void					StoreWorkspaceConfiguration(int32 index);

		void					AddWorkspacesView(WorkspacesView* view);
		void					RemoveWorkspacesView(WorkspacesView* view);

		void					MinimizeApplication(team_id team);
		void					BringApplicationToFront(team_id team);
		void					WindowAction(int32 windowToken, int32 action);

		void					WriteWindowList(team_id team,
									BPrivate::LinkSender& sender);
		void					WriteWindowInfo(int32 serverToken,
									BPrivate::LinkSender& sender);
		void					WriteApplicationOrder(int32 workspace,
									BPrivate::LinkSender& sender);
		void					WriteWindowOrder(int32 workspace,
									BPrivate::LinkSender& sender);

	private:
		void					_LaunchInputServer();
		void					_SetWorkspace(int32 index);
		void					_ShowWindow(Window* window,
									bool affectsOtherWindows = true);
		void					_HideWindow(Window* window);

		void					_UpdateSubsetWorkspaces(Window* window,
									int32 previousIndex = -1,
									int32 newIndex = -1);
		void					_ChangeWindowWorkspaces(Window* window,
									uint32 oldWorkspaces, uint32 newWorkspaces);
		void					_BringWindowsToFront(WindowList& windows,
									int32 list, bool wereVisible);
 		status_t				_ActivateApp(team_id team);
 		void					_SendFakeMouseMoved(Window* window = NULL);

		void					_RebuildClippingForAllWindows(
									BRegion& stillAvailableOnScreen);
		void					_TriggerWindowRedrawing(
									BRegion& newDirtyRegion);
		void					_SetBackground(BRegion& background);

		void					_UpdateFloating(int32 previousWorkspace = -1,
									int32 nextWorkspace = -1,
									Window* mouseEventWindow = NULL);
		void					_UpdateBack();
		void					_UpdateFront(bool updateFloating = true);
		void					_UpdateFronts(bool updateFloating = true);
		bool					_WindowHasModal(Window* window);

		void					_WindowChanged(Window* window);
		void					_WindowRemoved(Window* window);

		void					_GetLooperName(char* name, size_t size);
		void					_PrepareQuit();
		void					_DispatchMessage(int32 code,
									BPrivate::LinkReceiver &link);

		WindowList&				_CurrentWindows();
		WindowList&				_Windows(int32 index);

	private:
		friend class DesktopSettings;
		friend class LockedDesktopSettings;

		uid_t					fUserID;
		::VirtualScreen			fVirtualScreen;
		DesktopSettingsPrivate*	fSettings;
		port_id					fMessagePort;
		::EventDispatcher		fEventDispatcher;
		port_id					fInputPort;
		area_id					fSharedReadOnlyArea;
		server_read_only_memory* fServerReadOnlyMemory;

		BLocker					fApplicationsLock;
		BObjectList<ServerApp>	fApplications;

		sem_id					fShutdownSemaphore;
		int32					fShutdownCount;

		::Workspace::Private	fWorkspaces[kMaxWorkspaces];
		int32					fCurrentWorkspace;
		int32					fPreviousWorkspace;

		WindowList				fAllWindows;
		WindowList				fSubsetWindows;
		WindowList				fFocusList;

		BObjectList<WorkspacesView> fWorkspacesViews;
		BLocker					fWorkspacesLock;

		Screen*					fActiveScreen;

		CursorManager			fCursorManager;

#if USE_MULTI_LOCKER
		MultiLocker				fWindowLock;
#else
		BLocker					fWindowLock;
#endif

		BRegion					fBackgroundRegion;
		BRegion					fScreenRegion;

		Window*					fMouseEventWindow;
		const Window*			fWindowUnderMouse;
		const Window*			fLockedFocusWindow;
		int32					fViewUnderMouse;
		BPoint					fLastMousePosition;
		int32					fLastMouseButtons;

		Window*					fFocus;
		Window*					fFront;
		Window*					fBack;
};

#endif	// DESKTOP_H
