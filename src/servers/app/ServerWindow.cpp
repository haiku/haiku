/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Artur Wyszynski <harakash@gmail.com>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 *		Brecht Machiels <brecht@mos6581.org>
 */


/*!	\class ServerWindow

	The ServerWindow class handles all BWindow messaging; it forwards all
	BWindow requests to the corresponding app_server classes, that is Desktop,
	Window, and View.
	Furthermore, it also sends app_server requests/notices to its BWindow. There
	is one ServerWindow per BWindow.
*/


#include "ServerWindow.h"

#include <syslog.h>
#include <new>

#include <AppDefs.h>
#include <Autolock.h>
#include <Debug.h>
#include <DirectWindow.h>
#include <TokenSpace.h>
#include <View.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>

#include <MessagePrivate.h>
#include <PortLink.h>
#include <ServerProtocolStructs.h>
#include <ViewPrivate.h>
#include <WindowInfo.h>
#include <WindowPrivate.h>

#include "clipping.h"
#include "utf8_functions.h"

#include "AppServer.h"
#include "AutoDeleter.h"
#include "Desktop.h"
#include "DirectWindowInfo.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "HWInterface.h"
#include "Overlay.h"
#include "ProfileMessageSupport.h"
#include "RenderingBuffer.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerProtocol.h"
#include "Window.h"
#include "WorkspacesView.h"


using std::nothrow;


//#define TRACE_SERVER_WINDOW
#ifdef TRACE_SERVER_WINDOW
#	include <stdio.h>
#	define STRACE(x) debug_printf x
#else
#	define STRACE(x) ;
#endif

//#define TRACE_SERVER_WINDOW_MESSAGES
#ifdef TRACE_SERVER_WINDOW_MESSAGES
#	include <stdio.h>
static const char* kDrawingModeMap[] = {
	"B_OP_COPY",
	"B_OP_OVER",
	"B_OP_ERASE",
	"B_OP_INVERT",
	"B_OP_ADD",
	"B_OP_SUBTRACT",
	"B_OP_BLEND",
	"B_OP_MIN",
	"B_OP_MAX",
	"B_OP_SELECT",
	"B_OP_ALPHA",

	"fix kDrawingModeMap",
	"fix kDrawingModeMap",
	"fix kDrawingModeMap",
	"fix kDrawingModeMap",
	"fix kDrawingModeMap",
};
#	define DTRACE(x) debug_printf x
#else
#	define DTRACE(x) ;
#endif

//#define TRACE_SERVER_GRADIENTS
#ifdef TRACE_SERVER_GRADIENTS
#	include <OS.h>
#	define GTRACE(x) debug_printf x
#else
#	define GTRACE(x) ;
#endif

//#define PROFILE_MESSAGE_LOOP
#ifdef PROFILE_MESSAGE_LOOP
struct profile { int32 code; int32 count; bigtime_t time; };
static profile sMessageProfile[AS_LAST_CODE];
static profile sRedrawProcessingTime;
//static profile sNextMessageTime;
#endif


//	#pragma mark -


#ifdef PROFILE_MESSAGE_LOOP
static int
compare_message_profiles(const void* _a, const void* _b)
{
	profile* a = (profile*)*(void**)_a;
	profile* b = (profile*)*(void**)_b;
	if (a->time < b->time)
		return 1;
	if (a->time > b->time)
		return -1;
	return 0;
}
#endif


//	#pragma mark -


/*!	Sets up the basic BWindow counterpart - you have to call Init() before
	you can actually use it, though.
*/
ServerWindow::ServerWindow(const char* title, ServerApp* app,
		port_id clientPort, port_id looperPort, int32 clientToken)
	:
	MessageLooper(title && *title ? title : "Unnamed Window"),
	fTitle(NULL),
	fDesktop(app->GetDesktop()),
	fServerApp(app),
	fWindow(NULL),
	fWindowAddedToDesktop(false),

	fClientTeam(app->ClientTeam()),

	fMessagePort(-1),
	fClientReplyPort(clientPort),
	fClientLooperPort(looperPort),

	fClientToken(clientToken),

	fCurrentView(NULL),
	fCurrentDrawingRegion(),
	fCurrentDrawingRegionValid(false),

	fDirectWindowInfo(NULL),
	fIsDirectlyAccessing(false)
{
	STRACE(("ServerWindow(%s)::ServerWindow()\n", title));

	SetTitle(title);
	fServerToken = BPrivate::gDefaultTokens.NewToken(B_SERVER_TOKEN, this);

	BMessenger::Private(fFocusMessenger).SetTo(fClientTeam,
		looperPort, B_PREFERRED_TOKEN);
	BMessenger::Private(fHandlerMessenger).SetTo(fClientTeam,
		looperPort, clientToken);

	fEventTarget.SetTo(fFocusMessenger);

	fDeathSemaphore = create_sem(0, "window death");
}


/*! Tears down all connections the main app_server objects, and deletes some
	internals.
*/
ServerWindow::~ServerWindow()
{
	STRACE(("ServerWindow(%s@%p):~ServerWindow()\n", fTitle, this));

	if (!fWindow->IsOffscreenWindow()) {
		fWindowAddedToDesktop = false;
		fDesktop->RemoveWindow(fWindow);
	}

	if (App() != NULL) {
		App()->RemoveWindow(this);
		fServerApp = NULL;
	}

	delete fWindow;

	free(fTitle);
	delete_port(fMessagePort);

	BPrivate::gDefaultTokens.RemoveToken(fServerToken);

	delete fDirectWindowInfo;
	STRACE(("ServerWindow(%p) will exit NOW\n", this));

	delete_sem(fDeathSemaphore);

#ifdef PROFILE_MESSAGE_LOOP
	BList profiles;
	for (int32 i = 0; i < AS_LAST_CODE; i++) {
		if (sMessageProfile[i].count == 0)
			continue;
		sMessageProfile[i].code = i;
		profiles.AddItem(&sMessageProfile[i]);
	}

	profiles.SortItems(compare_message_profiles);

	BString codeName;
	int32 count = profiles.CountItems();
	for (int32 i = 0; i < count; i++) {
		profile* p = (profile*)profiles.ItemAtFast(i);
		string_for_message_code(p->code, codeName);
		printf("[%s] called %ld times, %g secs (%Ld usecs per call)\n",
			codeName.String(), p->count, p->time / 1000000.0,
			p->time / p->count);
	}
	if (sRedrawProcessingTime.count > 0) {
		printf("average redraw processing time: %g secs, count: %ld (%lld "
			"usecs per call)\n", sRedrawProcessingTime.time / 1000000.0,
			sRedrawProcessingTime.count,
			sRedrawProcessingTime.time / sRedrawProcessingTime.count);
	}
//	if (sNextMessageTime.count > 0) {
//		printf("average NextMessage() time: %g secs, count: %ld (%lld usecs per call)\n",
//			sNextMessageTime.time / 1000000.0, sNextMessageTime.count,
//			sNextMessageTime.time / sNextMessageTime.count);
//	}
#endif
}


status_t
ServerWindow::Init(BRect frame, window_look look, window_feel feel,
	uint32 flags, uint32 workspace)
{
	if (!App()->AddWindow(this)) {
		fServerApp = NULL;
		return B_NO_MEMORY;
	}

	if (fTitle == NULL)
		return B_NO_MEMORY;

	// fMessagePort is the port to which the app sends messages for the server
	fMessagePort = create_port(100, fTitle);
	if (fMessagePort < B_OK)
		return fMessagePort;

	fLink.SetSenderPort(fClientReplyPort);
	fLink.SetReceiverPort(fMessagePort);

	// We cannot call MakeWindow in the constructor, since it
	// is a virtual function!
	fWindow = MakeWindow(frame, fTitle, look, feel, flags, workspace);
	if (!fWindow || fWindow->InitCheck() != B_OK) {
		delete fWindow;
		fWindow = NULL;
		return B_NO_MEMORY;
	}

	if (!fWindow->IsOffscreenWindow()) {
		fDesktop->AddWindow(fWindow);
		fWindowAddedToDesktop = true;
	}

	return B_OK;
}


/*!	Returns the ServerWindow's Window, if it exists and has been
	added to the Desktop already.
	In other words, you cannot assume this method will always give you
	a valid pointer.
*/
Window*
ServerWindow::Window() const
{
	if (!fWindowAddedToDesktop)
		return NULL;

	return fWindow;
}


void
ServerWindow::_PrepareQuit()
{
	if (fThread == find_thread(NULL)) {
		// make sure we're hidden
		fDesktop->LockSingleWindow();
		_Hide();
		fDesktop->UnlockSingleWindow();
	} else if (fThread >= B_OK)
		PostMessage(AS_HIDE_WINDOW);
}


void
ServerWindow::_GetLooperName(char* name, size_t length)
{
	const char *title = Title();
	if (title == NULL || !title[0])
		title = "Unnamed Window";

	snprintf(name, length, "w:%ld:%s", ClientTeam(), title);
}


/*! Shows the window's Window.
*/
void
ServerWindow::_Show()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: _Show\n", Title()));

	if (fQuitting || fWindow->IsMinimized() || !fWindow->IsHidden()
		|| fWindow->IsOffscreenWindow() || fWindow->TopView() == NULL)
		return;

	// TODO: Maybe we need to dispatch a message to the desktop to show/hide us
	// instead of doing it from this thread.
	fDesktop->UnlockSingleWindow();
	fDesktop->ShowWindow(fWindow);
	if (fDirectWindowInfo && fDirectWindowInfo->IsFullScreen())
		_ResizeToFullScreen();
		
	fDesktop->LockSingleWindow();
}


/*! Hides the window's Window. You need to have all windows locked when
	calling this function.
*/
void
ServerWindow::_Hide()
{
	STRACE(("ServerWindow %s: _Hide\n", Title()));

	if (fWindow->IsHidden() || fWindow->IsOffscreenWindow())
		return;

	fDesktop->UnlockSingleWindow();
	fDesktop->HideWindow(fWindow);
	fDesktop->LockSingleWindow();
}


void
ServerWindow::RequestRedraw()
{
	PostMessage(AS_REDRAW, 0);
		// we don't care if this fails - it's only a notification, and if
		// it fails, there are obviously enough messages in the queue
		// already

	atomic_add(&fRedrawRequested, 1);
}


void
ServerWindow::SetTitle(const char* newTitle)
{
	char* oldTitle = fTitle;

	if (newTitle == NULL)
		newTitle = "";

	fTitle = strdup(newTitle);
	if (fTitle == NULL) {
		// out of memory condition
		fTitle = oldTitle;
		return;
	}

	free(oldTitle);

	if (Thread() >= B_OK) {
		char name[B_OS_NAME_LENGTH];
		_GetLooperName(name, sizeof(name));
		rename_thread(Thread(), name);
	}

	if (fWindow != NULL)
		fDesktop->SetWindowTitle(fWindow, newTitle);
}


//! Requests that the ServerWindow's BWindow quit
void
ServerWindow::NotifyQuitRequested()
{
	// NOTE: if you do something else, other than sending a port message,
	// PLEASE lock
	STRACE(("ServerWindow %s: Quit\n", fTitle));

	BMessage msg(B_QUIT_REQUESTED);
	SendMessageToClient(&msg);
}


void
ServerWindow::NotifyMinimize(bool minimize)
{
	if (fWindow->Feel() != B_NORMAL_WINDOW_FEEL)
		return;

	// The client is responsible for the actual minimization

	BMessage msg(B_MINIMIZE);
	msg.AddInt64("when", real_time_clock_usecs());
	msg.AddBool("minimize", minimize);

	SendMessageToClient(&msg);
}


//! Sends a message to the client to perform a Zoom
void
ServerWindow::NotifyZoom()
{
	// NOTE: if you do something else, other than sending a port message,
	// PLEASE lock
	BMessage msg(B_ZOOM);
	SendMessageToClient(&msg);
}


void
ServerWindow::GetInfo(window_info& info)
{
	info.team = ClientTeam();
	info.server_token = ServerToken();

	info.thread = Thread();
	info.client_token = ClientToken();
	info.client_port = fClientLooperPort;
	info.workspaces = fWindow->Workspaces();

	// logic taken from Switcher comments and experiments
	if (fWindow->IsHidden())
		info.layer = 0;
	else if (fWindow->IsVisible()) {
		if (fWindow->Feel() == kDesktopWindowFeel)
			info.layer = 2;
		else if (fWindow->IsFloating() || fWindow->IsModal())
			info.layer = 4;
		else
			info.layer = 3;
	} else
		info.layer = 1;

	info.feel = fWindow->Feel();
	info.flags = fWindow->Flags();
	info.window_left = (int)floor(fWindow->Frame().left);
	info.window_top = (int)floor(fWindow->Frame().top);
	info.window_right = (int)floor(fWindow->Frame().right);
	info.window_bottom = (int)floor(fWindow->Frame().bottom);

	// This is essentially opposite of the ShowLevel, meaning a window is
	// hidden if it is 1 or more, and shown if it is 0 or less.
	info.show_hide_level = fWindow->ShowLevel() <= 0 ? 1 : 0;
	info.is_mini = fWindow->IsMinimized();
}


void
ServerWindow::ResyncDrawState()
{
	_UpdateDrawState(fCurrentView);
}


