/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	\class ServerWindow
	\brief Shadow BWindow class

	A ServerWindow handles all the intraserver tasks required of it by its BWindow. There are 
	too many tasks to list as being done by them, but they include handling View transactions, 
	coordinating and linking a window's Window half with its messaging half, dispatching 
	mouse and key events from the server to its window, and other such things.
*/


#include "ServerWindow.h"

#include "AppServer.h"
#include "DebugInfoManager.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "Overlay.h"
#include "ProfileMessageSupport.h"
#include "RAMLinkMsgReader.h"
#include "RenderingBuffer.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerProtocol.h"
#include "Window.h"
#include "WorkspacesView.h"

#include "clipping.h"

#include <DirectWindowPrivate.h>
#include <MessagePrivate.h>
#include <PortLink.h>
#include <ViewPrivate.h>
#include <WindowInfo.h>
#include <WindowPrivate.h>

#include <AppDefs.h>
#include <Autolock.h>
#include <DirectWindow.h>
#include <TokenSpace.h>
#include <View.h>

#include <new>

using std::nothrow;

//#define TRACE_SERVER_WINDOW
//#define TRACE_SERVER_WINDOW_MESSAGES
//#define PROFILE_MESSAGE_LOOP


#ifdef TRACE_SERVER_WINDOW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef TRACE_SERVER_WINDOW_MESSAGES
#	include <stdio.h>
#	define DTRACE(x) printf x
#else
#	define DTRACE(x) ;
#endif

#ifdef PROFILE_MESSAGE_LOOP
struct profile { int32 code; int32 count; bigtime_t time; };
static profile sMessageProfile[AS_LAST_CODE];
static profile sRedrawProcessingTime;
//static profile sNextMessageTime;
#endif


struct direct_window_data {
	direct_window_data();
	~direct_window_data();

	status_t InitCheck() const;
	
	sem_id	sem;
	sem_id	sem_ack;
	area_id	area;
	bool	started;
	direct_buffer_info *buffer_info;
};


direct_window_data::direct_window_data()
	:
	sem(-1),
	sem_ack(-1),
	area(-1),
	started(false),
	buffer_info(NULL)
{
	area = create_area("direct area", (void **)&buffer_info,
		B_ANY_ADDRESS, B_PAGE_SIZE, B_NO_LOCK, B_READ_WRITE);

	sem = create_sem(0, "direct sem");
	sem_ack = create_sem(0, "direct sem ack");
}


direct_window_data::~direct_window_data()
{
	// this should make the client die in case it's still running
	buffer_info->bits = NULL;
	buffer_info->bytes_per_row = 0;

	delete_area(area);
	delete_sem(sem);
	delete_sem(sem_ack);
}


status_t
direct_window_data::InitCheck() const 
{
	if (area < B_OK)
		return area;
	if (sem < B_OK)
		return sem;
	if (sem_ack < B_OK)
		return sem_ack;

	return B_OK;
}


//	#pragma mark -


/*!
	Sets up the basic BWindow counterpart - you have to call Init() before
	you can actually use it, though.
*/
ServerWindow::ServerWindow(const char *title, ServerApp *app,
	port_id clientPort, port_id looperPort, int32 clientToken)
	: MessageLooper(title && *title ? title : "Unnamed Window"),
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

	fDirectWindowData(NULL)
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


//! Tears down all connections the main app_server objects, and deletes some internals.
ServerWindow::~ServerWindow()
{
	STRACE(("ServerWindow(%s@%p):~ServerWindow()\n", fTitle, this));

	if (!fWindow->IsOffscreenWindow()) {
		fWindowAddedToDesktop = false;
		fDesktop->RemoveWindow(fWindow);
	}

	if (App() != NULL)
		App()->RemoveWindow(this);

	delete fWindow;

	free(fTitle);
	delete_port(fMessagePort);

	BPrivate::gDefaultTokens.RemoveToken(fServerToken);

	delete fDirectWindowData;
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
		printf("average redraw processing time: %g secs, count: %ld (%lld usecs per call)\n",
			sRedrawProcessingTime.time / 1000000.0, sRedrawProcessingTime.count,
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
	if (!fWindow)
		return B_NO_MEMORY;

	if (!fWindow->IsOffscreenWindow()) {
		fDesktop->AddWindow(fWindow);
		fWindowAddedToDesktop = true;
	}

	return B_OK;
}


bool
ServerWindow::Run()
{
	if (!MessageLooper::Run())
		return false;

	// Send a reply to our window - it is expecting fMessagePort
	// port and some other info

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

	return true;
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


//! Forces the window layer to update its decorator
void
ServerWindow::ReplaceDecorator()
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::ReplaceDecorator()\n");

	STRACE(("ServerWindow %s: Replace Decorator\n", fTitle));
	//fWindow->UpdateDecorator();
}


//! Shows the window's Window
void
ServerWindow::_Show()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: _Show\n", Title()));

	if (fQuitting || !fWindow->IsHidden() || fWindow->IsOffscreenWindow())
		return;

	// TODO: Maybe we need to dispatch a message to the desktop to show/hide us
	// instead of doing it from this thread.
	fDesktop->UnlockSingleWindow();
	fDesktop->ShowWindow(fWindow);
	fDesktop->LockSingleWindow();

	if (fDirectWindowData != NULL)
		HandleDirectConnection(B_DIRECT_START | B_BUFFER_RESET);
}


//! Hides the window's Window
void
ServerWindow::_Hide()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: _Hide\n", Title()));

	if (fWindow->IsHidden() || fWindow->IsOffscreenWindow())
		return;

	if (fDirectWindowData != NULL)
		HandleDirectConnection(B_DIRECT_STOP);

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

	if (fWindow != NULL) {
//fDesktop->UnlockSingleWindow();
		fDesktop->SetWindowTitle(fWindow, newTitle);
//fDesktop->LockSingleWindow();
	}
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

	info.layer = 0; // ToDo: what is this???
	info.feel = fWindow->Feel();
	info.flags = fWindow->Flags();
	info.window_left = (int)floor(fWindow->Frame().left);
	info.window_top = (int)floor(fWindow->Frame().top);
	info.window_right = (int)floor(fWindow->Frame().right);
	info.window_bottom = (int)floor(fWindow->Frame().bottom);

	info.show_hide_level = fWindow->IsHidden() ? 1 : 0; // ???
	info.is_mini = fWindow->IsMinimized();
}


void
ServerWindow::ResyncDrawState()
{
	_UpdateDrawState(fCurrentView);
}


/*!
	Returns the ServerWindow's Window, if it exists and has been
	added to the Desktop already.
	In other words, you cannot assume this method will always give you
	a valid pointer.
*/
Window*
ServerWindow::Window() const
{
	// TODO: ensure desktop is locked!
	if (!fWindowAddedToDesktop)
		return NULL;

	return fWindow;
}


View*
ServerWindow::_CreateView(BPrivate::LinkReceiver &link, View **_parent)
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

	STRACE(("ServerWindow(%s)::_CreateView()-> layer %s, token %ld\n",
		fTitle, name, token));

	View* newLayer;

	if ((flags & kWorkspacesViewFlag) != 0) {
		newLayer = new (nothrow) WorkspacesView(frame, scrollingOffset, name,
			token, resizeMask, flags);
	} else {
		newLayer = new (nothrow) View(frame, scrollingOffset, name, token,
			resizeMask, flags);
	}

	free(name);

	if (newLayer == NULL)
		return NULL;

	// there is no way of setting this, other than manually :-)
	newLayer->SetViewColor(viewColor);
	newLayer->SetHidden(hidden);
	newLayer->SetEventMask(eventMask, eventOptions);

	if (eventMask != 0 || eventOptions != 0) {
//		fDesktop->UnlockSingleWindow();
//		fDesktop->LockAllWindows();
fDesktop->UnlockAllWindows();
		// TODO: possible deadlock
		fDesktop->EventDispatcher().AddListener(EventTarget(),
			newLayer->Token(), eventMask, eventOptions);
fDesktop->LockAllWindows();
//		fDesktop->UnlockAllWindows();
//		fDesktop->LockSingleWindow();
	}

	// TODO: default fonts should be created and stored in the Application
	DesktopSettings settings(fDesktop);
	ServerFont font;
	settings.GetDefaultPlainFont(font);
	newLayer->CurrentState()->SetFont(font);

	if (_parent) {
		View *parent;
		if (App()->ViewTokens().GetToken(parentToken, B_HANDLER_TOKEN,
				(void**)&parent) != B_OK
			|| parent->Window()->ServerWindow() != this) {
			CRITICAL("View token not found!\n");
			parent = NULL;
		}

		*_parent = parent;
	}

	return newLayer;
}


