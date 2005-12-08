/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Class used to encapsulate desktop management */


#include "Desktop.h"

#include "AppServer.h"
#include "DesktopSettingsPrivate.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "InputManager.h"
#include "ServerApp.h"
#include "ServerConfig.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "WindowLayer.h"
#include "Workspace.h"

#include <WindowInfo.h>
#include <ServerProtocol.h>

#include <Entry.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Region.h>

#include <stdio.h>

#if TEST_MODE
#	include "EventStream.h"
#endif

//#define DEBUG_DESKTOP
#ifdef DEBUG_DESKTOP
#	define STRACE(a) printf(a)
#else
#	define STRACE(a) ;
#endif


class KeyboardFilter : public EventFilter {
	public:
		KeyboardFilter(Desktop* desktop);

		virtual filter_result Filter(BMessage* message, EventTarget** _target,
			int32* _viewToken);

	private:
		Desktop*		fDesktop;
		EventTarget*	fLastFocus;
		bigtime_t		fTimestamp;
};

class MouseFilter : public EventFilter {
	public:
		MouseFilter(Desktop* desktop);

		virtual filter_result Filter(BMessage* message, EventTarget** _target,
			int32* _viewToken);

	private:
		Desktop*	fDesktop;
};


//	#pragma mark -


KeyboardFilter::KeyboardFilter(Desktop* desktop)
	:
	fDesktop(desktop),
	fLastFocus(NULL),
	fTimestamp(0)
{
}