View*
ServerWindow::_CreateView(BPrivate::LinkReceiver& link, View** _parent)
{
	// NOTE: no need to check for a lock. This is a private method.

	int32 token;
	BRect frame;
	uint32 resizeMask;
	uint32 eventMask;
	uint32 eventOptions;
	uint32 flags;
	bool hidden;
	int32 parentToken;
	char* name = NULL;
	rgb_color viewColor;
	BPoint scrollingOffset;

	link.Read<int32>(&token);
	link.ReadString(&name);
	link.Read<BRect>(&frame);
	link.Read<BPoint>(&scrollingOffset);
	link.Read<uint32>(&resizeMask);
	link.Read<uint32>(&eventMask);
	link.Read<uint32>(&eventOptions);
	link.Read<uint32>(&flags);
	link.Read<bool>(&hidden);
	link.Read<rgb_color>(&viewColor);
	link.Read<int32>(&parentToken);

	STRACE(("ServerWindow(%s)::_CreateView()-> view %s, token %ld\n",
		fTitle, name, token));

	View* newView;

	if ((flags & kWorkspacesViewFlag) != 0) {
		newView = new (nothrow) WorkspacesView(frame, scrollingOffset, name,
			token, resizeMask, flags);
	} else {
		newView = new (nothrow) View(frame, scrollingOffset, name, token,
			resizeMask, flags);
	}

	free(name);

	if (newView == NULL)
		return NULL;

	if (newView->InitCheck() != B_OK) {
		delete newView;
		return NULL;
	}

	// there is no way of setting this, other than manually :-)
	newView->SetViewColor(viewColor);
	newView->SetHidden(hidden);
	newView->SetEventMask(eventMask, eventOptions);

	if (eventMask != 0 || eventOptions != 0) {
//		fDesktop->UnlockSingleWindow();
//		fDesktop->LockAllWindows();
fDesktop->UnlockAllWindows();
		// TODO: possible deadlock
		fDesktop->EventDispatcher().AddListener(EventTarget(),
			newView->Token(), eventMask, eventOptions);
fDesktop->LockAllWindows();
//		fDesktop->UnlockAllWindows();
//		fDesktop->LockSingleWindow();
	}

	// Initialize the view with the current application plain font.
	// NOTE: This might be out of sync with the global app_server plain
	// font, but that is so on purpose! The client needs to resync itself
	// with the app_server fonts upon notification, but if we just use
	// the current font here, the be_plain_font on the client may still
	// hold old values. So this needs to be an update initiated by the
	// client application.
	newView->CurrentState()->SetFont(App()->PlainFont());

	if (_parent) {
		View *parent;
		if (App()->ViewTokens().GetToken(parentToken, B_HANDLER_TOKEN,
				(void**)&parent) != B_OK
			|| parent->Window()->ServerWindow() != this) {
			debug_printf("View token not found!\n");
			parent = NULL;
		}

		*_parent = parent;
	}

	return newView;
}


/*!	Dispatches all window messages, and those view messages that
	don't need a valid fCurrentView (ie. view creation).
*/
void
ServerWindow::_DispatchMessage(int32 code, BPrivate::LinkReceiver& link)
{
	switch (code) {
		case AS_SHOW_WINDOW:
		{
			DTRACE(("ServerWindow %s: Message AS_SHOW_WINDOW\n", Title()));
			_Show();

			int32 showLevel;
			if (link.Read<int32>(&showLevel) == B_OK) {
				fWindow->SetShowLevel(showLevel);
			}
			break;

		}
		case AS_HIDE_WINDOW:
		{
			DTRACE(("ServerWindow %s: Message AS_HIDE_WINDOW\n", Title()));
			_Hide();

			int32 showLevel;
			if (link.Read<int32>(&showLevel) == B_OK) {
				fWindow->SetShowLevel(showLevel);
			}
			break;
		}
		case AS_MINIMIZE_WINDOW:
		{
			bool minimize;

			if (link.Read<bool>(&minimize) == B_OK) {
				DTRACE(("ServerWindow %s: Message AS_MINIMIZE_WINDOW, "
					"minimize: %d\n", Title(), minimize));

				if (fWindow->ShowLevel() <= 0) {
					// Window is currently hidden - ignore the minimize
					// request, but keep the state in sync.
					fWindow->SetMinimized(minimize);
					break;
				}

				fDesktop->UnlockSingleWindow();
				fDesktop->MinimizeWindow(fWindow, minimize);
				fDesktop->LockSingleWindow();
			}
			break;
		}

		case AS_ACTIVATE_WINDOW:
		{
			bool activate = true;
			if (link.Read<bool>(&activate) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_ACTIVATE_WINDOW: activate: "
				"%d\n", Title(), activate));

			fDesktop->UnlockSingleWindow();

			if (activate)
				fDesktop->SelectWindow(fWindow);
			else
				fDesktop->SendWindowBehind(fWindow, NULL);

			fDesktop->LockSingleWindow();
			break;
		}
		case AS_SEND_BEHIND:
		{
			// Has the all-window lock
			int32 token;
			team_id teamID;
			status_t status = B_ERROR;

			link.Read<int32>(&token);
			if (link.Read<team_id>(&teamID) == B_OK) {
				::Window* behindOf = fDesktop->FindWindowByClientToken(token,
					teamID);

				DTRACE(("ServerWindow %s: Message AS_SEND_BEHIND %s\n",
					Title(), behindOf != NULL ? behindOf->Title() : "NULL"));

				if (behindOf != NULL || token == -1) {
					fDesktop->SendWindowBehind(fWindow, behindOf);
					status = B_OK;
				} else
					status = B_NAME_NOT_FOUND;
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}

		case B_QUIT_REQUESTED:
			DTRACE(("ServerWindow %s received quit request\n", Title()));
			NotifyQuitRequested();
			break;

		case AS_ENABLE_UPDATES:
			DTRACE(("ServerWindow %s: Message AS_ENABLE_UPDATES\n", Title()));
			fWindow->EnableUpdateRequests();
			break;

		case AS_DISABLE_UPDATES:
			DTRACE(("ServerWindow %s: Message AS_DISABLE_UPDATES\n", Title()));
			fWindow->DisableUpdateRequests();
			break;

		case AS_NEEDS_UPDATE:
			DTRACE(("ServerWindow %s: Message AS_NEEDS_UPDATE: %d\n",
				Title(), fWindow->NeedsUpdate()));
			if (fWindow->NeedsUpdate())
				fLink.StartMessage(B_OK);
			else
				fLink.StartMessage(B_ERROR);
			fLink.Flush();
			break;

		case AS_SET_WINDOW_TITLE:
		{
			char* newTitle;
			if (link.ReadString(&newTitle) == B_OK) {
				DTRACE(("ServerWindow %s: Message AS_SET_WINDOW_TITLE: %s\n",
					Title(), newTitle));

				SetTitle(newTitle);
				free(newTitle);
			}
			break;
		}

		case AS_ADD_TO_SUBSET:
		{
			// Has the all-window lock
			DTRACE(("ServerWindow %s: Message AS_ADD_TO_SUBSET\n", Title()));
			status_t status = B_ERROR;

			int32 token;
			if (link.Read<int32>(&token) == B_OK) {
				::Window* window = fDesktop->FindWindowByClientToken(token,
					App()->ClientTeam());
				if (window == NULL || window->Feel() != B_NORMAL_WINDOW_FEEL) {
					status = B_BAD_VALUE;
				} else {
					status = fDesktop->AddWindowToSubset(fWindow, window)
						? B_OK : B_NO_MEMORY;
				}
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
		case AS_REMOVE_FROM_SUBSET:
		{
			// Has the all-window lock
			DTRACE(("ServerWindow %s: Message AS_REM_FROM_SUBSET\n", Title()));
			status_t status = B_ERROR;

			int32 token;
			if (link.Read<int32>(&token) == B_OK) {
				::Window* window = fDesktop->FindWindowByClientToken(token,
					App()->ClientTeam());
				if (window != NULL) {
					fDesktop->RemoveWindowFromSubset(fWindow, window);
					status = B_OK;
				} else
					status = B_BAD_VALUE;
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}

		case AS_SET_LOOK:
		{
			// Has the all-window look
			DTRACE(("ServerWindow %s: Message AS_SET_LOOK\n", Title()));

			status_t status = B_ERROR;
			int32 look;
			if (link.Read<int32>(&look) == B_OK) {
				// test if look is valid
				status = Window::IsValidLook((window_look)look)
					? B_OK : B_BAD_VALUE;
			}

			if (status == B_OK && !fWindow->IsOffscreenWindow())
				fDesktop->SetWindowLook(fWindow, (window_look)look);

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
		case AS_SET_FEEL:
		{
			// Has the all-window look
			DTRACE(("ServerWindow %s: Message AS_SET_FEEL\n", Title()));

			status_t status = B_ERROR;
			int32 feel;
			if (link.Read<int32>(&feel) == B_OK) {
				// test if feel is valid
				status = Window::IsValidFeel((window_feel)feel)
					? B_OK : B_BAD_VALUE;
			}

			if (status == B_OK && !fWindow->IsOffscreenWindow())
				fDesktop->SetWindowFeel(fWindow, (window_feel)feel);

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
		case AS_SET_FLAGS:
		{
			// Has the all-window look
			DTRACE(("ServerWindow %s: Message AS_SET_FLAGS\n", Title()));

			status_t status = B_ERROR;
			uint32 flags;
			if (link.Read<uint32>(&flags) == B_OK) {
				// test if flags are valid
				status = (flags & ~Window::ValidWindowFlags()) == 0
					? B_OK : B_BAD_VALUE;
			}

			if (status == B_OK && !fWindow->IsOffscreenWindow())
				fDesktop->SetWindowFlags(fWindow, flags);

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
#if 0
		case AS_SET_ALIGNMENT:
		{
			// TODO: Implement AS_SET_ALIGNMENT
			DTRACE(("ServerWindow %s: Message Set_Alignment unimplemented\n",
				Title()));
			break;
		}
		case AS_GET_ALIGNMENT:
		{
			// TODO: Implement AS_GET_ALIGNMENT
			DTRACE(("ServerWindow %s: Message Get_Alignment unimplemented\n",
				Title()));
			break;
		}
#endif
		case AS_IS_FRONT_WINDOW:
		{
			bool isFront = fDesktop->FrontWindow() == fWindow;
			DTRACE(("ServerWindow %s: Message AS_IS_FRONT_WINDOW: %d\n",
				Title(), isFront));
			fLink.StartMessage(isFront ? B_OK : B_ERROR);
			fLink.Flush();
			break;
		}

		case AS_GET_WORKSPACES:
		{
			DTRACE(("ServerWindow %s: Message AS_GET_WORKSPACES\n", Title()));
			fLink.StartMessage(B_OK);
			fLink.Attach<uint32>(fWindow->Workspaces());
			fLink.Flush();
			break;
		}
		case AS_SET_WORKSPACES:
		{
			// Has the all-window lock (but would actually not need to lock at
			// all)
			uint32 newWorkspaces;
			if (link.Read<uint32>(&newWorkspaces) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_SET_WORKSPACES %lx\n",
				Title(), newWorkspaces));

			fDesktop->SetWindowWorkspaces(fWindow, newWorkspaces);
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			// Has the all-window look
			float xResizeTo;
			float yResizeTo;
			link.Read<float>(&xResizeTo);
			if (link.Read<float>(&yResizeTo) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_WINDOW_RESIZE %.1f, %.1f\n",
				Title(), xResizeTo, yResizeTo));

			// comment this code for the time being, as some apps rely
			// on the programmatically resize behavior during user resize
//			if (fWindow->IsResizing()) {
				// While the user resizes the window, we ignore
				// pragmatically set window bounds
//				fLink.StartMessage(B_BUSY);
//			} else {
				fDesktop->ResizeWindowBy(fWindow,
					xResizeTo - fWindow->Frame().Width(),
					yResizeTo - fWindow->Frame().Height());
				fLink.StartMessage(B_OK);
//			}
			fLink.Flush();
			break;
		}
		case AS_WINDOW_MOVE:
		{
			// Has the all-window look
			float xMoveTo;
			float yMoveTo;
			link.Read<float>(&xMoveTo);
			if (link.Read<float>(&yMoveTo) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_WINDOW_MOVE: %.1f, %.1f\n",
				Title(), xMoveTo, yMoveTo));

			if (fWindow->IsDragging()) {
				// While the user moves the window, we ignore
				// pragmatically set window positions
				fLink.StartMessage(B_BUSY);
			} else {
				fDesktop->MoveWindowBy(fWindow, xMoveTo - fWindow->Frame().left,
					yMoveTo - fWindow->Frame().top);
				fLink.StartMessage(B_OK);
			}
			fLink.Flush();
			break;
		}
		case AS_SET_SIZE_LIMITS:
		{
			// Has the all-window look

			// Attached Data:
			// 1) float minimum width
			// 2) float maximum width
			// 3) float minimum height
			// 4) float maximum height

			// TODO: for now, move the client to int32 as well!
			int32 minWidth, maxWidth, minHeight, maxHeight;
			float value;
			link.Read<float>(&value);	minWidth = (int32)value;
			link.Read<float>(&value);	maxWidth = (int32)value;
			link.Read<float>(&value);	minHeight = (int32)value;
			link.Read<float>(&value);	maxHeight = (int32)value;
/*
			link.Read<int32>(&minWidth);
			link.Read<int32>(&maxWidth);
			link.Read<int32>(&minHeight);
			link.Read<int32>(&maxHeight);
*/
			DTRACE(("ServerWindow %s: Message AS_SET_SIZE_LIMITS: "
				"x: %ld-%ld, y: %ld-%ld\n",
				Title(), minWidth, maxWidth, minHeight, maxHeight));

			fWindow->SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

			// and now, sync the client to the limits that we were able to enforce
			fWindow->GetSizeLimits(&minWidth, &maxWidth,
				&minHeight, &maxHeight);

			fLink.StartMessage(B_OK);
			fLink.Attach<BRect>(fWindow->Frame());
			fLink.Attach<float>((float)minWidth);
			fLink.Attach<float>((float)maxWidth);
			fLink.Attach<float>((float)minHeight);
			fLink.Attach<float>((float)maxHeight);

			fLink.Flush();

			fDesktop->NotifySizeLimitsChanged(fWindow, minWidth, maxWidth,
				minHeight, maxHeight);
			break;
		}

		case AS_SET_DECORATOR_SETTINGS:
		{
			// Has the all-window look
			DTRACE(("ServerWindow %s: Message AS_SET_DECORATOR_SETTINGS\n",
				Title()));

			int32 size;
			if (fWindow && link.Read<int32>(&size) == B_OK) {
				char buffer[size];
				if (link.Read(buffer, size) == B_OK) {
					BMessage settings;
					if (settings.Unflatten(buffer) == B_OK)
						fDesktop->SetWindowDecoratorSettings(fWindow, settings);
				}
			}
			break;
		}

		case AS_GET_DECORATOR_SETTINGS:
		{
			DTRACE(("ServerWindow %s: Message AS_GET_DECORATOR_SETTINGS\n",
				Title()));

			bool success = false;

			BMessage settings;
			if (fWindow->GetDecoratorSettings(&settings)) {
				int32 size = settings.FlattenedSize();
				char buffer[size];
				if (settings.Flatten(buffer, size) == B_OK) {
					success = true;
					fLink.StartMessage(B_OK);
					fLink.Attach<int32>(size);
					fLink.Attach(buffer, size);
				}
			}

			if (!success)
				fLink.StartMessage(B_ERROR);

			fLink.Flush();
			break;
		}

		case AS_SYSTEM_FONT_CHANGED:
		{
			// Has the all-window look
			fDesktop->FontsChanged(fWindow);
			// TODO: tell client about this, too, and relayout...
			break;
		}

		case AS_REDRAW:
			// Nothing to do here - the redraws are actually handled by looking
			// at the fRedrawRequested member variable in _MessageLooper().
			break;

		case AS_SYNC:
			DTRACE(("ServerWindow %s: Message AS_SYNC\n", Title()));
			// the synchronisation works by the fact that the client
			// window is waiting for this reply, after having received it,
			// client and server queues are in sync (earlier, the client
			// may have pushed drawing commands at the server and now it
			// knows they have all been carried out)
			fLink.StartMessage(B_OK);
			fLink.Flush();
			break;

		case AS_BEGIN_UPDATE:
			DTRACE(("ServerWindow %s: Message AS_BEGIN_UPDATE\n", Title()));
			fWindow->BeginUpdate(fLink);
			break;

		case AS_END_UPDATE:
			DTRACE(("ServerWindow %s: Message AS_END_UPDATE\n", Title()));
			fWindow->EndUpdate();
			break;

		case AS_GET_MOUSE:
		{
			// Has the all-window look
			DTRACE(("ServerWindow %s: Message AS_GET_MOUSE\n", fTitle));

			// Returns
			// 1) BPoint mouse location
			// 2) int32 button state

			BPoint where;
			int32 buttons;
			fDesktop->GetLastMouseState(&where, &buttons);

			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(where);
			fLink.Attach<int32>(buttons);
			fLink.Flush();
			break;
		}

		// BDirectWindow communication

		case AS_DIRECT_WINDOW_GET_SYNC_DATA:
		{
			status_t status = _EnableDirectWindowMode();

			fLink.StartMessage(status);
			if (status == B_OK) {
				struct direct_window_sync_data syncData;
				fDirectWindowInfo->GetSyncData(syncData);

				fLink.Attach(&syncData, sizeof(syncData));
			}

			fLink.Flush();
			break;
		}
		case AS_DIRECT_WINDOW_SET_FULLSCREEN:
		{
			// Has the all-window look
			bool enable;
			link.Read<bool>(&enable);

			status_t status = B_OK;
			if (fDirectWindowInfo != NULL)
				_DirectWindowSetFullScreen(enable);
			else
				status = B_BAD_TYPE;

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}

		// View creation and destruction (don't need a valid fCurrentView)

		case AS_SET_CURRENT_VIEW:
		{
			int32 token;
			if (link.Read<int32>(&token) != B_OK)
				break;

			View *current;
			if (App()->ViewTokens().GetToken(token, B_HANDLER_TOKEN,
					(void**)&current) != B_OK
				|| current->Window()->ServerWindow() != this) {
				// TODO: if this happens, we probably want to kill the app and
				// clean up
				debug_printf("ServerWindow %s: Message "
					"\n\n\nAS_SET_CURRENT_VIEW: view not found, token %ld\n",
					fTitle, token);
				current = NULL;
			} else {
				DTRACE(("\n\n\nServerWindow %s: Message AS_SET_CURRENT_VIEW: %s, "
					"token %ld\n", fTitle, current->Name(), token));
				_SetCurrentView(current);
			}
			break;
		}

		case AS_VIEW_CREATE_ROOT:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_CREATE_ROOT\n", fTitle));

			// Start receiving top_view data -- pass NULL as the parent view.
			// This should be the *only* place where this happens.
			if (fCurrentView != NULL) {
				debug_printf("ServerWindow %s: Message "
					"AS_VIEW_CREATE_ROOT: fCurrentView already set!!\n",
					fTitle);
				break;
			}

			_SetCurrentView(_CreateView(link, NULL));
			fWindow->SetTopView(fCurrentView);
			break;
		}

		case AS_VIEW_CREATE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_CREATE: View name: "
				"%s\n", fTitle, fCurrentView->Name()));

			View* parent = NULL;
			View* newView = _CreateView(link, &parent);
			if (parent != NULL && newView != NULL)
				parent->AddChild(newView);
			else {
				delete newView;
				debug_printf("ServerWindow %s: Message AS_VIEW_CREATE: "
					"parent or newView NULL!!\n", fTitle);
			}
			break;
		}

		case AS_TALK_TO_DESKTOP_LISTENER:
		{
			if (fDesktop->MessageForListener(fWindow, fLink.Receiver(),
				fLink.Sender()))
				break;
			// unhandled message at least send an error if needed
			if (link.NeedsReply()) {
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
		}

		default:
			if (fCurrentView == NULL) {
				BString codeName;
				string_for_message_code(code, codeName);
				debug_printf("ServerWindow %s received unexpected code - "
					"message '%s' before top_view attached.\n",
					Title(), codeName.String());
				if (link.NeedsReply()) {
					fLink.StartMessage(B_ERROR);
					fLink.Flush();
				}
				return;
			}

			_DispatchViewMessage(code, link);
			break;
	}
}