/*!
	Dispatches all window messages, and those view messages that
	don't need a valid fCurrentView (ie. layer creation).
*/
void
ServerWindow::_DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
	switch (code) {
		case AS_SHOW_WINDOW:
			STRACE(("ServerWindow %s: Message AS_SHOW_WINDOW\n", Title()));
			_Show();
			break;

		case AS_HIDE_WINDOW:
			STRACE(("ServerWindow %s: Message AS_HIDE_WINDOW\n", Title()));
			_Hide();
			break;

		case AS_MINIMIZE_WINDOW:
		{
			DTRACE(("ServerWindow %s: Message AS_MINIMIZE_WINDOW\n", Title()));
			int32 showLevel;
			bool minimize;

			link.Read<bool>(&minimize);
			if (link.Read<int32>(&showLevel) == B_OK) {
				if (showLevel <= 0) {
					// window is currently hidden - ignore the minimize request
					break;
				}

				if (minimize && !fWindow->IsHidden())
					_Hide();
				else if (!minimize && fWindow->IsHidden())
					_Show();

				fWindow->SetMinimized(minimize);
			}
			break;
		}

		case AS_ACTIVATE_WINDOW:
		{
			DTRACE(("ServerWindow %s: Message AS_ACTIVATE_WINDOW: View: %s\n", Title(), fCurrentView->Name()));
			bool activate = true;

			link.Read<bool>(&activate);

//fDesktop->UnlockSingleWindow();
			if (activate)
				fDesktop->ActivateWindow(fWindow);
			else
				fDesktop->SendWindowBehind(fWindow, NULL);
//fDesktop->LockSingleWindow();
			break;
		}
		case AS_SEND_BEHIND:
		{
			STRACE(("ServerWindow %s: Message AS_SEND_BEHIND\n", Title()));
			int32 token;
			team_id teamID;
			status_t status;

			link.Read<int32>(&token);
			link.Read<team_id>(&teamID);

			::Window *behindOf = fDesktop->FindWindowByClientToken(token,
				teamID);
			if (behindOf != NULL) {
//fDesktop->UnlockSingleWindow();
// TODO: there is a big race condition when we unlock here (window could be gone by now)!
				fDesktop->SendWindowBehind(fWindow, behindOf);
//fDesktop->LockSingleWindow();
				status = B_OK;
			} else
				status = B_NAME_NOT_FOUND;

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}

		case B_QUIT_REQUESTED:
			STRACE(("ServerWindow %s received quit request\n", Title()));
			NotifyQuitRequested();
			break;

		case AS_ENABLE_UPDATES:
		{
			STRACE(("ServerWindow %s: Message AS_ENABLE_UPDATES unimplemented\n",
				Title()));
			fWindow->EnableUpdateRequests();
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			STRACE(("ServerWindow %s: Message AS_DISABLE_UPDATES unimplemented\n",
				Title()));
			fWindow->DisableUpdateRequests();
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			STRACE(("ServerWindow %s: Message AS_NEEDS_UPDATE\n", Title()));
			if (fWindow->NeedsUpdate())
				fLink.StartMessage(B_OK);
			else
				fLink.StartMessage(B_ERROR);
			fLink.Flush();
			break;
		}
		case AS_SET_WINDOW_TITLE:
		{
			char* newTitle;
			if (link.ReadString(&newTitle) == B_OK) {
				SetTitle(newTitle);
				free(newTitle);
			}
			break;
		}

		case AS_ADD_TO_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_ADD_TO_SUBSET\n", Title()));
			status_t status = B_ERROR;

			int32 token;
			if (link.Read<int32>(&token) == B_OK) {
				::Window* window = fDesktop->FindWindowByClientToken(token,
					App()->ClientTeam());
				if (window == NULL || window->Feel() != B_NORMAL_WINDOW_FEEL) {
					status = B_BAD_VALUE;
				} else {
//fDesktop->UnlockSingleWindow();
// TODO: there is a big race condition when we unlock here (window could be gone by now)!
					status = fDesktop->AddWindowToSubset(fWindow, window)
						? B_OK : B_NO_MEMORY;
//fDesktop->LockSingleWindow();
				}
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
		case AS_REMOVE_FROM_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_REM_FROM_SUBSET\n", Title()));
			status_t status = B_ERROR;

			int32 token;
			if (link.Read<int32>(&token) == B_OK) {
				::Window* window = fDesktop->FindWindowByClientToken(token,
					App()->ClientTeam());
				if (window != NULL) {
//fDesktop->UnlockSingleWindow();
// TODO: there is a big race condition when we unlock here (window could be gone by now)!
					fDesktop->RemoveWindowFromSubset(fWindow, window);
//fDesktop->LockSingleWindow();
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
			STRACE(("ServerWindow %s: Message AS_SET_LOOK\n", Title()));

			status_t status = B_ERROR;
			int32 look;
			if (link.Read<int32>(&look) == B_OK) {
				// test if look is valid
				status = Window::IsValidLook((window_look)look)
					? B_OK : B_BAD_VALUE;
			}

			if (status == B_OK && !fWindow->IsOffscreenWindow()) {
//fDesktop->UnlockSingleWindow();
				fDesktop->SetWindowLook(fWindow, (window_look)look);
//fDesktop->LockSingleWindow();
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
		case AS_SET_FEEL:
		{
			STRACE(("ServerWindow %s: Message AS_SET_FEEL\n", Title()));

			status_t status = B_ERROR;
			int32 feel;
			if (link.Read<int32>(&feel) == B_OK) {
				// test if feel is valid
				status = Window::IsValidFeel((window_feel)feel)
					? B_OK : B_BAD_VALUE;
			}

			if (status == B_OK && !fWindow->IsOffscreenWindow()) {
//fDesktop->UnlockSingleWindow();
				fDesktop->SetWindowFeel(fWindow, (window_feel)feel);
//fDesktop->LockSingleWindow();
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
		case AS_SET_FLAGS:
		{
			STRACE(("ServerWindow %s: Message AS_SET_FLAGS\n", Title()));

			status_t status = B_ERROR;
			uint32 flags;
			if (link.Read<uint32>(&flags) == B_OK) {
				// test if flags are valid
				status = (flags & ~Window::ValidWindowFlags()) == 0
					? B_OK : B_BAD_VALUE;
			}

			if (status == B_OK && !fWindow->IsOffscreenWindow()) {
//fDesktop->UnlockSingleWindow();
				fDesktop->SetWindowFlags(fWindow, flags);
//fDesktop->LockSingleWindow();
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
#if 0
		case AS_SET_ALIGNMENT:
		{
			// TODO: Implement AS_SET_ALIGNMENT
			STRACE(("ServerWindow %s: Message Set_Alignment unimplemented\n", Title()));
			break;
		}
		case AS_GET_ALIGNMENT:
		{
			// TODO: Implement AS_GET_ALIGNMENT
			STRACE(("ServerWindow %s: Message Get_Alignment unimplemented\n", Title()));
			break;
		}
#endif
		case AS_IS_FRONT_WINDOW:
			fLink.StartMessage(fDesktop->FrontWindow() == fWindow ? B_OK : B_ERROR);
			fLink.Flush();
			break;

		case AS_GET_WORKSPACES:
		{
			STRACE(("ServerWindow %s: Message AS_GET_WORKSPACES\n", Title()));
			fLink.StartMessage(B_OK);
			fLink.Attach<uint32>(fWindow->Workspaces());
			fLink.Flush();
			break;
		}
		case AS_SET_WORKSPACES:
		{
			uint32 newWorkspaces;
			if (link.Read<uint32>(&newWorkspaces) != B_OK)
				break;

			STRACE(("ServerWindow %s: Message AS_SET_WORKSPACES %lx\n",
				Title(), newWorkspaces));

//fDesktop->UnlockSingleWindow();
			fDesktop->SetWindowWorkspaces(fWindow, newWorkspaces);
//fDesktop->LockSingleWindow();
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			float xResizeBy;
			float yResizeBy;
			link.Read<float>(&xResizeBy);
			if (link.Read<float>(&yResizeBy) != B_OK)
				break;

			STRACE(("ServerWindow %s: Message AS_WINDOW_RESIZE %.1f, %.1f\n",
				Title(), xResizeBy, yResizeBy));

			if (fWindow->IsResizing()) {
				// While the user resizes the window, we ignore
				// pragmatically set window bounds
				fLink.StartMessage(B_BUSY);
			} else {
//fDesktop->UnlockSingleWindow();
				fDesktop->ResizeWindowBy(fWindow, xResizeBy, yResizeBy);
//fDesktop->LockSingleWindow();
				fLink.StartMessage(B_OK);
			}
			fLink.Flush();
			break;
		}
		case AS_WINDOW_MOVE:
		{
			float xMoveBy;
			float yMoveBy;
			link.Read<float>(&xMoveBy);
			if (link.Read<float>(&yMoveBy) != B_OK)
				break;

			STRACE(("ServerWindow %s: Message AS_WINDOW_MOVE: %.1f, %.1f\n",
				Title(), xMoveBy, yMoveBy));

			if (fWindow->IsDragging()) {
				// While the user moves the window, we ignore
				// pragmatically set window positions
				fLink.StartMessage(B_BUSY);
			} else {
//fDesktop->UnlockSingleWindow();
				fDesktop->MoveWindowBy(fWindow, xMoveBy, yMoveBy);
//fDesktop->LockSingleWindow();
				fLink.StartMessage(B_OK);
			}
			fLink.Flush();
			break;
		}
		case AS_SET_SIZE_LIMITS:
		{
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
//fDesktop->UnlockSingleWindow();

			if (fDesktop->LockAllWindows()) {
				fWindow->SetSizeLimits(minWidth, maxWidth,
					minHeight, maxHeight);
				fDesktop->UnlockAllWindows();
			}

//fDesktop->LockSingleWindow();

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
			break;
		}

		case AS_SET_DECORATOR_SETTINGS:
		{
			STRACE(("ServerWindow %s: Message AS_SET_DECORATOR_SETTINGS\n"));

			int32 size;
			if (fWindow && link.Read<int32>(&size) == B_OK) {
				char buffer[size];
				if (link.Read(buffer, size) == B_OK) {
					BMessage settings;
					if (settings.Unflatten(buffer) == B_OK) {
//fDesktop->UnlockSingleWindow();
						fDesktop->SetWindowDecoratorSettings(fWindow, settings);
//fDesktop->LockSingleWindow();
					}
				}
			}
			break;
		}
	
		case AS_GET_DECORATOR_SETTINGS:
		{
			STRACE(("ServerWindow %s: Message AS_GET_DECORATOR_SETTINGS\n"));

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

		case AS_REDRAW:
			// Nothing to do here - the redraws are actually handled by looking
			// at the fRedrawRequested member variable in _MessageLooper().
			break;

		case AS_SYNC:
			// the synchronisation works by the fact that the client
			// window is waiting for this reply, after having received it,
			// client and server queues are in sync (earlier, the client
			// may have pushed drawing commands at the server and now it
			// knows they have all been carried out)
			fLink.StartMessage(B_OK);
			fLink.Flush();
			break;

		case AS_BEGIN_UPDATE:
			DTRACE(("ServerWindowo %s: AS_BEGIN_UPDATE\n", Title()));
			fWindow->BeginUpdate(fLink);
			break;

		case AS_END_UPDATE:
			DTRACE(("ServerWindowo %s: AS_END_UPDATE\n", Title()));
			fWindow->EndUpdate();
			break;

		case AS_GET_MOUSE:
		{
			DTRACE(("ServerWindow %s: Message AS_GET_MOUSE\n", fTitle));

//fDesktop->UnlockSingleWindow();
			// Returns
			// 1) BPoint mouse location
			// 2) int32 button state

			BPoint where;
			int32 buttons;
			fDesktop->GetLastMouseState(&where, &buttons);
//fDesktop->LockSingleWindow();

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
				struct direct_window_sync_data syncData = { 
					fDirectWindowData->area,
					fDirectWindowData->sem,
					fDirectWindowData->sem_ack
				};

				fLink.Attach(&syncData, sizeof(syncData));
			}

			fLink.Flush();
			break;
		}
		case AS_DIRECT_WINDOW_SET_FULLSCREEN:
		{
			// TODO: maybe there is more to do than this?
			bool enable;
			link.Read<bool>(&enable);

			status_t status = B_OK;
			if (!fWindow->IsOffscreenWindow()) {
//fDesktop->UnlockSingleWindow();
				fDesktop->SetWindowFeel(fWindow,
					enable ? kWindowScreenFeel : B_NORMAL_WINDOW_FEEL);
//fDesktop->LockSingleWindow();
			} else
				status = B_BAD_TYPE;

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}

		// View creation and destruction (don't need a valid fCurrentView)

		case AS_SET_CURRENT_LAYER:
		{
			int32 token;
			if (link.Read<int32>(&token) != B_OK)
				break;

			View *current;
			if (App()->ViewTokens().GetToken(token, B_HANDLER_TOKEN,
					(void**)&current) != B_OK
				|| current->Window()->ServerWindow() != this) {
				// ToDo: if this happens, we probably want to kill the app and clean up
				fprintf(stderr, "ServerWindow %s: Message AS_SET_CURRENT_LAYER: layer not found, token %ld\n", fTitle, token);
				current = NULL;
			} else {
				DTRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: %s, token %ld\n", fTitle, current->Name(), token));
				_SetCurrentView(current);
			}
			break;
		}

		case AS_LAYER_CREATE_ROOT:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE_ROOT\n", fTitle));

			// Start receiving top_view data -- pass NULL as the parent view.
			// This should be the *only* place where this happens.
			if (fCurrentView != NULL) {
				fprintf(stderr, "ServerWindow %s: Message AS_LAYER_CREATE_ROOT: fCurrentView already set!!\n", fTitle);
				break;
			}

			_SetCurrentView(_CreateView(link, NULL));
			fWindow->SetTopLayer(fCurrentView);
			break;
		}

		case AS_LAYER_CREATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE: View name: %s\n", fTitle, fCurrentView->Name()));

			View* parent = NULL;
			View* newLayer = _CreateView(link, &parent);
			if (parent != NULL && newLayer != NULL)
				parent->AddChild(newLayer);
			else
				fprintf(stderr, "ServerWindow %s: Message AS_LAYER_CREATE: parent or newLayer NULL!!\n", fTitle);
			break;
		}

		default:
			// TODO: when creating a View, check for yet non-existing View::InitCheck()
			// and take appropriate actions, then checking for fCurrentView->CurrentState()
			// is unnecessary
			if (fCurrentView == NULL || fCurrentView->CurrentState() == NULL) {
				BString codeName;
				string_for_message_code(code, codeName);
				printf("ServerWindow %s received unexpected code - "
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
		case AS_LAYER_SCROLL:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SCROLL: View name: %s\n", fTitle, fCurrentView->Name()));
			float dh;
			float dv;

			link.Read<float>(&dh);
			link.Read<float>(&dv);
			fWindow->ScrollViewBy(fCurrentView, dh, dv);
			break;
		}
		case AS_LAYER_COPY_BITS:
		{
			BRect src;
			BRect dst;

			link.Read<BRect>(&src);
			link.Read<BRect>(&dst);

			BRegion contentRegion;
			// TODO: avoid copy operation maybe?
			fWindow->GetContentRegion(&contentRegion);
			fCurrentView->CopyBits(src, dst, contentRegion);
			break;
		}
		case AS_LAYER_DELETE:
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

				STRACE(("ServerWindow %s: AS_LAYER_DELETE view: %p, parent: %p\n",
					fTitle, view, parent));

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
		case AS_LAYER_SET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_STATE: View name: %s\n", fTitle, fCurrentView->Name()));

			fCurrentView->CurrentState()->ReadFromLink(link);
			// TODO: When is this used?!?
			fCurrentView->RebuildClipping(true);
			_UpdateDrawState(fCurrentView);

			break;
		}
		case AS_LAYER_SET_FONT_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: View name: %s\n", fTitle, fCurrentView->Name()));
			fCurrentView->CurrentState()->ReadFontFromLink(link);
			fWindow->GetDrawingEngine()->SetFont(
				fCurrentView->CurrentState());
			break;
		}
		case AS_LAYER_GET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_STATE: View name: %s\n", fTitle, fCurrentView->Name()));

			fLink.StartMessage(B_OK);

			// attach state data
			fCurrentView->CurrentState()->WriteToLink(fLink.Sender());
			fLink.Flush();
			break;
		}
		case AS_LAYER_SET_EVENT_MASK:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_EVENT_MASK: View name: %s\n", fTitle, fCurrentView->Name()));			
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
		case AS_LAYER_SET_MOUSE_EVENT_MASK:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_MOUSE_EVENT_MASK: View name: %s\n", fTitle, fCurrentView->Name()));			
			uint32 eventMask, options;

			link.Read<uint32>(&eventMask);
			if (link.Read<uint32>(&options) == B_OK) {
fDesktop->UnlockSingleWindow();
				// TODO: possible deadlock
				if (eventMask != 0 || options != 0) {
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
		case AS_LAYER_MOVE_TO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_MOVE_TO: View name: %s\n",
				fTitle, fCurrentView->Name()));

			float x, y;
			link.Read<float>(&x);
			link.Read<float>(&y);

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
		case AS_LAYER_RESIZE_TO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_TO: View name: %s\n",
				fTitle, fCurrentView->Name()));

			float newWidth, newHeight;
			link.Read<float>(&newWidth);
			link.Read<float>(&newHeight);
			
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
		case AS_LAYER_GET_COORD:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: View: %s\n", Title(), fCurrentView->Name()));
			fLink.StartMessage(B_OK);
			// our offset in the parent -> will be originX and originY in BView
			BPoint parentOffset = fCurrentView->Frame().LeftTop();
			fLink.Attach<BPoint>(parentOffset);
			fLink.Attach<BRect>(fCurrentView->Bounds());
			fLink.Flush();
			break;
		}
		case AS_LAYER_SET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_ORIGIN: View: %s\n", Title(), fCurrentView->Name()));

			float x, y;
			link.Read<float>(&x);
			link.Read<float>(&y);
			
			fCurrentView->SetDrawingOrigin(BPoint(x, y));
			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_LAYER_GET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_ORIGIN: View: %s\n", Title(), fCurrentView->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(fCurrentView->DrawingOrigin());
			fLink.Flush();
			break;
		}
		case AS_LAYER_RESIZE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_MODE: View: %s\n",
				Title(), fCurrentView->Name()));

			uint32 resizeMode;
			if (link.Read<uint32>(&resizeMode) == B_OK)
				fCurrentView->SetResizeMode(resizeMode);
			break;
		}
		case AS_LAYER_SET_CURSOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_CURSOR: View: %s\n", Title(),	
				fCurrentView->Name()));

			int32 token;
			bool sync;
			link.Read<int32>(&token);
			if (link.Read<bool>(&sync) != B_OK)
				break;

			if (!fDesktop->GetCursorManager().Lock())
				break;

			ServerCursor* cursor = fDesktop->GetCursorManager().FindCursor(token);
			fCurrentView->SetCursor(cursor);

			fDesktop->GetCursorManager().Unlock();

			if (fWindow->IsFocus()) {
				// The cursor might need to be updated now
				if (fDesktop->ViewUnderMouse(fWindow) == fCurrentView->Token())
					fServerApp->SetCurrentCursor(cursor);
			}
			if (sync) {
				// sync the client (it can now delete the cursor)
				fLink.StartMessage(B_OK);
				fLink.Flush();
			}

			break;
		}
		case AS_LAYER_SET_FLAGS:
		{
			uint32 flags;
			link.Read<uint32>(&flags);
			fCurrentView->SetFlags(flags);
			_UpdateDrawState(fCurrentView);

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FLAGS: View: %s\n", Title(), fCurrentView->Name()));
			break;
		}
		case AS_LAYER_HIDE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_HIDE: View: %s\n", Title(), fCurrentView->Name()));
			fCurrentView->SetHidden(true);
			break;
		}
		case AS_LAYER_SHOW:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SHOW: View: %s\n", Title(), fCurrentView->Name()));
			fCurrentView->SetHidden(false);
			break;
		}
		case AS_LAYER_SET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_LINE_MODE: View: %s\n", Title(), fCurrentView->Name()));
			int8 lineCap, lineJoin;
			float miterLimit;

			link.Read<int8>(&lineCap);
			link.Read<int8>(&lineJoin);
			link.Read<float>(&miterLimit);
			
			fCurrentView->CurrentState()->SetLineCapMode((cap_mode)lineCap);
			fCurrentView->CurrentState()->SetLineJoinMode((join_mode)lineJoin);
			fCurrentView->CurrentState()->SetMiterLimit(miterLimit);

			fWindow->GetDrawingEngine()->SetStrokeMode((cap_mode)lineCap,
				(join_mode)lineJoin, miterLimit);