filter_result
KeyboardFilter::Filter(BMessage* message, EventTarget** _target,
	int32* /*_viewToken*/)
{
	int32 key;
	int32 modifiers;

	if (message->what != B_KEY_DOWN
		|| message->FindInt32("key", &key) != B_OK
		|| message->FindInt32("modifiers", &modifiers) != B_OK)
		return B_DISPATCH_MESSAGE;

	// Check for safe video mode (F12 + l-cmd + l-ctrl + l-shift)
	if (key == 0x0d
		&& (modifiers & (B_LEFT_COMMAND_KEY
				| B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY)) != 0)
	{
		// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN.
		STRACE(("Safe Video Mode invoked - code unimplemented\n"));
		return B_SKIP_MESSAGE;
	}

	if (key > 0x01 && key < 0x0e) {
		// workspace change, F1-F12

#if !TEST_MODE
		if (modifiers & B_COMMAND_KEY)
#else
		if (modifiers & B_CONTROL_KEY)
#endif
		{
			STRACE(("Set Workspace %ld\n", key - 1));

			fDesktop->SetWorkspace(key - 2);
			return B_SKIP_MESSAGE;
		}
	}

	// TODO: this should be moved client side!
	// (that's how it is done in BeOS, clients could need this key for
	// different purposes - also, it's preferrable to let the client
	// write the dump within his own environment)
	if (key == 0xe) {
		// screen dump, PrintScreen
		char filename[128];
		BEntry entry;

		int32 index = 1;
		do {
			sprintf(filename, "/boot/home/screen%ld.png", index++);
			entry.SetTo(filename);
		} while(entry.Exists());

		fDesktop->GetDrawingEngine()->DumpToFile(filename);
		return B_SKIP_MESSAGE;
	}

	bigtime_t now = system_time();

	fDesktop->ReadLockWindows();

	EventTarget* focus = NULL;
	if (fDesktop->FocusWindow() != NULL)
		focus = &fDesktop->FocusWindow()->EventTarget();

	// TODO: this is a try to not steal focus from the current window
	//	in case you enter some text and a window pops up you haven't
	//	triggered yourself (like a pop-up window in your browser while
	//	you're typing a password in another window) - maybe this should
	//	be done differently, though (using something like B_LOCK_WINDOW_FOCUS)
	//	(at least B_WINDOW_ACTIVATED must be postponed)

	if (focus != fLastFocus && now - fTimestamp > 100000) {
		// if the time span between the key presses is very short
		// we keep our previous focus alive - this is save even
		// if the target doesn't exist anymore, as we don't reset
		// it, and the event focus passed in is always valid (or NULL)
		*_target = focus;
		fLastFocus = focus;
	}	

	fDesktop->ReadUnlockWindows();

	// we always allow to switch focus after the enter key has pressed
	if (key == B_ENTER)
		fTimestamp = 0;
	else
		fTimestamp = now;

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


MouseFilter::MouseFilter(Desktop* desktop)
	:
	fDesktop(desktop)
{
}


filter_result
MouseFilter::Filter(BMessage* message, EventTarget** _target, int32* _viewToken)
{
	BPoint where;
	if (message->FindPoint("where", &where) != B_OK)
		return B_DISPATCH_MESSAGE;

	fDesktop->WriteLockWindows();

	WindowLayer* window = fDesktop->MouseEventWindow();
	if (window == NULL)
		window = fDesktop->WindowAt(where);

	if (window != NULL) {
		// dispatch event in the window layers
		switch (message->what) {
			case B_MOUSE_DOWN:
				window->MouseDown(message, where, _viewToken);
				break;

			case B_MOUSE_UP:
				window->MouseUp(message, where, _viewToken);
				fDesktop->SetMouseEventWindow(NULL);
				break;

			case B_MOUSE_MOVED:
				window->MouseMoved(message, where, _viewToken);
				break;
		}

		if (*_viewToken != B_NULL_TOKEN)
			*_target = &window->EventTarget();
		else
			*_target = NULL;
	} else
		*_target = NULL;

	fDesktop->WriteUnlockWindows();

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


static inline uint32
workspace_to_workspaces(int32 index)
{
	return 1UL << index;
}


static inline bool
workspaces_on_workspace(int32 index, uint32 workspaces)
{
	return (workspaces & (1UL << index)) != 0;
}


//	#pragma mark -


Desktop::Desktop(uid_t userID)
	: MessageLooper("desktop"),

	fUserID(userID),
	fSettings(new DesktopSettings::Private()),
	fApplicationsLock("application list"),
	fShutdownSemaphore(-1),
	fAllWindows(kAllWindowList),
	fActiveScreen(NULL),
	fWindowLock("window lock")
{
	char name[B_OS_NAME_LENGTH];
	Desktop::_GetLooperName(name, sizeof(name));

	for (int32 i = 0; i < kMaxWorkspaces; i++) {
		fWorkspaces[i].Windows().SetIndex(i);
	}

	fMessagePort = create_port(DEFAULT_MONITOR_PORT_SIZE, name);
	if (fMessagePort < B_OK)
		return;

	fLink.SetReceiverPort(fMessagePort);
	gFontManager->AttachUser(fUserID);
}


Desktop::~Desktop()
{
	delete fSettings;

	delete_port(fMessagePort);
	gFontManager->DetachUser(fUserID);
}


void
Desktop::Init()
{
	fVirtualScreen.RestoreConfiguration(*this, fSettings->WorkspacesMessage(0));

	// TODO: temporary workaround, fActiveScreen will be removed
	fActiveScreen = fVirtualScreen.ScreenAt(0);

#if TEST_MODE
	gInputManager->AddStream(new InputServerStream);
#endif
	fEventDispatcher.SetTo(gInputManager->GetStream());
	fEventDispatcher.SetHWInterface(fVirtualScreen.HWInterface());

	fEventDispatcher.SetMouseFilter(new MouseFilter(this));
	fEventDispatcher.SetKeyboardFilter(new KeyboardFilter(this));

	// take care of setting the default cursor
	ServerCursor *cursor = fCursorManager.GetCursor(B_CURSOR_DEFAULT);
	if (cursor)
		fVirtualScreen.HWInterface()->SetCursor(cursor);

	fVirtualScreen.HWInterface()->MoveCursorTo(fVirtualScreen.Frame().Width() / 2,
		fVirtualScreen.Frame().Height() / 2);
	fVirtualScreen.HWInterface()->SetCursorVisible(true);

	// draw the background

	fScreenRegion = fVirtualScreen.Frame();

	BRegion stillAvailableOnScreen;
	_RebuildClippingForAllWindows(stillAvailableOnScreen);
	_SetBackground(stillAvailableOnScreen);
}


void
Desktop::_GetLooperName(char* name, size_t length)
{
	snprintf(name, length, "d:%d:%s", /*id*/0, /*name*/"baron");
}


void
Desktop::_PrepareQuit()
{
	// let's kill all remaining applications

	fApplicationsLock.Lock();

	int32 count = fApplications.CountItems();
	for (int32 i = 0; i < count; i++) {
		ServerApp *app = fApplications.ItemAt(i);
		team_id clientTeam = app->ClientTeam();

		app->Quit();
		kill_team(clientTeam);
	}

	// wait for the last app to die
	if (count > 0)
		acquire_sem_etc(fShutdownSemaphore, fShutdownCount, B_RELATIVE_TIMEOUT, 250000);

	fApplicationsLock.Unlock();
}


void
Desktop::_DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
	switch (code) {
		case AS_CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication

			// Attached data:
			// 1) port_id - receiver port of a regular app
			// 2) port_id - client looper port - for sending messages to the client
			// 2) team_id - app's team ID
			// 3) int32 - handler token of the regular app
			// 4) char * - signature of the regular app

			// Find the necessary data
			team_id	clientTeamID = -1;
			port_id	clientLooperPort = -1;
			port_id clientReplyPort = -1;
			int32 htoken = B_NULL_TOKEN;
			char *appSignature = NULL;

			link.Read<port_id>(&clientReplyPort);
			link.Read<port_id>(&clientLooperPort);
			link.Read<team_id>(&clientTeamID);
			link.Read<int32>(&htoken);
			if (link.ReadString(&appSignature) != B_OK)
				break;

			ServerApp *app = new ServerApp(this, clientReplyPort,
				clientLooperPort, clientTeamID, htoken, appSignature);
			if (app->InitCheck() == B_OK
				&& app->Run()) {
				// add the new ServerApp to the known list of ServerApps
				fApplicationsLock.Lock();
				fApplications.AddItem(app);
				fApplicationsLock.Unlock();
			} else {
				delete app;

				// if everything went well, ServerApp::Run() will notify
				// the client - but since it didn't, we do it here
				BPrivate::LinkSender reply(clientReplyPort);
				reply.StartMessage(SERVER_FALSE);
				reply.Flush();
			}

			// This is necessary because BPortLink::ReadString allocates memory
			free(appSignature);
			break;
		}

		case AS_DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.

			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted

			thread_id thread = -1;
			if (link.Read<thread_id>(&thread) < B_OK)
				break;

			fApplicationsLock.Lock();

			// Run through the list of apps and nuke the proper one

			int32 count = fApplications.CountItems();
			ServerApp *removeApp = NULL;

			for (int32 i = 0; i < count; i++) {
				ServerApp *app = fApplications.ItemAt(i);

				if (app != NULL && app->Thread() == thread) {
					fApplications.RemoveItemAt(i);
					removeApp = app;
					break;
				}
			}

			fApplicationsLock.Unlock();

			if (removeApp != NULL)
				removeApp->Quit(fShutdownSemaphore);

			if (fQuitting && count <= 1) {
				// wait for the last app to die
				acquire_sem_etc(fShutdownSemaphore, fShutdownCount, B_RELATIVE_TIMEOUT, 500000);
				PostMessage(kMsgQuitLooper);
			}
			break;
		}

		case AS_ACTIVATE_APP:
		{
			// Someone is requesting to activation of a certain app.

			// Attached data:
			// 1) port_id reply port
			// 2) team_id team

			status_t status;

			// get the parameters
			port_id replyPort;
			team_id team;
			if (link.Read(&replyPort) == B_OK
				&& link.Read(&team) == B_OK)
				status = _ActivateApp(team);
			else
				status = B_ERROR;

			// send the reply
			BPrivate::PortLink replyLink(replyPort);
			replyLink.StartMessage(status);
			replyLink.Flush();
			break;
		}

		case AS_SET_SYSCURSOR_DEFAULTS:
		{
			GetCursorManager().SetDefaults();
			break;
		}

		case B_QUIT_REQUESTED:
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the server
			// is compiled as a regular Be application.

			fApplicationsLock.Lock();
			fShutdownSemaphore = create_sem(0, "desktop shutdown");
			fShutdownCount = fApplications.CountItems();
			fApplicationsLock.Unlock();

			fQuitting = true;
			BroadcastToAllApps(AS_QUIT_APP);

			// We now need to process the remaining AS_DELETE_APP messages and
			// wait for the kMsgShutdownServer message.
			// If an application does not quit as asked, the picasso thread
			// will send us this message in 2-3 seconds.

			// if there are no apps to quit, shutdown directly
			if (fShutdownCount == 0)
				PostMessage(kMsgQuitLooper);
			break;

		default:
			printf("Desktop %d:%s received unexpected code %ld\n", 0, "baron", code);

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
	}
}