/*!
	Dispatches all view messages that need a valid fCurrentView.
*/
void
ServerWindow::_DispatchViewMessage(int32 code,
	BPrivate::LinkReceiver &link)
{
	if (_DispatchPictureMessage(code, link))
		return;

	switch (code) {
		case AS_VIEW_SCROLL:
		{
			float dh;
			float dv;
			link.Read<float>(&dh);
			if (link.Read<float>(&dv) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SCROLL: View name: "
				"%s, %.1f x %.1f\n", fTitle, fCurrentView->Name(), dh, dv));
			fWindow->ScrollViewBy(fCurrentView, dh, dv);
			break;
		}
		case AS_VIEW_COPY_BITS:
		{
			BRect src;
			BRect dst;

			link.Read<BRect>(&src);
			if (link.Read<BRect>(&dst) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_COPY_BITS: View name: "
				"%s, BRect(%.1f, %.1f, %.1f, %.1f) -> "
				"BRect(%.1f, %.1f, %.1f, %.1f)\n", fTitle,
				fCurrentView->Name(), src.left, src.top, src.right, src.bottom,
				dst.left, dst.top, dst.right, dst.bottom));

			BRegion contentRegion;
			// TODO: avoid copy operation maybe?
			fWindow->GetContentRegion(&contentRegion);
			fCurrentView->CopyBits(src, dst, contentRegion);
			break;
		}
		case AS_VIEW_DELETE:
		{
			// Received when a view is detached from a window

			int32 token;
			if (link.Read<int32>(&token) != B_OK)
				break;

			View *view;
			if (App()->ViewTokens().GetToken(token, B_HANDLER_TOKEN,
					(void**)&view) == B_OK
				&& view->Window()->ServerWindow() == this) {
				View* parent = view->Parent();

				DTRACE(("ServerWindow %s: AS_VIEW_DELETE view: %p, "
					"parent: %p\n", fTitle, view, parent));

				if (parent != NULL) {
					parent->RemoveChild(view);

					if (view->EventMask() != 0) {
						// TODO: possible deadlock (event dispatcher already
						// locked itself, waits for Desktop write lock, but
						// we have it, now we are trying to lock the event
						// dispatcher -> deadlock)
fDesktop->UnlockSingleWindow();
						fDesktop->EventDispatcher().RemoveListener(
							EventTarget(), token);
fDesktop->LockSingleWindow();
					}
					if (fCurrentView == view)
						_SetCurrentView(parent);
					delete view;
				} // else we don't delete the root view
			}
			break;
		}
		case AS_VIEW_SET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_STATE: "
				"View name: %s\n", fTitle, fCurrentView->Name()));

			fCurrentView->CurrentState()->ReadFromLink(link);
			// TODO: When is this used?!?
			fCurrentView->RebuildClipping(true);
			_UpdateDrawState(fCurrentView);

			break;
		}
		case AS_VIEW_SET_FONT_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_FONT_STATE: "
				"View name: %s\n", fTitle, fCurrentView->Name()));

			fCurrentView->CurrentState()->ReadFontFromLink(link);
			fWindow->GetDrawingEngine()->SetFont(
				fCurrentView->CurrentState());
			break;
		}
		case AS_VIEW_GET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_STATE: "
				"View name: %s\n", fTitle, fCurrentView->Name()));

			fLink.StartMessage(B_OK);

			// attach state data
			fCurrentView->CurrentState()->WriteToLink(fLink.Sender());
			fLink.Flush();
			break;
		}
		case AS_VIEW_SET_EVENT_MASK:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_EVENT_MASK: "
				"View name: %s\n", fTitle, fCurrentView->Name()));
			uint32 eventMask, options;

			link.Read<uint32>(&eventMask);
			if (link.Read<uint32>(&options) == B_OK) {
				fCurrentView->SetEventMask(eventMask, options);

fDesktop->UnlockSingleWindow();
				// TODO: possible deadlock!
				if (eventMask != 0 || options != 0) {
					fDesktop->EventDispatcher().AddListener(EventTarget(),
						fCurrentView->Token(), eventMask, options);
				} else {
					fDesktop->EventDispatcher().RemoveListener(EventTarget(),
						fCurrentView->Token());
				}
fDesktop->LockSingleWindow();
			}
			break;
		}
		case AS_VIEW_SET_MOUSE_EVENT_MASK:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_MOUSE_EVENT_MASK: "
				"View name: %s\n", fTitle, fCurrentView->Name()));
			uint32 eventMask, options;

			link.Read<uint32>(&eventMask);
			if (link.Read<uint32>(&options) == B_OK) {
fDesktop->UnlockSingleWindow();
				// TODO: possible deadlock
				if (eventMask != 0 || options != 0) {
					if (options & B_LOCK_WINDOW_FOCUS)
						fDesktop->SetFocusLocked(fWindow);
					fDesktop->EventDispatcher().AddTemporaryListener(EventTarget(),
						fCurrentView->Token(), eventMask, options);
				} else {
					fDesktop->EventDispatcher().RemoveTemporaryListener(EventTarget(),
						fCurrentView->Token());
				}
fDesktop->LockSingleWindow();
			}

			// TODO: support B_LOCK_WINDOW_FOCUS option in Desktop
			break;
		}
		case AS_VIEW_MOVE_TO:
		{
			float x, y;
			link.Read<float>(&x);
			if (link.Read<float>(&y) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_MOVE_TO: View name: "
				"%s, x: %.1f, y: %.1f\n", fTitle, fCurrentView->Name(), x, y));

			float offsetX = x - fCurrentView->Frame().left;
			float offsetY = y - fCurrentView->Frame().top;

			BRegion dirty;
			fCurrentView->MoveBy(offsetX, offsetY, &dirty);

			// TODO: think about how to avoid this hack:
			// the parent clipping needs to be updated, it is not
			// done in MoveBy() since it would cause
			// too much computations when children are resized because
			// follow modes
			if (View* parent = fCurrentView->Parent())
				parent->RebuildClipping(false);

			fWindow->MarkContentDirty(dirty);
			break;
		}
		case AS_VIEW_RESIZE_TO:
		{
			float newWidth, newHeight;
			link.Read<float>(&newWidth);
			if (link.Read<float>(&newHeight) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_RESIZE_TO: View name: "
				"%s, width: %.1f, height: %.1f\n", fTitle,
				fCurrentView->Name(), newWidth, newHeight));

			float deltaWidth = newWidth - fCurrentView->Frame().Width();
			float deltaHeight = newHeight - fCurrentView->Frame().Height();

			BRegion dirty;
			fCurrentView->ResizeBy(deltaWidth, deltaHeight, &dirty);

			// TODO: see above
			if (View* parent = fCurrentView->Parent())
				parent->RebuildClipping(false);

			fWindow->MarkContentDirty(dirty);
			break;
		}
		case AS_VIEW_GET_COORD:
		{
			// our offset in the parent -> will be originX and originY
			// in BView
			BPoint parentOffset = fCurrentView->Frame().LeftTop();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_COORD: "
				"View: %s -> x: %.1f, y: %.1f\n", Title(),
				fCurrentView->Name(), parentOffset.x, parentOffset.y));

			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(parentOffset);
			fLink.Attach<BRect>(fCurrentView->Bounds());
			fLink.Flush();
			break;
		}
		case AS_VIEW_SET_ORIGIN:
		{
			float x, y;
			link.Read<float>(&x);
			if (link.Read<float>(&y) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_ORIGIN: "
				"View: %s -> x: %.1f, y: %.1f\n", Title(),
				fCurrentView->Name(), x, y));

			fCurrentView->SetDrawingOrigin(BPoint(x, y));
			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_VIEW_GET_ORIGIN:
		{
			BPoint drawingOrigin = fCurrentView->DrawingOrigin();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_ORIGIN: "
				"View: %s -> x: %.1f, y: %.1f\n", Title(),
				fCurrentView->Name(), drawingOrigin.x, drawingOrigin.y));

			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(drawingOrigin);
			fLink.Flush();
			break;
		}
		case AS_VIEW_RESIZE_MODE:
		{
			uint32 resizeMode;
			if (link.Read<uint32>(&resizeMode) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_RESIZE_MODE: "
				"View: %s -> %ld\n", Title(), fCurrentView->Name(),
				resizeMode));

			fCurrentView->SetResizeMode(resizeMode);
			break;
		}
		case AS_VIEW_SET_FLAGS:
		{
			uint32 flags;
			link.Read<uint32>(&flags);

			// The views clipping changes when the B_DRAW_ON_CHILDREN flag is
			// toggled.
			bool updateClipping = (flags & B_DRAW_ON_CHILDREN)
				^ (fCurrentView->Flags() & B_DRAW_ON_CHILDREN);

			fCurrentView->SetFlags(flags);
			_UpdateDrawState(fCurrentView);

			if (updateClipping) {
				fCurrentView->RebuildClipping(false);
				fCurrentDrawingRegionValid = false;
			}

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_FLAGS: "
				"View: %s -> flags: %lu\n", Title(), fCurrentView->Name(),
				flags));
			break;
		}
		case AS_VIEW_HIDE:
			DTRACE(("ServerWindow %s: Message AS_VIEW_HIDE: View: %s\n",
				Title(), fCurrentView->Name()));
			fCurrentView->SetHidden(true);
			break;

		case AS_VIEW_SHOW:
			DTRACE(("ServerWindow %s: Message AS_VIEW_SHOW: View: %s\n",
				Title(), fCurrentView->Name()));
			fCurrentView->SetHidden(false);
			break;

		case AS_VIEW_SET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_LINE_MODE: "
				"View: %s\n", Title(), fCurrentView->Name()));
			ViewSetLineModeInfo info;
			if (link.Read<ViewSetLineModeInfo>(&info) != B_OK)
				break;

			fCurrentView->CurrentState()->SetLineCapMode(info.lineCap);
			fCurrentView->CurrentState()->SetLineJoinMode(info.lineJoin);
			fCurrentView->CurrentState()->SetMiterLimit(info.miterLimit);

			fWindow->GetDrawingEngine()->SetStrokeMode(info.lineCap,
				info.lineJoin, info.miterLimit);

			break;
		}
		case AS_VIEW_GET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_LINE_MODE: "
				"View: %s\n", Title(), fCurrentView->Name()));
			ViewSetLineModeInfo info;
			info.lineJoin = fCurrentView->CurrentState()->LineJoinMode();
			info.lineCap = fCurrentView->CurrentState()->LineCapMode();
			info.miterLimit = fCurrentView->CurrentState()->MiterLimit();

			fLink.StartMessage(B_OK);
			fLink.Attach<ViewSetLineModeInfo>(info);
			fLink.Flush();

			break;
		}
		case AS_VIEW_PUSH_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_PUSH_STATE: View: "
				"%s\n", Title(), fCurrentView->Name()));

			fCurrentView->PushState();
			// TODO: is this necessary?
