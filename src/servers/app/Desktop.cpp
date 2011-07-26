/*
 * Copyright 2001-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>
 *		Brecht Machiels <brecht@mos6581.org>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


/*!	Class used to encapsulate desktop management */


#include "Desktop.h"
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <Debug.h>
#include <debugger.h>
#include <DirectWindow.h>
#include <Entry.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Region.h>
#include <Roster.h>

#include <PrivateScreen.h>
#include <ServerProtocol.h>
#include <ViewPrivate.h>
#include <WindowInfo.h>

#include "AppServer.h"
#include "ClickTarget.h"
#include "DecorManager.h"
#include "DesktopSettingsPrivate.h"
#include "DrawingEngine.h"
#include "FontManager.h"
#include "HWInterface.h"
#include "InputManager.h"
#include "Screen.h"
#include "ServerApp.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerWindow.h"
#include "SystemPalette.h"
#include "WindowPrivate.h"
#include "Window.h"
#include "Workspace.h"
#include "WorkspacesView.h"

#if TEST_MODE
#	include "EventStream.h"
#endif


//#define DEBUG_DESKTOP
#ifdef DEBUG_DESKTOP
#	define STRACE(a) printf a
#else
#	define STRACE(a) ;
#endif


static inline float
square_vector_length(float x, float y)
{
	return x * x + y * y;
}


static inline float
square_distance(const BPoint& a, const BPoint& b)
{
	return square_vector_length(a.x - b.x, a.y - b.y);
}


class KeyboardFilter : public EventFilter {
	public:
		KeyboardFilter(Desktop* desktop);

		virtual filter_result Filter(BMessage* message, EventTarget** _target,
			int32* _viewToken, BMessage* latestMouseMoved);
		virtual void RemoveTarget(EventTarget* target);

	private:
		void _UpdateFocus(int32 key, uint32 modifiers, EventTarget** _target);

		Desktop*		fDesktop;
		EventTarget*	fLastFocus;
		bigtime_t		fTimestamp;
};


class MouseFilter : public EventFilter {
public:
	MouseFilter(Desktop* desktop);

	virtual filter_result Filter(BMessage* message, EventTarget** _target,
		int32* _viewToken, BMessage* latestMouseMoved);

private:
	Desktop*	fDesktop;
	int32		fLastClickButtons;
	int32		fLastClickModifiers;
	int32		fResetClickCount;
	BPoint		fLastClickPoint;
	ClickTarget	fLastClickTarget;
};


//	#pragma mark -


KeyboardFilter::KeyboardFilter(Desktop* desktop)
	:
	fDesktop(desktop),
	fLastFocus(NULL),
	fTimestamp(0)
{
}


void
KeyboardFilter::_UpdateFocus(int32 key, uint32 modifiers, EventTarget** _target)
{
	if (!fDesktop->LockSingleWindow())
		return;

	EventTarget* focus = fDesktop->KeyboardEventTarget();

#if 0
	bigtime_t now = system_time();

	// TODO: this is a try to not steal focus from the current window
	//	in case you enter some text and a window pops up you haven't
	//	triggered yourself (like a pop-up window in your browser while
	//	you're typing a password in another window) - maybe this should
	//	be done differently, though (using something like B_LOCK_WINDOW_FOCUS)
	//	(at least B_WINDOW_ACTIVATED must be postponed)

	if (fLastFocus == NULL
		|| (focus != fLastFocus && now - fTimestamp > 100000)) {
		// if the time span between the key presses is very short
		// we keep our previous focus alive - this is safe even
		// if the target doesn't exist anymore, as we don't reset
		// it, and the event focus passed in is always valid (or NULL)
		*_target = focus;
		fLastFocus = focus;
	}
#endif
	*_target = focus;
	fLastFocus = focus;

	fDesktop->UnlockSingleWindow();

#if 0
	// we always allow to switch focus after the enter key has pressed
	if (key == B_ENTER || modifiers == B_COMMAND_KEY
		|| modifiers == B_CONTROL_KEY || modifiers == B_OPTION_KEY)
		fTimestamp = 0;
	else
		fTimestamp = now;
#endif
}


filter_result
KeyboardFilter::Filter(BMessage* message, EventTarget** _target,
	int32* /*_viewToken*/, BMessage* /*latestMouseMoved*/)
{
	int32 key = 0;
	int32 modifiers = 0;

	message->FindInt32("key", &key);
	message->FindInt32("modifiers", &modifiers);

	if ((message->what == B_KEY_DOWN || message->what == B_UNMAPPED_KEY_DOWN)) {
		// Check for safe video mode (cmd + ctrl + escape)
		if (key == 0x01 && (modifiers & B_COMMAND_KEY) != 0
			&& (modifiers & B_CONTROL_KEY) != 0) {
			system("screenmode --fall-back &");
			return B_SKIP_MESSAGE;
		}

		bool takeWindow = (modifiers & B_SHIFT_KEY) != 0
			|| fDesktop->MouseEventWindow() != NULL;
		if (key >= B_F1_KEY && key <= B_F12_KEY) {
			// workspace change

#if !TEST_MODE
			if ((modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY))
					== B_COMMAND_KEY)
#else
			if ((modifiers & B_CONTROL_KEY) != 0)
#endif
			{
				STRACE(("Set Workspace %ld\n", key - 1));

				fDesktop->SetWorkspaceAsync(key - B_F1_KEY, takeWindow);
				return B_SKIP_MESSAGE;
			}
		} if (key == 0x11
			&& (modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY))
					== B_COMMAND_KEY) {
			// switch to previous workspace (command + `)
			fDesktop->SetWorkspaceAsync(-1, takeWindow);
			return B_SKIP_MESSAGE;
		}
	}

	if (message->what == B_KEY_DOWN
		|| message->what == B_MODIFIERS_CHANGED
		|| message->what == B_UNMAPPED_KEY_DOWN
		|| message->what == B_INPUT_METHOD_EVENT)
		_UpdateFocus(key, modifiers, _target);

	return fDesktop->KeyEvent(message->what, key, modifiers);
}


void
KeyboardFilter::RemoveTarget(EventTarget* target)
{
	if (target == fLastFocus)
		fLastFocus = NULL;
}


//	#pragma mark -


MouseFilter::MouseFilter(Desktop* desktop)
	:
	fDesktop(desktop),
	fLastClickButtons(0),
	fLastClickModifiers(0),
	fResetClickCount(0),
	fLastClickPoint(),
	fLastClickTarget()
{
}