/*!
	\brief activate one of the app's windows.
*/
status_t
Desktop::_ActivateApp(team_id team)
{
	status_t status = B_BAD_TEAM_ID;

	// search for an unhidden window to give focus to

	for (WindowLayer* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		// if window is a normal window of the team, and not hidden, 
		// we've found our target
		if (!window->IsHidden() && window->IsNormal()
			&& window->ServerWindow()->ClientTeam() == team) {
			ActivateWindow(window);
			return B_OK;
		}
	}

	return status;
}


/*!
	\brief Send a quick (no attachments) message to all applications
	
	Quite useful for notification for things like server shutdown, system 
	color changes, etc.
*/
void
Desktop::BroadcastToAllApps(int32 code)
{
	BAutolock locker(fApplicationsLock);

	for (int32 i = 0; i < fApplications.CountItems(); i++) {
		fApplications.ItemAt(i)->PostMessage(code);
	}
}


void
Desktop::UpdateWorkspaces()
{
	// TODO: maybe this should be replaced by a SetWorkspacesCount() method

/*	if (fWorkspacesLayer == NULL)
		return;

	_WindowsChanged();
*/
}


void
Desktop::SetWorkspace(int32 index)
{
	BAutolock _(this);
	DesktopSettings settings(this);

	if (index < 0 || index >= settings.WorkspacesCount() || index == fCurrentWorkspace)
		return;

	WriteLockWindows();

	int32 previousIndex = fCurrentWorkspace;

	if (fMouseEventWindow != NULL) {
		// the window currently being dragged will follow us to this workspace
		// if it's not already on it
		if (!fMouseEventWindow->OnWorkspace(index)) {
			fWorkspaces[index].Windows().AddWindow(fMouseEventWindow);
			fMouseEventWindow->SetWorkspaces((fMouseEventWindow->Workspaces()
					| workspace_to_workspaces(index))
				& ~workspace_to_workspaces(previousIndex));
			fWorkspaces[previousIndex].Windows().RemoveWindow(fMouseEventWindow);
		} else {
			// make sure it's frontmost
			fWorkspaces[index].Windows().RemoveWindow(fMouseEventWindow);
			fWorkspaces[index].Windows().AddWindow(fMouseEventWindow,
				fMouseEventWindow->Frontmost(fWorkspaces[index].Windows().FirstWindow(), index));
		}

		fMouseEventWindow->Anchor(index).position = fMouseEventWindow->Frame().LeftTop();
	}

	// build region of windows that are no longer visible in the new workspace

	BRegion dirty;
	for (WindowLayer* window = _CurrentWindows().FirstWindow();
			window != NULL; window = window->NextWindow(previousIndex)) {
		window->Anchor(previousIndex).position = window->Frame().LeftTop();

		if (window->OnWorkspace(index))
			continue;

		if (!window->IsHidden()) {
			// this window will no longer be visible
			dirty.Include(&window->VisibleRegion());
		}

		window->SetCurrentWorkspace(-1);			
	}

	fCurrentWorkspace = index;

	// show windows, and include them in the changed region - but only
	// those that were not visible before (or whose position changed)

	for (WindowLayer* window = _CurrentWindows().FirstWindow();
			window != NULL; window = window->NextWindow(index)) {
		BPoint position = window->Anchor(index).position;

		window->SetCurrentWorkspace(index);

		if (window->IsHidden())
			continue;

		if (position == kInvalidWindowPosition) {
			// if you enter a workspace for the first time, the position
			// of the window in the previous workspace is adopted
			position = window->Frame().LeftTop();
				// TODO: make sure the window is still on-screen if it
				//	was before!
		}

		if (!window->OnWorkspace(previousIndex)) {
			// this window was not visible before
			continue;
		}

		if (window->Frame().LeftTop() != position) {
			// the window was visible before, but its on-screen location changed
			BPoint offset = position - window->Frame().LeftTop();
			MoveWindowBy(window, offset.x, offset.y);
				// TODO: be a bit smarter than this...
		}
	}

	BRegion stillAvailableOnScreen;
	_RebuildClippingForAllWindows(stillAvailableOnScreen);
	_SetBackground(stillAvailableOnScreen);

	for (WindowLayer* window = _CurrentWindows().FirstWindow(); window != NULL;
			window = window->NextWindow(index)) {
		if (window->OnWorkspace(previousIndex) || window == fMouseEventWindow) {
			// this window was visible before, and is already handled in the above loop
			continue;
		}

		dirty.Include(&window->VisibleRegion());
	}

	_UpdateFronts();
	SetFocusWindow(FrontWindow());

	MarkDirty(dirty);

	//_WindowsChanged();
	WriteUnlockWindows();
}