//			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_VIEW_POP_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_POP_STATE: View: %s\n",
				Title(), fCurrentView->Name()));

			fCurrentView->PopState();
			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_VIEW_SET_SCALE:
		{
			float scale;
			if (link.Read<float>(&scale) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_SCALE: "
				"View: %s -> scale: %.2f\n", Title(), fCurrentView->Name(),
				scale));

			fCurrentView->SetScale(scale);
			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_VIEW_GET_SCALE:
		{
			float scale = fCurrentView->CurrentState()->Scale();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_SCALE: "
				"View: %s -> scale: %.2f\n",
				Title(), fCurrentView->Name(), scale));

			fLink.StartMessage(B_OK);
			fLink.Attach<float>(scale);
			fLink.Flush();
			break;
		}
		case AS_VIEW_SET_PEN_LOC:
		{
			BPoint location;
			if (link.Read<BPoint>(&location) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_PEN_LOC: "
				"View: %s -> BPoint(%.1f, %.1f)\n", Title(),
				fCurrentView->Name(), location.x, location.y));

			fCurrentView->CurrentState()->SetPenLocation(location);
			break;
		}
		case AS_VIEW_GET_PEN_LOC:
		{
			BPoint location = fCurrentView->CurrentState()->PenLocation();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_PEN_LOC: "
				"View: %s -> BPoint(%.1f, %.1f)\n", Title(),
				fCurrentView->Name(), location.x, location.y));

			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(location);
			fLink.Flush();

			break;
		}
		case AS_VIEW_SET_PEN_SIZE:
		{
			float penSize;
			if (link.Read<float>(&penSize) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_PEN_SIZE: "
				"View: %s -> %.1f\n", Title(), fCurrentView->Name(), penSize));

			fCurrentView->CurrentState()->SetPenSize(penSize);
			fWindow->GetDrawingEngine()->SetPenSize(
				fCurrentView->CurrentState()->PenSize());
			break;
		}
		case AS_VIEW_GET_PEN_SIZE:
		{
			float penSize = fCurrentView->CurrentState()->UnscaledPenSize();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_PEN_SIZE: "
				"View: %s -> %.1f\n", Title(), fCurrentView->Name(), penSize));

			fLink.StartMessage(B_OK);
			fLink.Attach<float>(penSize);
			fLink.Flush();

			break;
		}
		case AS_VIEW_SET_VIEW_COLOR:
		{
			rgb_color color;
			if (link.Read(&color, sizeof(rgb_color)) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_VIEW_COLOR: "
				"View: %s -> rgb_color(%d, %d, %d, %d)\n", Title(),
				fCurrentView->Name(), color.red, color.green, color.blue,
				color.alpha));

			fCurrentView->SetViewColor(color);
			break;
		}
		case AS_VIEW_GET_VIEW_COLOR:
		{
			rgb_color color = fCurrentView->ViewColor();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_VIEW_COLOR: "
				"View: %s -> rgb_color(%d, %d, %d, %d)\n",
				Title(), fCurrentView->Name(), color.red, color.green,
				color.blue, color.alpha));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(color);
			fLink.Flush();
			break;
		}
		case AS_VIEW_SET_HIGH_COLOR:
		{
			rgb_color color;
			if (link.Read(&color, sizeof(rgb_color)) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_HIGH_COLOR: "
				"View: %s -> rgb_color(%d, %d, %d, %d)\n",
				Title(), fCurrentView->Name(), color.red, color.green,
				color.blue, color.alpha));

			fCurrentView->CurrentState()->SetHighColor(color);
			fWindow->GetDrawingEngine()->SetHighColor(color);
			break;
		}
		case AS_VIEW_GET_HIGH_COLOR:
		{
			rgb_color color = fCurrentView->CurrentState()->HighColor();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_HIGH_COLOR: "
				"View: %s -> rgb_color(%d, %d, %d, %d)\n",
				Title(), fCurrentView->Name(), color.red, color.green,
				color.blue, color.alpha));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(color);
			fLink.Flush();
			break;
		}
		case AS_VIEW_SET_LOW_COLOR:
		{
			rgb_color color;
			if (link.Read(&color, sizeof(rgb_color)) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_LOW_COLOR: "
				"View: %s -> rgb_color(%d, %d, %d, %d)\n",
				Title(), fCurrentView->Name(), color.red, color.green,
				color.blue, color.alpha));

			fCurrentView->CurrentState()->SetLowColor(color);
			fWindow->GetDrawingEngine()->SetLowColor(color);
			break;
		}
		case AS_VIEW_GET_LOW_COLOR:
		{
			rgb_color color = fCurrentView->CurrentState()->LowColor();

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_LOW_COLOR: "
				"View: %s -> rgb_color(%d, %d, %d, %d)\n",
				Title(), fCurrentView->Name(), color.red, color.green,
				color.blue, color.alpha));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(color);
			fLink.Flush();
			break;
		}
		case AS_VIEW_SET_PATTERN:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_PATTERN: "
				"View: %s\n", fTitle, fCurrentView->Name()));

			pattern pat;
			if (link.Read(&pat, sizeof(pattern)) != B_OK)
				break;

			fCurrentView->CurrentState()->SetPattern(Pattern(pat));
			fWindow->GetDrawingEngine()->SetPattern(pat);
			break;
		}

		case AS_VIEW_SET_BLENDING_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_BLEND_MODE: "
				"View: %s\n", Title(), fCurrentView->Name()));

			ViewBlendingModeInfo info;
			if (link.Read<ViewBlendingModeInfo>(&info) != B_OK)
				break;

			fCurrentView->CurrentState()->SetBlendingMode(
				info.sourceAlpha, info.alphaFunction);
			fWindow->GetDrawingEngine()->SetBlendingMode(
				info.sourceAlpha, info.alphaFunction);
			break;
		}
		case AS_VIEW_GET_BLENDING_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_BLEND_MODE: "
				"View: %s\n", Title(), fCurrentView->Name()));

			ViewBlendingModeInfo info;
			info.sourceAlpha = fCurrentView->CurrentState()->AlphaSrcMode();
			info.alphaFunction = fCurrentView->CurrentState()->AlphaFncMode();

			fLink.StartMessage(B_OK);
			fLink.Attach<ViewBlendingModeInfo>(info);
			fLink.Flush();

			break;
		}
		case AS_VIEW_SET_DRAWING_MODE:
		{
			int8 drawingMode;
			if (link.Read<int8>(&drawingMode) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_DRAW_MODE: "
				"View: %s -> %s\n", Title(), fCurrentView->Name(),
				kDrawingModeMap[drawingMode]));

			fCurrentView->CurrentState()->SetDrawingMode(
				(drawing_mode)drawingMode);
			fWindow->GetDrawingEngine()->SetDrawingMode(
				(drawing_mode)drawingMode);
			break;
		}
		case AS_VIEW_GET_DRAWING_MODE:
		{
			int8 drawingMode
				= (int8)(fCurrentView->CurrentState()->GetDrawingMode());

			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_DRAW_MODE: "
				"View: %s -> %s\n", Title(), fCurrentView->Name(),
				kDrawingModeMap[drawingMode]));

			fLink.StartMessage(B_OK);
			fLink.Attach<int8>(drawingMode);
			fLink.Flush();

			break;
		}
		case AS_VIEW_SET_VIEW_BITMAP:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_SET_VIEW_BITMAP: "
				"View: %s\n", Title(), fCurrentView->Name()));

			int32 bitmapToken, resizingMode, options;
			BRect srcRect, dstRect;

			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&srcRect);
			link.Read<BRect>(&dstRect);
			link.Read<int32>(&resizingMode);
			status_t status = link.Read<int32>(&options);

			rgb_color colorKey = {0};

			if (status == B_OK) {
				ServerBitmap* bitmap = fServerApp->GetBitmap(bitmapToken);
				if (bitmapToken == -1 || bitmap != NULL) {
					bool wasOverlay = fCurrentView->ViewBitmap() != NULL
						&& fCurrentView->ViewBitmap()->Overlay() != NULL;

					fCurrentView->SetViewBitmap(bitmap, srcRect, dstRect,
						resizingMode, options);

					// TODO: if we revert the view color overlay handling
					//	in View::Draw() to the BeOS version, we never
					//	need to invalidate the view for overlays.

					// Invalidate view - but only if this is a non-overlay
					// switch
					if (bitmap == NULL || bitmap->Overlay() == NULL
						|| !wasOverlay) {
						BRegion dirty((BRect)fCurrentView->Bounds());
						fWindow->InvalidateView(fCurrentView, dirty);
					}

					if (bitmap != NULL && bitmap->Overlay() != NULL) {
						bitmap->Overlay()->SetFlags(options);
						colorKey = bitmap->Overlay()->Color();
					}

					if (bitmap != NULL)
						bitmap->ReleaseReference();
				} else
					status = B_BAD_VALUE;
			}

			fLink.StartMessage(status);
			if (status == B_OK && (options & AS_REQUEST_COLOR_KEY) != 0) {
				// Attach color key for the overlay bitmap
				fLink.Attach<rgb_color>(colorKey);
			}

			fLink.Flush();
			break;
		}
		case AS_VIEW_PRINT_ALIASING:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_PRINT_ALIASING: "
				"View: %s\n", Title(), fCurrentView->Name()));

			bool fontAliasing;
			if (link.Read<bool>(&fontAliasing) == B_OK) {
				fCurrentView->CurrentState()->SetForceFontAliasing(fontAliasing);
				_UpdateDrawState(fCurrentView);
			}
			break;
		}
		case AS_VIEW_CLIP_TO_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_CLIP_TO_PICTURE: "
				"View: %s\n", Title(), fCurrentView->Name()));

			// TODO: you are not allowed to use View regions here!!!

			int32 pictureToken;
			BPoint where;
			bool inverse = false;

			link.Read<int32>(&pictureToken);
			link.Read<BPoint>(&where);
			if (link.Read<bool>(&inverse) != B_OK)
				break;

			ServerPicture* picture = fServerApp->GetPicture(pictureToken);
			if (picture == NULL)
				break;

			BRegion region;
			// TODO: I think we also need the BView's token
			// I think PictureToRegion would fit better into the View class (?)
			if (PictureToRegion(picture, region, inverse, where) == B_OK)
				fCurrentView->SetUserClipping(&region);

			picture->ReleaseReference();
			break;
		}

		case AS_VIEW_GET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_GET_CLIP_REGION: "
				"View: %s\n", Title(), fCurrentView->Name()));

			// if this view is hidden, it has no visible region
			fLink.StartMessage(B_OK);
			if (!fWindow->IsVisible() || !fCurrentView->IsVisible()) {
				BRegion empty;
				fLink.AttachRegion(empty);
			} else {
				_UpdateCurrentDrawingRegion();
				BRegion region(fCurrentDrawingRegion);
				fCurrentView->ConvertFromScreen(&region);
				fLink.AttachRegion(region);
			}
			fLink.Flush();

			break;
		}
		case AS_VIEW_SET_CLIP_REGION:
		{
			int32 rectCount;
			status_t status = link.Read<int32>(&rectCount);
				// a negative count means no
				// region for the current draw state,
				// but an *empty* region is actually valid!
				// even if it means no drawing is allowed

			if (status < B_OK)
				break;

			if (rectCount >= 0) {
				// we are supposed to set the clipping region
				BRegion region;
				if (rectCount > 0 && link.ReadRegion(&region) < B_OK)
					break;

				DTRACE(("ServerWindow %s: Message AS_VIEW_SET_CLIP_REGION: "
					"View: %s -> rect count: %ld, frame = "
					"BRect(%.1f, %.1f, %.1f, %.1f)\n",
					Title(), fCurrentView->Name(), rectCount,
					region.Frame().left, region.Frame().top,
					region.Frame().right, region.Frame().bottom));

				fCurrentView->SetUserClipping(&region);
			} else {
				// we are supposed to unset the clipping region
				// passing NULL sets this states region to that
				// of the previous state

				DTRACE(("ServerWindow %s: Message AS_VIEW_SET_CLIP_REGION: "
					"View: %s -> unset\n", Title(), fCurrentView->Name()));

				fCurrentView->SetUserClipping(NULL);
			}
			fCurrentDrawingRegionValid = false;

			break;
		}

		case AS_VIEW_INVALIDATE_RECT:
		{
			// NOTE: looks like this call is NOT affected by origin and scale
			// on R5 so this implementation is "correct"
			BRect invalidRect;
			if (link.Read<BRect>(&invalidRect) == B_OK) {
				DTRACE(("ServerWindow %s: Message AS_VIEW_INVALIDATE_RECT: "
					"View: %s -> BRect(%.1f, %.1f, %.1f, %.1f)\n", Title(),
					fCurrentView->Name(), invalidRect.left, invalidRect.top,
					invalidRect.right, invalidRect.bottom));

				BRegion dirty(invalidRect);
				fWindow->InvalidateView(fCurrentView, dirty);
			}
			break;
		}
		case AS_VIEW_INVALIDATE_REGION:
		{
			// NOTE: looks like this call is NOT affected by origin and scale
			// on R5 so this implementation is "correct"
			BRegion region;
			if (link.ReadRegion(&region) < B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_VIEW_INVALIDATE_REGION: "
					"View: %s -> rect count: %ld, frame: BRect(%.1f, %.1f, "
					"%.1f, %.1f)\n", Title(),
					fCurrentView->Name(), region.CountRects(),
					region.Frame().left, region.Frame().top,
					region.Frame().right, region.Frame().bottom));

			fWindow->InvalidateView(fCurrentView, region);
			break;
		}

		case AS_VIEW_DRAG_IMAGE:
		{
			// TODO: flesh out AS_VIEW_DRAG_IMAGE
			DTRACE(("ServerWindow %s: Message AS_DRAG_IMAGE\n", Title()));

			int32 bitmapToken;
			drawing_mode dragMode;
			BPoint offset;
			int32 bufferSize;

			link.Read<int32>(&bitmapToken);
			link.Read<int32>((int32*)&dragMode);
			link.Read<BPoint>(&offset);
			link.Read<int32>(&bufferSize);

			if (bufferSize > 0) {
				char* buffer = new (nothrow) char[bufferSize];
				BMessage dragMessage;
				if (link.Read(buffer, bufferSize) == B_OK
					&& dragMessage.Unflatten(buffer) == B_OK) {
						ServerBitmap* bitmap
							= fServerApp->GetBitmap(bitmapToken);
						// TODO: possible deadlock
fDesktop->UnlockSingleWindow();
						fDesktop->EventDispatcher().SetDragMessage(dragMessage,
							bitmap, offset);
fDesktop->LockSingleWindow();
						bitmap->ReleaseReference();
				}
				delete[] buffer;
			}
			// sync the client (it can now delete the bitmap)
			fLink.StartMessage(B_OK);
			fLink.Flush();

			break;
		}
		case AS_VIEW_DRAG_RECT:
		{
			// TODO: flesh out AS_VIEW_DRAG_RECT
			DTRACE(("ServerWindow %s: Message AS_DRAG_RECT\n", Title()));

			BRect dragRect;
			BPoint offset;
			int32 bufferSize;

			link.Read<BRect>(&dragRect);
			link.Read<BPoint>(&offset);
			link.Read<int32>(&bufferSize);

			if (bufferSize > 0) {
				char* buffer = new (nothrow) char[bufferSize];
				BMessage dragMessage;
				if (link.Read(buffer, bufferSize) == B_OK
					&& dragMessage.Unflatten(buffer) == B_OK) {
						// TODO: possible deadlock
fDesktop->UnlockSingleWindow();
						fDesktop->EventDispatcher().SetDragMessage(dragMessage,
							NULL /* should be dragRect */, offset);
fDesktop->LockSingleWindow();
				}
				delete[] buffer;
			}
			break;
		}

		case AS_VIEW_BEGIN_RECT_TRACK:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_BEGIN_RECT_TRACK\n",
				Title()));
			BRect dragRect;
			uint32 style;

			link.Read<BRect>(&dragRect);
			link.Read<uint32>(&style);

			// TODO: implement rect tracking (used sometimes for selecting
			// a group of things, also sometimes used to appear to drag
			// something, but without real drag message)
			break;
		}
		case AS_VIEW_END_RECT_TRACK:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_END_RECT_TRACK\n",
				Title()));
			// TODO: implement rect tracking
			break;
		}

		case AS_VIEW_BEGIN_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_BEGIN_PICTURE\n",
				Title()));
			ServerPicture* picture = App()->CreatePicture();
			if (picture != NULL) {
				picture->SyncState(fCurrentView);
				fCurrentView->SetPicture(picture);
			}
			break;
		}

		case AS_VIEW_APPEND_TO_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_APPEND_TO_PICTURE\n",
				Title()));

			int32 token;
			link.Read<int32>(&token);

			ServerPicture* picture = App()->GetPicture(token);
			if (picture != NULL)
				picture->SyncState(fCurrentView);

			fCurrentView->SetPicture(picture);

			if (picture != NULL)
				picture->ReleaseReference();
			break;
		}

		case AS_VIEW_END_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_VIEW_END_PICTURE\n",
				Title()));

			ServerPicture* picture = fCurrentView->Picture();
			if (picture != NULL) {
				fCurrentView->SetPicture(NULL);
				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(picture->Token());
			} else
				fLink.StartMessage(B_ERROR);

			fLink.Flush();
			break;
		}

		default:
			_DispatchViewDrawingMessage(code, link);
			break;
	}
}