//			_UpdateDrawState(fCurrentView);

			break;
		}
		case AS_LAYER_GET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_LINE_MODE: View: %s\n", Title(), fCurrentView->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<int8>((int8)(fCurrentView->CurrentState()->LineCapMode()));
			fLink.Attach<int8>((int8)(fCurrentView->CurrentState()->LineJoinMode()));
			fLink.Attach<float>(fCurrentView->CurrentState()->MiterLimit());
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_PUSH_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_PUSH_STATE: View: %s\n", Title(), fCurrentView->Name()));
			
			fCurrentView->PushState();
			// TODO: is this necessary?
			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_LAYER_POP_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_POP_STATE: View: %s\n", Title(), fCurrentView->Name()));
			
			fCurrentView->PopState();
			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_LAYER_SET_SCALE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: View: %s\n", Title(), fCurrentView->Name()));
			float scale;
			link.Read<float>(&scale);

			fCurrentView->SetScale(scale);
			_UpdateDrawState(fCurrentView);
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_SCALE: View: %s\n", Title(), fCurrentView->Name()));		

			fLink.StartMessage(B_OK);
			fLink.Attach<float>(fCurrentView->CurrentState()->Scale());
			fLink.Flush();
			break;
		}
		case AS_LAYER_SET_PEN_LOC:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_LOC: View: %s\n", Title(), fCurrentView->Name()));
			float x, y;

			link.Read<float>(&x);
			link.Read<float>(&y);

			fCurrentView->CurrentState()->SetPenLocation(BPoint(x, y));
			break;
		}
		case AS_LAYER_GET_PEN_LOC:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_LOC: View: %s\n", Title(), fCurrentView->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(fCurrentView->CurrentState()->PenLocation());
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_SIZE: View: %s\n", Title(), fCurrentView->Name()));
			float penSize;
			link.Read<float>(&penSize);

			fCurrentView->CurrentState()->SetPenSize(penSize);
			fWindow->GetDrawingEngine()->SetPenSize(
				fCurrentView->CurrentState()->PenSize());
			break;
		}
		case AS_LAYER_GET_PEN_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_SIZE: View: %s\n", Title(), fCurrentView->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<float>(
				fCurrentView->CurrentState()->UnscaledPenSize());
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: View: %s\n", Title(), fCurrentView->Name()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));

			fCurrentView->SetViewColor(c);
			break;
		}

		case AS_LAYER_GET_HIGH_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_HIGH_COLOR: View: %s\n",
				Title(), fCurrentView->Name()));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(fCurrentView->CurrentState()->HighColor());
			fLink.Flush();
			break;

		case AS_LAYER_GET_LOW_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_LOW_COLOR: View: %s\n",
				Title(), fCurrentView->Name()));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(fCurrentView->CurrentState()->LowColor());
			fLink.Flush();
			break;

		case AS_LAYER_GET_VIEW_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_VIEW_COLOR: View: %s\n",
				Title(), fCurrentView->Name()));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(fCurrentView->ViewColor());
			fLink.Flush();
			break;

		case AS_LAYER_SET_BLENDING_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_BLEND_MODE: View: %s\n", Title(), fCurrentView->Name()));
			int8 srcAlpha, alphaFunc;
			
			link.Read<int8>(&srcAlpha);
			link.Read<int8>(&alphaFunc);
			
			fCurrentView->CurrentState()->SetBlendingMode((source_alpha)srcAlpha,
				(alpha_function)alphaFunc);
			//_UpdateDrawState(fCurrentView);
			fWindow->GetDrawingEngine()->SetBlendingMode((source_alpha)srcAlpha,
				(alpha_function)alphaFunc);
			break;
		}
		case AS_LAYER_GET_BLENDING_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_BLEND_MODE: View: %s\n", Title(), fCurrentView->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<int8>((int8)(fCurrentView->CurrentState()->AlphaSrcMode()));
			fLink.Attach<int8>((int8)(fCurrentView->CurrentState()->AlphaFncMode()));
			fLink.Flush();

			break;
		}
		case AS_LAYER_SET_DRAWING_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_DRAW_MODE: View: %s\n", Title(), fCurrentView->Name()));
			int8 drawingMode;
			
			link.Read<int8>(&drawingMode);
			
			fCurrentView->CurrentState()->SetDrawingMode((drawing_mode)drawingMode);
			//_UpdateDrawState(fCurrentView);
			fWindow->GetDrawingEngine()->SetDrawingMode((drawing_mode)drawingMode);
			break;
		}
		case AS_LAYER_GET_DRAWING_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_DRAW_MODE: View: %s\n", Title(), fCurrentView->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<int8>((int8)(fCurrentView->CurrentState()->GetDrawingMode()));
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_SET_VIEW_BITMAP:
		{
			int32 bitmapToken, resizingMode, options;
			BRect srcRect, dstRect;

			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&srcRect);
			link.Read<BRect>(&dstRect);
			link.Read<int32>(&resizingMode);
			status_t status = link.Read<int32>(&options);

			rgb_color colorKey = {0};

			if (status == B_OK) {
				ServerBitmap* bitmap = fServerApp->FindBitmap(bitmapToken);
				if (bitmapToken == -1 || bitmap != NULL) {
					bool wasOverlay = fCurrentView->ViewBitmap() != NULL
						&& fCurrentView->ViewBitmap()->Overlay() != NULL;

					// TODO: this is a race condition: the bitmap could have been
					//	deleted in the mean time!!
					fCurrentView->SetViewBitmap(bitmap, srcRect, dstRect,
						resizingMode, options);

					// TODO: if we revert the view color overlay handling
					//	in View::Draw() to the R5 version, we never
					//	need to invalidate the view for overlays.

					// invalidate view - but only if this is a non-overlay switch
					if (bitmap == NULL || bitmap->Overlay() == NULL || !wasOverlay) {
						BRegion dirty((BRect)fCurrentView->Bounds());
						fWindow->InvalidateView(fCurrentView, dirty);
					}

					if (bitmap != NULL && bitmap->Overlay() != NULL) {
						bitmap->Overlay()->SetFlags(options);
						colorKey = bitmap->Overlay()->Color();
					}
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
		case AS_LAYER_PRINT_ALIASING:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_PRINT_ALIASING: View: %s\n", Title(), fCurrentView->Name()));
			bool fontAliasing;
			if (link.Read<bool>(&fontAliasing) == B_OK) {
				fCurrentView->CurrentState()->SetForceFontAliasing(fontAliasing);	
				_UpdateDrawState(fCurrentView);
			}
			break;
		}
		case AS_LAYER_CLIP_TO_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_PICTURE: View: %s\n", Title(), fCurrentView->Name()));
			// TODO: you are not allowed to use View regions here!!!

			int32 pictureToken;
			BPoint where;
			bool inverse = false;

			link.Read<int32>(&pictureToken);
			link.Read<BPoint>(&where);
			link.Read<bool>(&inverse);

			// search for a picture with the specified token.
			ServerPicture *picture = fServerApp->FindPicture(pictureToken);
			// TODO: Increase that picture's reference count.(~ allocate a picture)
			if (picture == NULL)
				break;

			BRegion region;
			// TODO: I think we also need the BView's token
			// I think PictureToRegion would fit better into the View class (?)
			if (PictureToRegion(picture, region, inverse, where) < B_OK)
				break;

			fCurrentView->SetUserClipping(&region);

// TODO: reenable AS_LAYER_CLIP_TO_PICTURE
#if 0
			if (rootLayer && !(fCurrentView->IsHidden()) && !fWindow->InUpdate()) {
				BRegion invalidRegion;
				fCurrentView->GetOnScreenRegion(invalidRegion);

				// TODO: this is broken! a smaller area may be invalidated!

				fCurrentView->fParent->MarkForRebuild(invalidRegion);
				fCurrentView->fParent->TriggerRebuild();
				rootLayer->MarkForRedraw(invalidRegion);
				rootLayer->TriggerRedraw();
			}
#endif
			break;
		}

		case AS_LAYER_GET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_CLIP_REGION: View: %s\n", Title(), fCurrentView->Name()));
			
			// if this View is hidden, it is clear that its visible region is void.
			fLink.StartMessage(B_OK);
			if (fCurrentView->IsHidden()) {
				BRegion empty;
				fLink.AttachRegion(empty);
			} else {
				BRegion drawingRegion = fCurrentView->LocalClipping();
				fLink.AttachRegion(drawingRegion);
			}
			fLink.Flush();

			break;
		}
		case AS_LAYER_SET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_CLIP_REGION: View: %s\n", Title(), fCurrentView->Name()));
			
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
				fCurrentView->SetUserClipping(&region);
			} else {
				// we are supposed to unset the clipping region
				// passing NULL sets this states region to that
				// of the previous state
				fCurrentView->SetUserClipping(NULL);
			}
			fCurrentDrawingRegionValid = false;

			break;
		}

		case AS_LAYER_INVALIDATE_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVALIDATE_RECT: View: %s\n", Title(), fCurrentView->Name()));

			// NOTE: looks like this call is NOT affected by origin and scale on R5
			// so this implementation is "correct"
			BRect invalidRect;
			if (link.Read<BRect>(&invalidRect) == B_OK) {
				BRegion dirty(invalidRect);
				fWindow->InvalidateView(fCurrentView, dirty);
			}
			break;
		}
		case AS_LAYER_INVALIDATE_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVALIDATE_RECT: View: %s\n", Title(), fCurrentView->Name()));

			// NOTE: looks like this call is NOT affected by origin and scale on R5
			// so this implementation is "correct"
			BRegion region;
			if (link.ReadRegion(&region) < B_OK)
				break;

			fWindow->InvalidateView(fCurrentView, region);
			break;
		}

		case AS_LAYER_SET_HIGH_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_HIGH_COLOR: View: %s\n", Title(), fCurrentView->Name()));

			rgb_color c;
			link.Read(&c, sizeof(rgb_color));
			
			fCurrentView->CurrentState()->SetHighColor(c);
			fWindow->GetDrawingEngine()->SetHighColor(c);
			break;
		}
		case AS_LAYER_SET_LOW_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_LOW_COLOR: View: %s\n", Title(), fCurrentView->Name()));

			rgb_color c;
			link.Read(&c, sizeof(rgb_color));

			fCurrentView->CurrentState()->SetLowColor(c);
			fWindow->GetDrawingEngine()->SetLowColor(c);
			break;
		}
		case AS_LAYER_SET_PATTERN:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PATTERN: View: %s\n", fTitle, fCurrentView->Name()));

			pattern pat;
			link.Read(&pat, sizeof(pattern));

			fCurrentView->CurrentState()->SetPattern(Pattern(pat));
			fWindow->GetDrawingEngine()->SetPattern(pat);
			break;
		}
		case AS_LAYER_DRAG_IMAGE:
		{
			// TODO: flesh out AS_LAYER_DRAG_IMAGE
			STRACE(("ServerWindow %s: Message AS_DRAG_IMAGE\n", Title()));

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
						ServerBitmap* bitmap = fServerApp->FindBitmap(bitmapToken);
						// TODO: possible deadlock
fDesktop->UnlockSingleWindow();
						fDesktop->EventDispatcher().SetDragMessage(dragMessage,
							bitmap, offset);
fDesktop->LockSingleWindow();
				}
				delete[] buffer;
			}
			// sync the client (it can now delete the bitmap)
			fLink.StartMessage(B_OK);
			fLink.Flush();

			break;
		}
		case AS_LAYER_DRAG_RECT:
		{
			// TODO: flesh out AS_LAYER_DRAG_RECT
			STRACE(("ServerWindow %s: Message AS_DRAG_RECT\n", Title()));

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

		case AS_LAYER_BEGIN_RECT_TRACK:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_BEGIN_RECT_TRACK\n", Title()));
			BRect dragRect;
			uint32 style;

			link.Read<BRect>(&dragRect);
			link.Read<uint32>(&style);

			// TODO: implement rect tracking (used sometimes for selecting
			// a group of things, also sometimes used to appear to drag something,
			// but without real drag message)
			break;
		}
		case AS_LAYER_END_RECT_TRACK:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_END_RECT_TRACK\n", Title()));
			// TODO: implement rect tracking
			break;
		}

		case AS_LAYER_BEGIN_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_BEGIN_PICTURE\n", Title()));
			ServerPicture *picture = App()->CreatePicture();
			picture->SyncState(fCurrentView);
			fCurrentView->SetPicture(picture);
			break;
		}

		case AS_LAYER_APPEND_TO_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_APPEND_TO_PICTURE\n", Title()));
			
			int32 pictureToken;
			link.Read<int32>(&pictureToken);
			ServerPicture *picture = App()->FindPicture(pictureToken);
			if (picture)
				picture->SyncState(fCurrentView);
			fCurrentView->SetPicture(picture);
				// we don't care if it's NULL
			break;
		}

		case AS_LAYER_END_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_END_PICTURE\n", Title()));
			
			ServerPicture *picture = fCurrentView->Picture();
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