void
Desktop::ScreenChanged(Screen* screen)
{
	// TODO: confirm that everywhere this is used,
	// the Window WriteLock is held

// TODO: can this be removed? I would think it can.
// In fact, this lock should nowhere be used anymore, no?
//	BAutolock locker(this);

	// the entire screen is dirty, because we're actually
	// operating on an all new buffer in memory
	BRegion dirty(screen->Frame());
	// update our cached screen region
	fScreenRegion.Set(screen->Frame());

	BRegion background;
	_RebuildClippingForAllWindows(background);

	fBackgroundRegion.MakeEmpty();
		// makes sure that the complete background is redrawn
	_SetBackground(background);

	// figure out dirty region
	dirty.Exclude(&background);
	_TriggerWindowRedrawing(dirty);

	// send B_SCREEN_CHANGED to windows on that screen
	BMessage update(B_SCREEN_CHANGED);
	update.AddInt64("when", real_time_clock_usecs());
	update.AddRect("frame", screen->Frame());
	update.AddInt32("mode", screen->ColorSpace());

	// TODO: currently ignores the screen argument!
	for (WindowLayer* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		window->ServerWindow()->SendMessageToClient(&update);
	}
}


//	#pragma mark - Methods for WindowLayer manipulation


WindowList&
Desktop::_CurrentWindows()
{
	return fWorkspaces[fCurrentWorkspace].Windows();
}