/*!	Dispatches all view drawing messages.
	The desktop clipping must be read locked when entering this method.
	Requires a valid fCurrentView.
*/
void
ServerWindow::_DispatchViewDrawingMessage(int32 code,
	BPrivate::LinkReceiver &link)
{
	if (!fCurrentView->IsVisible() || !fWindow->IsVisible()) {
		if (link.NeedsReply()) {
			debug_printf("ServerWindow::DispatchViewDrawingMessage() got "
				"message %ld that needs a reply!\n", code);
			// the client is now blocking and waiting for a reply!
			fLink.StartMessage(B_ERROR);
			fLink.Flush();
		}
		return;
	}

	DrawingEngine* drawingEngine = fWindow->GetDrawingEngine();
	if (!drawingEngine) {
		// ?!?
		debug_printf("ServerWindow %s: no drawing engine!!\n", Title());
		if (link.NeedsReply()) {
			// the client is now blocking and waiting for a reply!
			fLink.StartMessage(B_ERROR);
			fLink.Flush();
		}
		return;
	}

	_UpdateCurrentDrawingRegion();
	if (fCurrentDrawingRegion.CountRects() <= 0) {
		DTRACE(("ServerWindow %s: _DispatchViewDrawingMessage(): View: %s, "
			"INVALID CLIPPING!\n", Title(), fCurrentView->Name()));
		if (link.NeedsReply()) {
			// the client is now blocking and waiting for a reply!
			fLink.StartMessage(B_ERROR);
			fLink.Flush();
		}
		return;
	}

	drawingEngine->LockParallelAccess();
	// NOTE: the region is not copied, Painter keeps a pointer,
	// that's why you need to use the clipping only for as long
	// as you have it locked
	drawingEngine->ConstrainClippingRegion(&fCurrentDrawingRegion);

	switch (code) {
		case AS_STROKE_LINE:
		{
			ViewStrokeLineInfo info;
			if (link.Read<ViewStrokeLineInfo>(&info) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_STROKE_LINE: View: %s -> "
				"BPoint(%.1f, %.1f) - BPoint(%.1f, %.1f)\n", Title(),
					fCurrentView->Name(),
					info.startPoint.x, info.startPoint.y,
					info.endPoint.x, info.endPoint.y));

			BPoint penPos = info.endPoint;
			fCurrentView->ConvertToScreenForDrawing(&info.startPoint);
			fCurrentView->ConvertToScreenForDrawing(&info.endPoint);
			drawingEngine->StrokeLine(info.startPoint, info.endPoint);

			// We update the pen here because many DrawingEngine calls which
			// do not update the pen position actually call StrokeLine

			// TODO: Decide where to put this, for example, it cannot be done
			// for DrawString(), also there needs to be a decision, if the pen
			// location is in View coordinates (I think it should be) or in
			// screen coordinates.
			fCurrentView->CurrentState()->SetPenLocation(penPos);
			break;
		}
		case AS_VIEW_INVERT_RECT:
		{
			BRect rect;
			if (link.Read<BRect>(&rect) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_INVERT_RECT: View: %s -> "
				"BRect(%.1f, %.1f, %.1f, %.1f)\n", Title(),
				fCurrentView->Name(), rect.left, rect.top, rect.right,
				rect.bottom));

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->InvertRect(rect);
			break;
		}
		case AS_STROKE_RECT:
		{
			BRect rect;
			if (link.Read<BRect>(&rect) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_STROKE_RECT: View: %s -> "
				"BRect(%.1f, %.1f, %.1f, %.1f)\n", Title(),
				fCurrentView->Name(), rect.left, rect.top, rect.right,
				rect.bottom));

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->StrokeRect(rect);
			break;
		}
		case AS_FILL_RECT:
		{
			BRect rect;
			if (link.Read<BRect>(&rect) != B_OK)
				break;

			DTRACE(("ServerWindow %s: Message AS_FILL_RECT: View: %s -> "
				"BRect(%.1f, %.1f, %.1f, %.1f)\n", Title(),
				fCurrentView->Name(), rect.left, rect.top, rect.right,
				rect.bottom));

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->FillRect(rect);
			break;
		}
		case AS_FILL_RECT_GRADIENT:
		{
			BRect rect;
			link.Read<BRect>(&rect);
			BGradient* gradient;
			if (link.ReadGradient(&gradient) != B_OK)
				break;

			GTRACE(("ServerWindow %s: Message AS_FILL_RECT_GRADIENT: View: %s "
				"-> BRect(%.1f, %.1f, %.1f, %.1f)\n", Title(),
				fCurrentView->Name(), rect.left, rect.top, rect.right,
				rect.bottom));

			fCurrentView->ConvertToScreenForDrawing(&rect);
			fCurrentView->ConvertToScreenForDrawing(gradient);
			drawingEngine->FillRect(rect, *gradient);
			delete gradient;
			break;
		}
		case AS_VIEW_DRAW_BITMAP:
		{
			ViewDrawBitmapInfo info;
			if (link.Read<ViewDrawBitmapInfo>(&info) != B_OK)
				break;

#if 0
			if (strcmp(fServerApp->SignatureLeaf(), "x-vnd.videolan-vlc") == 0)
				info.options |= B_FILTER_BITMAP_BILINEAR;
#endif

			ServerBitmap* bitmap = fServerApp->GetBitmap(info.bitmapToken);
			if (bitmap != NULL) {
				DTRACE(("ServerWindow %s: Message AS_VIEW_DRAW_BITMAP: "
					"View: %s, bitmap: %ld (size %ld x %ld), "
					"BRect(%.1f, %.1f, %.1f, %.1f) -> "
					"BRect(%.1f, %.1f, %.1f, %.1f)\n",
					fTitle, fCurrentView->Name(), info.bitmapToken,
					bitmap->Width(), bitmap->Height(),
					info.bitmapRect.left, info.bitmapRect.top,
					info.bitmapRect.right, info.bitmapRect.bottom,
					info.viewRect.left, info.viewRect.top,
					info.viewRect.right, info.viewRect.bottom));

				fCurrentView->ConvertToScreenForDrawing(&info.viewRect);

// TODO: Unbreak...
//				if ((info.options & B_WAIT_FOR_RETRACE) != 0)
//					fDesktop->HWInterface()->WaitForRetrace(20000);

				drawingEngine->DrawBitmap(bitmap, info.bitmapRect,
					info.viewRect, info.options);

				bitmap->ReleaseReference();
			}
			break;
		}
		case AS_STROKE_ARC:
		case AS_FILL_ARC:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_ARC\n", Title()));

			float angle, span;
			BRect r;

			link.Read<BRect>(&r);
			link.Read<float>(&angle);
			if (link.Read<float>(&span) != B_OK)
				break;

			fCurrentView->ConvertToScreenForDrawing(&r);
			drawingEngine->DrawArc(r, angle, span, code == AS_FILL_ARC);
			break;
		}
		case AS_FILL_ARC_GRADIENT:
		{
			GTRACE(("ServerWindow %s: Message AS_FILL_ARC_GRADIENT\n",
				Title()));

			float angle, span;
			BRect r;
			link.Read<BRect>(&r);
			link.Read<float>(&angle);
			link.Read<float>(&span);
			BGradient* gradient;
			if (link.ReadGradient(&gradient) != B_OK)
				break;
			fCurrentView->ConvertToScreenForDrawing(&r);
			fCurrentView->ConvertToScreenForDrawing(gradient);
			drawingEngine->FillArc(r, angle, span, *gradient);
			delete gradient;
			break;
		}
		case AS_STROKE_BEZIER:
		case AS_FILL_BEZIER:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_BEZIER\n",
				Title()));

			BPoint pts[4];
			status_t status;
			for (int32 i = 0; i < 4; i++) {
				status = link.Read<BPoint>(&(pts[i]));
				fCurrentView->ConvertToScreenForDrawing(&pts[i]);
			}
			if (status != B_OK)
				break;

			drawingEngine->DrawBezier(pts, code == AS_FILL_BEZIER);
			break;
		}
		case AS_FILL_BEZIER_GRADIENT:
		{
			GTRACE(("ServerWindow %s: Message AS_FILL_BEZIER_GRADIENT\n",
				Title()));

			BPoint pts[4];
			for (int32 i = 0; i < 4; i++) {
				link.Read<BPoint>(&(pts[i]));
				fCurrentView->ConvertToScreenForDrawing(&pts[i]);
			}
			BGradient* gradient;
			if (link.ReadGradient(&gradient) != B_OK)
				break;
			fCurrentView->ConvertToScreenForDrawing(gradient);
			drawingEngine->FillBezier(pts, *gradient);
			delete gradient;
			break;
		}
		case AS_STROKE_ELLIPSE:
		case AS_FILL_ELLIPSE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_ELLIPSE\n",
				Title()));

			BRect rect;
			if (link.Read<BRect>(&rect) != B_OK)
				break;

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->DrawEllipse(rect, code == AS_FILL_ELLIPSE);
			break;
		}
		case AS_FILL_ELLIPSE_GRADIENT:
		{
			GTRACE(("ServerWindow %s: Message AS_FILL_ELLIPSE_GRADIENT\n",
				Title()));

			BRect rect;
			link.Read<BRect>(&rect);
			BGradient* gradient;
			if (link.ReadGradient(&gradient) != B_OK)
				break;
			fCurrentView->ConvertToScreenForDrawing(&rect);
			fCurrentView->ConvertToScreenForDrawing(gradient);
			drawingEngine->FillEllipse(rect, *gradient);
			delete gradient;
			break;
		}
		case AS_STROKE_ROUNDRECT:
		case AS_FILL_ROUNDRECT:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_ROUNDRECT\n",
				Title()));

			BRect rect;
			float xrad,yrad;
			link.Read<BRect>(&rect);
			link.Read<float>(&xrad);
			if (link.Read<float>(&yrad) != B_OK)
				break;

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->DrawRoundRect(rect, xrad, yrad,
				code == AS_FILL_ROUNDRECT);
			break;
		}
		case AS_FILL_ROUNDRECT_GRADIENT:
		{
			GTRACE(("ServerWindow %s: Message AS_FILL_ROUNDRECT_GRADIENT\n",
				Title()));

			BRect rect;
			float xrad,yrad;
			link.Read<BRect>(&rect);
			link.Read<float>(&xrad);
			link.Read<float>(&yrad);
			BGradient* gradient;
			if (link.ReadGradient(&gradient) != B_OK)
				break;
			fCurrentView->ConvertToScreenForDrawing(&rect);
			fCurrentView->ConvertToScreenForDrawing(gradient);
			drawingEngine->FillRoundRect(rect, xrad, yrad, *gradient);
			delete gradient;
			break;
		}
		case AS_STROKE_TRIANGLE:
		case AS_FILL_TRIANGLE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_TRIANGLE\n",
				Title()));

			BPoint pts[3];
			BRect rect;

			for (int32 i = 0; i < 3; i++) {
				link.Read<BPoint>(&(pts[i]));
				fCurrentView->ConvertToScreenForDrawing(&pts[i]);
			}

			if (link.Read<BRect>(&rect) != B_OK)
				break;

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->DrawTriangle(pts, rect, code == AS_FILL_TRIANGLE);
			break;
		}
		case AS_FILL_TRIANGLE_GRADIENT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_TRIANGLE_GRADIENT\n",
				Title()));

			BPoint pts[3];
			BRect rect;
			for (int32 i = 0; i < 3; i++) {
				link.Read<BPoint>(&(pts[i]));
				fCurrentView->ConvertToScreenForDrawing(&pts[i]);
			}
			link.Read<BRect>(&rect);
			BGradient* gradient;
			if (link.ReadGradient(&gradient) != B_OK)
				break;
			fCurrentView->ConvertToScreenForDrawing(&rect);
			fCurrentView->ConvertToScreenForDrawing(gradient);
			drawingEngine->FillTriangle(pts, rect, *gradient);
			delete gradient;
			break;
		}
		case AS_STROKE_POLYGON:
		case AS_FILL_POLYGON:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_POLYGON\n",
				Title()));

			BRect polyFrame;
			bool isClosed = true;
			int32 pointCount;

			link.Read<BRect>(&polyFrame);
			if (code == AS_STROKE_POLYGON)
				link.Read<bool>(&isClosed);
			link.Read<int32>(&pointCount);

			BPoint* pointList = new(nothrow) BPoint[pointCount];
			if (link.Read(pointList, pointCount * sizeof(BPoint)) >= B_OK) {
				for (int32 i = 0; i < pointCount; i++)
					fCurrentView->ConvertToScreenForDrawing(&pointList[i]);
				fCurrentView->ConvertToScreenForDrawing(&polyFrame);

				drawingEngine->DrawPolygon(pointList, pointCount, polyFrame,
					code == AS_FILL_POLYGON, isClosed && pointCount > 2);
			}
			delete[] pointList;
			break;
		}
		case AS_FILL_POLYGON_GRADIENT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_POLYGON_GRADIENT\n",
				Title()));

			BRect polyFrame;
			bool isClosed = true;
			int32 pointCount;
			link.Read<BRect>(&polyFrame);
			link.Read<int32>(&pointCount);

			BPoint* pointList = new(nothrow) BPoint[pointCount];
			BGradient* gradient;
			if (link.Read(pointList, pointCount * sizeof(BPoint)) == B_OK
				&& link.ReadGradient(&gradient) == B_OK) {
				for (int32 i = 0; i < pointCount; i++)
					fCurrentView->ConvertToScreenForDrawing(&pointList[i]);
				fCurrentView->ConvertToScreenForDrawing(&polyFrame);
				fCurrentView->ConvertToScreenForDrawing(gradient);

				drawingEngine->FillPolygon(pointList, pointCount,
					polyFrame, *gradient, isClosed && pointCount > 2);
				delete gradient;
			}
			delete[] pointList;
			break;
		}
		case AS_STROKE_SHAPE:
		case AS_FILL_SHAPE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_SHAPE\n",
				Title()));

			BRect shapeFrame;
			int32 opCount;
			int32 ptCount;

			link.Read<BRect>(&shapeFrame);
			link.Read<int32>(&opCount);
			link.Read<int32>(&ptCount);

			uint32* opList = new(nothrow) uint32[opCount];
			BPoint* ptList = new(nothrow) BPoint[ptCount];
			if (link.Read(opList, opCount * sizeof(uint32)) >= B_OK &&
				link.Read(ptList, ptCount * sizeof(BPoint)) >= B_OK) {

				// this might seem a bit weird, but under R5, the shapes
				// are always offset by the current pen location
				BPoint screenOffset
					= fCurrentView->CurrentState()->PenLocation();
				shapeFrame.OffsetBy(screenOffset);

				fCurrentView->ConvertToScreenForDrawing(&screenOffset);
				fCurrentView->ConvertToScreenForDrawing(&shapeFrame);

				drawingEngine->DrawShape(shapeFrame, opCount, opList, ptCount,
					ptList, code == AS_FILL_SHAPE, screenOffset,
					fCurrentView->Scale());
			}

			delete[] opList;
			delete[] ptList;
			break;
		}
		case AS_FILL_SHAPE_GRADIENT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_SHAPE_GRADIENT\n",
				Title()));

			BRect shapeFrame;
			int32 opCount;
			int32 ptCount;

			link.Read<BRect>(&shapeFrame);
			link.Read<int32>(&opCount);
			link.Read<int32>(&ptCount);

			uint32* opList = new(nothrow) uint32[opCount];
			BPoint* ptList = new(nothrow) BPoint[ptCount];
			BGradient* gradient;
			if (link.Read(opList, opCount * sizeof(uint32)) == B_OK
				&& link.Read(ptList, ptCount * sizeof(BPoint)) == B_OK
				&& link.ReadGradient(&gradient) == B_OK) {

				// this might seem a bit weird, but under R5, the shapes
				// are always offset by the current pen location
				BPoint screenOffset
					= fCurrentView->CurrentState()->PenLocation();
				shapeFrame.OffsetBy(screenOffset);

				fCurrentView->ConvertToScreenForDrawing(&screenOffset);
				fCurrentView->ConvertToScreenForDrawing(&shapeFrame);
				fCurrentView->ConvertToScreenForDrawing(gradient);
				drawingEngine->FillShape(shapeFrame, opCount, opList,
					ptCount, ptList, *gradient, screenOffset,
					fCurrentView->Scale());
				delete gradient;
			}

			delete[] opList;
			delete[] ptList;
			break;
		}
		case AS_FILL_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_REGION\n", Title()));

			BRegion region;
			if (link.ReadRegion(&region) < B_OK)
				break;

			fCurrentView->ConvertToScreenForDrawing(&region);
			drawingEngine->FillRegion(region);

			break;
		}
		case AS_FILL_REGION_GRADIENT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_REGION_GRADIENT\n",
				Title()));

			BRegion region;
			link.ReadRegion(&region);

			BGradient* gradient;
			if (link.ReadGradient(&gradient) != B_OK)
				break;

			fCurrentView->ConvertToScreenForDrawing(&region);
			fCurrentView->ConvertToScreenForDrawing(gradient);
			drawingEngine->FillRegion(region, *gradient);
			delete gradient;
			break;
		}
		case AS_STROKE_LINEARRAY:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_LINEARRAY\n",
				Title()));

			// Attached Data:
			// 1) int32 Number of lines in the array
			// 2) LineArrayData

			int32 lineCount;
			if (link.Read<int32>(&lineCount) != B_OK || lineCount <= 0)
				break;

			// To speed things up, try to use a stack allocation and only
			// fall back to the heap if there are enough lines...
			ViewLineArrayInfo* lineData;
			const int32 kStackBufferLineDataCount = 64;
			ViewLineArrayInfo lineDataStackBuffer[kStackBufferLineDataCount];
			if (lineCount > kStackBufferLineDataCount) {
				lineData = new(std::nothrow) ViewLineArrayInfo[lineCount];
				if (lineData == NULL)
					break;
			} else
				lineData = lineDataStackBuffer;

			// Read them all in one go
			size_t dataSize = lineCount * sizeof(ViewLineArrayInfo);
			if (link.Read(lineData, dataSize) != B_OK) {
				if (lineData != lineDataStackBuffer)
					delete[] lineData;
				break;
			}

			// Convert to screen coords and draw
			for (int32 i = 0; i < lineCount; i++) {
				fCurrentView->ConvertToScreenForDrawing(
					&lineData[i].startPoint);
				fCurrentView->ConvertToScreenForDrawing(
					&lineData[i].endPoint);
			}
			drawingEngine->StrokeLineArray(lineCount, lineData);

			if (lineData != lineDataStackBuffer)
				delete[] lineData;
			break;
		}
		case AS_DRAW_STRING:
		case AS_DRAW_STRING_WITH_DELTA:
		{
			ViewDrawStringInfo info;
			if (link.Read<ViewDrawStringInfo>(&info) != B_OK
				|| info.stringLength <= 0) {
				break;
			}

			const ssize_t kMaxStackStringSize = 4096;
			char stackString[kMaxStackStringSize];
			char* string = stackString;
			if (info.stringLength >= kMaxStackStringSize) {
				// NOTE: Careful, the + 1 is for termination!
				string = (char*)malloc((info.stringLength + 1 + 63) / 64 * 64);
				if (string == NULL)
					break;
			}

			escapement_delta* delta = NULL;
			if (code == AS_DRAW_STRING_WITH_DELTA) {
				// In this case, info.delta will contain valid values.
				delta = &info.delta;
			}

			if (link.Read(string, info.stringLength) != B_OK) {
				if (string != stackString)
					free(string);
				break;
			}
			// Terminate the string, if nothing else, it's important
			// for the DTRACE call below...
			string[info.stringLength] = '\0';

			DTRACE(("ServerWindow %s: Message AS_DRAW_STRING, View: %s "
				"-> %s\n", Title(), fCurrentView->Name(), string));

			fCurrentView->ConvertToScreenForDrawing(&info.location);
			BPoint penLocation = drawingEngine->DrawString(string,
				info.stringLength, info.location, delta);

			fCurrentView->ConvertFromScreenForDrawing(&penLocation);
			fCurrentView->CurrentState()->SetPenLocation(penLocation);

			if (string != stackString)
				free(string);
			break;
		}
		case AS_DRAW_STRING_WITH_OFFSETS:
		{
			int32 stringLength;
			if (link.Read<int32>(&stringLength) != B_OK || stringLength <= 0)
				break;

			int32 glyphCount;
			if (link.Read<int32>(&glyphCount) != B_OK || glyphCount <= 0)
				break;

			const ssize_t kMaxStackStringSize = 512;
			char stackString[kMaxStackStringSize];
			char* string = stackString;
			BPoint stackLocations[kMaxStackStringSize];
			BPoint* locations = stackLocations;
			MemoryDeleter stringDeleter;
			MemoryDeleter locationsDeleter;
			if (stringLength >= kMaxStackStringSize) {
				// NOTE: Careful, the + 1 is for termination!
				string = (char*)malloc((stringLength + 1 + 63) / 64 * 64);
				if (string == NULL)
					break;
				stringDeleter.SetTo(string);
			}
			if (glyphCount > kMaxStackStringSize) {
				locations = (BPoint*)malloc(
					((glyphCount * sizeof(BPoint)) + 63) / 64 * 64);
				if (locations == NULL)
					break;
				locationsDeleter.SetTo(locations);
			}

			if (link.Read(string, stringLength) != B_OK)
				break;
			// Count UTF8 glyphs and make sure we have enough locations
			if ((int32)UTF8CountChars(string, stringLength) > glyphCount)
				break;
			if (link.Read(locations, glyphCount * sizeof(BPoint)) != B_OK)
				break;
			// Terminate the string, if nothing else, it's important
			// for the DTRACE call below...
			string[stringLength] = '\0';

			DTRACE(("ServerWindow %s: Message AS_DRAW_STRING_WITH_OFFSETS, View: %s "
				"-> %s\n", Title(), fCurrentView->Name(), string));

			for (int32 i = 0; i < glyphCount; i++)
				fCurrentView->ConvertToScreenForDrawing(&locations[i]);

			BPoint penLocation = drawingEngine->DrawString(string,
				stringLength, locations);

			fCurrentView->ConvertFromScreenForDrawing(&penLocation);
			fCurrentView->CurrentState()->SetPenLocation(penLocation);

			break;
		}

		case AS_VIEW_DRAW_PICTURE:
		{
			int32 token;
			link.Read<int32>(&token);

			BPoint where;
			if (link.Read<BPoint>(&where) == B_OK) {
				ServerPicture* picture = App()->GetPicture(token);
				if (picture != NULL) {
					// Setting the drawing origin outside of the
					// state makes sure that everything the picture
					// does is relative to the global picture offset.
					fCurrentView->PushState();
					fCurrentView->SetDrawingOrigin(where);

					fCurrentView->PushState();
					picture->Play(fCurrentView);
					fCurrentView->PopState();

					fCurrentView->PopState();

					picture->ReleaseReference();
				}
			}
			break;
		}

		default:
			BString codeString;
			string_for_message_code(code, codeString);
			debug_printf("ServerWindow %s received unexpected code: %s\n",
				Title(), codeString.String());

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
	}

	drawingEngine->UnlockParallelAccess();
}