filter_result
MouseFilter::Filter(BMessage* message, EventTarget** _target, int32* _viewToken,
	BMessage* latestMouseMoved)
{
	BPoint where;
	if (message->FindPoint("where", &where) != B_OK)
		return B_DISPATCH_MESSAGE;

	int32 buttons;
	if (message->FindInt32("buttons", &buttons) != B_OK)
		buttons = 0;

	if (!fDesktop->LockAllWindows())
		return B_DISPATCH_MESSAGE;

	int32 viewToken = B_NULL_TOKEN;

	Window* window = fDesktop->MouseEventWindow();
	if (window == NULL)
		window = fDesktop->WindowAt(where);

	if (window != NULL) {
		// dispatch event to the window
		switch (message->what) {
			case B_MOUSE_DOWN:
			{
				int32 windowToken = window->ServerWindow()->ServerToken();

				// First approximation of click count validation. We reset the
				// click count when modifiers or pressed buttons have changed
				// or when we've got a different click target, or when the
				// previous click location is too far from the new one. We can
				// only check the window of the click target here; we'll recheck
				// after asking the window.
				int32 modifiers = message->FindInt32("modifiers");

				int32 originalClickCount = message->FindInt32("clicks");
				if (originalClickCount <= 0)
					originalClickCount = 1;

				int32 clickCount = originalClickCount;
				if (clickCount > 1) {
					if (modifiers != fLastClickModifiers
						|| buttons != fLastClickButtons
						|| !fLastClickTarget.IsValid()
						|| fLastClickTarget.WindowToken() != windowToken
						|| square_distance(where, fLastClickPoint) >= 16
						|| clickCount - fResetClickCount < 1) {
						clickCount = 1;
					} else
						clickCount -= fResetClickCount;
				}

				// notify the window
				ClickTarget clickTarget;
				window->MouseDown(message, where, fLastClickTarget, clickCount,
					clickTarget);

				// If the click target changed, always reset the click count.
				if (clickCount != 1 && clickTarget != fLastClickTarget)
					clickCount = 1;

				// update our click count management attributes
				fResetClickCount = originalClickCount - clickCount;
				fLastClickTarget = clickTarget;
				fLastClickButtons = buttons;
				fLastClickModifiers = modifiers;
				fLastClickPoint = where;

				// get the view token from the click target
				if (clickTarget.GetType() == ClickTarget::TYPE_WINDOW_CONTENTS)
					viewToken = clickTarget.WindowElement();

				// update the message's "clicks" field, if necessary
				if (clickCount != originalClickCount) {
					if (message->HasInt32("clicks"))
						message->ReplaceInt32("clicks", clickCount);
					else
						message->AddInt32("clicks", clickCount);
				}

				// notify desktop listeners
				fDesktop->NotifyMouseDown(window, message, where);
				break;
			}

			case B_MOUSE_UP:
				window->MouseUp(message, where, &viewToken);
				if (buttons == 0)
					fDesktop->SetMouseEventWindow(NULL);
				fDesktop->NotifyMouseUp(window, message, where);
				break;

			case B_MOUSE_MOVED:
				window->MouseMoved(message, where, &viewToken,
					latestMouseMoved == NULL || latestMouseMoved == message,
					false);
				fDesktop->NotifyMouseMoved(window, message, where);
				break;
		}

		if (viewToken != B_NULL_TOKEN) {
			fDesktop->SetViewUnderMouse(window, viewToken);

			*_viewToken = viewToken;
			*_target = &window->EventTarget();
		}
	} else if (message->what == B_MOUSE_DOWN) {
		// the mouse-down didn't hit a window -- reset the click target
		fResetClickCount = 0;
		fLastClickTarget = ClickTarget();
		fLastClickButtons = message->FindInt32("buttons");
		fLastClickModifiers = message->FindInt32("modifiers");
		fLastClickPoint = where;
	}

	if (window == NULL || viewToken == B_NULL_TOKEN) {
		// mouse is not over a window or over a decorator
		fDesktop->SetViewUnderMouse(window, B_NULL_TOKEN);
		fDesktop->SetCursor(NULL);

		*_target = NULL;
	}

	fDesktop->SetLastMouseState(where, buttons, window);

	fDesktop->NotifyMouseEvent(message);

	fDesktop->UnlockAllWindows();

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


static inline uint32
workspace_to_workspaces(int32 index)
{
	return 1UL << index;
}


static inline bool
workspace_in_workspaces(int32 index, uint32 workspaces)
{
	return (workspaces & (1UL << index)) != 0;
}


//	#pragma mark -


Desktop::Desktop(uid_t userID, const char* targetScreen)
	:
	MessageLooper("desktop"),

	fUserID(userID),
	fTargetScreen(strdup(targetScreen)),
	fSettings(NULL),
	fSharedReadOnlyArea(-1),
	fApplicationsLock("application list"),
	fShutdownSemaphore(-1),
	fShutdownCount(0),
	fScreenLock("screen lock"),
	fDirectScreenLock("direct screen lock"),
	fDirectScreenTeam(-1),
	fCurrentWorkspace(0),
	fPreviousWorkspace(0),
	fAllWindows(kAllWindowList),
	fSubsetWindows(kSubsetList),
	fFocusList(kFocusList),
	fWorkspacesViews(false),

	fWorkspacesLock("workspaces list"),
	fWindowLock("window lock"),

	fMouseEventWindow(NULL),
	fWindowUnderMouse(NULL),
	fLockedFocusWindow(NULL),
	fViewUnderMouse(B_NULL_TOKEN),
	fLastMousePosition(B_ORIGIN),
	fLastMouseButtons(0),

	fFocus(NULL),
	fFront(NULL),
	fBack(NULL)
{
	memset(fLastWorkspaceFocus, 0, sizeof(fLastWorkspaceFocus));

	char name[B_OS_NAME_LENGTH];
	Desktop::_GetLooperName(name, sizeof(name));

	fMessagePort = create_port(DEFAULT_MONITOR_PORT_SIZE, name);
	if (fMessagePort < B_OK)
		return;

	fLink.SetReceiverPort(fMessagePort);

	// register listeners
	RegisterListener(&fStackAndTile);

	const DesktopListenerList& newListeners
		= gDecorManager.GetDesktopListeners();
	for (int i = 0; i < newListeners.CountItems(); i++)
 		RegisterListener(newListeners.ItemAt(i));
}


Desktop::~Desktop()
{
	delete fSettings;

	delete_area(fSharedReadOnlyArea);
	delete_port(fMessagePort);
	gFontManager->DetachUser(fUserID);

	free(fTargetScreen);
}


void
Desktop::RegisterListener(DesktopListener* listener)
{
	DesktopObservable::RegisterListener(listener, this);
}


status_t
Desktop::Init()
{
	if (fMessagePort < B_OK)
		return fMessagePort;

	// the system palette needs to be initialized before the
	// desktop settings, since it is used there already
	InitializeColorMap();

	const size_t areaSize = B_PAGE_SIZE;
	char name[B_OS_NAME_LENGTH];
	snprintf(name, sizeof(name), "d:%d:shared read only", fUserID);
	fSharedReadOnlyArea = create_area(name, (void **)&fServerReadOnlyMemory,
		B_ANY_ADDRESS, areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (fSharedReadOnlyArea < B_OK)
		return fSharedReadOnlyArea;

	gFontManager->AttachUser(fUserID);

	fSettings = new DesktopSettingsPrivate(fServerReadOnlyMemory);

	for (int32 i = 0; i < kMaxWorkspaces; i++) {
		_Windows(i).SetIndex(i);
		fWorkspaces[i].RestoreConfiguration(*fSettings->WorkspacesMessage(i));
	}

	fVirtualScreen.SetConfiguration(*this,
		fWorkspaces[0].CurrentScreenConfiguration());

	if (fVirtualScreen.HWInterface() == NULL) {
		debug_printf("Could not initialize graphics output. Exiting.\n");
		return B_ERROR;
	}

	fVirtualScreen.HWInterface()->MoveCursorTo(
		fVirtualScreen.Frame().Width() / 2,
		fVirtualScreen.Frame().Height() / 2);

#if TEST_MODE
	gInputManager->AddStream(new InputServerStream);
#endif

	EventStream* stream = fVirtualScreen.HWInterface()->CreateEventStream();
	if (stream == NULL)
		stream = gInputManager->GetStream();

	fEventDispatcher.SetDesktop(this);
	fEventDispatcher.SetTo(stream);
	if (fEventDispatcher.InitCheck() != B_OK)
		_LaunchInputServer();

	fEventDispatcher.SetHWInterface(fVirtualScreen.HWInterface());

	fEventDispatcher.SetMouseFilter(new MouseFilter(this));
	fEventDispatcher.SetKeyboardFilter(new KeyboardFilter(this));

	// draw the background

	fScreenRegion = fVirtualScreen.Frame();

	BRegion stillAvailableOnScreen;
	_RebuildClippingForAllWindows(stillAvailableOnScreen);
	_SetBackground(stillAvailableOnScreen);

	SetCursor(NULL);
		// this will set the default cursor

	fVirtualScreen.HWInterface()->SetCursorVisible(true);

	return B_OK;
}


/*!	\brief Send a quick (no attachments) message to all applications.

	Quite useful for notification for things like server shutdown, system
	color changes, etc.
*/
void
Desktop::BroadcastToAllApps(int32 code)
{
	BAutolock locker(fApplicationsLock);

	for (int32 i = fApplications.CountItems(); i-- > 0;) {
		fApplications.ItemAt(i)->PostMessage(code);
	}
}


/*!	\brief Send a quick (no attachments) message to all windows.
*/
void
Desktop::BroadcastToAllWindows(int32 code)
{
	AutoWriteLocker _(fWindowLock);

	for (Window* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		window->ServerWindow()->PostMessage(code);
	}
}


filter_result
Desktop::KeyEvent(uint32 what, int32 key, int32 modifiers)
{
	filter_result result = B_DISPATCH_MESSAGE;
	if (LockAllWindows()) {
		Window* window = MouseEventWindow();
		if (window == NULL)
			window = WindowAt(fLastMousePosition);

		if (window != NULL) {
			if (what == B_MODIFIERS_CHANGED)
				window->ModifiersChanged(modifiers);
		}

		if (NotifyKeyPressed(what, key, modifiers))
			result = B_SKIP_MESSAGE;

		UnlockAllWindows();
	}

	return result;
}


// #pragma mark - Mouse and cursor methods


void
Desktop::SetCursor(ServerCursor* newCursor)
{
	if (newCursor == NULL)
		newCursor = fCursorManager.GetCursor(B_CURSOR_ID_SYSTEM_DEFAULT);

	if (newCursor == fCursor)
		return;

	fCursor = newCursor;

	if (fManagementCursor.Get() == NULL)
		HWInterface()->SetCursor(newCursor);
}


ServerCursorReference
Desktop::Cursor() const
{
	return fCursor;
}


void
Desktop::SetManagementCursor(ServerCursor* newCursor)
{
	if (newCursor == fManagementCursor)
		return;

	fManagementCursor = newCursor;

	HWInterface()->SetCursor(newCursor != NULL ? newCursor : fCursor.Get());
}


void
Desktop::SetLastMouseState(const BPoint& position, int32 buttons,
	Window* windowUnderMouse)
{
	// The all-window-lock is write-locked.
	fLastMousePosition = position;
	fLastMouseButtons = buttons;

	if (fLastMouseButtons == 0 && fLockedFocusWindow) {
		fLockedFocusWindow = NULL;
		if (fSettings->FocusFollowsMouse())
			SetFocusWindow(windowUnderMouse);
	}
}


void
Desktop::GetLastMouseState(BPoint* position, int32* buttons) const
{
	*position = fLastMousePosition;
	*buttons = fLastMouseButtons;
}


//	#pragma mark - Screen methods


status_t
Desktop::SetScreenMode(int32 workspace, int32 id, const display_mode& mode,
	bool makeDefault)
{
	AutoWriteLocker _(fWindowLock);

	if (workspace == B_CURRENT_WORKSPACE_INDEX)
		workspace = fCurrentWorkspace;

	if (workspace < 0 || workspace >= kMaxWorkspaces)
		return B_BAD_VALUE;

	Screen* screen = fVirtualScreen.ScreenByID(id);
	if (screen == NULL)
		return B_NAME_NOT_FOUND;

	// Check if the mode has actually changed

	if (workspace == fCurrentWorkspace) {
		// retrieve from current screen
		display_mode oldMode;
		screen->GetMode(oldMode);

		if (!memcmp(&oldMode, &mode, sizeof(display_mode)))
			return B_OK;

		// Set the new one

		_SuspendDirectFrameBufferAccess();

		AutoWriteLocker locker(fScreenLock);

		status_t status = screen->SetMode(mode);
		if (status != B_OK) {
			locker.Unlock();

			_ResumeDirectFrameBufferAccess();
			return status;
		}
	} else {
		// retrieve from settings
		screen_configuration* configuration
			= fWorkspaces[workspace].CurrentScreenConfiguration().CurrentByID(
				screen->ID());
		if (configuration != NULL
			&& !memcmp(&configuration->mode, &mode, sizeof(display_mode)))
			return B_OK;
	}

	// Update our configurations

	monitor_info info;
	bool hasInfo = screen->GetMonitorInfo(info) == B_OK;

	fWorkspaces[workspace].CurrentScreenConfiguration().Set(id,
		hasInfo ? &info : NULL, screen->Frame(), mode);
	if (makeDefault) {
		fWorkspaces[workspace].StoredScreenConfiguration().Set(id,
			hasInfo ? &info : NULL, screen->Frame(), mode);
		StoreWorkspaceConfiguration(workspace);
	}

	_ScreenChanged(screen);
	if (workspace == fCurrentWorkspace)
		_ResumeDirectFrameBufferAccess();

	return B_OK;
}


status_t
Desktop::GetScreenMode(int32 workspace, int32 id, display_mode& mode)
{
	AutoReadLocker _(fScreenLock);

	if (workspace == B_CURRENT_WORKSPACE_INDEX)
		workspace = fCurrentWorkspace;

	if (workspace < 0 || workspace >= kMaxWorkspaces)
		return B_BAD_VALUE;

	if (workspace == fCurrentWorkspace) {
		// retrieve from current screen
		Screen* screen = fVirtualScreen.ScreenByID(id);
		if (screen == NULL)
			return B_NAME_NOT_FOUND;

		screen->GetMode(mode);
		return B_OK;
	}

	// retrieve from settings
	screen_configuration* configuration
		= fWorkspaces[workspace].CurrentScreenConfiguration().CurrentByID(id);
	if (configuration == NULL)
		return B_NAME_NOT_FOUND;

	mode = configuration->mode;
	return B_OK;
}


status_t
Desktop::GetScreenFrame(int32 workspace, int32 id, BRect& frame)
{
	AutoReadLocker _(fScreenLock);

	if (workspace == B_CURRENT_WORKSPACE_INDEX)
		workspace = fCurrentWorkspace;

	if (workspace < 0 || workspace >= kMaxWorkspaces)
		return B_BAD_VALUE;

	if (workspace == fCurrentWorkspace) {
		// retrieve from current screen
		Screen* screen = fVirtualScreen.ScreenByID(id);
		if (screen == NULL)
			return B_NAME_NOT_FOUND;

		frame = screen->Frame();
		return B_OK;
	}

	// retrieve from settings
	screen_configuration* configuration
		= fWorkspaces[workspace].CurrentScreenConfiguration().CurrentByID(id);
	if (configuration == NULL)
		return B_NAME_NOT_FOUND;

	frame = configuration->frame;
	return B_OK;
}


void
Desktop::RevertScreenModes(uint32 workspaces)
{
	if (workspaces == 0)
		return;

	AutoWriteLocker _(fWindowLock);

	for (int32 workspace = 0; workspace < kMaxWorkspaces; workspace++) {
		if ((workspaces & (1U << workspace)) == 0)
			continue;

		// Revert all screens on this workspace

		// TODO: ideally, we would know which screens to revert - this way, too
		// many of them could be reverted

		for (int32 index = 0; index < fVirtualScreen.CountScreens(); index++) {
			Screen* screen = fVirtualScreen.ScreenAt(index);

			// retrieve configurations
			screen_configuration* stored = fWorkspaces[workspace]
				.StoredScreenConfiguration().CurrentByID(screen->ID());
			screen_configuration* current = fWorkspaces[workspace]
				.CurrentScreenConfiguration().CurrentByID(screen->ID());

			if ((stored != NULL && current != NULL
					&& !memcmp(&stored->mode, &current->mode,
							sizeof(display_mode)))
				|| (stored == NULL && current == NULL))
				continue;

			if (stored == NULL) {
				fWorkspaces[workspace].CurrentScreenConfiguration()
					.Remove(current);

				if (workspace == fCurrentWorkspace) {
					_SuspendDirectFrameBufferAccess();
					_SetCurrentWorkspaceConfiguration();
					_ResumeDirectFrameBufferAccess();
				}
			} else
				SetScreenMode(workspace, screen->ID(), stored->mode, false);
		}
	}
}


status_t
Desktop::LockDirectScreen(team_id team)
{
	// TODO: BWindowScreens should use the same mechanism as BDirectWindow,
	// which would make this method superfluous.

	status_t status = fDirectScreenLock.LockWithTimeout(1000000L);
	if (status == B_OK)
		fDirectScreenTeam = team;

	return status;
}


status_t
Desktop::UnlockDirectScreen(team_id team)
{
	if (fDirectScreenTeam == team) {
		fDirectScreenLock.Unlock();
		fDirectScreenTeam = -1;
		return B_OK;
	}

	return B_PERMISSION_DENIED;
}


// #pragma mark - Workspaces methods


/*!	Changes the current workspace to the one specified by \a index.
*/
void
Desktop::SetWorkspaceAsync(int32 index, bool moveFocusWindow)
{
	BPrivate::LinkSender link(MessagePort());
	link.StartMessage(AS_ACTIVATE_WORKSPACE);
	link.Attach<int32>(index);
	link.Attach<bool>(moveFocusWindow);
	link.Flush();
}


/*!	Changes the current workspace to the one specified by \a index.
	You must not hold any window lock when calling this method.
*/
void
Desktop::SetWorkspace(int32 index, bool moveFocusWindow)
{
	LockAllWindows();
	DesktopSettings settings(this);

	if (index < 0 || index >= settings.WorkspacesCount()
		|| index == fCurrentWorkspace) {
		UnlockAllWindows();
		return;
	}

	_SetWorkspace(index, moveFocusWindow);
	UnlockAllWindows();

	_SendFakeMouseMoved();
}


status_t
Desktop::SetWorkspacesLayout(int32 newColumns, int32 newRows)
{
	int32 newCount = newColumns * newRows;
	if (newCount < 1 || newCount > kMaxWorkspaces)
		return B_BAD_VALUE;

	if (!LockAllWindows())
		return B_ERROR;

	fSettings->SetWorkspacesLayout(newColumns, newRows);

	// either update the workspaces window, or switch to
	// the last available workspace - which will update
	// the workspaces window automatically
	bool workspaceChanged = CurrentWorkspace() >= newCount;
	if (workspaceChanged)
		_SetWorkspace(newCount - 1);
	else
		_WindowChanged(NULL);

	UnlockAllWindows();

	if (workspaceChanged)
		_SendFakeMouseMoved();

	return B_OK;
}


/*!	Returns the virtual screen frame of the workspace specified by \a index.
*/
BRect
Desktop::WorkspaceFrame(int32 index) const
{
	BRect frame;
	if (index == fCurrentWorkspace)
		frame = fVirtualScreen.Frame();
	else if (index >= 0 && index < fSettings->WorkspacesCount()) {
		BMessage screenData;
		if (fSettings->WorkspacesMessage(index)->FindMessage("screen",
				&screenData) != B_OK
			|| screenData.FindRect("frame", &frame) != B_OK) {
			frame = fVirtualScreen.Frame();
		}
	}

	return frame;
}


/*!	\brief Stores the workspace configuration.
	You must hold the window lock when calling this method.
*/
void
Desktop::StoreWorkspaceConfiguration(int32 index)
{
	// Retrieve settings

	BMessage settings;
	fWorkspaces[index].StoreConfiguration(settings);

	// and store them

	fSettings->SetWorkspacesMessage(index, settings);
	fSettings->Save(kWorkspacesSettings);
}


void
Desktop::AddWorkspacesView(WorkspacesView* view)
{
	if (view->Window() == NULL || view->Window()->IsHidden())
		return;

	BAutolock _(fWorkspacesLock);

	if (!fWorkspacesViews.HasItem(view))
		fWorkspacesViews.AddItem(view);
}


void
Desktop::RemoveWorkspacesView(WorkspacesView* view)
{
	BAutolock _(fWorkspacesLock);
	fWorkspacesViews.RemoveItem(view);
}


//	#pragma mark - Methods for Window manipulation


/*!	\brief Activates or focusses the window based on the pointer position.
*/
void
Desktop::SelectWindow(Window* window)
{
	if (fSettings->MouseMode() == B_CLICK_TO_FOCUS_MOUSE) {
		// Only bring the window to front when it is not the window under the
		// mouse pointer. This should result in sensible behaviour.
		if (window != fWindowUnderMouse
			|| (window == fWindowUnderMouse && window != FocusWindow()))
			ActivateWindow(window);
		else
			SetFocusWindow(window);
	} else
		ActivateWindow(window);
}


/*!	\brief Tries to move the specified window to the front of the screen,
		and make it the focus window.

	If there are any modal windows on this screen, it might not actually
	become the frontmost window, though, as modal windows stay in front
	of their subset.
*/
void
Desktop::ActivateWindow(Window* window)
{
	STRACE(("ActivateWindow(%p, %s)\n", window, window
		? window->Title() : "<none>"));

	if (window == NULL) {
		fBack = NULL;
		fFront = NULL;
		return;
	}
	if (window->Workspaces() == 0 && window->IsNormal())
		return;

	AutoWriteLocker _(fWindowLock);

	NotifyWindowActivated(window);

	bool windowOnOtherWorkspace = !window->InWorkspace(fCurrentWorkspace);
	if (windowOnOtherWorkspace
		&& (window->Flags() & B_NOT_ANCHORED_ON_ACTIVATE) == 0) {
		if ((window->Flags() & B_NO_WORKSPACE_ACTIVATION) == 0) {
			// Switch to the workspace on which this window is
			// (we'll take the first one that the window is on)
			uint32 workspaces = window->Workspaces();
			for (int32 i = 0; i < fSettings->WorkspacesCount(); i++) {
				uint32 workspace = workspace_to_workspaces(i);
				if (workspaces & workspace) {
					SetWorkspace(i);
					windowOnOtherWorkspace = false;
					break;
				}
			}
		} else
			return;
	}

	if (windowOnOtherWorkspace) {
		if (!window->IsNormal()) {
			// Bring a window to front that this floating window belongs to
			Window* front = _LastFocusSubsetWindow(window);
			if (front == NULL) {
				// We can't do anything about those.
				return;
			}

			ActivateWindow(front);

			if (!window->InWorkspace(fCurrentWorkspace)) {
				// This window can't be made active
				return;
			}
		} else {
			// Bring the window to the current workspace
			// TODO: what if this window is on multiple workspaces?!?
			uint32 workspaces = workspace_to_workspaces(fCurrentWorkspace);
			SetWindowWorkspaces(window, workspaces);
		}
	}

	if (window->IsMinimized()) {
		// Unlike WindowAction(), this is called from the application itself,
		// so we will just unminimize the window here.
		window->SetMinimized(false);
		ShowWindow(window);
	}

	if (window == FrontWindow()) {
		// see if there is a normal B_AVOID_FRONT window still in front of us
		Window* avoidsFront = window->NextWindow(fCurrentWorkspace);
		while (avoidsFront && avoidsFront->IsNormal()
			&& (avoidsFront->Flags() & B_AVOID_FRONT) == 0) {
			avoidsFront = avoidsFront->NextWindow(fCurrentWorkspace);
		}

		if (avoidsFront == NULL) {
			// we're already the frontmost window, we might just not have focus
			// yet
			if ((window->Flags() & B_AVOID_FOCUS) == 0)
				SetFocusWindow(window);
			return;
		}
	}

	WindowList windows(kWorkingList);
	Window* frontmost = window->Frontmost();

	CurrentWindows().RemoveWindow(window);
	windows.AddWindow(window);
	window->MoveToTopStackLayer();

	if (frontmost != NULL && frontmost->IsModal()) {
		// all modal windows follow their subsets to the front
		// (ie. they are staying in front of them, but they are
		// not supposed to change their order because of that)

		Window* nextModal;
		for (Window* modal = frontmost; modal != NULL; modal = nextModal) {
			// get the next modal window
			nextModal = modal->NextWindow(fCurrentWorkspace);
			while (nextModal != NULL && !nextModal->IsModal()) {
				nextModal = nextModal->NextWindow(fCurrentWorkspace);
			}
			if (nextModal != NULL && !nextModal->HasInSubset(window))
				nextModal = NULL;

			CurrentWindows().RemoveWindow(modal);
			windows.AddWindow(modal);
		}
	}

	_BringWindowsToFront(windows, kWorkingList, true);

	if ((window->Flags() & B_AVOID_FOCUS) == 0)
		SetFocusWindow(window);
}


void
Desktop::SendWindowBehind(Window* window, Window* behindOf)
{
	if (!LockAllWindows())
		return;

	// TODO: should the "not in current workspace" be handled anyway?
	//	(the code below would have to be changed then, though)
	if (window == BackWindow()
		|| !window->InWorkspace(fCurrentWorkspace)
		|| (behindOf != NULL && !behindOf->InWorkspace(fCurrentWorkspace))) {
		UnlockAllWindows();
		return;
	}

	// Is this a valid behindOf window?
	if (behindOf != NULL && window->HasInSubset(behindOf))
		behindOf = NULL;

	// what is currently visible of the window
	// might be dirty after the window is send to back
	BRegion dirty(window->VisibleRegion());

	Window* backmost = window->Backmost(behindOf);

	CurrentWindows().RemoveWindow(window);
	CurrentWindows().AddWindow(window, backmost
		? backmost->NextWindow(fCurrentWorkspace) : BackWindow());

	BRegion dummy;
	_RebuildClippingForAllWindows(dummy);

	// mark everything dirty that is no longer visible
	BRegion clean(window->VisibleRegion());
	dirty.Exclude(&clean);
	MarkDirty(dirty);

	_UpdateFronts();
	if (fSettings->MouseMode() == B_FOCUS_FOLLOWS_MOUSE)
		SetFocusWindow(WindowAt(fLastMousePosition));
	else if (fSettings->MouseMode() == B_NORMAL_MOUSE)
		SetFocusWindow(NULL);

	bool sendFakeMouseMoved = false;
	if (FocusWindow() != window)
		sendFakeMouseMoved = true;

	_WindowChanged(window);

	NotifyWindowSentBehind(window, behindOf);

	UnlockAllWindows();

	if (sendFakeMouseMoved)
		_SendFakeMouseMoved();
}


void
Desktop::ShowWindow(Window* window)
{
	if (!window->IsHidden())
		return;

	AutoWriteLocker locker(fWindowLock);

	window->SetHidden(false);
	fFocusList.AddWindow(window);

	// If the window is on the current workspace, we'll show it. Special
	// handling for floating windows, as they can only be shown if their
	// subset is.
	if (window->InWorkspace(fCurrentWorkspace)
		|| (window->IsFloating() && _LastFocusSubsetWindow(window) != NULL)) {
		_ShowWindow(window, true);
		_UpdateSubsetWorkspaces(window);
		ActivateWindow(window);
	} else {
		// then we don't need to send the fake mouse event either
		_WindowChanged(window);
		return;
	}

	if (window->HasWorkspacesViews()) {
		// find workspaces views in view hierarchy
		BAutolock _(fWorkspacesLock);
		window->FindWorkspacesViews(fWorkspacesViews);
	}

	// If the mouse cursor is directly over the newly visible window,
	// we'll send a fake mouse moved message to the window, so that
	// it knows the mouse is over it.

	_SendFakeMouseMoved(window);
}


void
Desktop::HideWindow(Window* window)
{
	if (window->IsHidden())
		return;

	if (!LockAllWindows())
		return;

	window->SetHidden(true);
	fFocusList.RemoveWindow(window);

	if (fMouseEventWindow == window) {
		// Make its decorator lose the current mouse action
		BMessage message;
		int32 viewToken;
		window->MouseUp(&message, fLastMousePosition, &viewToken);

		fMouseEventWindow = NULL;
	}

	if (fLockedFocusWindow == window) {
		// Remove the focus lock so the focus can be changed below
		fLockedFocusWindow = NULL;
	}

	if (window->InWorkspace(fCurrentWorkspace)) {
		_UpdateSubsetWorkspaces(window);
		_HideWindow(window);
		_UpdateFronts();
	} else
		_WindowChanged(window);

	if (FocusWindow() == window)
		SetFocusWindow();

	_WindowRemoved(window);

	if (window->HasWorkspacesViews()) {
		// remove workspaces views from this window
		BObjectList<WorkspacesView> list(false);
		window->FindWorkspacesViews(list);

		BAutolock _(fWorkspacesLock);

		while (WorkspacesView* view = list.RemoveItemAt(0)) {
			fWorkspacesViews.RemoveItem(view);
		}
	}

	UnlockAllWindows();

	if (window == fWindowUnderMouse)
		_SendFakeMouseMoved();
}


void
Desktop::MinimizeWindow(Window* window, bool minimize)
{
	if (!LockAllWindows())
		return;

	if (minimize && !window->IsHidden()) {
		HideWindow(window);
		window->SetMinimized(minimize);
		NotifyWindowMinimized(window, minimize);
	} else if (!minimize && window->IsHidden()) {
		ActivateWindow(window);
			// this will unminimize the window for us
		NotifyWindowMinimized(window, minimize);
	}

	UnlockAllWindows();
}


void
Desktop::MoveWindowBy(Window* window, float x, float y, int32 workspace)
{
	if (x == 0 && y == 0)
		return;

	if (!LockAllWindows())
		return;

	Window* topWindow = window->TopLayerStackWindow();
	if (topWindow)
		window = topWindow;

	if (workspace == -1)
		workspace = fCurrentWorkspace;
	if (!window->IsVisible() || workspace != fCurrentWorkspace) {
		if (workspace != fCurrentWorkspace) {
			// move the window on another workspace - this doesn't change it's
			// current position
			if (window->Anchor(workspace).position == kInvalidWindowPosition)
				window->Anchor(workspace).position = window->Frame().LeftTop();

			window->Anchor(workspace).position += BPoint(x, y);
			window->SetCurrentWorkspace(workspace);
			_WindowChanged(window);
		} else
			window->MoveBy((int32)x, (int32)y);

		NotifyWindowMoved(window);
		UnlockAllWindows();
		return;
	}

	// the dirty region starts with the visible area of the window being moved
	BRegion newDirtyRegion(window->VisibleRegion());

	// stop direct frame buffer access
	bool direct = false;
	if (window->ServerWindow()->IsDirectlyAccessing()) {
		window->ServerWindow()->HandleDirectConnection(B_DIRECT_STOP);
		direct = true;
	}

	window->MoveBy((int32)x, (int32)y);

	BRegion background;
	_RebuildClippingForAllWindows(background);

	// construct the region that is possible to be blitted
	// to move the contents of the window
	BRegion copyRegion(window->VisibleRegion());
	copyRegion.OffsetBy((int32)-x, (int32)-y);
	copyRegion.IntersectWith(&newDirtyRegion);
		// newDirtyRegion == the windows old visible region

	// include the the new visible region of the window being
	// moved into the dirty region (for now)
	newDirtyRegion.Include(&window->VisibleRegion());

	// NOTE: Having all windows locked should prevent any
	// problems with locking the drawing engine here.
	if (GetDrawingEngine()->LockParallelAccess()) {
		GetDrawingEngine()->CopyRegion(&copyRegion, (int32)x, (int32)y);
		GetDrawingEngine()->UnlockParallelAccess();
	}

	// in the dirty region, exclude the parts that we
	// could move by blitting
	copyRegion.OffsetBy((int32)x, (int32)y);
	newDirtyRegion.Exclude(&copyRegion);

	MarkDirty(newDirtyRegion);
	_SetBackground(background);
	_WindowChanged(window);

	// resume direct frame buffer access
	if (direct) {
		// TODO: the clipping actually only changes when we move our window
		// off screen, or behind some other window
		window->ServerWindow()->HandleDirectConnection(
			B_DIRECT_START | B_BUFFER_MOVED | B_CLIPPING_MODIFIED);
	}

	NotifyWindowMoved(window);

	UnlockAllWindows();
}


void
Desktop::ResizeWindowBy(Window* window, float x, float y)
{
	if (x == 0 && y == 0)
		return;

	if (!LockAllWindows())
		return;

	if (!window->IsVisible()) {
		window->ResizeBy((int32)x, (int32)y, NULL);
		NotifyWindowResized(window);
		UnlockAllWindows();
		return;
	}

	// the dirty region for the inside of the window is
	// constructed by the window itself in ResizeBy()
	BRegion newDirtyRegion;
	// track the dirty region outside the window in case
	// it is shrunk in "previouslyOccupiedRegion"
	BRegion previouslyOccupiedRegion(window->VisibleRegion());

	// stop direct frame buffer access
	bool direct = false;
	if (window->ServerWindow()->IsDirectlyAccessing()) {
		window->ServerWindow()->HandleDirectConnection(B_DIRECT_STOP);
		direct = true;
	}

	window->ResizeBy((int32)x, (int32)y, &newDirtyRegion);

	BRegion background;
	_RebuildClippingForAllWindows(background);

	// we just care for the region outside the window
	previouslyOccupiedRegion.Exclude(&window->VisibleRegion());

	// make sure the window cannot mark stuff dirty outside
	// its visible region...
	newDirtyRegion.IntersectWith(&window->VisibleRegion());
	// ...because we do this outself
	newDirtyRegion.Include(&previouslyOccupiedRegion);

	MarkDirty(newDirtyRegion);
	_SetBackground(background);
	_WindowChanged(window);

	// resume direct frame buffer access
	if (direct) {
		window->ServerWindow()->HandleDirectConnection(
			B_DIRECT_START | B_BUFFER_RESIZED | B_CLIPPING_MODIFIED);
	}

	NotifyWindowResized(window);

	UnlockAllWindows();
}


bool
Desktop::SetWindowTabLocation(Window* window, float location, bool isShifting)
{
	AutoWriteLocker _(fWindowLock);

	BRegion dirty;
	bool changed = window->SetTabLocation(location, isShifting, dirty);
	if (changed)
		RebuildAndRedrawAfterWindowChange(window, dirty);

	NotifyWindowTabLocationChanged(window, location, isShifting);

	return changed;
}


bool
Desktop::SetWindowDecoratorSettings(Window* window, const BMessage& settings)
{
	AutoWriteLocker _(fWindowLock);

	BRegion dirty;
	bool changed = window->SetDecoratorSettings(settings, dirty);
	bool listenerChanged = SetDecoratorSettings(window, settings);
	if (changed || listenerChanged)
		RebuildAndRedrawAfterWindowChange(window, dirty);

	return changed;
}


void
Desktop::SetWindowWorkspaces(Window* window, uint32 workspaces)
{
	LockAllWindows();

	if (window->IsNormal() && workspaces == B_CURRENT_WORKSPACE)
		workspaces = workspace_to_workspaces(CurrentWorkspace());

	uint32 oldWorkspaces = window->Workspaces();

	window->WorkspacesChanged(oldWorkspaces, workspaces);
	_ChangeWindowWorkspaces(window, oldWorkspaces, workspaces);

	UnlockAllWindows();
}


/*!	\brief Adds the window to the desktop.
	At this point, the window is still hidden and must be shown explicetly
	via ShowWindow().
*/
void
Desktop::AddWindow(Window *window)
{
	LockAllWindows();

	fAllWindows.AddWindow(window);
	if (!window->IsNormal())
		fSubsetWindows.AddWindow(window);

	if (window->IsNormal()) {
		if (window->Workspaces() == B_CURRENT_WORKSPACE)
			window->SetWorkspaces(workspace_to_workspaces(CurrentWorkspace()));
	} else {
		// subset windows are visible on all workspaces their subset is on
		window->SetWorkspaces(window->SubsetWorkspaces());
	}

	_ChangeWindowWorkspaces(window, 0, window->Workspaces());

	NotifyWindowAdded(window);

	UnlockAllWindows();
}


void
Desktop::RemoveWindow(Window *window)
{
	LockAllWindows();

	if (!window->IsHidden())
		HideWindow(window);

	fAllWindows.RemoveWindow(window);
	if (!window->IsNormal())
		fSubsetWindows.RemoveWindow(window);

	_ChangeWindowWorkspaces(window, window->Workspaces(), 0);

	NotifyWindowRemoved(window);

	UnlockAllWindows();

	// make sure this window won't get any events anymore

	EventDispatcher().RemoveTarget(window->EventTarget());
}


bool
Desktop::AddWindowToSubset(Window* subset, Window* window)
{
	if (!subset->AddToSubset(window))
		return false;

	_ChangeWindowWorkspaces(subset, subset->Workspaces(),
		subset->SubsetWorkspaces());
	return true;
}


void
Desktop::RemoveWindowFromSubset(Window* subset, Window* window)
{
	subset->RemoveFromSubset(window);
	_ChangeWindowWorkspaces(subset, subset->Workspaces(),
		subset->SubsetWorkspaces());
}


void
Desktop::FontsChanged(Window* window)
{
	AutoWriteLocker _(fWindowLock);

	BRegion dirty;
	window->FontsChanged(&dirty);

	RebuildAndRedrawAfterWindowChange(window, dirty);
}


void
Desktop::SetWindowLook(Window* window, window_look newLook)
{
	if (window->Look() == newLook)
		return;

	AutoWriteLocker _(fWindowLock);

	BRegion dirty;
	window->SetLook(newLook, &dirty);
		// TODO: test what happens when the window
		// finds out it needs to resize itself...

	RebuildAndRedrawAfterWindowChange(window, dirty);

	NotifyWindowLookChanged(window, newLook);
}


void
Desktop::SetWindowFeel(Window* window, window_feel newFeel)
{
	if (window->Feel() == newFeel)
		return;

	LockAllWindows();

	bool wasNormal = window->IsNormal();

	window->SetFeel(newFeel);

	// move the window out of or into the subset window list as needed
	if (window->IsNormal() && !wasNormal)
		fSubsetWindows.RemoveWindow(window);
	else if (!window->IsNormal() && wasNormal)
		fSubsetWindows.AddWindow(window);

	// A normal window that was once a floating or modal window will
	// adopt the window's current workspaces

	if (!window->IsNormal()) {
		_ChangeWindowWorkspaces(window, window->Workspaces(),
			window->SubsetWorkspaces());
	}

	// make sure the window has the correct position in the window lists
	// (ie. all floating windows have to be on the top, ...)

	for (int32 i = 0; i < kMaxWorkspaces; i++) {
		if (!workspace_in_workspaces(i, window->Workspaces()))
			continue;

		bool changed = false;
		BRegion visibleBefore;
		if (i == fCurrentWorkspace && window->IsVisible())
			visibleBefore = window->VisibleRegion();

		Window* backmost = window->Backmost(_Windows(i).LastWindow(), i);
		if (backmost != NULL) {
			// check if the backmost window is really behind it
			Window* previous = window->PreviousWindow(i);
			while (previous != NULL) {
				if (previous == backmost)
					break;

				previous = previous->PreviousWindow(i);
			}

			if (previous == NULL) {
				// need to reinsert window before its backmost window
				_Windows(i).RemoveWindow(window);
				_Windows(i).AddWindow(window, backmost->NextWindow(i));
				changed = true;
			}
		}

		Window* frontmost = window->Frontmost(_Windows(i).FirstWindow(), i);
		if (frontmost != NULL) {
			// check if the frontmost window is really in front of it
			Window* next = window->NextWindow(i);
			while (next != NULL) {
				if (next == frontmost)
					break;

				next = next->NextWindow(i);
			}

			if (next == NULL) {
				// need to reinsert window behind its frontmost window
				_Windows(i).RemoveWindow(window);
				_Windows(i).AddWindow(window, frontmost);
				changed = true;
			}
		}

		if (i == fCurrentWorkspace && changed) {
			BRegion dummy;
			_RebuildClippingForAllWindows(dummy);

			// mark everything dirty that is no longer visible, or
			// is now visible and wasn't before
			BRegion visibleAfter(window->VisibleRegion());
			BRegion dirty(visibleAfter);
			dirty.Exclude(&visibleBefore);
			visibleBefore.Exclude(&visibleAfter);
			dirty.Include(&visibleBefore);

			MarkDirty(dirty);
		}
	}

	_UpdateFronts();

	if (window == FocusWindow() && !window->IsVisible())
		SetFocusWindow();

	NotifyWindowFeelChanged(window, newFeel);

	UnlockAllWindows();
}


void
Desktop::SetWindowFlags(Window *window, uint32 newFlags)
{
	if (window->Flags() == newFlags)
		return;

	AutoWriteLocker _(fWindowLock);

	BRegion dirty;
	window->SetFlags(newFlags, &dirty);
		// TODO: test what happens when the window
		// finds out it needs to resize itself...

	RebuildAndRedrawAfterWindowChange(window, dirty);
}


void
Desktop::SetWindowTitle(Window *window, const char* title)
{
	AutoWriteLocker _(fWindowLock);

	BRegion dirty;
	window->SetTitle(title, dirty);

	RebuildAndRedrawAfterWindowChange(window, dirty);
}


/*!	Returns the window under the mouse cursor.
	You need to have acquired the All Windows lock when calling this method.
*/
Window*
Desktop::WindowAt(BPoint where)
{
	for (Window* window = CurrentWindows().LastWindow(); window;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (window->IsVisible() && window->VisibleRegion().Contains(where))
			return window->StackedWindowAt(where);
	}

	return NULL;
}


void
Desktop::SetMouseEventWindow(Window* window)
{
	fMouseEventWindow = window;
}


void
Desktop::SetViewUnderMouse(const Window* window, int32 viewToken)
{
	fWindowUnderMouse = window;
	fViewUnderMouse = viewToken;
}


int32
Desktop::ViewUnderMouse(const Window* window)
{
	if (window != NULL && fWindowUnderMouse == window)
		return fViewUnderMouse;

	return B_NULL_TOKEN;
}


/*!	Returns the current keyboard event target candidate - which is either the
	top-most window (in case it has the kAcceptKeyboardFocusFlag flag set), or
	the one having focus.
	The window lock must be held when calling this function.
*/
EventTarget*
Desktop::KeyboardEventTarget()
{
	// Get the top most non-hidden window
	Window* window = CurrentWindows().LastWindow();
	while (window != NULL && window->IsHidden()) {
		window = window->PreviousWindow(fCurrentWorkspace);
	}

	if (window != NULL && (window->Flags() & kAcceptKeyboardFocusFlag) != 0)
		return &window->EventTarget();

	if (FocusWindow() != NULL)
		return &FocusWindow()->EventTarget();

	return NULL;
}


/*!	Tries to set the focus to the specified \a focus window. It will make sure,
	however, that the window actually can have focus. You are allowed to pass
	in a NULL pointer for \a focus.

	Besides the B_AVOID_FOCUS flag, a modal window, or a BWindowScreen can both
	prevent it from getting focus.

	In any case, this method makes sure that there is a focus window, if there
	is any window at all, that is.
*/
void
Desktop::SetFocusWindow(Window* focus)
{
	if (!LockAllWindows())
		return;

	// test for B_LOCK_WINDOW_FOCUS
	if (fLockedFocusWindow && focus != fLockedFocusWindow) {
		UnlockAllWindows();
		return;
	}

	bool hasModal = _WindowHasModal(focus);
	bool hasWindowScreen = false;

	if (!hasModal && focus != NULL) {
		// Check whether or not a window screen is in front of the window
		// (if it has a modal, the right thing is done, anyway)
		Window* window = focus;
		while (true) {
			window = window->NextWindow(fCurrentWorkspace);
			if (window == NULL || window->Feel() == kWindowScreenFeel)
				break;
		}
		if (window != NULL)
			hasWindowScreen = true;
	}

	if (focus == fFocus && focus != NULL && !focus->IsHidden()
		&& (focus->Flags() & B_AVOID_FOCUS) == 0
		&& !hasModal && !hasWindowScreen) {
		// the window that is supposed to get focus already has focus
		UnlockAllWindows();
		return;
	}

	uint32 list = /*fCurrentWorkspace;
	if (fSettings->FocusFollowsMouse())
		list = */kFocusList;

	if (focus == NULL || hasModal || hasWindowScreen) {
		/*if (!fSettings->FocusFollowsMouse())
			focus = CurrentWindows().LastWindow();
		else*/
			focus = fFocusList.LastWindow();
	}

	// make sure no window is chosen that doesn't want focus or cannot have it
	while (focus != NULL
		&& (!focus->InWorkspace(fCurrentWorkspace)
			|| (focus->Flags() & B_AVOID_FOCUS) != 0
			|| _WindowHasModal(focus)
			|| focus->IsHidden())) {
		focus = focus->PreviousWindow(list);
	}

	if (fFocus == focus) {
		// turns out the window that is supposed to get focus now already has it
		UnlockAllWindows();
		return;
	}

	team_id oldActiveApp = -1;
	team_id newActiveApp = -1;

	if (fFocus != NULL) {
		fFocus->SetFocus(false);
		oldActiveApp = fFocus->ServerWindow()->App()->ClientTeam();
	}

	fFocus = focus;

	if (fFocus != NULL) {
		fFocus->SetFocus(true);
		newActiveApp = fFocus->ServerWindow()->App()->ClientTeam();

		// move current focus to the end of the focus list
		fFocusList.RemoveWindow(fFocus);
		fFocusList.AddWindow(fFocus);
	}

	if (newActiveApp == -1) {
		// make sure the cursor is visible
		HWInterface()->SetCursorVisible(true);
	}

	UnlockAllWindows();

	// change the "active" app if appropriate
	if (oldActiveApp == newActiveApp)
		return;

	BAutolock locker(fApplicationsLock);

	for (int32 i = 0; i < fApplications.CountItems(); i++) {
		ServerApp* app = fApplications.ItemAt(i);

		if (oldActiveApp != -1 && app->ClientTeam() == oldActiveApp)
			app->Activate(false);
		else if (newActiveApp != -1 && app->ClientTeam() == newActiveApp)
			app->Activate(true);
	}
}


void
Desktop::SetFocusLocked(const Window* window)
{
	AutoWriteLocker _(fWindowLock);

	if (window != NULL) {
		// Don't allow this to be set when no mouse buttons
		// are pressed. (BView::SetMouseEventMask() should only be called
		// from mouse hooks.)
		if (fLastMouseButtons == 0)
			return;
	}

	fLockedFocusWindow = window;
}


Window*
Desktop::FindWindowByClientToken(int32 token, team_id teamID)
{
	for (Window *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->ServerWindow()->ClientToken() == token
			&& window->ServerWindow()->ClientTeam() == teamID) {
			return window;
		}
	}

	return NULL;
}


::EventTarget*
Desktop::FindTarget(BMessenger& messenger)
{
	for (Window *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->EventTarget().Messenger() == messenger)
			return &window->EventTarget();
	}

	return NULL;
}


