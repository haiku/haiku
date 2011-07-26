/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 *		Brecht Machiels <brecht@mos6581.org>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef DESKTOP_H
#define DESKTOP_H


#include <Autolock.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Menu.h>
#include <ObjectList.h>
#include <Region.h>
#include <Window.h>

#include "CursorManager.h"
#include "DesktopListener.h"
#include "DesktopSettings.h"
#include "EventDispatcher.h"
#include "MessageLooper.h"
#include "MultiLocker.h"
#include "Screen.h"
#include "ScreenManager.h"
#include "ServerCursor.h"
#include "StackAndTile.h"
#include "VirtualScreen.h"
#include "WindowList.h"
#include "Workspace.h"
#include "WorkspacePrivate.h"


class BMessage;

class DecorAddOn;
class DrawingEngine;
class HWInterface;
class ServerApp;
class Window;
class WorkspacesView;
struct server_read_only_memory;

namespace BPrivate {
	class LinkSender;
};


class Desktop : public DesktopObservable, public MessageLooper,
	public ScreenOwner {
public:
								Desktop(uid_t userID, const char* targetScreen);
	virtual						~Desktop();

			void				RegisterListener(DesktopListener* listener);

			status_t			Init();

			uid_t				UserID() const { return fUserID; }
			const char*			TargetScreen() { return fTargetScreen; }
	virtual port_id				MessagePort() const { return fMessagePort; }
			area_id				SharedReadOnlyArea() const
									{ return fSharedReadOnlyArea; }

			::EventDispatcher&	EventDispatcher() { return fEventDispatcher; }

			void				BroadcastToAllApps(int32 code);
			void				BroadcastToAllWindows(int32 code);

			filter_result		KeyEvent(uint32 what, int32 key,
									int32 modifiers);
	// Locking
			bool				LockSingleWindow()
									{ return fWindowLock.ReadLock(); }
			void				UnlockSingleWindow()
									{ fWindowLock.ReadUnlock(); }

			bool				LockAllWindows()
									{ return fWindowLock.WriteLock(); }
			void				UnlockAllWindows()
									{ fWindowLock.WriteUnlock(); }

			const MultiLocker&	WindowLocker() { return fWindowLock; }

	// Mouse and cursor methods

			void				SetCursor(ServerCursor* cursor);
			ServerCursorReference Cursor() const;
			void				SetManagementCursor(ServerCursor* newCursor);

			void				SetLastMouseState(const BPoint& position,
									int32 buttons, Window* windowUnderMouse);
									// for use by the mouse filter only
									// both mouse position calls require
									// the Desktop object to be locked
									// already
			void				GetLastMouseState(BPoint* position,
									int32* buttons) const;
									// for use by ServerWindow

			CursorManager&		GetCursorManager() { return fCursorManager; }

	// Screen and drawing related methods

			status_t			SetScreenMode(int32 workspace, int32 id,
									const display_mode& mode, bool makeDefault);
			status_t			GetScreenMode(int32 workspace, int32 id,
									display_mode& mode);
			status_t			GetScreenFrame(int32 workspace, int32 id,
									BRect& frame);
			void				RevertScreenModes(uint32 workspaces);

			MultiLocker&		ScreenLocker() { return fScreenLock; }

			status_t			LockDirectScreen(team_id team);
			status_t			UnlockDirectScreen(team_id team);

			const ::VirtualScreen& VirtualScreen() const
									{ return fVirtualScreen; }
			DrawingEngine*		GetDrawingEngine() const
									{ return fVirtualScreen.DrawingEngine(); }
			::HWInterface*		HWInterface() const
									{ return fVirtualScreen.HWInterface(); }

			void				RebuildAndRedrawAfterWindowChange(
									Window* window, BRegion& dirty);
									// the window lock must be held when calling
									// this function

	// ScreenOwner implementation
	virtual	void				ScreenRemoved(Screen* screen) {}
	virtual	void				ScreenAdded(Screen* screen) {}
	virtual	bool				ReleaseScreen(Screen* screen) { return false; }

	// Workspace methods

			void				SetWorkspaceAsync(int32 index,
									bool moveFocusWindow = false);
			void				SetWorkspace(int32 index,
									bool moveFocusWindow = false);
			int32				CurrentWorkspace()
									{ return fCurrentWorkspace; }
			Workspace::Private&	WorkspaceAt(int32 index)
									{ return fWorkspaces[index]; }
			status_t			SetWorkspacesLayout(int32 columns, int32 rows);
			BRect				WorkspaceFrame(int32 index) const;

			void				StoreWorkspaceConfiguration(int32 index);

			void				AddWorkspacesView(WorkspacesView* view);
			void				RemoveWorkspacesView(WorkspacesView* view);

	// Window methods

			void				SelectWindow(Window* window);
			void				ActivateWindow(Window* window);
			void				SendWindowBehind(Window* window,
									Window* behindOf = NULL);

			void				ShowWindow(Window* window);
			void				HideWindow(Window* window);
			void				MinimizeWindow(Window* window, bool minimize);

			void				MoveWindowBy(Window* window, float x, float y,
									int32 workspace = -1);
			void				ResizeWindowBy(Window* window, float x,
									float y);
			bool				SetWindowTabLocation(Window* window,
									float location, bool isShifting);
			bool				SetWindowDecoratorSettings(Window* window,
									const BMessage& settings);

			void				SetWindowWorkspaces(Window* window,
									uint32 workspaces);

			void				AddWindow(Window* window);
			void				RemoveWindow(Window* window);

			bool				AddWindowToSubset(Window* subset,
									Window* window);
			void				RemoveWindowFromSubset(Window* subset,
									Window* window);

			void				FontsChanged(Window* window);

			void				SetWindowLook(Window* window, window_look look);
			void				SetWindowFeel(Window* window, window_feel feel);
			void				SetWindowFlags(Window* window, uint32 flags);
			void				SetWindowTitle(Window* window,
									const char* title);

			Window*				FocusWindow() const { return fFocus; }
			Window*				FrontWindow() const { return fFront; }
			Window*				BackWindow() const { return fBack; }

			Window*				WindowAt(BPoint where);

			Window*				MouseEventWindow() const
									{ return fMouseEventWindow; }
			void				SetMouseEventWindow(Window* window);

			void				SetViewUnderMouse(const Window* window,
									int32 viewToken);
			int32				ViewUnderMouse(const Window* window);

			EventTarget*		KeyboardEventTarget();

			void				SetFocusWindow(Window* window = NULL);
			void				SetFocusLocked(const Window* window);

			Window*				FindWindowByClientToken(int32 token,
									team_id teamID);
			EventTarget*		FindTarget(BMessenger& messenger);

			void				MarkDirty(BRegion& region);
			void				Redraw();
			void				RedrawBackground();

			bool				ReloadDecor(DecorAddOn* oldDecor);

			BRegion&			BackgroundRegion()
									{ return fBackgroundRegion; }

			void				MinimizeApplication(team_id team);
			void				BringApplicationToFront(team_id team);
			void				WindowAction(int32 windowToken, int32 action);

			void				WriteWindowList(team_id team,
									BPrivate::LinkSender& sender);
			void				WriteWindowInfo(int32 serverToken,
									BPrivate::LinkSender& sender);
			void				WriteApplicationOrder(int32 workspace,
									BPrivate::LinkSender& sender);
			void				WriteWindowOrder(int32 workspace,
									BPrivate::LinkSender& sender);

			//! The window lock must be held when accessing a window list!
			WindowList&			CurrentWindows();
			WindowList&			AllWindows();

			Window*				WindowForClientLooperPort(port_id port);

			StackAndTile*		GetStackAndTile() { return &fStackAndTile; }
private:
			WindowList&			_Windows(int32 index);

			void				_LaunchInputServer();
			void				_GetLooperName(char* name, size_t size);
			void				_PrepareQuit();
			void				_DispatchMessage(int32 code,
									BPrivate::LinkReceiver &link);

			void				_UpdateFloating(int32 previousWorkspace = -1,
									int32 nextWorkspace = -1,
									Window* mouseEventWindow = NULL);
			void				_UpdateBack();
			void				_UpdateFront(bool updateFloating = true);
			void				_UpdateFronts(bool updateFloating = true);
			bool				_WindowHasModal(Window* window);

			void				_WindowChanged(Window* window);
			void				_WindowRemoved(Window* window);

			void				_ShowWindow(Window* window,
									bool affectsOtherWindows = true);
			void				_HideWindow(Window* window);

			void				_UpdateSubsetWorkspaces(Window* window,
									int32 previousIndex = -1,
									int32 newIndex = -1);
			void				_ChangeWindowWorkspaces(Window* window,
									uint32 oldWorkspaces, uint32 newWorkspaces);
			void				_BringWindowsToFront(WindowList& windows,
									int32 list, bool wereVisible);
			Window*				_LastFocusSubsetWindow(Window* window);
			void				_SendFakeMouseMoved(Window* window = NULL);

			Screen*				_DetermineScreenFor(BRect frame);
			void				_RebuildClippingForAllWindows(
									BRegion& stillAvailableOnScreen);
			void				_TriggerWindowRedrawing(
									BRegion& newDirtyRegion);
			void				_SetBackground(BRegion& background);

			status_t			_ActivateApp(team_id team);

			void				_SuspendDirectFrameBufferAccess();
			void				_ResumeDirectFrameBufferAccess();

			void				_ScreenChanged(Screen* screen);
			void				_SetCurrentWorkspaceConfiguration();
			void				_SetWorkspace(int32 index,
									bool moveFocusWindow = false);

private:
	friend class DesktopSettings;
	friend class LockedDesktopSettings;

			uid_t				fUserID;
			char*				fTargetScreen;
			::VirtualScreen		fVirtualScreen;
			DesktopSettingsPrivate*	fSettings;
			port_id				fMessagePort;
			::EventDispatcher	fEventDispatcher;
			area_id				fSharedReadOnlyArea;
			server_read_only_memory* fServerReadOnlyMemory;

			BLocker				fApplicationsLock;
			BObjectList<ServerApp> fApplications;

			sem_id				fShutdownSemaphore;
			int32				fShutdownCount;

			::Workspace::Private fWorkspaces[kMaxWorkspaces];
			MultiLocker			fScreenLock;
			BLocker				fDirectScreenLock;
			team_id				fDirectScreenTeam;
			int32				fCurrentWorkspace;
			int32				fPreviousWorkspace;

			WindowList			fAllWindows;
			WindowList			fSubsetWindows;
			WindowList			fFocusList;
			Window*				fLastWorkspaceFocus[kMaxWorkspaces];

			BObjectList<WorkspacesView> fWorkspacesViews;
			BLocker				fWorkspacesLock;

			CursorManager		fCursorManager;
			ServerCursorReference fCursor;
			ServerCursorReference fManagementCursor;

			MultiLocker			fWindowLock;

			BRegion				fBackgroundRegion;
			BRegion				fScreenRegion;

			Window*				fMouseEventWindow;
			const Window*		fWindowUnderMouse;
			const Window*		fLockedFocusWindow;
			int32				fViewUnderMouse;
			BPoint				fLastMousePosition;
			int32				fLastMouseButtons;

			Window*				fFocus;
			Window*				fFront;
			Window*				fBack;

			StackAndTile		fStackAndTile;
};

#endif	// DESKTOP_H