bool
ServerWindow::_DispatchPictureMessage(int32 code, BPrivate::LinkReceiver& link)
{
	ServerPicture* picture = fCurrentView->Picture();
	if (picture == NULL)
		return false;

	switch (code) {
		case AS_VIEW_SET_ORIGIN:
		{
			float x, y;
			link.Read<float>(&x);
			link.Read<float>(&y);

			picture->WriteSetOrigin(BPoint(x, y));
			break;
		}

		case AS_VIEW_INVERT_RECT:
		{
			BRect rect;
			link.Read<BRect>(&rect);
			picture->WriteInvertRect(rect);
			break;
		}

		case AS_VIEW_PUSH_STATE:
		{
			picture->WritePushState();
			break;
		}

		case AS_VIEW_POP_STATE:
		{
			picture->WritePopState();
			break;
		}

		case AS_VIEW_SET_DRAWING_MODE:
		{
			int8 drawingMode;
			link.Read<int8>(&drawingMode);

			picture->WriteSetDrawingMode((drawing_mode)drawingMode);

			fCurrentView->CurrentState()->SetDrawingMode(
				(drawing_mode)drawingMode);
			fWindow->GetDrawingEngine()->SetDrawingMode(
				(drawing_mode)drawingMode);
			break;
		}

		case AS_VIEW_SET_PEN_LOC:
		{
			BPoint location;
			link.Read<BPoint>(&location);
			picture->WriteSetPenLocation(location);

			fCurrentView->CurrentState()->SetPenLocation(location);
			break;
		}
		case AS_VIEW_SET_PEN_SIZE:
		{
			float penSize;
			link.Read<float>(&penSize);
			picture->WriteSetPenSize(penSize);

			fCurrentView->CurrentState()->SetPenSize(penSize);
			fWindow->GetDrawingEngine()->SetPenSize(
				fCurrentView->CurrentState()->PenSize());
			break;
		}

		case AS_VIEW_SET_LINE_MODE:
		{

			ViewSetLineModeInfo info;
			link.Read<ViewSetLineModeInfo>(&info);

			picture->WriteSetLineMode(info.lineCap, info.lineJoin,
				info.miterLimit);

			fCurrentView->CurrentState()->SetLineCapMode(info.lineCap);
			fCurrentView->CurrentState()->SetLineJoinMode(info.lineJoin);
			fCurrentView->CurrentState()->SetMiterLimit(info.miterLimit);

			fWindow->GetDrawingEngine()->SetStrokeMode(info.lineCap,
				info.lineJoin, info.miterLimit);
			break;
		}
		case AS_VIEW_SET_SCALE:
		{
			float scale;
			link.Read<float>(&scale);
			picture->WriteSetScale(scale);

			fCurrentView->SetScale(scale);
			_UpdateDrawState(fCurrentView);
			break;
		}

		case AS_VIEW_SET_PATTERN:
		{
			pattern pat;
			link.Read(&pat, sizeof(pattern));
			picture->WriteSetPattern(pat);
			break;
		}

		case AS_VIEW_SET_FONT_STATE:
		{
			picture->SetFontFromLink(link);
			break;
		}

		case AS_FILL_RECT:
		case AS_STROKE_RECT:
		{
			BRect rect;
			link.Read<BRect>(&rect);

			picture->WriteDrawRect(rect, code == AS_FILL_RECT);
			break;
		}

		case AS_FILL_REGION:
		{
			// There is no B_PIC_FILL_REGION op, we have to
			// implement it using B_PIC_FILL_RECT
			BRegion region;
			if (link.ReadRegion(&region) < B_OK)
				break;
			for (int32 i = 0; i < region.CountRects(); i++)
				picture->WriteDrawRect(region.RectAt(i), true);
			break;
		}

		case AS_STROKE_ROUNDRECT:
		case AS_FILL_ROUNDRECT:
		{
			BRect rect;
			link.Read<BRect>(&rect);

			BPoint radii;
			link.Read<float>(&radii.x);
			link.Read<float>(&radii.y);

			picture->WriteDrawRoundRect(rect, radii, code == AS_FILL_ROUNDRECT);
			break;
		}

		case AS_STROKE_ELLIPSE:
		case AS_FILL_ELLIPSE:
		{
			BRect rect;
			link.Read<BRect>(&rect);
			picture->WriteDrawEllipse(rect, code == AS_FILL_ELLIPSE);
			break;
		}

		case AS_STROKE_ARC:
		case AS_FILL_ARC:
		{
			BRect rect;
			link.Read<BRect>(&rect);
			float startTheta, arcTheta;
			link.Read<float>(&startTheta);
			link.Read<float>(&arcTheta);

			BPoint radii((rect.Width() + 1) / 2, (rect.Height() + 1) / 2);
			BPoint center = rect.LeftTop() + radii;

			picture->WriteDrawArc(center, radii, startTheta, arcTheta,
				code == AS_FILL_ARC);
			break;
		}

		case AS_STROKE_TRIANGLE:
		case AS_FILL_TRIANGLE:
		{
			// There is no B_PIC_FILL/STROKE_TRIANGLE op,
			// we implement it using B_PIC_FILL/STROKE_POLYGON
			BPoint points[3];

			for (int32 i = 0; i < 3; i++) {
				link.Read<BPoint>(&(points[i]));
			}

			BRect rect;
			link.Read<BRect>(&rect);

			picture->WriteDrawPolygon(3, points,
					true, code == AS_FILL_TRIANGLE);
			break;
		}
		case AS_STROKE_POLYGON:
		case AS_FILL_POLYGON:
		{
			BRect polyFrame;
			bool isClosed = true;
			int32 pointCount;
			const bool fill = (code == AS_FILL_POLYGON);

			link.Read<BRect>(&polyFrame);
			if (code == AS_STROKE_POLYGON)
				link.Read<bool>(&isClosed);
			link.Read<int32>(&pointCount);

			BPoint* pointList = new(nothrow) BPoint[pointCount];
			if (link.Read(pointList, pointCount * sizeof(BPoint)) >= B_OK) {
				picture->WriteDrawPolygon(pointCount, pointList,
					isClosed && pointCount > 2, fill);
			}
			delete[] pointList;
			break;
		}

		case AS_STROKE_BEZIER:
		case AS_FILL_BEZIER:
		{
			BPoint points[4];
			for (int32 i = 0; i < 4; i++) {
				link.Read<BPoint>(&(points[i]));
			}
			picture->WriteDrawBezier(points, code == AS_FILL_BEZIER);
			break;
		}

		case AS_STROKE_LINE:
		{
			ViewStrokeLineInfo info;
			link.Read<ViewStrokeLineInfo>(&info);

			picture->WriteStrokeLine(info.startPoint, info.endPoint);
			break;
		}

		case AS_STROKE_LINEARRAY:
		{
			int32 lineCount;
			if (link.Read<int32>(&lineCount) != B_OK || lineCount <= 0)
				break;

			// To speed things up, try to use a stack allocation and only
			// fall back to the heap if there are enough lines...
			ViewLineArrayInfo* lineData;
			const int32 kStackBufferLineDataCount = 64;
			ViewLineArrayInfo lineDataStackBuffer[kStackBufferLineDataCount];
			if (lineCount > kStackBufferLineDataCount) {
				lineData = new(std::nothrow) ViewLineArrayInfo[lineCount];
				if (lineData == NULL)
					break;
			} else
				lineData = lineDataStackBuffer;

			// Read them all in one go
			size_t dataSize = lineCount * sizeof(ViewLineArrayInfo);
			if (link.Read(lineData, dataSize) != B_OK) {
				if (lineData != lineDataStackBuffer)
					delete[] lineData;
				break;
			}

			picture->WritePushState();

			for (int32 i = 0; i < lineCount; i++) {
				picture->WriteSetHighColor(lineData[i].color);
				picture->WriteStrokeLine(lineData[i].startPoint,
					lineData[i].endPoint);
			}

			picture->WritePopState();

			if (lineData != lineDataStackBuffer)
				delete[] lineData;
			break;
		}

		case AS_VIEW_SET_LOW_COLOR:
		case AS_VIEW_SET_HIGH_COLOR:
		{
			rgb_color color;
			link.Read(&color, sizeof(rgb_color));

			if (code == AS_VIEW_SET_HIGH_COLOR) {
				picture->WriteSetHighColor(color);
				fCurrentView->CurrentState()->SetHighColor(color);
				fWindow->GetDrawingEngine()->SetHighColor(color);
			} else {
				picture->WriteSetLowColor(color);
				fCurrentView->CurrentState()->SetLowColor(color);
				fWindow->GetDrawingEngine()->SetLowColor(color);
			}
		}	break;

		case AS_DRAW_STRING:
		case AS_DRAW_STRING_WITH_DELTA:
		{
			ViewDrawStringInfo info;
			if (link.Read<ViewDrawStringInfo>(&info) != B_OK)
				break;

			char* string = (char*)malloc(info.stringLength + 1);
			if (string == NULL)
				break;

			if (code != AS_DRAW_STRING_WITH_DELTA) {
				// In this case, info.delta will NOT contain valid values.
				info.delta = (escapement_delta){ 0, 0 };
			}

			if (link.Read(string, info.stringLength) != B_OK) {
				free(string);
				break;
			}
			// Terminate the string
			string[info.stringLength] = '\0';

			picture->WriteDrawString(info.location, string, info.stringLength,
				info.delta);

			free(string);
			break;
		}

		case AS_STROKE_SHAPE:
		case AS_FILL_SHAPE:
		{
			BRect shapeFrame;
			int32 opCount;
			int32 ptCount;

			link.Read<BRect>(&shapeFrame);
			link.Read<int32>(&opCount);
			link.Read<int32>(&ptCount);

			uint32* opList = new(std::nothrow) uint32[opCount];
			BPoint* ptList = new(std::nothrow) BPoint[ptCount];
			if (opList != NULL && ptList != NULL
				&& link.Read(opList, opCount * sizeof(uint32)) >= B_OK
				&& link.Read(ptList, ptCount * sizeof(BPoint)) >= B_OK) {
				// This might seem a bit weird, but under BeOS, the shapes
				// are always offset by the current pen location
				BPoint penLocation
					= fCurrentView->CurrentState()->PenLocation();
				for (int32 i = 0; i < ptCount; i++) {
					ptList[i] += penLocation;
				}
				const bool fill = (code == AS_FILL_SHAPE);
				picture->WriteDrawShape(opCount, opList, ptCount, ptList, fill);
			}

			delete[] opList;
			delete[] ptList;
			break;
		}

		case AS_VIEW_DRAW_BITMAP:
		{
			ViewDrawBitmapInfo info;
			link.Read<ViewDrawBitmapInfo>(&info);

			ServerBitmap* bitmap = App()->GetBitmap(info.bitmapToken);
			if (bitmap == NULL)
				break;

			picture->WriteDrawBitmap(info.bitmapRect, info.viewRect,
				bitmap->Width(), bitmap->Height(), bitmap->BytesPerRow(),
				bitmap->ColorSpace(), info.options, bitmap->Bits(),
				bitmap->BitsLength());

			bitmap->ReleaseReference();
			break;
		}

		case AS_VIEW_DRAW_PICTURE:
		{
			int32 token;
			link.Read<int32>(&token);

			BPoint where;
			if (link.Read<BPoint>(&where) == B_OK) {
				ServerPicture* pictureToDraw = App()->GetPicture(token);
				if (pictureToDraw != NULL) {
					// We need to make a copy of the picture, since it can
					// change after it has been drawn
					ServerPicture* copy = App()->CreatePicture(pictureToDraw);
					picture->NestPicture(copy);
					picture->WriteDrawPicture(where, copy->Token());

					pictureToDraw->ReleaseReference();
				}
			}
			break;
		}

		case AS_VIEW_SET_CLIP_REGION:
		{
			int32 rectCount;
			status_t status = link.Read<int32>(&rectCount);
				// a negative count means no
				// region for the current draw state,
				// but an *empty* region is actually valid!
				// even if it means no drawing is allowed

			if (status < B_OK)
				break;

			if (rectCount >= 0) {
				// we are supposed to set the clipping region
				BRegion region;
				if (rectCount > 0 && link.ReadRegion(&region) < B_OK)
					break;
				picture->WriteSetClipping(region);
			} else {
				// we are supposed to clear the clipping region
				picture->WriteClearClipping();
			}
			break;
		}

		case AS_VIEW_BEGIN_PICTURE:
		{
			ServerPicture* newPicture = App()->CreatePicture();
			if (newPicture != NULL) {
				newPicture->PushPicture(picture);
				newPicture->SyncState(fCurrentView);
				fCurrentView->SetPicture(newPicture);
			}
			break;
		}

		case AS_VIEW_APPEND_TO_PICTURE:
		{
			int32 token;
			link.Read<int32>(&token);

			ServerPicture* appendPicture = App()->GetPicture(token);
			if (appendPicture != NULL) {
				//picture->SyncState(fCurrentView);
				appendPicture->AppendPicture(picture);
			}

			fCurrentView->SetPicture(appendPicture);

			if (appendPicture != NULL)
				appendPicture->ReleaseReference();
			break;
		}

		case AS_VIEW_END_PICTURE:
		{
			ServerPicture* poppedPicture = picture->PopPicture();
			fCurrentView->SetPicture(poppedPicture);
			if (poppedPicture != NULL)
				poppedPicture->ReleaseReference();

			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(picture->Token());
			fLink.Flush();
			return true;
		}
/*
		case AS_VIEW_SET_BLENDING_MODE:
		{
			ViewBlendingModeInfo info;
			link.Read<ViewBlendingModeInfo>(&info);

			picture->BeginOp(B_PIC_SET_BLENDING_MODE);
			picture->AddInt16((int16)info.sourceAlpha);
			picture->AddInt16((int16)info.alphaFunction);
			picture->EndOp();

			fCurrentView->CurrentState()->SetBlendingMode(info.sourceAlpha,
				info.alphaFunction);
			fWindow->GetDrawingEngine()->SetBlendingMode(info.sourceAlpha,
				info.alphaFunction);
			break;
		}*/
		default:
			return false;
	}

	if (link.NeedsReply()) {
		fLink.StartMessage(B_ERROR);
		fLink.Flush();
	}
	return true;
}