void
Desktop::MarkDirty(BRegion& region)
{
	if (region.CountRects() == 0)
		return;

	if (LockAllWindows()) {
		// send redraw messages to all windows intersecting the dirty region
		_TriggerWindowRedrawing(region);

		UnlockAllWindows();
	}
}


void
Desktop::Redraw()
{
	BRegion dirty(fVirtualScreen.Frame());
	MarkDirty(dirty);
}


/*!	\brief Redraws the background (ie. the desktop window, if any).
*/
void
Desktop::RedrawBackground()
{
	LockAllWindows();

	BRegion redraw;

	Window* window = CurrentWindows().FirstWindow();
	if (window->Feel() == kDesktopWindowFeel) {
		redraw = window->VisibleContentRegion();

		// look for desktop background view, and update its background color
		// TODO: is there a better way to do this?
		View* view = window->TopView();
		if (view != NULL)
			view = view->FirstChild();

		while (view) {
			if (view->IsDesktopBackground()) {
				view->SetViewColor(fWorkspaces[fCurrentWorkspace].Color());
				break;
			}
			view = view->NextSibling();
		}

		window->ProcessDirtyRegion(redraw);
	} else {
		redraw = BackgroundRegion();
		fBackgroundRegion.MakeEmpty();
		_SetBackground(redraw);
	}

	_WindowChanged(NULL);
		// update workspaces view as well

	UnlockAllWindows();
}