/*! Search the visible windows for a valid back window
	(only normal windows can be back windows)
*/
void
Desktop::_UpdateBack()
{
	fBack = NULL;

	for (WindowLayer* window = _CurrentWindows().FirstWindow();
			window != NULL; window = window->NextWindow(fCurrentWorkspace)) {
		if (window->IsHidden() || !window->SupportsFront())
			continue;

		fBack = window;
		break;
	}
}


/*! Search the visible windows for a valid front window
	(only normal windows can be front windows)
*/
void
Desktop::_UpdateFront()
{
	// TODO: for now, just choose the top window
	fFront = NULL;

	for (WindowLayer* window = _CurrentWindows().LastWindow();
			window != NULL; window = window->PreviousWindow(fCurrentWorkspace)) {
		if (window->IsHidden() || !window->SupportsFront())
			continue;

		fFront = window;
		break;
	}
}


void
Desktop::_UpdateFronts()
{
	_UpdateBack();
	_UpdateFront();
}



void
Desktop::SetFocusWindow(WindowLayer* focus)
{
	// TODO: test for FFM and B_LOCK_WINDOW_FOCUS
	if (focus == fFocus && focus != NULL && (focus->Flags() & B_AVOID_FOCUS) == 0)
		return;

	if (WriteLockWindows()) {
		// make sure no window is chosen that doesn't want focus
		while (focus != NULL && (focus->Flags() & B_AVOID_FOCUS) != 0) {
			focus = focus->PreviousWindow(fCurrentWorkspace);
		}

		if (fFocus != NULL)
			fFocus->SetFocus(false);

		fFocus = focus;

		if (focus != NULL)
			focus->SetFocus(true);

		WriteUnlockWindows();
	}
}


void
Desktop::_BringWindowsToFront(WindowList& windows, int32 list,
	bool wereVisible)
{
	// we don't need to redraw what is currently
	// visible of the window
	BRegion clean;

	for (WindowLayer* window = windows.FirstWindow(); window != NULL;
			window = window->NextWindow(list)) {
		if (wereVisible)
			clean.Include(&window->VisibleRegion());

		_CurrentWindows().AddWindow(window,
			window->Frontmost(_CurrentWindows().FirstWindow(),
				fCurrentWorkspace));
	}

	BRegion dummy;
	_RebuildClippingForAllWindows(dummy);

	// redraw what became visible of the window(s)

	BRegion dirty;
	for (WindowLayer* window = windows.FirstWindow(); window != NULL;
			window = window->NextWindow(list)) {
		dirty.Include(&window->VisibleRegion());
	}

	dirty.Exclude(&clean);
	MarkDirty(dirty);

	_UpdateFront();

	if (windows.FirstWindow() == fBack || fBack == NULL)
		_UpdateBack();

	SetFocusWindow(FrontWindow());
}


/*!
	\brief Tries to move the specified window to the front of the screen,
		and make it the focus window.

	If there are any modal windows on this screen, it might not actually
	become the frontmost window, though, as modal windows stay in front
	of their subset.
*/
void
Desktop::ActivateWindow(WindowLayer* window)
{
//	printf("ActivateWindow(%p, %s)\n", window, window ? window->Title() : "<none>");

	if (window == NULL) {
		fBack = NULL;
		fFront = NULL;
		return;
	}
	if (window == FrontWindow())
		return;

	// TODO: take care about floating windows

	if (!WriteLockWindows())
		return;

	// we don't need to redraw what is currently
	// visible of the window
	BRegion clean(window->VisibleRegion());
	WindowList windows(kWorkingList);

	WindowLayer* frontmost = window->Frontmost();

	_CurrentWindows().RemoveWindow(window);
	windows.AddWindow(window);

	if (frontmost != NULL && frontmost->IsModal()) {
		// all modal windows follow their subsets to the front
		// (ie. they are staying in front of them, but they are
		// not supposed to change their order because of that)

		WindowLayer* nextModal;
		for (WindowLayer* modal = frontmost; modal != NULL; modal = nextModal) {
			// get the next modal window
			nextModal = modal->NextWindow(fCurrentWorkspace);
			while (nextModal != NULL && !nextModal->IsModal()) {
				nextModal = nextModal->NextWindow(fCurrentWorkspace);
			}
			if (nextModal != NULL && !nextModal->HasInSubset(window))
				nextModal = NULL;

			_CurrentWindows().RemoveWindow(modal);
			windows.AddWindow(modal);
		}
	}

	_BringWindowsToFront(windows, kWorkingList, true);
	WriteUnlockWindows();
}