/*!
	Dispatches all view drawing messages.
	The desktop clipping must be read locked when entering this method.
	Requires a valid fCurrentView.
*/
void
ServerWindow::_DispatchViewDrawingMessage(int32 code, BPrivate::LinkReceiver &link)
{
	if (!fCurrentView->IsVisible() || !fWindow->IsVisible()) {
		if (link.NeedsReply()) {
			printf("ServerWindow::DispatchViewDrawingMessage() got message %ld that needs a reply!\n", code);
			// the client is now blocking and waiting for a reply!
			fLink.StartMessage(B_ERROR);
			fLink.Flush();
		}
		return;
	}

	DrawingEngine* drawingEngine = fWindow->GetDrawingEngine();
	if (!drawingEngine) {
		// ?!?
		DTRACE(("ServerWindow %s: no drawing engine!!\n", Title()));
		if (link.NeedsReply()) {
			// the client is now blocking and waiting for a reply!
			fLink.StartMessage(B_ERROR);
			fLink.Flush();
		}
		return;
	}

	if (!fCurrentDrawingRegionValid || fWindow->DrawingRegionChanged(fCurrentView)) {
		fWindow->GetEffectiveDrawingRegion(fCurrentView, fCurrentDrawingRegion);
		fCurrentDrawingRegionValid = true;
	}

	if (fCurrentDrawingRegion.CountRects() <= 0) {
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
			DTRACE(("ServerWindow %s: Message AS_STROKE_LINE\n", Title()));

			float x1, y1, x2, y2;

			link.Read<float>(&x1);
			link.Read<float>(&y1);
			link.Read<float>(&x2);
			link.Read<float>(&y2);

			BPoint p1(x1, y1);
			BPoint p2(x2, y2);
			BPoint penPos = p2;
			fCurrentView->ConvertToScreenForDrawing(&p1);
			fCurrentView->ConvertToScreenForDrawing(&p2);
			drawingEngine->StrokeLine(p1, p2);
			
			// We update the pen here because many DrawingEngine calls which do not update the
			// pen position actually call StrokeLine

			// TODO: Decide where to put this, for example, it cannot be done
			// for DrawString(), also there needs to be a decision, if penlocation
			// is in View coordinates (I think it should be) or in screen coordinates.
			fCurrentView->CurrentState()->SetPenLocation(penPos);
			break;
		}
		case AS_LAYER_INVERT_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_INVERT_RECT\n", Title()));
			
			BRect rect;
			link.Read<BRect>(&rect);
			
			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->InvertRect(rect);
			break;
		}
		case AS_STROKE_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_RECT\n", Title()));
			
			BRect rect;
			link.Read<BRect>(&rect);

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->StrokeRect(rect);
			break;
		}
		case AS_FILL_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_RECT\n", Title()));

			BRect rect;
			link.Read<BRect>(&rect);

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->FillRect(rect);
			break;
		}
		case AS_LAYER_DRAW_BITMAP:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP: View name: %s\n", fTitle, fCurrentView->Name()));
			int32 bitmapToken;
			BRect srcRect, dstRect;

			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&dstRect);
			link.Read<BRect>(&srcRect);

			ServerBitmap* bitmap = fServerApp->FindBitmap(bitmapToken);
			if (bitmap) {
				fCurrentView->ConvertToScreenForDrawing(&dstRect);

				drawingEngine->DrawBitmap(bitmap, srcRect, dstRect);
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
			link.Read<float>(&span);

			fCurrentView->ConvertToScreenForDrawing(&r);
			drawingEngine->DrawArc(r, angle, span, code == AS_FILL_ARC);
			break;
		}
		case AS_STROKE_BEZIER:
		case AS_FILL_BEZIER:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_BEZIER\n", Title()));

			BPoint pts[4];
			for (int32 i = 0; i < 4; i++) {
				link.Read<BPoint>(&(pts[i]));
				fCurrentView->ConvertToScreenForDrawing(&pts[i]);
			}

			drawingEngine->DrawBezier(pts, code == AS_FILL_BEZIER);
			break;
		}
		case AS_STROKE_ELLIPSE:
		case AS_FILL_ELLIPSE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_ELLIPSE\n", Title()));

			BRect rect;
			link.Read<BRect>(&rect);

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->DrawEllipse(rect, code == AS_FILL_ELLIPSE);
			break;
		}
		case AS_STROKE_ROUNDRECT:
		case AS_FILL_ROUNDRECT:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_ROUNDRECT\n", Title()));

			BRect rect;
			float xrad,yrad;
			link.Read<BRect>(&rect);
			link.Read<float>(&xrad);
			link.Read<float>(&yrad);

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->DrawRoundRect(rect, xrad, yrad, code == AS_FILL_ROUNDRECT);
			break;
		}
		case AS_STROKE_TRIANGLE:
		case AS_FILL_TRIANGLE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_TRIANGLE\n", Title()));
 
 			BPoint pts[3];
			BRect rect;

 			for (int32 i = 0; i < 3; i++) {
				link.Read<BPoint>(&(pts[i]));
				fCurrentView->ConvertToScreenForDrawing(&pts[i]);
 			}

			link.Read<BRect>(&rect);

			fCurrentView->ConvertToScreenForDrawing(&rect);
			drawingEngine->DrawTriangle(pts, rect, code == AS_FILL_TRIANGLE);
			break;
		}
		case AS_STROKE_POLYGON:
		case AS_FILL_POLYGON:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_POLYGON\n", Title()));

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
		case AS_STROKE_SHAPE:
		case AS_FILL_SHAPE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_SHAPE\n", Title()));

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
				BPoint penLocation = fCurrentView->CurrentState()->PenLocation();
				for (int32 i = 0; i < ptCount; i++) {
					ptList[i] += penLocation;
					fCurrentView->ConvertToScreenForDrawing(&ptList[i]);
				}

				drawingEngine->DrawShape(shapeFrame, opCount, opList, ptCount, ptList,
					code == AS_FILL_SHAPE);
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
		case AS_STROKE_LINEARRAY:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_LINEARRAY\n", Title()));

			// Attached Data:
			// 1) int32 Number of lines in the array
			// 2) LineArrayData

			int32 lineCount;

			link.Read<int32>(&lineCount);
			if (lineCount > 0) {
				LineArrayData lineData[lineCount];

				for (int32 i = 0; i < lineCount; i++) {
					LineArrayData* index = &lineData[i];

					link.Read<float>(&(index->pt1.x));
					link.Read<float>(&(index->pt1.y));
					link.Read<float>(&(index->pt2.x));
					link.Read<float>(&(index->pt2.y));
					link.Read<rgb_color>(&(index->color));

					fCurrentView->ConvertToScreenForDrawing(&index->pt1);
					fCurrentView->ConvertToScreenForDrawing(&index->pt2);
				}
				drawingEngine->StrokeLineArray(lineCount, lineData);
			}
			break;
		}
		case AS_DRAW_STRING:
		case AS_DRAW_STRING_WITH_DELTA:
		{
			DTRACE(("ServerWindow %s: Message AS_DRAW_STRING\n", Title()));
			char* string;
			int32 length;
			BPoint location;
			escapement_delta _delta;
			escapement_delta* delta = NULL;

			link.Read<int32>(&length);
			link.Read<BPoint>(&location);
			if (code == AS_DRAW_STRING_WITH_DELTA) {
				link.Read<escapement_delta>(&_delta);
				if (_delta.nonspace != 0.0 || _delta.space != 0.0)
					delta = &_delta;
			}
			link.ReadString(&string);


			fCurrentView->ConvertToScreenForDrawing(&location);
			BPoint penLocation = drawingEngine->DrawString(string, length,
				location, delta);

			fCurrentView->ConvertFromScreenForDrawing(&penLocation);
			fCurrentView->CurrentState()->SetPenLocation(penLocation);

			free(string);
			break;
		}

		case AS_LAYER_DRAW_PICTURE:
		{
			int32 token;
			if (link.Read<int32>(&token) == B_OK) {
				BPoint where;
				link.Read<BPoint>(&where);
				
				ServerPicture *picture = App()->FindPicture(token);
				if (picture != NULL) {
					fCurrentView->CurrentState()->SetOrigin(where);
					picture->Play(fCurrentView);
				}
			}
			break;
		}

		default:
			BString codeString;
			string_for_message_code(code, codeString);
			printf("ServerWindow %s received unexpected code: %s\n",
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
ServerWindow::_DispatchPictureMessage(int32 code, BPrivate::LinkReceiver &link)
{
	ServerPicture *picture = fCurrentView->Picture();
	if (picture == NULL)
		return false;
	
	switch (code) {
		case AS_LAYER_SET_ORIGIN:
		{
			float x, y;
			link.Read<float>(&x);
			link.Read<float>(&y);
			
			picture->WriteSetOrigin(BPoint(x, y));
			break;
		}

		case AS_LAYER_INVERT_RECT:
		{
			BRect rect;
			link.Read<BRect>(&rect);
			picture->WriteInvertRect(rect);

			break;
		}

		case AS_LAYER_PUSH_STATE:
		{
			picture->WritePushState();
			break;
		}

		case AS_LAYER_POP_STATE:
		{
			picture->WritePopState();
			break;
		}

		case AS_LAYER_SET_DRAWING_MODE:
		{
			int8 drawingMode;
			link.Read<int8>(&drawingMode);
			
			picture->WriteSetDrawingMode((drawing_mode)drawingMode);
			break;
		}
		
		case AS_LAYER_SET_PEN_LOC:
		{
			float x, y;
			link.Read<float>(&x);
			link.Read<float>(&y);
			picture->WriteSetPenLocation(BPoint(x, y));
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			float penSize;
			link.Read<float>(&penSize);
			picture->WriteSetPenSize(penSize);
			break;
		}

		case AS_LAYER_SET_LINE_MODE:
		{
			int8 lineCap, lineJoin;
			float miterLimit;

			link.Read<int8>(&lineCap);
			link.Read<int8>(&lineJoin);
			link.Read<float>(&miterLimit);
			
			picture->WriteSetLineMode((cap_mode)lineCap, (join_mode)lineJoin, miterLimit);
		
			break;
		}
		case AS_LAYER_SET_SCALE:
		{
			float scale;
			link.Read<float>(&scale);
			picture->WriteSetScale(scale);
			break;
		}

		case AS_LAYER_SET_PATTERN:
		{
			pattern pat;
			link.Read(&pat, sizeof(pattern));
			picture->WriteSetPattern(pat);
			break;
		}

		case AS_LAYER_SET_FONT_STATE:
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

			picture->WriteDrawArc(center, radii, startTheta, arcTheta, code == AS_FILL_ARC);
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
			float x1, y1, x2, y2;

			link.Read<float>(&x1);
			link.Read<float>(&y1);
			link.Read<float>(&x2);
			link.Read<float>(&y2);

			picture->WriteStrokeLine(BPoint(x1, y1), BPoint(x2, y2));
			break;
		}
		
		case AS_STROKE_LINEARRAY:
		{
			int32 lineCount;
			link.Read<int32>(&lineCount);
			if (lineCount <= 0)
				break;

			picture->WritePushState();

			for (int32 i = 0; i < lineCount; i++) {
				float x1, y1, x2, y2;
				link.Read<float>(&x1);
				link.Read<float>(&y1);
				link.Read<float>(&x2);
				link.Read<float>(&y2);

				rgb_color color;
				link.Read<rgb_color>(&color);
			
				picture->WriteSetHighColor(color);
				picture->WriteStrokeLine(BPoint(x1, y1), BPoint(x2, y2));
			}
			
			picture->WritePopState();
			break;
		}

		case AS_LAYER_SET_LOW_COLOR:
		case AS_LAYER_SET_HIGH_COLOR:
		{
			rgb_color color;
			link.Read(&color, sizeof(rgb_color));

			if (code == AS_LAYER_SET_HIGH_COLOR)
				picture->WriteSetHighColor(color);
			else
				picture->WriteSetLowColor(color);
			break;
		}
		
		case AS_DRAW_STRING:
		case AS_DRAW_STRING_WITH_DELTA:
		{
			char* string = NULL;
			int32 length;
			BPoint location;
			
			link.Read<int32>(&length);
			link.Read<BPoint>(&location);
			escapement_delta delta = { 0, 0 };
			if (code == AS_DRAW_STRING_WITH_DELTA)
				link.Read<escapement_delta>(&delta);
			link.ReadString(&string);

			picture->WriteDrawString(location, string, length, delta);
			
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

			uint32 *opList = new(nothrow) uint32[opCount];
			BPoint *ptList = new(nothrow) BPoint[ptCount];
			if (opList != NULL && ptList != NULL
				&& link.Read(opList, opCount * sizeof(uint32)) >= B_OK
				&& link.Read(ptList, ptCount * sizeof(BPoint)) >= B_OK) {

				// This might seem a bit weird, but under R5, the shapes
				// are always offset by the current pen location
				BPoint penLocation = fCurrentView->CurrentState()->PenLocation();
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

		case AS_LAYER_DRAW_BITMAP:
		{
			int32 token;
			link.Read<int32>(&token);
			
			BRect destRect;
			link.Read<BRect>(&destRect);
			
			BRect sourceRect;
			link.Read<BRect>(&sourceRect);
			
			ServerBitmap *bitmap = App()->FindBitmap(token);
			if (bitmap == NULL)
				break;

			picture->WriteDrawBitmap(sourceRect, destRect, bitmap->Width(), bitmap->Height(),
						bitmap->BytesPerRow(), bitmap->ColorSpace(), /*bitmap->Flags()*/0,
						bitmap->Bits(), bitmap->BitsLength());

			break;
		}
		
		case AS_LAYER_DRAW_PICTURE:
		{
			int32 token;
			if (link.Read<int32>(&token) == B_OK) {
				BPoint where;
				link.Read<BPoint>(&where);
				
				ServerPicture *pictureToDraw = App()->FindPicture(token);
				if (picture != NULL) {
					// We need to make a copy of the picture, since it can change
					// after it has been drawn
					ServerPicture *copy = App()->CreatePicture(pictureToDraw);
					picture->NestPicture(copy);
					picture->WriteDrawPicture(where, copy->Token());
				}			
			}
			break;
		}

		case AS_LAYER_SET_CLIP_REGION:
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
		case AS_LAYER_BEGIN_PICTURE:
		{
			ServerPicture *newPicture = App()->CreatePicture();
			newPicture->Usurp(picture);			
			newPicture->SyncState(fCurrentView);
			fCurrentView->SetPicture(newPicture);
						
			break;
		}
		
		case AS_LAYER_APPEND_TO_PICTURE:
		{
			int32 pictureToken;
			link.Read<int32>(&pictureToken);
			ServerPicture *appendPicture = App()->FindPicture(pictureToken);
			if (appendPicture) {
				//picture->SyncState(fCurrentView);
				appendPicture->Usurp(picture);			
			}
			fCurrentView->SetPicture(appendPicture);
				// we don't care if it's NULL
			break;
		}

		case AS_LAYER_END_PICTURE:
		{
			ServerPicture *steppedDown = picture->StepDown();
			fCurrentView->SetPicture(steppedDown);
			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(picture->Token());
			fLink.Flush();
			return true;
		}
/*
		case AS_LAYER_SET_BLENDING_MODE:
		{
			int8 srcAlpha, alphaFunc;
			
			link.Read<int8>(&srcAlpha);
			link.Read<int8>(&alphaFunc);
			
			picture->BeginOp(B_PIC_SET_BLENDING_MODE);
			picture->AddInt16((int16)srcAlpha);
			picture->AddInt16((int16)alphaFunc);
			picture->EndOp();
			
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


/*!
	\brief Message-dispatching loop for the ServerWindow

	Watches the ServerWindow's message port and dispatches as necessary
*/
void
ServerWindow::_MessageLooper()
{
	BPrivate::LinkReceiver& receiver = fLink.Receiver();
	bool quitLoop = false;

	while (!quitLoop) {
		//STRACE(("info: ServerWindow::MonitorWin listening on port %ld.\n",
		//	fMessagePort));

		int32 code;
		status_t status = receiver.GetNextMessage(code);
		if (status < B_OK) {
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
		if (diff > 10000)
			printf("ServerWindow %s: lock acquisition took %Ld usecs\n", Title(), diff);
#endif

		int32 messagesProcessed = 0;
		bool lockedDesktop = false;
		bool needsAllWindowsLocked = false;

		while (true) {
			if (code == AS_DELETE_WINDOW || code == kMsgQuitLooper) {
				// this means the client has been killed
				STRACE(("ServerWindow %s received 'AS_DELETE_WINDOW' message code\n",
					Title()));

				if (code == AS_DELETE_WINDOW) {
					fLink.StartMessage(B_OK);
					fLink.Flush();
				}

				if (lockedDesktop)
					fDesktop->UnlockSingleWindow();

				quitLoop = true;

				// ServerWindow's destructor takes care of pulling this object off the desktop.
				if (!fWindow->IsHidden())
					CRITICAL("ServerWindow: a window must be hidden before it's deleted\n");
				
				break;
			}

			needsAllWindowsLocked = _MessageNeedsAllWindowsLocked(code);

			if (!lockedDesktop && !needsAllWindowsLocked) {
				// only lock it once
				fDesktop->LockSingleWindow();
				lockedDesktop = true;
			} else if (lockedDesktop && !needsAllWindowsLocked) {
				// nothing to do
			} else if (needsAllWindowsLocked) {
				if (lockedDesktop) {
					// unlock single before locking all
					fDesktop->UnlockSingleWindow();
					lockedDesktop = false;
				}
				fDesktop->LockAllWindows();
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
				if (diff > 10000)
					printf("ServerWindow %s: message %ld took %Ld usecs\n", Title(), code, diff);
			}
#endif

			if (needsAllWindowsLocked)
				fDesktop->UnlockAllWindows();

			// only process up to 70 waiting messages at once (we have the Desktop locked)
			if (!receiver.HasMessages() || ++messagesProcessed > 70) {
				if (lockedDesktop)
					fDesktop->UnlockSingleWindow();
				break;
			}

			// next message
			status_t status = receiver.GetNextMessage(code);
			if (status < B_OK) {
				// that shouldn't happen, it's our port
				printf("Someone deleted our message port!\n");
				if (lockedDesktop)
					fDesktop->UnlockSingleWindow();

				// try to let our client die happily
				NotifyQuitRequested();
				break;
			}
		}

		Unlock();
	}

	// we were asked to quit the message loop - either on request or because of an error
	Quit();
		// does not return
}


status_t
ServerWindow::SendMessageToClient(const BMessage* msg, int32 target) const
{
	if (target == B_NULL_TOKEN)
		target = fClientToken;

	BMessenger reply;
	BMessage::Private messagePrivate((BMessage *)msg);
	return messagePrivate.SendMessage(fClientLooperPort, fClientTeam, target,
		0, false, reply);
}


Window*
ServerWindow::MakeWindow(BRect frame, const char* name,
	window_look look, window_feel feel, uint32 flags, uint32 workspace)
{
	// The non-offscreen ServerWindow uses the DrawingEngine instance from
	// the desktop.
	return new (nothrow) ::Window(frame, name, look, feel, flags,
		workspace, this, new DrawingEngine(fDesktop->HWInterface()));
}


status_t
ServerWindow::_EnableDirectWindowMode()
{
	if (fDirectWindowData != NULL) {
		// already in direct window mode
		return B_ERROR;
	}

	fDirectWindowData = new (nothrow) direct_window_data;
	if (fDirectWindowData == NULL)
		return B_NO_MEMORY;

	status_t status = fDirectWindowData->InitCheck();
	if (status < B_OK) {
		delete fDirectWindowData;
		fDirectWindowData = NULL;

		return status;
	}

	return B_OK;
}


void
ServerWindow::HandleDirectConnection(int32 bufferState, int32 driverState)
{
	STRACE(("HandleDirectConnection(bufferState = %ld, driverState = %ld)\n",
		bufferState, driverState));

	if (fDirectWindowData == NULL
		|| (!fDirectWindowData->started
			&& (bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_START))
		return;
	
	// Don't issue a DirectConnected() notification
	// if the connection is stopped, and we are called
	// with bufferState == B_DIRECT_MODIFY.
	if ((fDirectWindowData->buffer_info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP
		&& (bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_START) {
		return;		
	}

	fDirectWindowData->started = true;

	if (bufferState != -1)
		fDirectWindowData->buffer_info->buffer_state = (direct_buffer_state)bufferState;
		
	if (driverState != -1)
		fDirectWindowData->buffer_info->driver_state = (direct_driver_state)driverState;

	if ((bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_STOP) {
		// TODO: Locking ?
		RenderingBuffer *buffer = fDesktop->HWInterface()->FrontBuffer();
		fDirectWindowData->buffer_info->bits = buffer->Bits();
		fDirectWindowData->buffer_info->pci_bits = NULL; // TODO	
		fDirectWindowData->buffer_info->bytes_per_row = buffer->BytesPerRow();
		switch (buffer->ColorSpace()) {
			case B_RGB32:
			case B_RGBA32:
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
				fDirectWindowData->buffer_info->bits_per_pixel = 32;
				break;
			case B_RGB24:
			case B_RGB24_BIG:
				fDirectWindowData->buffer_info->bits_per_pixel = 24;
				break;
			case B_RGB16:
			case B_RGB16_BIG:
			case B_RGB15:
			case B_RGB15_BIG:
				fDirectWindowData->buffer_info->bits_per_pixel = 16;
				break;
			case B_CMAP8:
				fDirectWindowData->buffer_info->bits_per_pixel = 8;
				break;
			default:
				fprintf(stderr, "unkown colorspace in HandleDirectConnection()!\n");
				fDirectWindowData->buffer_info->bits_per_pixel = 0;
				break;
		}
		fDirectWindowData->buffer_info->pixel_format = buffer->ColorSpace();
		fDirectWindowData->buffer_info->layout = B_BUFFER_NONINTERLEAVED;
		fDirectWindowData->buffer_info->orientation = B_BUFFER_TOP_TO_BOTTOM; // TODO
		fDirectWindowData->buffer_info->window_bounds = to_clipping_rect(fWindow->Frame());

		// TODO: Review this
		const int32 kMaxClipRectsCount = (B_PAGE_SIZE - sizeof(direct_buffer_info))
			/ sizeof(clipping_rect);

		// We just want the region inside the window, border excluded.
		BRegion clipRegion = fWindow->VisibleContentRegion();

		fDirectWindowData->buffer_info->clip_list_count = min_c(clipRegion.CountRects(),
			kMaxClipRectsCount);
		fDirectWindowData->buffer_info->clip_bounds = clipRegion.FrameInt();

		for (uint32 i = 0; i < fDirectWindowData->buffer_info->clip_list_count; i++)
			fDirectWindowData->buffer_info->clip_list[i] = clipRegion.RectAtInt(i);
	}

	// Releasing this sem causes the client to call BDirectWindow::DirectConnected()
	release_sem(fDirectWindowData->sem);

	// TODO: Waiting half a second in the ServerWindow thread is not a problem,
	// but since we are called from the Desktop's thread too, very bad things could happen.
	// Find some way to call this method only within ServerWindow's thread (messaging ?)
	status_t status;
	do {
		// TODO: The timeout is 3000000 usecs (3 seconds) on beos.
		// Test, but I think half a second is enough.
		status = acquire_sem_etc(fDirectWindowData->sem_ack, 1, B_TIMEOUT, 500000);
	} while (status == B_INTERRUPTED);

	if (status < B_OK) {
		// The client application didn't release the semaphore
		// within the given timeout. Or something else went wrong.
		// Deleting this member should make it crash.
		delete fDirectWindowData;
		fDirectWindowData = NULL;
	}
}


void
ServerWindow::_SetCurrentView(View* layer)
{
	if (fCurrentView == layer)
		return;

	fCurrentView = layer;
	fCurrentDrawingRegionValid = false;
	_UpdateDrawState(fCurrentView);

#if 0
#if DELAYED_BACKGROUND_CLEARING
	if (fCurrentView && fCurrentView->IsBackgroundDirty()
		&& fWindow->InUpdate()) {
		DrawingEngine* drawingEngine = fWindow->GetDrawingEngine();
		if (drawingEngine->LockParallelAccess()) {
	
			fWindow->GetEffectiveDrawingRegion(fCurrentView, fCurrentDrawingRegion);
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
ServerWindow::_UpdateDrawState(View* layer)
{
	// switch the drawing state
	// TODO: is it possible to scroll a view while it
	// is being drawn? probably not... otherwise the
	// "offsets" passed below would need to be updated again
	DrawingEngine* drawingEngine = fWindow->GetDrawingEngine();
	if (layer && drawingEngine) {
		BPoint p(0, 0);
		layer->ConvertToScreenForDrawing(&p);
		drawingEngine->SetDrawState(layer->CurrentState(), p.x, p.y);
	}
}


bool
ServerWindow::_MessageNeedsAllWindowsLocked(uint32 code) const
{
	switch (code) {
		case AS_ACTIVATE_WINDOW:
		case AS_SET_WINDOW_TITLE:
		case AS_ADD_TO_SUBSET:
		case AS_REMOVE_FROM_SUBSET:
		case AS_LAYER_CREATE_ROOT:
		case AS_LAYER_CREATE:
		case AS_SEND_BEHIND:
		case AS_SET_LOOK:
		case AS_SET_FEEL:
		case AS_SET_FLAGS:
		case AS_SET_WORKSPACES:
		case AS_WINDOW_MOVE:
		case AS_WINDOW_RESIZE:
		case AS_SET_SIZE_LIMITS:
		case AS_SET_DECORATOR_SETTINGS:
		case AS_GET_MOUSE:
		case AS_DIRECT_WINDOW_SET_FULLSCREEN:
//		case AS_LAYER_SET_EVENT_MASK:
//		case AS_LAYER_SET_MOUSE_EVENT_MASK:
			return true;
		default:
			return false;
	}
}


status_t
ServerWindow::PictureToRegion(ServerPicture *picture, BRegion &region,
	bool inverse, BPoint where)
{
	fprintf(stderr, "ServerWindow::PictureToRegion() not implemented\n");
	region.MakeEmpty();
	return B_ERROR;
}