bool
Desktop::ReloadDecor(DecorAddOn* oldDecor)
{
	AutoWriteLocker _(fWindowLock);

	bool returnValue = true;

	if (oldDecor != NULL) {
		const DesktopListenerList* oldListeners
			= &oldDecor->GetDesktopListeners();
		for (int i = 0; i < oldListeners->CountItems(); i++)
			UnregisterListener(oldListeners->ItemAt(i));
	}

	for (Window* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		BRegion oldBorder;
		window->GetBorderRegion(&oldBorder);

		if (!window->ReloadDecor()) {
			// prevent unloading previous add-on
			returnValue = false;
		}

		BRegion border;
		window->GetBorderRegion(&border);

		border.Include(&oldBorder);
		RebuildAndRedrawAfterWindowChange(window, border);
	}

	// register new listeners
	const DesktopListenerList& newListeners
		= gDecorManager.GetDesktopListeners();
	for (int i = 0; i < newListeners.CountItems(); i++)
 		RegisterListener(newListeners.ItemAt(i));

 	return returnValue;
}


void
Desktop::MinimizeApplication(team_id team)
{
	AutoWriteLocker locker(fWindowLock);

	// Just minimize all windows of that application

	for (Window *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->ServerWindow()->ClientTeam() != team)
			continue;

		window->ServerWindow()->NotifyMinimize(true);
	}
}


