/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DESKTOP_H
#define DESKTOP_H


#include "CursorManager.h"
#include "EventDispatcher.h"
#include "ScreenManager.h"
#include "ServerScreen.h"
#include "VirtualScreen.h"
#include "DesktopSettings.h"
#include "MessageLooper.h"
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
class WorkspacesLayer;
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
		area_id					SharedReadOnlyArea() const { return fSharedReadOnlyArea; }

		::EventDispatcher&		EventDispatcher() { return fEventDispatcher; }

		void					BroadcastToAllApps(int32 code);

		// Screen and drawing related methods

		Screen*					ScreenAt(int32 index) const
									{ return fActiveScreen; }
		Screen*					ActiveScreen() const
									{ return fActiveScreen; }

		CursorManager&			GetCursorManager() { return fCursorManager; }

		void					SetCursor(ServerCursor* cursor);
		ServerCursor*			Cursor() const;

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

		void					SetWorkspace(int32 index);
		int32					CurrentWorkspace()
									{ return fCurrentWorkspace; }
		Workspace::Private&		WorkspaceAt(int32 index)
									{ return fWorkspaces[index]; }
		void					UpdateWorkspaces();

		// WindowLayer methods

		void					ActivateWindow(WindowLayer* window);
		void					SendWindowBehind(WindowLayer* window,
									WindowLayer* behindOf = NULL);

		void					ShowWindow(WindowLayer* window);
		void					HideWindow(WindowLayer* window);

		void					MoveWindowBy(WindowLayer* window, float x, float y,
									int32 workspace = -1);
		void					ResizeWindowBy(WindowLayer* window, float x, float y);
		void					SetWindowTabLocation(WindowLayer* window, float location);

		void					SetWindowWorkspaces(WindowLayer* window,
									uint32 workspaces);

		void					AddWindow(WindowLayer* window);
		void					RemoveWindow(WindowLayer* window);

		bool					AddWindowToSubset(WindowLayer* subset,
									WindowLayer* window);
		void					RemoveWindowFromSubset(WindowLayer* subset,
									WindowLayer* window);

		void					SetWindowLook(WindowLayer* window, window_look look);
		void					SetWindowFeel(WindowLayer* window, window_feel feel);
		void					SetWindowFlags(WindowLayer* window, uint32 flags);
		void					SetWindowTitle(WindowLayer* window, const char* title);

		WindowLayer*			FocusWindow() const { return fFocus; }
		WindowLayer*			FrontWindow() const { return fFront; }
		WindowLayer*			BackWindow() const { return fBack; }

		WindowLayer*			WindowAt(BPoint where);

		WindowLayer*			MouseEventWindow() const { return fMouseEventWindow; }
		void					SetMouseEventWindow(WindowLayer* window);

		void					SetViewUnderMouse(const WindowLayer* window, int32 viewToken);
		int32					ViewUnderMouse(const WindowLayer* window);

		void					SetFocusWindow(WindowLayer* window);

		WindowLayer*			FindWindowLayerByClientToken(int32 token, team_id teamID);
		//WindowLayer*			FindWindowLayerByServerToken(int32 token);

#if USE_MULTI_LOCKER
		bool					LockSingleWindow() { return fWindowLock.ReadLock(); }
		void					UnlockSingleWindow() { fWindowLock.ReadUnlock(); }

		bool					LockAllWindows() { return fWindowLock.WriteLock(); }
		void					UnlockAllWindows() { fWindowLock.WriteUnlock(); }
#else // USE_MULTI_LOCKER
		bool					LockSingleWindow() { return fWindowLock.Lock(); }
		void					UnlockSingleWindow() { fWindowLock.Unlock(); }

		bool					LockAllWindows() { return fWindowLock.Lock(); }
		void					UnlockAllWindows() { fWindowLock.Unlock(); }
#endif // USE_MULTI_LOCKER

		void					MarkDirty(BRegion& region);

		BRegion&				BackgroundRegion()
									{ return fBackgroundRegion; }

		void					RedrawBackground();
		void					StoreWorkspaceConfiguration(int32 index);

		void					MinimizeApplication(team_id team);
		void					BringApplicationToFront(team_id team);
		void					WindowAction(int32 windowToken, int32 action);

		void					WriteWindowList(team_id team,
									BPrivate::LinkSender& sender);
		void					WriteWindowInfo(int32 serverToken,
									BPrivate::LinkSender& sender);

	private:
		void					_ShowWindow(WindowLayer* window,
									bool affectsOtherWindows = true);
		void					_HideWindow(WindowLayer* window);

		void					_UpdateSubsetWorkspaces(WindowLayer* window);
		void					_ChangeWindowWorkspaces(WindowLayer* window,
									uint32 oldWorkspaces, uint32 newWorkspaces);
		void					_BringWindowsToFront(WindowList& windows,
									int32 list, bool wereVisible);
 		status_t				_ActivateApp(team_id team);
 		void					_SendFakeMouseMoved(WindowLayer* window = NULL);

		void					_RebuildClippingForAllWindows(BRegion& stillAvailableOnScreen);
		void					_TriggerWindowRedrawing(BRegion& newDirtyRegion);
		void					_SetBackground(BRegion& background);

		void					_UpdateFloating(int32 previousWorkspace = -1,
									int32 nextWorkspace = -1);
		void					_UpdateBack();
		void					_UpdateFront(bool updateFloating = true);
		void					_UpdateFronts(bool updateFloating = true);
		bool					_WindowHasModal(WindowLayer* window);

		void					_WindowChanged(WindowLayer* window);

		void					_GetLooperName(char* name, size_t size);
		void					_PrepareQuit();
		void					_DispatchMessage(int32 code,
									BPrivate::LinkReceiver &link);

		WindowList&				_CurrentWindows();
		WindowList&				_Windows(int32 index);

	private:
		friend class DesktopSettings;

		uid_t					fUserID;
		::VirtualScreen			fVirtualScreen;
		DesktopSettings::Private* fSettings;
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

		WindowList				fAllWindows;
		WindowList				fSubsetWindows;
		WorkspacesLayer*		fWorkspacesLayer;

		Screen*					fActiveScreen;

		CursorManager			fCursorManager;

#if USE_MULTI_LOCKER
		MultiLocker				fWindowLock;
#else
		BLocker					fWindowLock;
#endif

		BRegion					fBackgroundRegion;
		BRegion					fScreenRegion;

		bool					fFocusFollowsMouse;

		WindowLayer*			fMouseEventWindow;
		const WindowLayer*		fWindowUnderMouse;
		int32					fViewUnderMouse;

		WindowLayer*			fFocus;
		WindowLayer*			fFront;
		WindowLayer*			fBack;
};

#endif	// DESKTOP_H