/*!	\brief Message-dispatching loop for the ServerWindow

	Watches the ServerWindow's message port and dispatches as necessary
*/
void
ServerWindow::_MessageLooper()
{
	// Send a reply to our window - it is expecting fMessagePort
	// port and some other info.

	fLink.StartMessage(B_OK);
	fLink.Attach<port_id>(fMessagePort);

	int32 minWidth, maxWidth, minHeight, maxHeight;
	fWindow->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	fLink.Attach<BRect>(fWindow->Frame());
	fLink.Attach<float>((float)minWidth);
	fLink.Attach<float>((float)maxWidth);
	fLink.Attach<float>((float)minHeight);
	fLink.Attach<float>((float)maxHeight);
	fLink.Flush();

	BPrivate::LinkReceiver& receiver = fLink.Receiver();
	bool quitLoop = false;

	while (!quitLoop) {
		//STRACE(("info: ServerWindow::MonitorWin listening on port %ld.\n",
		//	fMessagePort));

		int32 code;
		status_t status = receiver.GetNextMessage(code);
		if (status != B_OK) {
			// that shouldn't happen, it's our port
			printf("Someone deleted our message port!\n");

			// try to let our client die happily
			NotifyQuitRequested();
			break;
		}

#ifdef PROFILE_MESSAGE_LOOP
		bigtime_t start = system_time();
#endif

		Lock();

#ifdef PROFILE_MESSAGE_LOOP
		bigtime_t diff = system_time() - start;
		if (diff > 10000) {
			printf("ServerWindow %s: lock acquisition took %Ld usecs\n",
				Title(), diff);
		}
#endif

		int32 messagesProcessed = 0;
		bigtime_t processingStart = system_time();
		bool lockedDesktopSingleWindow = false;

		while (true) {
			if (code == AS_DELETE_WINDOW || code == kMsgQuitLooper) {
				// this means the client has been killed
				DTRACE(("ServerWindow %s received 'AS_DELETE_WINDOW' message "
					"code\n", Title()));

				if (code == AS_DELETE_WINDOW) {
					fLink.StartMessage(B_OK);
					fLink.Flush();
				}

				if (lockedDesktopSingleWindow)
					fDesktop->UnlockSingleWindow();

				quitLoop = true;

				// ServerWindow's destructor takes care of pulling this object
				// off the desktop.
				ASSERT(fWindow->IsHidden());
				break;
			}

			// Acquire the appropriate lock
			bool needsAllWindowsLocked = _MessageNeedsAllWindowsLocked(code);
			if (needsAllWindowsLocked) {
				// We may already still hold the read-lock from the previous
				// inner-loop iteration.
				if (lockedDesktopSingleWindow) {
					fDesktop->UnlockSingleWindow();
					lockedDesktopSingleWindow = false;
				}
				fDesktop->LockAllWindows();
			} else {
				// We never keep the write-lock across inner-loop iterations,
				// so there is nothing else to do besides read-locking unless
				// we already have the read-lock from the previous iteration.
				if (!lockedDesktopSingleWindow) {
					fDesktop->LockSingleWindow();
					lockedDesktopSingleWindow = true;
				}
			}

			if (atomic_and(&fRedrawRequested, 0) != 0) {
#ifdef PROFILE_MESSAGE_LOOP
				bigtime_t redrawStart = system_time();
#endif
				fWindow->RedrawDirtyRegion();
#ifdef PROFILE_MESSAGE_LOOP
				diff = system_time() - redrawStart;
				atomic_add(&sRedrawProcessingTime.count, 1);
# ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
				atomic_add64(&sRedrawProcessingTime.time, diff);
# else
				sRedrawProcessingTime.time += diff;
# endif
#endif
			}

#ifdef PROFILE_MESSAGE_LOOP
			bigtime_t dispatchStart = system_time();
#endif
			_DispatchMessage(code, receiver);

#ifdef PROFILE_MESSAGE_LOOP
			if (code >= 0 && code < AS_LAST_CODE) {
				diff = system_time() - dispatchStart;
				atomic_add(&sMessageProfile[code].count, 1);
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
				atomic_add64(&sMessageProfile[code].time, diff);
#else
				sMessageProfile[code].time += diff;
#endif
				if (diff > 10000) {
					printf("ServerWindow %s: message %ld took %Ld usecs\n",
						Title(), code, diff);
				}
			}
#endif

			if (needsAllWindowsLocked)
				fDesktop->UnlockAllWindows();

			// Only process up to 70 waiting messages at once (we have the
			// Desktop locked), but don't hold the lock longer than 10 ms
			if (!receiver.HasMessages() || ++messagesProcessed > 70
				|| system_time() - processingStart > 10000) {
				if (lockedDesktopSingleWindow)
					fDesktop->UnlockSingleWindow();
				break;
			}

			// next message
			status_t status = receiver.GetNextMessage(code);
			if (status != B_OK) {
				// that shouldn't happen, it's our port
				printf("Someone deleted our message port!\n");
				if (lockedDesktopSingleWindow)
					fDesktop->UnlockSingleWindow();

				// try to let our client die happily
				NotifyQuitRequested();
				break;
			}
		}

		Unlock();
	}

	// We were asked to quit the message loop - either on request or because of
	// an error.
	Quit();
		// does not return
}