void
Desktop::BringApplicationToFront(team_id team)
{
	AutoWriteLocker locker(fWindowLock);

	// TODO: for now, just maximize all windows of that application
	// TODO: have the ability to lock the current workspace

	for (Window *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->ServerWindow()->ClientTeam() != team)
			continue;

		window->ServerWindow()->NotifyMinimize(false);
	}
}


void
Desktop::WindowAction(int32 windowToken, int32 action)
{
	if (action != B_MINIMIZE_WINDOW && action != B_BRING_TO_FRONT)
		return;

	LockAllWindows();

	::ServerWindow* serverWindow;
	Window* window;
	if (BPrivate::gDefaultTokens.GetToken(windowToken,
			B_SERVER_TOKEN, (void**)&serverWindow) != B_OK
		|| (window = serverWindow->Window()) == NULL) {
		UnlockAllWindows();
		return;
	}

	if (action == B_BRING_TO_FRONT && !window->IsMinimized()) {
		// the window is visible, we just need to make it the front window
		ActivateWindow(window);
	} else {
		// if not, ask the window if it wants to be unminimized
		serverWindow->NotifyMinimize(action == B_MINIMIZE_WINDOW);
	}

	UnlockAllWindows();
}


void
Desktop::WriteWindowList(team_id team, BPrivate::LinkSender& sender)
{
	AutoWriteLocker locker(fWindowLock);

	// compute the number of windows

	int32 count = 0;

	for (Window *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (team < B_OK || window->ServerWindow()->ClientTeam() == team)
			count++;
	}

	// write list

	sender.StartMessage(B_OK);
	sender.Attach<int32>(count);

	// first write the windows of the current workspace correctly ordered
	for (Window *window = CurrentWindows().LastWindow(); window != NULL;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (team >= B_OK && window->ServerWindow()->ClientTeam() != team)
			continue;

		sender.Attach<int32>(window->ServerWindow()->ServerToken());
	}

	// then write all the other windows
	for (Window *window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if ((team >= B_OK && window->ServerWindow()->ClientTeam() != team)
			|| window->InWorkspace(fCurrentWorkspace))
			continue;

		sender.Attach<int32>(window->ServerWindow()->ServerToken());
	}

	sender.Flush();
}