void
Desktop::SendWindowBehind(WindowLayer* window, WindowLayer* behindOf)
{
	if (window == BackWindow() || !WriteLockWindows())
		return;

	// Is this a valid behindOf window?
	if (behindOf != NULL && window->HasInSubset(behindOf))
		behindOf = NULL;

	// what is currently visible of the window
	// might be dirty after the window is send to back
	BRegion dirty(window->VisibleRegion());

	// detach window and re-attach at desired position
	WindowLayer* backmost = window->Backmost(behindOf);

	_CurrentWindows().RemoveWindow(window);
	_CurrentWindows().AddWindow(window, backmost
		? backmost->NextWindow(fCurrentWorkspace) : BackWindow());

	BRegion dummy;
	_RebuildClippingForAllWindows(dummy);

	// mark everything dirty that is no longer visible
	BRegion clean(window->VisibleRegion());
	dirty.Exclude(&clean);
	MarkDirty(dirty);

	// TODO: if this window has any floating windows, remove them here

	_UpdateFronts();
	SetFocusWindow(FrontWindow());
	//_WindowsChanged();

	WriteUnlockWindows();
}


void
Desktop::ShowWindow(WindowLayer* window)
{
	if (!window->IsHidden())
		return;

	WriteLockWindows();

	window->SetHidden(false);

	if (window->OnWorkspace(fCurrentWorkspace)) {
		_ShowWindow(window, true);
		ActivateWindow(window);
	} else {
		// then we don't need to send the fake mouse event either
		WriteUnlockWindows();
		return;
	}

	WriteUnlockWindows();

	// If the mouse cursor is directly over the newly visible window,
	// we'll send a fake mouse moved message to the window, so that
	// it knows the mouse is over it.

	BPoint where;
	int32 buttons;
	EventDispatcher().GetMouse(where, buttons);

	int32 viewToken = B_NULL_TOKEN;

	WriteLockWindows();

	if (WindowAt(where) == window) {
		ViewLayer* view = window->ViewAt(where);
		if (view != NULL)
			viewToken = view->Token();
	}
	WriteUnlockWindows();

	if (viewToken != B_NULL_TOKEN)
		EventDispatcher().SendFakeMouseMoved(window->EventTarget(), viewToken);
}


void
Desktop::HideWindow(WindowLayer* window)
{
	if (window->IsHidden())
		return;

	if (!WriteLockWindows())
		return;

	window->SetHidden(true);

	if (window->OnWorkspace(fCurrentWorkspace)) {
		_HideWindow(window);
		_UpdateFronts();

		if (FocusWindow() == window)
			SetFocusWindow(FrontWindow());
	}

	WriteUnlockWindows();
}


/*!
	Shows the window on the screen - it does this independently of the
	WindowLayer::IsHidden() state.
*/
void
Desktop::_ShowWindow(WindowLayer* window, bool affectsOtherWindows)
{
	BRegion background;
	_RebuildClippingForAllWindows(background);
	_SetBackground(background);

	BRegion dirty(window->VisibleRegion());

	if (!affectsOtherWindows) {
		// everything that is now visible in the
		// window needs a redraw, but other windows
		// are not affected, we can call ProcessDirtyRegion()
		// of the window, and don't have to use MarkDirty()
		window->ProcessDirtyRegion(dirty);
	} else
		MarkDirty(dirty);
}


/*!
	Hides the window from the screen - it does this independently of the
	WindowLayer::IsHidden() state.
*/
void
Desktop::_HideWindow(WindowLayer* window)
{
	// after rebuilding the clipping,
	// this window will not have a visible
	// region anymore, so we need to remember
	// it now
	// (actually that's not true, since
	// hidden windows are excluded from the
	// clipping calculation, but anyways)
	BRegion dirty(window->VisibleRegion());

	BRegion background;
	_RebuildClippingForAllWindows(background);
	_SetBackground(background);

	MarkDirty(dirty);
}


void
Desktop::MoveWindowBy(WindowLayer* window, float x, float y)
{
	if (!WriteLockWindows())
		return;

	// the dirty region starts with the visible area of the window being moved
	BRegion newDirtyRegion(window->VisibleRegion());

	window->MoveBy(x, y);

	BRegion background;
	_RebuildClippingForAllWindows(background);

	// construct the region that is possible to be blitted
	// to move the contents of the window
	BRegion copyRegion(window->VisibleRegion());
	copyRegion.OffsetBy(-x, -y);
	copyRegion.IntersectWith(&newDirtyRegion);

	// include the the new visible region of the window being
	// moved into the dirty region (for now)
	newDirtyRegion.Include(&window->VisibleRegion());

	if (GetDrawingEngine()->Lock()) {
		GetDrawingEngine()->CopyRegion(&copyRegion, x, y);

		// in the dirty region, exclude the parts that we
		// could move by blitting
		copyRegion.OffsetBy(x, y);
		newDirtyRegion.Exclude(&copyRegion);

		GetDrawingEngine()->Unlock();
	}

	MarkDirty(newDirtyRegion);
	_SetBackground(background);

	//_WindowsChanged(changed);

	WriteUnlockWindows();
}