void
ServerWindow::ScreenChanged(const BMessage* message)
{
	SendMessageToClient(message);

	if (fDirectWindowInfo != NULL && fDirectWindowInfo->IsFullScreen())
		_ResizeToFullScreen();
}


status_t
ServerWindow::SendMessageToClient(const BMessage* msg, int32 target) const
{
	if (target == B_NULL_TOKEN)
		target = fClientToken;

	BMessenger reply;
	BMessage::Private messagePrivate((BMessage*)msg);
	return messagePrivate.SendMessage(fClientLooperPort, fClientTeam, target,
		0, false, reply);
}


Window*
ServerWindow::MakeWindow(BRect frame, const char* name,
	window_look look, window_feel feel, uint32 flags, uint32 workspace)
{
	// The non-offscreen ServerWindow uses the DrawingEngine instance from
	// the desktop.
	return new(std::nothrow) ::Window(frame, name, look, feel, flags,
		workspace, this, fDesktop->HWInterface()->CreateDrawingEngine());
}


void
ServerWindow::HandleDirectConnection(int32 bufferState, int32 driverState)
{
	ASSERT_MULTI_LOCKED(fDesktop->WindowLocker());

	if (fDirectWindowInfo == NULL)
		return;

	STRACE(("HandleDirectConnection(bufferState = %ld, driverState = %ld)\n",
		bufferState, driverState));

	status_t status = fDirectWindowInfo->SetState(
		(direct_buffer_state)bufferState, (direct_driver_state)driverState,
		fDesktop->HWInterface()->FrontBuffer(), fWindow->Frame(),
		fWindow->VisibleContentRegion());

	if (status != B_OK) {
		char errorString[256];
		snprintf(errorString, sizeof(errorString),
			"%s killed for a problem in DirectConnected(): %s",
			App()->Signature(), strerror(status));
		syslog(LOG_ERR, errorString);

		// The client application didn't release the semaphore
		// within the given timeout. Or something else went wrong.
		// Deleting this member should make it crash.
		delete fDirectWindowInfo;
		fDirectWindowInfo = NULL;
	} else if ((bufferState & B_DIRECT_MODE_MASK) == B_DIRECT_START)
		fIsDirectlyAccessing = true;
	else if ((bufferState & B_DIRECT_MODE_MASK) == B_DIRECT_STOP)
		fIsDirectlyAccessing = false;
}


void
ServerWindow::_SetCurrentView(View* view)
{
	if (fCurrentView == view)
		return;

	fCurrentView = view;
	fCurrentDrawingRegionValid = false;
	_UpdateDrawState(fCurrentView);

#if 0
#if DELAYED_BACKGROUND_CLEARING
	if (fCurrentView && fCurrentView->IsBackgroundDirty()
		&& fWindow->InUpdate()) {
		DrawingEngine* drawingEngine = fWindow->GetDrawingEngine();
		if (drawingEngine->LockParallelAccess()) {
			fWindow->GetEffectiveDrawingRegion(fCurrentView,
				fCurrentDrawingRegion);
			fCurrentDrawingRegionValid = true;
			BRegion dirty(fCurrentDrawingRegion);

			BRegion content;
			fWindow->GetContentRegion(&content);

			fCurrentView->Draw(drawingEngine, &dirty, &content, false);

			drawingEngine->UnlockParallelAccess();
		}
	}
#endif
#endif // 0
}


void
ServerWindow::_UpdateDrawState(View* view)
{
	// switch the drawing state
	// TODO: is it possible to scroll a view while it
	// is being drawn? probably not... otherwise the
	// "offsets" passed below would need to be updated again
	DrawingEngine* drawingEngine = fWindow->GetDrawingEngine();
	if (view && drawingEngine) {
		BPoint leftTop(0, 0);
		view->ConvertToScreenForDrawing(&leftTop);
		drawingEngine->SetDrawState(view->CurrentState(), leftTop.x, leftTop.y);
	}
}


void
ServerWindow::_UpdateCurrentDrawingRegion()
{
	if (!fCurrentDrawingRegionValid
		|| fWindow->DrawingRegionChanged(fCurrentView)) {
		fWindow->GetEffectiveDrawingRegion(fCurrentView, fCurrentDrawingRegion);
		fCurrentDrawingRegionValid = true;
	}
}


bool
ServerWindow::_MessageNeedsAllWindowsLocked(uint32 code) const
{
	switch (code) {
		case AS_SET_WINDOW_TITLE:
		case AS_ADD_TO_SUBSET:
		case AS_REMOVE_FROM_SUBSET:
		case AS_VIEW_CREATE_ROOT:
		case AS_VIEW_CREATE:
		case AS_SEND_BEHIND:
		case AS_SET_LOOK:
		case AS_SET_FEEL:
		case AS_SET_FLAGS:
		case AS_SET_WORKSPACES:
		case AS_WINDOW_MOVE:
		case AS_WINDOW_RESIZE:
		case AS_SET_SIZE_LIMITS:
		case AS_SYSTEM_FONT_CHANGED:
		case AS_SET_DECORATOR_SETTINGS:
		case AS_GET_MOUSE:
		case AS_DIRECT_WINDOW_SET_FULLSCREEN:
//		case AS_VIEW_SET_EVENT_MASK:
//		case AS_VIEW_SET_MOUSE_EVENT_MASK:
		case AS_TALK_TO_DESKTOP_LISTENER:
			return true;
		default:
			return false;
	}
}


void
ServerWindow::_ResizeToFullScreen()
{
	BRect screenFrame;

	{
		AutoReadLocker _(fDesktop->ScreenLocker());
		const Screen* screen = fWindow->Screen();
		if (screen == NULL)
			return;

		screenFrame = fWindow->Screen()->Frame();
	}

	fDesktop->MoveWindowBy(fWindow,
		screenFrame.left - fWindow->Frame().left,
		screenFrame.top - fWindow->Frame().top);
	fDesktop->ResizeWindowBy(fWindow,
		screenFrame.Width() - fWindow->Frame().Width(),
		screenFrame.Height() - fWindow->Frame().Height());
}


status_t
ServerWindow::_EnableDirectWindowMode()
{
	if (fDirectWindowInfo != NULL) {
		// already in direct window mode
		return B_ERROR;
	}

	if (fDesktop->HWInterface()->FrontBuffer() == NULL) {
		// direct window mode not supported
		return B_UNSUPPORTED;
	}

	fDirectWindowInfo = new(std::nothrow) DirectWindowInfo;
	if (fDirectWindowInfo == NULL)
		return B_NO_MEMORY;

	status_t status = fDirectWindowInfo->InitCheck();
	if (status != B_OK) {
		delete fDirectWindowInfo;
		fDirectWindowInfo = NULL;

		return status;
	}

	return B_OK;
}


void
ServerWindow::_DirectWindowSetFullScreen(bool enable)
{
	window_feel feel = kWindowScreenFeel;

	if (enable) {
		fDesktop->HWInterface()->SetCursorVisible(false);

		fDirectWindowInfo->EnableFullScreen(fWindow->Frame(), fWindow->Feel());
		_ResizeToFullScreen();
	} else {
		const BRect& originalFrame = fDirectWindowInfo->OriginalFrame();

		fDirectWindowInfo->DisableFullScreen();

		// Resize window back to its original size
		fDesktop->MoveWindowBy(fWindow,
			originalFrame.left - fWindow->Frame().left,
			originalFrame.top - fWindow->Frame().top);
		fDesktop->ResizeWindowBy(fWindow,
			originalFrame.Width() - fWindow->Frame().Width(),
			originalFrame.Height() - fWindow->Frame().Height());

		fDesktop->HWInterface()->SetCursorVisible(true);
	}

	fDesktop->SetWindowFeel(fWindow, feel);
}


status_t
ServerWindow::PictureToRegion(ServerPicture* picture, BRegion& region,
	bool inverse, BPoint where)
{
	fprintf(stderr, "ServerWindow::PictureToRegion() not implemented\n");
	region.MakeEmpty();
	return B_ERROR;
}