void
Desktop::WriteWindowInfo(int32 serverToken, BPrivate::LinkSender& sender)
{
	AutoWriteLocker locker(fWindowLock);
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

	float tabSize = 0.0;
	float borderSize = 0.0;
	::Window* tmp = window->Window();
	if (tmp) {
		BMessage message;
		if (tmp->GetDecoratorSettings(&message)) {
			BRect tabFrame;
			message.FindRect("tab frame", &tabFrame);
			tabSize = tabFrame.bottom - tabFrame.top;
			message.FindFloat("border width", &borderSize);
		}
	}

	int32 length = window->Title() ? strlen(window->Title()) : 0;

	sender.StartMessage(B_OK);
	sender.Attach<int32>(sizeof(client_window_info) + length);
	sender.Attach(&info, sizeof(window_info));
	sender.Attach<float>(tabSize);
	sender.Attach<float>(borderSize);

	if (length > 0)
		sender.Attach(window->Title(), length + 1);
	else
		sender.Attach<char>('\0');

	sender.Flush();
}


void
Desktop::WriteWindowOrder(int32 workspace, BPrivate::LinkSender& sender)
{
	LockSingleWindow();

	if (workspace < 0)
		workspace = fCurrentWorkspace;
	else if (workspace >= kMaxWorkspaces) {
		sender.StartMessage(B_BAD_VALUE);
		sender.Flush();
		UnlockSingleWindow();
		return;
	}

	int32 count = _Windows(workspace).Count();

	// write list

	sender.StartMessage(B_OK);
	sender.Attach<int32>(count);

	for (Window *window = _Windows(workspace).LastWindow(); window != NULL;
			window = window->PreviousWindow(workspace)) {
		sender.Attach<int32>(window->ServerWindow()->ServerToken());
	}

	sender.Flush();

	UnlockSingleWindow();
}


void
Desktop::WriteApplicationOrder(int32 workspace, BPrivate::LinkSender& sender)
{
	fApplicationsLock.Lock();
	LockSingleWindow();

	int32 maxCount = fApplications.CountItems();

	fApplicationsLock.Unlock();
		// as long as we hold the window lock, no new window can appear

	if (workspace < 0)
		workspace = fCurrentWorkspace;
	else if (workspace >= kMaxWorkspaces) {
		sender.StartMessage(B_BAD_VALUE);
		sender.Flush();
		UnlockSingleWindow();
		return;
	}

	// compute the list of applications on this workspace

	team_id* teams = (team_id*)malloc(maxCount * sizeof(team_id));
	if (teams == NULL) {
		sender.StartMessage(B_NO_MEMORY);
		sender.Flush();
		UnlockSingleWindow();
		return;
	}

	int32 count = 0;

	for (Window *window = _Windows(workspace).LastWindow(); window != NULL;
			window = window->PreviousWindow(workspace)) {
		team_id team = window->ServerWindow()->ClientTeam();
		if (count > 1) {
			// see if we already have this team
			bool found = false;
			for (int32 i = 0; i < count; i++) {
				if (teams[i] == team) {
					found = true;
					break;
				}
			}
			if (found)
				continue;
		}

		ASSERT(count < maxCount);
		teams[count++] = team;
	}

	UnlockSingleWindow();

	// write list

	sender.StartMessage(B_OK);
	sender.Attach<int32>(count);

	for (int32 i = 0; i < count; i++) {
		sender.Attach<int32>(teams[i]);
	}

	sender.Flush();
	free(teams);
}


void
Desktop::_LaunchInputServer()
{
	BRoster roster;
	status_t status = roster.Launch("application/x-vnd.Be-input_server");
	if (status == B_OK || status == B_ALREADY_RUNNING)
		return;

	// Could not load input_server by signature, try well-known location

	BEntry entry("/system/servers/input_server");
	entry_ref ref;
	status_t entryStatus = entry.GetRef(&ref);
	if (entryStatus == B_OK)
		entryStatus = roster.Launch(&ref);
	if (entryStatus == B_OK || entryStatus == B_ALREADY_RUNNING) {
		syslog(LOG_ERR, "Failed to launch the input server by signature: %s!\n",
			strerror(status));
		return;
	}

	syslog(LOG_ERR, "Failed to launch the input server: %s!\n",
		strerror(entryStatus));
}


void
Desktop::_GetLooperName(char* name, size_t length)
{
	snprintf(name, length, "d:%d:%s", fUserID,
		fTargetScreen == NULL ? "baron" : fTargetScreen);
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
	if (count > 0) {
		acquire_sem_etc(fShutdownSemaphore, fShutdownCount, B_RELATIVE_TIMEOUT,
			250000);
	}

	fApplicationsLock.Unlock();
}


void
Desktop::_DispatchMessage(int32 code, BPrivate::LinkReceiver& link)
{
	switch (code) {
		case AS_CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication

			// Attached data:
			// 1) port_id - receiver port of a regular app
			// 2) port_id - client looper port - for sending messages to the
			//		client
			// 2) team_id - app's team ID
			// 3) int32 - handler token of the regular app
			// 4) char * - signature of the regular app

			// Find the necessary data
			team_id	clientTeamID = -1;
			port_id	clientLooperPort = -1;
			port_id clientReplyPort = -1;
			int32 htoken = B_NULL_TOKEN;
			char* appSignature = NULL;

			link.Read<port_id>(&clientReplyPort);
			link.Read<port_id>(&clientLooperPort);
			link.Read<team_id>(&clientTeamID);
			link.Read<int32>(&htoken);
			if (link.ReadString(&appSignature) != B_OK)
				break;

			ServerApp* app = new ServerApp(this, clientReplyPort,
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
				reply.StartMessage(B_ERROR);
				reply.Flush();
			}

			// This is necessary because BPortLink::ReadString allocates memory
			free(appSignature);
			break;
		}

		case AS_DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp
			// when a BApplication asks it to quit.

			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted

			thread_id thread = -1;
			if (link.Read<thread_id>(&thread) < B_OK)
				break;

			fApplicationsLock.Lock();

			// Run through the list of apps and nuke the proper one

			int32 count = fApplications.CountItems();
			ServerApp* removeApp = NULL;

			for (int32 i = 0; i < count; i++) {
				ServerApp* app = fApplications.ItemAt(i);

				if (app->Thread() == thread) {
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
				acquire_sem_etc(fShutdownSemaphore, fShutdownCount,
					B_RELATIVE_TIMEOUT, 500000);
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

		case AS_APP_CRASHED:
		case AS_DUMP_ALLOCATOR:
		case AS_DUMP_BITMAPS:
		{
			BAutolock locker(fApplicationsLock);

			team_id team;
			if (link.Read(&team) != B_OK)
				break;

			for (int32 i = 0; i < fApplications.CountItems(); i++) {
				ServerApp* app = fApplications.ItemAt(i);

				if (app->ClientTeam() == team)
					app->PostMessage(code);
			}
			break;
		}

		case AS_EVENT_STREAM_CLOSED:
			_LaunchInputServer();
			break;

		case B_QUIT_REQUESTED:
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the
			// server is compiled as a regular Be application.

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

		case AS_ACTIVATE_WORKSPACE:
		{
			int32 index;
			link.Read<int32>(&index);
			if (index == -1)
				index = fPreviousWorkspace;

			bool moveFocusWindow;
			link.Read<bool>(&moveFocusWindow);

			SetWorkspace(index, moveFocusWindow);
			break;
		}

		case AS_TALK_TO_DESKTOP_LISTENER:
		{
			port_id clientReplyPort;
			if (link.Read<port_id>(&clientReplyPort) != B_OK)
				break;

			BPrivate::LinkSender reply(clientReplyPort);
			AutoWriteLocker locker(fWindowLock);
			if (MessageForListener(NULL, link, reply) != true) {
				// unhandled message, at least send an error if needed
				if (link.NeedsReply()) {
					reply.StartMessage(B_ERROR);
					reply.Flush();
				}
			}
			break;
		}

		// ToDo: Remove this again. It is a message sent by the
		// invalidate_on_exit kernel debugger add-on to trigger a redraw
		// after exiting a kernel debugger session.
		case 'KDLE':
		{
			BRegion dirty;
			dirty.Include(fVirtualScreen.Frame());
			MarkDirty(dirty);
			break;
		}

		default:
			printf("Desktop %d:%s received unexpected code %ld\n", 0, "baron",
				code);

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
	}
}


WindowList&
Desktop::CurrentWindows()
{
	return fWorkspaces[fCurrentWorkspace].Windows();
}


WindowList&
Desktop::AllWindows()
{
	return fAllWindows;
}


Window*
Desktop::WindowForClientLooperPort(port_id port)
{
	ASSERT(fWindowLock.IsReadLocked());

	for (Window* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->ServerWindow()->ClientLooperPort() == port)
			return window;
	}
	return NULL;
}


WindowList&
Desktop::_Windows(int32 index)
{
	return fWorkspaces[index].Windows();
}


void
Desktop::_UpdateFloating(int32 previousWorkspace, int32 nextWorkspace,
	Window* mouseEventWindow)
{
	if (previousWorkspace == -1)
		previousWorkspace = fCurrentWorkspace;
	if (nextWorkspace == -1)
		nextWorkspace = previousWorkspace;

	for (Window* floating = fSubsetWindows.FirstWindow(); floating != NULL;
			floating = floating->NextWindow(kSubsetList)) {
		// we only care about app/subset floating windows
		if (floating->Feel() != B_FLOATING_SUBSET_WINDOW_FEEL
			&& floating->Feel() != B_FLOATING_APP_WINDOW_FEEL)
			continue;

		if (fFront != NULL && fFront->IsNormal()
			&& floating->HasInSubset(fFront)) {
			// is now visible
			if (_Windows(previousWorkspace).HasWindow(floating)
				&& previousWorkspace != nextWorkspace
				&& !floating->InSubsetWorkspace(previousWorkspace)) {
				// but no longer on the previous workspace
				_Windows(previousWorkspace).RemoveWindow(floating);
				floating->SetCurrentWorkspace(-1);
			}

			if (!_Windows(nextWorkspace).HasWindow(floating)) {
				// but wasn't before
				_Windows(nextWorkspace).AddWindow(floating,
					floating->Frontmost(_Windows(nextWorkspace).FirstWindow(),
					nextWorkspace));
				floating->SetCurrentWorkspace(nextWorkspace);
				if (mouseEventWindow != fFront)
					_ShowWindow(floating);

				// TODO: put the floating last in the floating window list to
				// preserve the on screen window order
			}
		} else if (_Windows(previousWorkspace).HasWindow(floating)
			&& !floating->InSubsetWorkspace(previousWorkspace)) {
			// was visible, but is no longer

			_Windows(previousWorkspace).RemoveWindow(floating);
			floating->SetCurrentWorkspace(-1);
			_HideWindow(floating);

			if (FocusWindow() == floating)
				SetFocusWindow();
		}
	}
}