void
Desktop::ResizeWindowBy(WindowLayer* window, float x, float y)
{
	if (!WriteLockWindows())
		return;

	BRegion newDirtyRegion;
	BRegion previouslyOccupiedRegion(window->VisibleRegion());
	
	window->ResizeBy(x, y, &newDirtyRegion);

	BRegion background;
	_RebuildClippingForAllWindows(background);

	previouslyOccupiedRegion.Exclude(&window->VisibleRegion());

	newDirtyRegion.IntersectWith(&window->VisibleRegion());
	newDirtyRegion.Include(&previouslyOccupiedRegion);

	MarkDirty(newDirtyRegion);
	_SetBackground(background);

	//_WindowsChanged(changed);

	WriteUnlockWindows();
}


/*!
	\brief Adds or removes the window to or from the workspaces it's on.
*/
void
Desktop::_ChangeWindowWorkspaces(WindowLayer* window, uint32 oldWorkspaces,
	uint32 newWorkspaces)
{
	// apply changes to the workspaces' window lists

	WriteLockWindows();

	for (int32 i = 0; i < kMaxWorkspaces; i++) {
		if (workspaces_on_workspace(i, oldWorkspaces)) {
			// window is on this workspace, is it anymore?
			if (!workspaces_on_workspace(i, newWorkspaces)) {
				fWorkspaces[i].Windows().RemoveWindow(window);
				window->SetCurrentWorkspace(-1);

				if (i == CurrentWorkspace())
					_HideWindow(window);
			}
		} else {
			// window was not on this workspace, is it now?
			if (workspaces_on_workspace(i, newWorkspaces)) {
				fWorkspaces[i].Windows().AddWindow(window);
				window->SetCurrentWorkspace(fCurrentWorkspace);

				if (i == CurrentWorkspace()) {
					// this only affects other windows if this windows has floating or
					// modal windows that need to be shown as well
					// TODO: take care of this
					_ShowWindow(window, FrontWindow() == window);
				}
			}
		}
	}

	WriteUnlockWindows();
}


void
Desktop::SetWindowWorkspaces(WindowLayer* window, uint32 workspaces)
{
	BAutolock _(this);

	if (workspaces == B_CURRENT_WORKSPACE)
		workspaces = workspace_to_workspaces(CurrentWorkspace());

	WriteLockWindows();
	_ChangeWindowWorkspaces(window, window->Workspaces(), workspaces);
	window->SetWorkspaces(workspaces);
	WriteUnlockWindows();
}


void
Desktop::AddWindow(WindowLayer *window)
{
	BAutolock _(this);

	fAllWindows.AddWindow(window);

	if (window->Workspaces() == B_CURRENT_WORKSPACE)
		window->SetWorkspaces(workspace_to_workspaces(CurrentWorkspace()));

	_ChangeWindowWorkspaces(window, 0, window->Workspaces());
}


void
Desktop::RemoveWindow(WindowLayer *window)
{
	BAutolock _(this);

	fAllWindows.RemoveWindow(window);
	_ChangeWindowWorkspaces(window, window->Workspaces(), 0);

	// make sure this window won't get any events anymore

	EventDispatcher().RemoveTarget(window->EventTarget());
}


void
Desktop::SetWindowLook(WindowLayer *window, window_look newLook)
{
	if (window->Look() == newLook)
		return;

	if (!WriteLockWindows())
		return;

	BRegion dirty;
	window->SetLook(newLook, &dirty);
		// TODO: test what happens when the window
		// finds out it needs to resize itself...

	BRegion stillAvailableOnScreen;
	_RebuildClippingForAllWindows(stillAvailableOnScreen);
	_SetBackground(stillAvailableOnScreen);

	_TriggerWindowRedrawing(dirty);

	//_WindowsChanged();

	WriteUnlockWindows();
}


void
Desktop::SetWindowFeel(WindowLayer *window, window_feel newFeel)
{
	if (window->Feel() == newFeel)
		return;

	BAutolock _(this);

	window->SetFeel(newFeel);

	// TODO: implement window feels!
}