/*!	Search the visible windows for a valid back window
	(only desktop windows can't be back windows)
*/
void
Desktop::_UpdateBack()
{
	fBack = NULL;

	for (Window* window = CurrentWindows().FirstWindow(); window != NULL;
			window = window->NextWindow(fCurrentWorkspace)) {
		if (window->IsHidden() || window->Feel() == kDesktopWindowFeel)
			continue;

		fBack = window;
		break;
	}
}


/*!	Search the visible windows for a valid front window
	(only normal and modal windows can be front windows)

	The only place where you don't want to update floating windows is
	during a workspace change - because then you'll call _UpdateFloating()
	yourself.
*/
void
Desktop::_UpdateFront(bool updateFloating)
{
	fFront = NULL;

	for (Window* window = CurrentWindows().LastWindow(); window != NULL;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (window->IsHidden() || window->IsFloating()
			|| !window->SupportsFront())
			continue;

		fFront = window;
		break;
	}

	if (updateFloating)
		_UpdateFloating();
}


void
Desktop::_UpdateFronts(bool updateFloating)
{
	_UpdateBack();
	_UpdateFront(updateFloating);
}


bool
Desktop::_WindowHasModal(Window* window)
{
	if (window == NULL)
		return false;

	for (Window* modal = fSubsetWindows.FirstWindow(); modal != NULL;
			modal = modal->NextWindow(kSubsetList)) {
		// only visible modal windows count
		if (!modal->IsModal() || modal->IsHidden())
			continue;

		if (modal->HasInSubset(window))
			return true;
	}

	return false;
}


/*!	You must at least hold a single window lock when calling this method.
*/
void
Desktop::_WindowChanged(Window* window)
{
	ASSERT_MULTI_LOCKED(fWindowLock);

	BAutolock _(fWorkspacesLock);

	for (uint32 i = fWorkspacesViews.CountItems(); i-- > 0;) {
		WorkspacesView* view = fWorkspacesViews.ItemAt(i);
		view->WindowChanged(window);
	}
}


/*!	You must at least hold a single window lock when calling this method.
*/
void
Desktop::_WindowRemoved(Window* window)
{
	ASSERT_MULTI_LOCKED(fWindowLock);

	BAutolock _(fWorkspacesLock);

	for (uint32 i = fWorkspacesViews.CountItems(); i-- > 0;) {
		WorkspacesView* view = fWorkspacesViews.ItemAt(i);
		view->WindowRemoved(window);
	}
}


/*!	Shows the window on the screen - it does this independently of the
	Window::IsHidden() state.
*/
void
Desktop::_ShowWindow(Window* window, bool affectsOtherWindows)
{
	BRegion background;
	_RebuildClippingForAllWindows(background);
	_SetBackground(background);
	_WindowChanged(window);

	BRegion dirty(window->VisibleRegion());

	if (!affectsOtherWindows) {
		// everything that is now visible in the
		// window needs a redraw, but other windows
		// are not affected, we can call ProcessDirtyRegion()
		// of the window, and don't have to use MarkDirty()
		window->ProcessDirtyRegion(dirty);
	} else
		MarkDirty(dirty);

	if (window->ServerWindow()->HasDirectFrameBufferAccess()) {
		window->ServerWindow()->HandleDirectConnection(
			B_DIRECT_START | B_BUFFER_RESET);
	}
}


/*!	Hides the window from the screen - it does this independently of the
	Window::IsHidden() state.
*/
void
Desktop::_HideWindow(Window* window)
{
	if (window->ServerWindow()->IsDirectlyAccessing())
		window->ServerWindow()->HandleDirectConnection(B_DIRECT_STOP);

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
	_WindowChanged(window);

	MarkDirty(dirty);
}


/*!	Updates the workspaces of all subset windows with regard to the
	specifed window.
	If newIndex is not -1, it will move all subset windows that belong to
	the specifed window to the new workspace; this form is only called by
	SetWorkspace().
*/
void
Desktop::_UpdateSubsetWorkspaces(Window* window, int32 previousIndex,
	int32 newIndex)
{
	STRACE(("_UpdateSubsetWorkspaces(window %p, %s)\n", window,
		window->Title()));

	// if the window is hidden, the subset windows are up-to-date already
	if (!window->IsNormal() || window->IsHidden())
		return;

	for (Window* subset = fSubsetWindows.FirstWindow(); subset != NULL;
			subset = subset->NextWindow(kSubsetList)) {
		if (subset->Feel() == B_MODAL_ALL_WINDOW_FEEL
			|| subset->Feel() == B_FLOATING_ALL_WINDOW_FEEL) {
			// These windows are always visible on all workspaces,
			// no need to update them.
			continue;
		}

		if (subset->IsFloating()) {
			// Floating windows are inserted and removed to the current
			// workspace as the need arises - they are not handled here
			// but in _UpdateFront()
			continue;
		}

		if (subset->HasInSubset(window)) {
			// adopt the workspace change
			SetWindowWorkspaces(subset, subset->SubsetWorkspaces());
		}
	}
}


/*!	\brief Adds or removes the window to or from the workspaces it's on.
*/
void
Desktop::_ChangeWindowWorkspaces(Window* window, uint32 oldWorkspaces,
	uint32 newWorkspaces)
{
	if (oldWorkspaces == newWorkspaces)
		return;

	// apply changes to the workspaces' window lists

	LockAllWindows();

	// NOTE: we bypass the anchor-mechanism by intention when switching
	// the workspace programmatically.

	for (int32 i = 0; i < kMaxWorkspaces; i++) {
		if (workspace_in_workspaces(i, oldWorkspaces)) {
			// window is on this workspace, is it anymore?
			if (!workspace_in_workspaces(i, newWorkspaces)) {
				_Windows(i).RemoveWindow(window);
				if (fLastWorkspaceFocus[i] == window)
					fLastWorkspaceFocus[i] = NULL;

				if (i == CurrentWorkspace()) {
					// remove its appearance from the current workspace
					window->SetCurrentWorkspace(-1);

					if (!window->IsHidden())
						_HideWindow(window);
				}
			}
		} else {
			// window was not on this workspace, is it now?
			if (workspace_in_workspaces(i, newWorkspaces)) {
				_Windows(i).AddWindow(window,
					window->Frontmost(_Windows(i).FirstWindow(), i));

				if (i == CurrentWorkspace()) {
					// make the window visible in current workspace
					window->SetCurrentWorkspace(fCurrentWorkspace);

					if (!window->IsHidden()) {
						// This only affects other windows if this window has
						// floating or modal windows that need to be shown as
						// well
						// TODO: take care of this
						_ShowWindow(window, FrontWindow() == window);
					}
				}
			}
		}
	}

	// If the window is visible only on one workspace, we set it's current
	// position in that workspace (so that WorkspacesView will find us).
	int32 firstWorkspace = -1;
	for (int32 i = 0; i < kMaxWorkspaces; i++) {
		if ((newWorkspaces & (1L << i)) != 0) {
			if (firstWorkspace != -1) {
				firstWorkspace = -1;
				break;
			}
			firstWorkspace = i;
		}
	}
	if (firstWorkspace >= 0)
		window->Anchor(firstWorkspace).position = window->Frame().LeftTop();

	// take care about modals and floating windows
	_UpdateSubsetWorkspaces(window);

	NotifyWindowWorkspacesChanged(window, newWorkspaces);

	UnlockAllWindows();
}


void
Desktop::_BringWindowsToFront(WindowList& windows, int32 list,
	bool wereVisible)
{
	// we don't need to redraw what is currently
	// visible of the window
	BRegion clean;

	for (Window* window = windows.FirstWindow(); window != NULL;
			window = window->NextWindow(list)) {
		if (wereVisible)
			clean.Include(&window->VisibleRegion());

		CurrentWindows().AddWindow(window,
			window->Frontmost(CurrentWindows().FirstWindow(),
				fCurrentWorkspace));

		_WindowChanged(window);
	}

	BRegion dummy;
	_RebuildClippingForAllWindows(dummy);

	// redraw what became visible of the window(s)

	BRegion dirty;
	for (Window* window = windows.FirstWindow(); window != NULL;
			window = window->NextWindow(list)) {
		dirty.Include(&window->VisibleRegion());
	}

	dirty.Exclude(&clean);
	MarkDirty(dirty);

	_UpdateFront();

	if (windows.FirstWindow() == fBack || fBack == NULL)
		_UpdateBack();
}


/*!	Returns the last focussed non-hidden subset window belonging to the
	specified \a window.
*/
Window*
Desktop::_LastFocusSubsetWindow(Window* window)
{
	if (window == NULL)
		return NULL;

	for (Window* front = fFocusList.LastWindow(); front != NULL;
			front = front->PreviousWindow(kFocusList)) {
		if (front != window && !front->IsHidden()
			&& window->HasInSubset(front))
			return front;
	}

	return NULL;
}


/*!	\brief Sends a fake B_MOUSE_MOVED event to the window under the mouse,
		and also updates the current view under the mouse.

	This has only to be done in case the view changed without user interaction,
	ie. because of a workspace change or a closing window.
*/
void
Desktop::_SendFakeMouseMoved(Window* window)
{
	int32 viewToken = B_NULL_TOKEN;
	EventTarget* target = NULL;

	LockAllWindows();

	if (window == NULL)
		window = MouseEventWindow();
	if (window == NULL)
		window = WindowAt(fLastMousePosition);

	if (window != NULL) {
		BMessage message;
		window->MouseMoved(&message, fLastMousePosition, &viewToken, true,
			true);

		if (viewToken != B_NULL_TOKEN)
			target = &window->EventTarget();
	}

	if (viewToken != B_NULL_TOKEN)
		SetViewUnderMouse(window, viewToken);
	else {
		SetViewUnderMouse(NULL, B_NULL_TOKEN);
		SetCursor(NULL);
	}

	UnlockAllWindows();

	if (target != NULL)
		EventDispatcher().SendFakeMouseMoved(*target, viewToken);
}


Screen*
Desktop::_DetermineScreenFor(BRect frame)
{
	AutoReadLocker _(fScreenLock);

	// TODO: choose the screen depending on where most of the area is
	return fVirtualScreen.ScreenAt(0);
}


void
Desktop::_RebuildClippingForAllWindows(BRegion& stillAvailableOnScreen)
{
	// the available region on screen starts with the entire screen area
	// each window on the screen will take a portion from that area

	// figure out what the entire screen area is
	stillAvailableOnScreen = fScreenRegion;

	// set clipping of each window
	for (Window* window = CurrentWindows().LastWindow(); window != NULL;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (!window->IsHidden()) {
			window->SetClipping(&stillAvailableOnScreen);
			window->SetScreen(_DetermineScreenFor(window->Frame()));

			if (window->ServerWindow()->IsDirectlyAccessing()) {
				window->ServerWindow()->HandleDirectConnection(
					B_DIRECT_MODIFY | B_CLIPPING_MODIFIED);
			}

			// that windows region is not available on screen anymore
			stillAvailableOnScreen.Exclude(&window->VisibleRegion());
		}
	}
}


void
Desktop::_TriggerWindowRedrawing(BRegion& newDirtyRegion)
{
	// send redraw messages to all windows intersecting the dirty region
	for (Window* window = CurrentWindows().LastWindow(); window != NULL;
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
		if (GetDrawingEngine()->LockParallelAccess()) {
			GetDrawingEngine()->FillRegion(dirtyBackground,
				fWorkspaces[fCurrentWorkspace].Color());

			GetDrawingEngine()->UnlockParallelAccess();
		}
	}
}


//!	The all window lock must be held when calling this function.
void
Desktop::RebuildAndRedrawAfterWindowChange(Window* changedWindow,
	BRegion& dirty)
{
	ASSERT_MULTI_WRITE_LOCKED(fWindowLock);
	if (!changedWindow->IsVisible() || dirty.CountRects() == 0)
		return;

	// The following loop is pretty much a copy of
	// _RebuildClippingForAllWindows(), but will also
	// take care about restricting our dirty region.

	// figure out what the entire screen area is
	BRegion stillAvailableOnScreen(fScreenRegion);

	// set clipping of each window
	for (Window* window = CurrentWindows().LastWindow(); window != NULL;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (!window->IsHidden()) {
			if (window == changedWindow)
				dirty.IntersectWith(&stillAvailableOnScreen);

			window->SetClipping(&stillAvailableOnScreen);
			window->SetScreen(_DetermineScreenFor(window->Frame()));

			if (window->ServerWindow()->IsDirectlyAccessing()) {
				window->ServerWindow()->HandleDirectConnection(
					B_DIRECT_MODIFY | B_CLIPPING_MODIFIED);
			}

			// that windows region is not available on screen anymore
			stillAvailableOnScreen.Exclude(&window->VisibleRegion());
		}
	}

	_SetBackground(stillAvailableOnScreen);
	_WindowChanged(changedWindow);

	_TriggerWindowRedrawing(dirty);
}


//! Suspend all windows with direct access to the frame buffer
void
Desktop::_SuspendDirectFrameBufferAccess()
{
	ASSERT_MULTI_LOCKED(fWindowLock);

	for (Window* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->ServerWindow()->IsDirectlyAccessing())
			window->ServerWindow()->HandleDirectConnection(B_DIRECT_STOP);
	}
}


//! Resume all windows with direct access to the frame buffer
void
Desktop::_ResumeDirectFrameBufferAccess()
{
	ASSERT_MULTI_LOCKED(fWindowLock);

	for (Window* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->IsHidden() || !window->InWorkspace(fCurrentWorkspace))
			continue;

		if (window->ServerWindow()->HasDirectFrameBufferAccess()) {
			window->ServerWindow()->HandleDirectConnection(
				B_DIRECT_START | B_BUFFER_RESET, B_MODE_CHANGED);
		}
	}
}


void
Desktop::_ScreenChanged(Screen* screen)
{
	ASSERT_MULTI_WRITE_LOCKED(fWindowLock);

	// the entire screen is dirty, because we're actually
	// operating on an all new buffer in memory
	BRegion dirty(screen->Frame());

	// update our cached screen region
	fScreenRegion.Set(screen->Frame());
	gInputManager->UpdateScreenBounds(screen->Frame());

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

	fVirtualScreen.UpdateFrame();

	for (Window* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		if (window->Screen() == screen)
			window->ServerWindow()->ScreenChanged(&update);
	}
}


/*!	\brief activate one of the app's windows.
*/
status_t
Desktop::_ActivateApp(team_id team)
{
	// search for an unhidden window in the current workspace

	AutoWriteLocker locker(fWindowLock);

	for (Window* window = CurrentWindows().LastWindow(); window != NULL;
			window = window->PreviousWindow(fCurrentWorkspace)) {
		if (!window->IsHidden() && window->IsNormal()
			&& window->ServerWindow()->ClientTeam() == team) {
			ActivateWindow(window);
			return B_OK;
		}
	}

	// search for an unhidden window to give focus to

	for (Window* window = fAllWindows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList)) {
		// if window is a normal window of the team, and not hidden,
		// we've found our target
		if (!window->IsHidden() && window->IsNormal()
			&& window->ServerWindow()->ClientTeam() == team) {
			ActivateWindow(window);
			return B_OK;
		}
	}

	// TODO: we cannot maximize minimized windows here (with the window lock
	// write locked). To work-around this, we could forward the request to
	// the ServerApp of this team - it maintains its own window list, and can
	// therefore call ActivateWindow() without holding the window lock.
	return B_BAD_VALUE;
}


void
Desktop::_SetCurrentWorkspaceConfiguration()
{
	ASSERT_MULTI_WRITE_LOCKED(fWindowLock);

	status_t status = fDirectScreenLock.LockWithTimeout(1000000L);
	if (status != B_OK) {
		// The application having the direct screen lock didn't give it up in
		// time, make it crash
		syslog(LOG_ERR, "Team %ld did not give up its direct screen lock.\n",
			fDirectScreenTeam);

		debug_thread(fDirectScreenTeam);
		fDirectScreenTeam = -1;
	} else
		fDirectScreenLock.Unlock();

	AutoWriteLocker _(fScreenLock);

	uint32 changedScreens;
	fVirtualScreen.SetConfiguration(*this,
		fWorkspaces[fCurrentWorkspace].CurrentScreenConfiguration(),
		&changedScreens);

	for (int32 i = 0; changedScreens != 0; i++, changedScreens /= 2) {
		if ((changedScreens & (1 << i)) != 0)
			_ScreenChanged(fVirtualScreen.ScreenAt(i));
	}
}


/*!	Changes the current workspace to the one specified by \a index.
	You must hold the all window lock when calling this method.
*/
void
Desktop::_SetWorkspace(int32 index, bool moveFocusWindow)
{
	ASSERT_MULTI_WRITE_LOCKED(fWindowLock);

	int32 previousIndex = fCurrentWorkspace;
	rgb_color previousColor = fWorkspaces[fCurrentWorkspace].Color();
	bool movedMouseEventWindow = false;
	Window* movedWindow = NULL;
	if (moveFocusWindow) {
		if (fMouseEventWindow != NULL)
			movedWindow = fMouseEventWindow;
		else
			movedWindow = FocusWindow();
	}

	if (movedWindow != NULL) {
		if (movedWindow->IsNormal()) {
			if (!movedWindow->InWorkspace(index)) {
				// The window currently being dragged will follow us to this
				// workspace if it's not already on it.
				// But only normal windows are following
				uint32 oldWorkspaces = movedWindow->Workspaces();

				_Windows(previousIndex).RemoveWindow(movedWindow);
				_Windows(index).AddWindow(movedWindow,
					movedWindow->Frontmost(_Windows(index).FirstWindow(),
					index));

				// TODO: subset windows will always flicker this way

				movedMouseEventWindow = true;

				// send B_WORKSPACES_CHANGED message
				movedWindow->WorkspacesChanged(oldWorkspaces,
					movedWindow->Workspaces());
				NotifyWindowWorkspacesChanged(movedWindow,
					movedWindow->Workspaces());
			} else {
				// make sure it's frontmost
				_Windows(index).RemoveWindow(movedWindow);
				_Windows(index).AddWindow(movedWindow,
					movedWindow->Frontmost(_Windows(index).FirstWindow(),
					index));
			}
		}

		movedWindow->Anchor(index).position = movedWindow->Frame().LeftTop();
	}

	if (movedWindow == NULL || movedWindow->InWorkspace(previousIndex))
		fLastWorkspaceFocus[previousIndex] = FocusWindow();
	else
		fLastWorkspaceFocus[previousIndex] = NULL;

	// build region of windows that are no longer visible in the new workspace

	BRegion dirty;

	for (Window* window = CurrentWindows().FirstWindow();
			window != NULL; window = window->NextWindow(previousIndex)) {
		// store current position in Workspace anchor
		window->Anchor(previousIndex).position = window->Frame().LeftTop();

		if (!window->IsHidden()
			&& window->ServerWindow()->IsDirectlyAccessing())
			window->ServerWindow()->HandleDirectConnection(B_DIRECT_STOP);

		window->WorkspaceActivated(previousIndex, false);

		if (window->InWorkspace(index))
			continue;

		if (!window->IsHidden()) {
			// this window will no longer be visible
			dirty.Include(&window->VisibleRegion());
		}

		window->SetCurrentWorkspace(-1);
	}

	fPreviousWorkspace = fCurrentWorkspace;
	fCurrentWorkspace = index;

	// Change the display modes, if needed
	_SetCurrentWorkspaceConfiguration();

	// Show windows, and include them in the changed region - but only
	// those that were not visible before (or whose position changed)

	WindowList windows(kWorkingList);
	BList previousRegions;

	for (Window* window = _Windows(index).FirstWindow();
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

		if (!window->InWorkspace(previousIndex)) {
			// This window was not visible before, make sure its frame
			// is up-to-date
			if (window->Frame().LeftTop() != position) {
				BPoint offset = position - window->Frame().LeftTop();
				window->MoveBy((int32)offset.x, (int32)offset.y);
			}
			continue;
		}

		if (window->Frame().LeftTop() != position) {
			// the window was visible before, but its on-screen location changed
			BPoint offset = position - window->Frame().LeftTop();
			MoveWindowBy(window, offset.x, offset.y);
				// TODO: be a bit smarter than this...
		} else {
			// We need to remember the previous visible region of the
			// window if they changed their order
			BRegion* region = new (std::nothrow)
				BRegion(window->VisibleRegion());
			if (region != NULL) {
				if (previousRegions.AddItem(region))
					windows.AddWindow(window);
				else
					delete region;
			}
		}
	}

	_UpdateFronts(false);
	_UpdateFloating(previousIndex, index,
		movedMouseEventWindow ? movedWindow : NULL);

	BRegion stillAvailableOnScreen;
	_RebuildClippingForAllWindows(stillAvailableOnScreen);
	_SetBackground(stillAvailableOnScreen);

	for (Window* window = _Windows(index).FirstWindow(); window != NULL;
			window = window->NextWindow(index)) {
		// send B_WORKSPACE_ACTIVATED message
		window->WorkspaceActivated(index, true);

		if (!window->IsHidden()
			&& window->ServerWindow()->HasDirectFrameBufferAccess()) {
			window->ServerWindow()->HandleDirectConnection(
				B_DIRECT_START | B_BUFFER_RESET, B_MODE_CHANGED);
		}

		if (window->InWorkspace(previousIndex) || window->IsHidden()
			|| (window == movedWindow && movedWindow->IsNormal())
			|| (!window->IsNormal()
				&& window->HasInSubset(movedWindow))) {
			// This window was visible before, and is already handled in the
			// above loop
			continue;
		}

		dirty.Include(&window->VisibleRegion());
	}

	// Catch order changes in the new workspaces window list
	int32 i = 0;
	for (Window* window = windows.FirstWindow(); window != NULL;
			window = window->NextWindow(kWorkingList), i++) {
		BRegion* region = (BRegion*)previousRegions.ItemAt(i);
		region->ExclusiveInclude(&window->VisibleRegion());
		dirty.Include(region);
		delete region;
	}

	// Set new focus, but keep focus to a floating window if still visible
	if (movedWindow != NULL)
		SetFocusWindow(movedWindow);
	else if (!_Windows(index).HasWindow(FocusWindow())
		|| (FocusWindow() != NULL && !FocusWindow()->IsFloating()))
		SetFocusWindow(fLastWorkspaceFocus[index]);

	_WindowChanged(NULL);
	MarkDirty(dirty);

#if 0
	// Show the dirty regions of this workspace switch
	if (GetDrawingEngine()->LockParallelAccess()) {
		GetDrawingEngine()->FillRegion(dirty, (rgb_color){255, 0, 0});
		GetDrawingEngine()->UnlockParallelAccess();
		snooze(100000);
	}
#endif

	if (previousColor != fWorkspaces[fCurrentWorkspace].Color())
		RedrawBackground();
}