void
Desktop::SetWindowFlags(WindowLayer *window, uint32 newFlags)
{
	if (window->Flags() == newFlags)
		return;

	if (!WriteLockWindows())
		return;

	BRegion dirty;
	window->SetFlags(newFlags, &dirty);
		// TODO: test what happens when the window
		// finds out it needs to resize itself...

	BRegion stillAvailableOnScreen;
	_RebuildClippingForAllWindows(stillAvailableOnScreen);
	_SetBackground(stillAvailableOnScreen);

	_TriggerWindowRedrawing(dirty);

	//_WindowsChanged();

	WriteUnlockWindows();
}


WindowLayer*
Desktop::WindowAt(BPoint where)
{
	for (WindowLayer* window = _CurrentWindows().LastWindow(); window;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (window->VisibleRegion().Contains(where))
			return window;
	}

	return NULL;
}


void
Desktop::SetMouseEventWindow(WindowLayer* window)
{
	fMouseEventWindow = window;
}


WindowLayer *
Desktop::FindWindowLayerByClientToken(int32 token, team_id teamID)
{
	BAutolock locker(this);

	for (WindowLayer *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->ServerWindow()->ClientToken() == token
			&& window->ServerWindow()->ClientTeam() == teamID)
			return window;
	}

	return NULL;
}


void
Desktop::WriteWindowList(team_id team, BPrivate::LinkSender& sender)
{
	BAutolock locker(this);

	// compute the number of windows

	int32 count = 0;

	for (WindowLayer *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (team < B_OK || window->ServerWindow()->ClientTeam() == team)
			count++;
	}

	// write list

	sender.StartMessage(B_OK);
	sender.Attach<int32>(count);

	for (WindowLayer *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (team >= B_OK && window->ServerWindow()->ClientTeam() != team)
			continue;

		sender.Attach<int32>(window->ServerWindow()->ServerToken());
	}

	sender.Flush();
}


void
Desktop::WriteWindowInfo(int32 serverToken, BPrivate::LinkSender& sender)
{
	BAutolock locker(this);
	BAutolock tokenLocker(BPrivate::gDefaultTokens);

	::ServerWindow* window;
	if (BPrivate::gDefaultTokens.GetToken(serverToken,
			B_SERVER_TOKEN, (void**)&window) != B_OK) {
		sender.StartMessage(B_ENTRY_NOT_FOUND);
		sender.Flush();
		return;
	}

	window_info info;
	window->GetInfo(info);

	int32 length = window->Title() ? strlen(window->Title()) : 0;

	sender.StartMessage(B_OK);
	sender.Attach<int32>(sizeof(window_info) + length + 1);
	sender.Attach(&info, sizeof(window_info));
	if (length > 0)
		sender.Attach(window->Title(), length + 1);
	else
		sender.Attach<char>('\0');
	sender.Flush();
}


void
Desktop::MarkDirty(BRegion& region)
{
	if (region.CountRects() == 0)
		return;

	if (WriteLockWindows()) {
		// send redraw messages to all windows intersecting the dirty region
		_TriggerWindowRedrawing(region);

		WriteUnlockWindows();
	}
}


void
Desktop::_RebuildClippingForAllWindows(BRegion& stillAvailableOnScreen)
{
	// the available region on screen starts with the entire screen area
	// each window on the screen will take a portion from that area

	// figure out what the entire screen area is
	stillAvailableOnScreen = fScreenRegion;

	// set clipping of each window
	for (WindowLayer* window = _CurrentWindows().LastWindow(); window != NULL;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (!window->IsHidden()) {
			window->SetClipping(&stillAvailableOnScreen);
			// that windows region is not available on screen anymore
			stillAvailableOnScreen.Exclude(&window->VisibleRegion());
		}
	}
}


void
Desktop::_TriggerWindowRedrawing(BRegion& newDirtyRegion)
{
	// send redraw messages to all windows intersecting the dirty region
	for (WindowLayer* window = _CurrentWindows().LastWindow(); window != NULL;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (!window->IsHidden()
			&& newDirtyRegion.Intersects(window->VisibleRegion().Frame()))
			window->ProcessDirtyRegion(newDirtyRegion);
	}
}


void
Desktop::_SetBackground(BRegion& background)
{
	// NOTE: the drawing operation is caried out
	// in the clipping region rebuild, but it is
	// ok actually, because it also avoids trails on
	// moving windows

	// remember the region not covered by any windows
	// and redraw the dirty background 
	BRegion dirtyBackground(background);
	dirtyBackground.Exclude(&fBackgroundRegion);
	dirtyBackground.IntersectWith(&background);
	fBackgroundRegion = background;
	if (dirtyBackground.Frame().IsValid()) {
		if (GetDrawingEngine()->Lock()) {
			GetDrawingEngine()->ConstrainClippingRegion(NULL);
			GetDrawingEngine()->FillRegion(dirtyBackground,
				fWorkspaces[fCurrentWorkspace].Color());

			GetDrawingEngine()->Unlock();
		}
	}
}
