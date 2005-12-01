/*
 * Copyright 2001-2005, Haiku.
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
	coordinating and linking a window's WindowLayer half with its messaging half, dispatching 
	mouse and key events from the server to its window, and other such things.
*/

#include <new>

#include <AppDefs.h>
#include <DirectWindow.h>
#include <GraphicsDefs.h>
#include <Message.h>
#include <PortLink.h>
#include <Rect.h>
#include <View.h>
#include <ViewAux.h>
#include <Autolock.h>
#include <TokenSpace.h>
#include <WindowInfo.h>
#include <WindowPrivate.h>

#include <MessagePrivate.h>

#include "AppServer.h"
#include "BGet++.h"
#include "DebugInfoManager.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "Layer.h"
#include "RAMLinkMsgReader.h"
#include "RenderingBuffer.h"
#include "RootLayer.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerProtocol.h"
#include "WindowLayer.h"
#include "Workspace.h"
#include "WorkspacesLayer.h"

#include "ServerWindow.h"

#include "clipping.h"

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
static struct profile { int32 count; bigtime_t time; } sMessageProfile[AS_LAST_CODE];
#endif

struct dw_data {
	sem_id direct_sem;
	sem_id direct_sem_ack;
	area_id direct_area;
	direct_buffer_info *direct_info;
	
	dw_data() :
		direct_sem(-1),
		direct_sem_ack(-1),
		direct_area(-1),
		direct_info(NULL)
	{
		direct_area = create_area("direct area", (void **)&direct_info,
							B_ANY_ADDRESS, B_PAGE_SIZE, B_NO_LOCK, B_READ_WRITE);
	
		direct_sem = create_sem(1, "direct sem");
		direct_sem_ack = create_sem(1, "direct sem ack");
	}
	
	~dw_data()
	{
		if (direct_area >= 0)
			delete_area(direct_area);
		if (direct_sem >= 0)
			delete_sem(direct_sem);
		if (direct_sem_ack >= 0)
			delete_sem(direct_sem_ack);
	}
	
	bool IsValid() const 
	{
		return (direct_area >= 0) && (direct_sem >= 0)
			&& (direct_sem_ack >= 0) && (direct_info != NULL);
	}
};


// TODO: Copied from DirectWindow.cpp: Move to a common header
struct dw_sync_data {
	area_id area;
	sem_id disableSem;
	sem_id disableSemAck;
};


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
	fWindowLayer(NULL),
	fClientTeam(app->ClientTeam()),
	fMessagePort(-1),
	fClientReplyPort(clientPort),
	fClientLooperPort(looperPort),
	fClientViewsWithInvalidCoords(B_VIEW_RESIZED),
	fClientToken(clientToken),
	fCurrentLayer(NULL),
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


//! Tears down all connections the main app_server objects, and deletes some internals.
ServerWindow::~ServerWindow()
{
	STRACE(("ServerWindow(%s@%p):~ServerWindow()\n", fTitle, this));

	if (!fWindowLayer->IsOffscreenWindow())
		fDesktop->RemoveWindow(fWindowLayer);

	delete fWindowLayer;

	free(fTitle);
	delete_port(fMessagePort);

	BPrivate::gDefaultTokens.RemoveToken(fServerToken);

	delete fDirectWindowData;
	STRACE(("ServerWindow(%p) will exit NOW\n", this));

	delete_sem(fDeathSemaphore);

#ifdef PROFILE_MESSAGE_LOOP
	for (int32 i = 0; i < AS_LAST_CODE; i++) {
		if (sMessageProfile[i].count == 0)
			continue;
		printf("[%ld] called %ld times, %g secs (%Ld usecs per call)\n",
			i, sMessageProfile[i].count, sMessageProfile[i].time / 1000000.0,
			sMessageProfile[i].time / sMessageProfile[i].count);
	}
#endif
}


status_t
ServerWindow::Init(BRect frame, uint32 look, uint32 feel, uint32 flags, uint32 workspace)
{
	if (fTitle == NULL)
		return B_NO_MEMORY;

	// fMessagePort is the port to which the app sends messages for the server
	fMessagePort = create_port(100, fTitle);
	if (fMessagePort < B_OK)
		return fMessagePort;

	fLink.SetSenderPort(fClientReplyPort);
	fLink.SetReceiverPort(fMessagePort);

	// We cannot call MakeWindowLayer in the constructor, since it 
	fWindowLayer = MakeWindowLayer(frame, fTitle, look, feel, flags, workspace);
	if (!fWindowLayer)
		return B_NO_MEMORY;

	if (!fWindowLayer->IsOffscreenWindow())
		fDesktop->AddWindow(fWindowLayer);

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

	float minWidth, maxWidth, minHeight, maxHeight;
	fWindowLayer->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	fLink.Attach<BRect>(fWindowLayer->Frame());
	fLink.Attach<float>(minWidth);
	fLink.Attach<float>(maxWidth);
	fLink.Attach<float>(minHeight);
	fLink.Attach<float>(maxHeight);
	fLink.Flush();

	return true;
}


void
ServerWindow::_PrepareQuit()
{
	if (fThread == find_thread(NULL)) {
		// make sure we're hidden
		Hide();

		App()->RemoveWindow(this);
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
	fWindowLayer->UpdateDecorator();
}


//! Shows the window's WindowLayer
void
ServerWindow::Show()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Show\n", Title()));

	if (fQuitting || !fWindowLayer->IsHidden())
		return;

	// TODO: that check is just there for offscreen-windows - this should be made better
	if (fWindowLayer->GetRootLayer() != NULL)
		fDesktop->ShowWindow(fWindowLayer);

	if (fDirectWindowData != NULL)
		HandleDirectConnection(B_DIRECT_START | B_BUFFER_RESET);
}


//! Hides the window's WindowLayer
void
ServerWindow::Hide()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Hide\n", Title()));

	if (fWindowLayer->IsHidden())
		return;

	if (fDirectWindowData != NULL)
		HandleDirectConnection(B_DIRECT_STOP);

	// TODO: that check is just there for offscreen-windows - this should be made better
	if (fWindowLayer->GetRootLayer() != NULL)
		fDesktop->HideWindow(fWindowLayer);
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

	if (fWindowLayer != NULL)
		fWindowLayer->SetName(newTitle);
}


//! Requests that the ServerWindow's BWindow quit
void
ServerWindow::NotifyQuitRequested()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Quit\n", fTitle));

	BMessage msg(B_QUIT_REQUESTED);
	SendMessageToClient(&msg);
}


void
ServerWindow::NotifyMinimize(bool minimize)
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	// This function doesn't need much -- check to make sure that we should and
	// send the message to the client. According to the BeBook, the BWindow hook function
	// does all the heavy lifting for us. :)
	bool sendMessages = false;

	if (minimize) {
		if (!fWindowLayer->IsHidden()) {
			Hide();
			sendMessages = true;
		}
	} else {
		if (fWindowLayer->IsHidden()) {
			Show();
			sendMessages = true;
		}
	}

	if (sendMessages) {
		BMessage msg(B_MINIMIZE);
		msg.AddInt64("when", real_time_clock_usecs());
		msg.AddBool("minimize", minimize);

		SendMessageToClient(&msg);
	}
}

//! Sends a message to the client to perform a Zoom
void
ServerWindow::NotifyZoom()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
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
	info.workspaces = fWindowLayer->Workspaces();

	info.layer = 0; // ToDo: what is this???
	info.feel = fWindowLayer->Feel();
	info.flags = fWindowLayer->WindowFlags();
	info.window_left = (int)floor(fWindowLayer->Frame().left);
	info.window_top = (int)floor(fWindowLayer->Frame().top);
	info.window_right = (int)floor(fWindowLayer->Frame().right);
	info.window_bottom = (int)floor(fWindowLayer->Frame().bottom);

	info.show_hide_level = fWindowLayer->IsHidden() ? 1 : -1; // ???
	info.is_mini = fWindowLayer->IsHidden();
}


/*!
	\brief Sets the font state for a layer
	\param layer The layer to set the font
*/
inline void
ServerWindow::SetLayerFontState(Layer *layer, BPrivate::LinkReceiver &link)
{
	STRACE(("ServerWindow %s: SetLayerFontStateMessage for layer %s\n",
			fTitle, layer->Name()));
	// NOTE: no need to check for a lock. This is a private method.

	layer->CurrentState()->ReadFontFromLink(link);
}


inline void
ServerWindow::SetLayerState(Layer *layer, BPrivate::LinkReceiver &link)
{
	STRACE(("ServerWindow %s: SetLayerState for layer %s\n", Title(),
			 layer->Name()));
	// NOTE: no need to check for a lock. This is a private method.

	layer->CurrentState()->ReadFromLink(link);
	// TODO: Rebuild clipping here!
}


Layer*
ServerWindow::CreateLayerTree(BPrivate::LinkReceiver &link, Layer **_parent)
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
	char *name = NULL;
	rgb_color viewColor;

	link.Read<int32>(&token);
	link.ReadString(&name);
	link.Read<BRect>(&frame);
	link.Read<uint32>(&resizeMask);
	link.Read<uint32>(&eventMask);
	link.Read<uint32>(&eventOptions);
	link.Read<uint32>(&flags);
	link.Read<bool>(&hidden);
	link.Read<rgb_color>(&viewColor);
	link.Read<int32>(&parentToken);

	STRACE(("ServerWindow(%s)::CreateLayerTree()-> layer %s, token %ld\n",
		fTitle, name, token));

	Layer *newLayer;

	if (link.Code() == AS_LAYER_CREATE_ROOT
		&& (fWindowLayer->WindowFlags() & kWorkspacesWindowFlag) != 0) {
		// this is a workspaces window!
		newLayer = new (nothrow) WorkspacesLayer(frame, name, token, resizeMask,
			flags, fWindowLayer->GetDrawingEngine());
	} else {
		newLayer = new (nothrow) Layer(frame, name, token, resizeMask, flags,
			fWindowLayer->GetDrawingEngine());
	}

	if (newLayer == NULL)
		return NULL;

	free(name);

	// there is no way of setting this, other than manually :-)
	newLayer->SetViewColor(viewColor);
	newLayer->fHidden = hidden;
	newLayer->fEventMask = eventMask;
	newLayer->fEventOptions = eventOptions;

	DesktopSettings settings(fDesktop);
	ServerFont font;
	settings.GetDefaultPlainFont(font);
	newLayer->CurrentState()->SetFont(font);

// TODO: rework the clipping stuff to remove RootLayer dependency and then
// remove this hack:
if (fWindowLayer->IsOffscreenWindow()) {
	newLayer->fVisible.Set(newLayer->fFrame);
	newLayer->fDrawingRegion.Set(newLayer->fFrame);
}

	if (_parent) {
		Layer *parent;
		if (App()->ViewTokens().GetToken(parentToken, B_HANDLER_TOKEN,
				(void**)&parent) != B_OK
			|| parent->Window() != this) {
			CRITICAL("View token not found!\n");
			parent = NULL;
		}

		*_parent = parent;
	}

	return newLayer;
}


void
ServerWindow::_DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
	// TODO: when creating a Layer, check for yet non-existing Layer::InitCheck()
	// and take appropriate actions, then checking for fCurrentLayer->fLayerData
	// is unnecessary
	if ((fCurrentLayer == NULL || fCurrentLayer->CurrentState() == NULL) &&
		code != AS_LAYER_CREATE_ROOT && code != AS_LAYER_CREATE) {

		printf("ServerWindow %s received unexpected code - message offset %ld before top_view attached.\n", Title(), code - B_OK);
		return;
	}

	RootLayer *rootLayer = fWindowLayer->GetRootLayer();
	// NOTE: is NULL when fWindowLayer is offscreen!
	if (rootLayer) {
#ifdef PROFILE_MESSAGE_LOOP
		bigtime_t start = system_time();
#endif
		rootLayer->Lock();
#ifdef PROFILE_MESSAGE_LOOP
		bigtime_t diff = system_time() - start;
		if (diff > 10000)
			printf("ServerWindow %s: root lock acquisition took %Ld usecs\n", Title(), diff);
#endif
	}

	switch (code) {
		//--------- BView Messages -----------------
		case AS_LAYER_SCROLL:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SCROLL: Layer name: %s\n", fTitle, fCurrentLayer->Name()));
			float dh;
			float dv;

			link.Read<float>(&dh);
			link.Read<float>(&dv);
			fCurrentLayer->ScrollBy(dh, dv);

			break;
		}
		case AS_LAYER_COPY_BITS:
		{
			BRect src;
			BRect dst;

			link.Read<BRect>(&src);
			link.Read<BRect>(&dst);

			// TODO: confirm that in R5 this call is affected by origin and scale
//	fCurrentLayer->ConvertToScreenForDrawing(&src);
//	fCurrentLayer->ConvertToScreenForDrawing(&dst);
			fCurrentLayer->ConvertToScreen(&src);
			fCurrentLayer->ConvertToScreen(&dst);
		
			int32 xOffset = (int32)(dst.left - src.left);
			int32 yOffset = (int32)(dst.top - src.top);

			fCurrentLayer->CopyBits(src, dst, xOffset, yOffset);
			break;
		}
		case AS_SET_CURRENT_LAYER:
		{
			int32 token;
			
			link.Read<int32>(&token);

			Layer *current;
			if (App()->ViewTokens().GetToken(token, B_HANDLER_TOKEN,
					(void**)&current) != B_OK
				|| current->Window() != this) {
				// ToDo: if this happens, we probably want to kill the app and clean up
				DTRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: layer not found, token %ld\n", fTitle, token));
				current = NULL;
			} else {
				DTRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: %s, token %ld\n", fTitle, current->Name(), token));
				fCurrentLayer = current;
			}
			break;
		}

		case AS_LAYER_CREATE_ROOT:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE_ROOT\n", fTitle));

			// Start receiving top_view data -- pass NULL as the parent view.
			// This should be the *only* place where this happens.
			if (fCurrentLayer != NULL)
				break;

			fWindowLayer->SetTopLayer(CreateLayerTree(link, NULL));
			fCurrentLayer = fWindowLayer->TopLayer();
			break;
		}

		case AS_LAYER_CREATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE: Layer name: %s\n", fTitle, fCurrentLayer->Name()));

			Layer* parent = NULL;
			Layer* newLayer = CreateLayerTree(link, &parent);
			if (parent != NULL)
				parent->AddChild(newLayer, this);

			if (rootLayer && !newLayer->IsHidden() && parent) {
				BRegion invalidRegion;
				newLayer->GetOnScreenRegion(invalidRegion);
				parent->MarkForRebuild(invalidRegion);
				parent->TriggerRebuild();
				if (newLayer->VisibleRegion().Frame().IsValid()) {
					rootLayer->MarkForRedraw(newLayer->VisibleRegion());
					rootLayer->TriggerRedraw();
				}
			}
			break;
		}
		case AS_LAYER_DELETE:
		{
			// Received when a view is detached from a window. This is definitely
			// the less taxing operation - we call PruneTree() on the removed
			// layer, detach the layer itself, delete it, and invalidate the
			// area assuming that the view was visible when removed

			Layer *parent = fCurrentLayer->fParent;

			STRACE(("ServerWindow %s: AS_LAYER_DELETE view: %p, parent: %p\n", fTitle,
				fCurrentLayer, parent));

			if (!fCurrentLayer->IsHidden() && parent && rootLayer
				&& fCurrentLayer->FullVisible().Frame().IsValid()) {
				parent->MarkForRebuild(fCurrentLayer->FullVisible());
				rootLayer->MarkForRedraw(fCurrentLayer->FullVisible());
			}

			// here we remove current layer from list.
			fCurrentLayer->RemoveSelf();

			if (parent)
				parent->TriggerRebuild();			

			if (rootLayer) {
				rootLayer->LayerRemoved(fCurrentLayer);
				if (!fCurrentLayer->IsHidden())
					rootLayer->TriggerRedraw();
			}

			if (fCurrentLayer->EventMask() != 0) {
				fDesktop->EventDispatcher().RemoveListener(EventTarget(),
					fCurrentLayer->ViewToken());
			}

			#ifdef DEBUG_SERVERWINDOW
			parent->PrintTree();
			#endif

			delete fCurrentLayer;
			// TODO: It is necessary to do this, but I find it very obscure.
			fCurrentLayer = parent;
			break;
		}
		case AS_LAYER_SET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_STATE: Layer name: %s\n", fTitle, fCurrentLayer->Name()));
			SetLayerState(fCurrentLayer, link);
			// TODO: should this be moved into SetLayerState?
			// If it _always_ needs to be done afterwards, then yes!
			break;
		}
		case AS_LAYER_SET_FONT_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer name: %s\n", fTitle, fCurrentLayer->Name()));
			SetLayerFontState(fCurrentLayer, link);
			break;
		}
		case AS_LAYER_GET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_STATE: Layer name: %s\n", fTitle, fCurrentLayer->Name()));

			fLink.StartMessage(B_OK);

			// attach state data
			fCurrentLayer->CurrentState()->WriteToLink(fLink.Sender());
			fLink.Flush();
			break;
		}
		case AS_LAYER_SET_EVENT_MASK:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_MOUSE_EVENT_MASK: Layer name: %s\n", fTitle, fCurrentLayer->Name()));			
			rootLayer->Unlock();
				// we don't want RootLayer to be locked while we play with the events

			uint32 eventMask, options;

			link.Read<uint32>(&eventMask);
			if (link.Read<uint32>(&options) == B_OK) {
				fCurrentLayer->SetEventMask(eventMask, options);

				if (eventMask != 0 || options != 0) {
					fDesktop->EventDispatcher().AddListener(EventTarget(),
						fCurrentLayer->ViewToken(), eventMask, options);
				} else {
					fDesktop->EventDispatcher().RemoveListener(EventTarget(),
						fCurrentLayer->ViewToken());
				}
			}

			rootLayer->Lock();
			break;
		}
		case AS_LAYER_SET_MOUSE_EVENT_MASK:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_MOUSE_EVENT_MASK: Layer name: %s\n", fTitle, fCurrentLayer->Name()));			
			rootLayer->Unlock();
				// we don't want RootLayer to be locked while we play with the events

			uint32 eventMask, options;

			link.Read<uint32>(&eventMask);
			if (link.Read<uint32>(&options) == B_OK) {
				if (eventMask != 0 || options != 0) {
					fDesktop->EventDispatcher().AddTemporaryListener(EventTarget(),
						fCurrentLayer->ViewToken(), eventMask, options);
				} else {
					fDesktop->EventDispatcher().RemoveTemporaryListener(EventTarget(),
						fCurrentLayer->ViewToken());
				}
			}

			// TODO: support B_LOCK_WINDOW_FOCUS option in RootLayer
			rootLayer->Lock();
			break;
		}
		case AS_LAYER_MOVE_TO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_MOVE_TO: Layer name: %s\n",
				fTitle, fCurrentLayer->Name()));

			float x, y;
			link.Read<float>(&x);
			link.Read<float>(&y);

			float offsetX = x - fCurrentLayer->fFrame.left;
			float offsetY = y - fCurrentLayer->fFrame.top;

			fCurrentLayer->MoveBy(offsetX, offsetY);
			break;
		}
		case AS_LAYER_RESIZE_TO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_TO: Layer name: %s\n",
				fTitle, fCurrentLayer->Name()));

			float newWidth, newHeight;
			link.Read<float>(&newWidth);
			link.Read<float>(&newHeight);
			
			// TODO: If fCurrentLayer is a window, check for minimum size allowed.
			// Need WindowLayer::GetSizeLimits
			float deltaWidth = newWidth - fCurrentLayer->fFrame.Width();
			float deltaHeight = newHeight - fCurrentLayer->fFrame.Height();

			fCurrentLayer->ResizeBy(deltaWidth, deltaHeight);
			break;
		}
		case AS_LAYER_GET_COORD:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(B_OK);
			// our offset in the parent -> will be originX and originY in BView
			fLink.Attach<float>(fCurrentLayer->fFrame.left);
			fLink.Attach<float>(fCurrentLayer->fFrame.top);
			fLink.Attach<BRect>(fCurrentLayer->Bounds());
			fLink.Flush();
			break;
		}
		case AS_LAYER_SET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_ORIGIN: Layer: %s\n", Title(), fCurrentLayer->Name()));
			float x, y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);
			
			fCurrentLayer->SetDrawingOrigin(BPoint(x, y));
			break;
		}
		case AS_LAYER_GET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_ORIGIN: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(B_OK);
			// TODO: rename this where it is used in the BView code!
			// (it wants to know scrolling offset, not drawing origin)
			fLink.Attach<BPoint>(fCurrentLayer->ScrollingOffset());
			fLink.Flush();
			break;
		}
		case AS_LAYER_RESIZE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_MODE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			link.Read<uint32>(&(fCurrentLayer->fResizeMode));
			break;
		}
		case AS_LAYER_CURSOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_CURSOR: Layer: %s - NOT IMPLEMENTED\n", Title(), fCurrentLayer->Name()));
			int32 token;

			link.Read<int32>(&token);

			// TODO: implement; I think each Layer should have a member pointing
			// to this requested cursor.
			
			break;
		}
		case AS_LAYER_SET_FLAGS:
		{
			uint32 flags;
			link.Read<uint32>(&flags);
			fCurrentLayer->SetFlags(flags);
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FLAGS: Layer: %s\n", Title(), fCurrentLayer->Name()));
			break;
		}
		case AS_LAYER_HIDE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_HIDE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fCurrentLayer->Hide();
			break;
		}
		case AS_LAYER_SHOW:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SHOW: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fCurrentLayer->Show();
			break;
		}
		case AS_LAYER_SET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_LINE_MODE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			int8 lineCap, lineJoin;
			float miterLimit;

			// TODO: Look into locking scheme relating to Layers and modifying redraw-related members

			link.Read<int8>(&lineCap);
			link.Read<int8>(&lineJoin);
			link.Read<float>(&miterLimit);
			
			fCurrentLayer->CurrentState()->SetLineCapMode((cap_mode)lineCap);
			fCurrentLayer->CurrentState()->SetLineJoinMode((join_mode)lineJoin);
			fCurrentLayer->CurrentState()->SetMiterLimit(miterLimit);
		
			break;
		}
		case AS_LAYER_GET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_LINE_MODE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<int8>((int8)(fCurrentLayer->CurrentState()->LineCapMode()));
			fLink.Attach<int8>((int8)(fCurrentLayer->CurrentState()->LineJoinMode()));
			fLink.Attach<float>(fCurrentLayer->CurrentState()->MiterLimit());
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_PUSH_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_PUSH_STATE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			fCurrentLayer->PushState();

			break;
		}
		case AS_LAYER_POP_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_POP_STATE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			fCurrentLayer->PopState();

			break;
		}
		case AS_LAYER_SET_SCALE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			float scale;
			link.Read<float>(&scale);

			fCurrentLayer->SetScale(scale);
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_SCALE: Layer: %s\n", Title(), fCurrentLayer->Name()));		

			fLink.StartMessage(B_OK);
			fLink.Attach<float>(fCurrentLayer->CurrentState()->Scale());
			fLink.Flush();
			break;
		}
		case AS_LAYER_SET_PEN_LOC:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_LOC: Layer: %s\n", Title(), fCurrentLayer->Name()));
			float x, y;

			link.Read<float>(&x);
			link.Read<float>(&y);

			fCurrentLayer->CurrentState()->SetPenLocation(BPoint(x, y));
			break;
		}
		case AS_LAYER_GET_PEN_LOC:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_LOC: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(fCurrentLayer->CurrentState()->PenLocation());
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_SIZE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			float penSize;
			link.Read<float>(&penSize);
			fCurrentLayer->CurrentState()->SetPenSize(penSize);
		
			break;
		}
		case AS_LAYER_GET_PEN_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_SIZE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<float>(fCurrentLayer->CurrentState()->PenSize());
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: Layer: %s\n", Title(), fCurrentLayer->Name()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));

			fCurrentLayer->SetViewColor(RGBColor(c));

			if (rootLayer && !fCurrentLayer->IsHidden()) {
				rootLayer->MarkForRedraw(fCurrentLayer->VisibleRegion());
				rootLayer->TriggerRedraw();
			}
			break;
		}

		case AS_LAYER_GET_HIGH_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_HIGH_COLOR: Layer: %s\n",
				Title(), fCurrentLayer->Name()));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(fCurrentLayer->CurrentState()->HighColor().GetColor32());
			fLink.Flush();
			break;

		case AS_LAYER_GET_LOW_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_LOW_COLOR: Layer: %s\n",
				Title(), fCurrentLayer->Name()));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(fCurrentLayer->CurrentState()->LowColor().GetColor32());
			fLink.Flush();
			break;

		case AS_LAYER_GET_VIEW_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_VIEW_COLOR: Layer: %s\n",
				Title(), fCurrentLayer->Name()));

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(fCurrentLayer->ViewColor().GetColor32());
			fLink.Flush();
			break;

		case AS_LAYER_SET_BLEND_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_BLEND_MODE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			int8 srcAlpha, alphaFunc;
			
			link.Read<int8>(&srcAlpha);
			link.Read<int8>(&alphaFunc);
			
			fCurrentLayer->CurrentState()->SetBlendingMode((source_alpha)srcAlpha,
											(alpha_function)alphaFunc);

			break;
		}
		case AS_LAYER_GET_BLEND_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_BLEND_MODE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<int8>((int8)(fCurrentLayer->CurrentState()->AlphaSrcMode()));
			fLink.Attach<int8>((int8)(fCurrentLayer->CurrentState()->AlphaFncMode()));
			fLink.Flush();

			break;
		}
		case AS_LAYER_SET_DRAW_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_DRAW_MODE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			int8 drawingMode;
			
			link.Read<int8>(&drawingMode);
			
			fCurrentLayer->CurrentState()->SetDrawingMode((drawing_mode)drawingMode);
			
			break;
		}
		case AS_LAYER_GET_DRAW_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_DRAW_MODE: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(B_OK);
			fLink.Attach<int8>((int8)(fCurrentLayer->CurrentState()->GetDrawingMode()));
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_PRINT_ALIASING:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_PRINT_ALIASING: Layer: %s\n", Title(), fCurrentLayer->Name()));
			bool fontAliasing;
			link.Read<bool>(&fontAliasing);
			fCurrentLayer->CurrentState()->SetForceFontAliasing(fontAliasing);
			
			break;
		}
		case AS_LAYER_CLIP_TO_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_PICTURE: Layer: %s\n", Title(), fCurrentLayer->Name()));
		// TODO: you are not allowed to use Layer regions here!!!
		// If there is no other way, then first lock RootLayer object first.
			
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
			// I think PictureToRegion would fit better into the Layer class (?)
			if (PictureToRegion(picture, region, inverse, where) < B_OK)
				break;

			fCurrentLayer->CurrentState()->SetClippingRegion(region);

			if (rootLayer && !(fCurrentLayer->IsHidden()) && !fWindowLayer->InUpdate()) {
				BRegion invalidRegion;
				fCurrentLayer->GetOnScreenRegion(invalidRegion);

				// TODO: this is broken! a smaller area may be invalidated!

				fCurrentLayer->fParent->MarkForRebuild(invalidRegion);
				fCurrentLayer->fParent->TriggerRebuild();
				rootLayer->MarkForRedraw(invalidRegion);
				rootLayer->TriggerRedraw();
			}
				
			break;
		}
		
		case AS_LAYER_GET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_CLIP_REGION: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			// if this Layer is hidden, it is clear that its visible region is void.
			if (fCurrentLayer->IsHidden()) {
				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(0L);
				fLink.Flush();
			} else {
				BRegion drawingRegion = fCurrentLayer->DrawingRegion();
				int32 rectCount = drawingRegion.CountRects();

				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(rectCount);

				for (int32 i = 0; i < rectCount; i++) {
					BRect converted(drawingRegion.RectAt(i));
					fCurrentLayer->ConvertFromScreen(&converted);
					fLink.Attach<BRect>(converted);
				}

				fLink.Flush();
			}

			break;
		}
		case AS_LAYER_SET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_CLIP_REGION: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			int32 rectCount;
			link.Read<int32>(&rectCount);

			BRegion region;
			for (int32 i = 0; i < rectCount; i++) {
				BRect r;
				link.Read<BRect>(&r);
				region.Include(r);
			}
			fCurrentLayer->SetUserClipping(region);

			break;
		}
		case AS_LAYER_INVAL_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			// NOTE: looks like this call is NOT affected by origin and scale on R5
			// so this implementation is "correct"
			BRect invalRect;
			
			link.Read<BRect>(&invalRect);

			if (rootLayer && !fCurrentLayer->IsHidden()) {
				BRect converted(invalRect.LeftTop(), invalRect.RightBottom());

				fCurrentLayer->ConvertToScreen(&converted);

				BRegion invalidRegion(converted);
				invalidRegion.IntersectWith(&fCurrentLayer->VisibleRegion());
				rootLayer->MarkForRedraw(invalidRegion);
				rootLayer->TriggerRedraw();
			}
			break;
		}
		case AS_LAYER_INVAL_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			// NOTE: looks like this call is NOT affected by origin and scale on R5
			// so this implementation is "correct"
			BRegion invalidReg;
			int32 noOfRects;
			BRect rect;
			
			link.Read<int32>(&noOfRects);
			
			for (int i = 0; i < noOfRects; i++) {
				link.Read<BRect>(&rect);
				invalidReg.Include(rect);
			}

			if (rootLayer && !fCurrentLayer->IsHidden()) {
				fCurrentLayer->ConvertToScreen(&invalidReg);

				rootLayer->MarkForRedraw(invalidReg);
				rootLayer->TriggerRedraw();
			}
			break;
		}
		case AS_BEGIN_UPDATE:
		{
			DTRACE(("ServerWindowo %s: AS_BEGIN_UPDATE\n", Title()));
			fWindowLayer->UpdateStart();
			break;
		}
		case AS_END_UPDATE:
		{
			DTRACE(("ServerWindowo %s: AS_END_UPDATE\n", Title()));
			fWindowLayer->UpdateEnd();
			break;
		}

		// ********** END: BView Messages ***********
		
		// ********* BWindow Messages ***********
		case AS_LAYER_DELETE_ROOT:
		{
			// Received when a window deletes its internal top view
			
			// TODO: Implement AS_LAYER_DELETE_ROOT
			STRACE(("ServerWindow %s: Message Delete_Layer_Root unimplemented\n", Title()));
			break;
		}
		case AS_BEGIN_TRANSACTION:
		{
			STRACE(("ServerWindow %s: Message AS_BEGIN_TRANSACTION unimplemented\n",
				Title()));
			// TODO: we could probably do a bit more here...
			fWindowLayer->DisableUpdateRequests();
			break;
		}
		case AS_END_TRANSACTION:
		{
			STRACE(("ServerWindow %s: Message AS_END_TRANSACTION unimplemented\n",
				Title()));
			fWindowLayer->EnableUpdateRequests();
			break;
		}
		case AS_ENABLE_UPDATES:
		{
			STRACE(("ServerWindow %s: Message AS_ENABLE_UPDATES unimplemented\n",
				Title()));
			fWindowLayer->EnableUpdateRequests();
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			STRACE(("ServerWindow %s: Message AS_DISABLE_UPDATES unimplemented\n",
				Title()));
			fWindowLayer->DisableUpdateRequests();
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			STRACE(("ServerWindow %s: Message Needs_Update unimplemented\n", Title()));
			if (fWindowLayer->CulmulatedUpdateRegion().Frame().IsValid())
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
			WindowLayer *windowLayer;
			int32 mainToken;
			team_id	teamID;

			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));

			windowLayer = NULL; //fDesktop->FindWindowLayerByClientToken(mainToken, teamID);
			if (windowLayer) {
				fLink.StartMessage(B_OK);
				fLink.Flush();

				//fDesktop->AddWindowLayerToSubset(fWindowLayer, windowLayer);
			} else {
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_REM_FROM_SUBSET\n", Title()));
			WindowLayer *windowLayer;
			int32 mainToken;
			team_id teamID;

			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));
			
			windowLayer = NULL; //fDesktop->FindWindowLayerByClientToken(mainToken, teamID);
			if (windowLayer) {
				fLink.StartMessage(B_OK);
				fLink.Flush();

				//fDesktop->RemoveWindowLayerFromSubset(fWindowLayer, windowLayer);
			} else {
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
		}
#if 0
		case AS_SET_LOOK:
		{
			// TODO: Implement AS_SET_LOOK
			STRACE(("ServerWindow %s: Message Set_Look unimplemented\n", Title()));
			break;
		}
		case AS_SET_FLAGS:
		{
			// TODO: Implement AS_SET_FLAGS
			STRACE(("ServerWindow %s: Message Set_Flags unimplemented\n", Title()));
			break;
		}
		case AS_SET_FEEL:
		{
			STRACE(("ServerWindow %s: Message AS_SET_FEEL\n", Title()));
			int32 newFeel;
			link.Read<int32>(&newFeel);

			fWindowLayer->GetRootLayer()->ChangeWindowLayerFeel(winLayer, newFeel);
			break;
		}
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
		case AS_GET_WORKSPACES:
		{
			STRACE(("ServerWindow %s: Message Get_Workspaces unimplemented\n", Title()));
			fLink.StartMessage(B_OK);
			fLink.Attach<uint32>(fWindowLayer->Workspaces());
			fLink.Flush();
			break;
		}
		case AS_SET_WORKSPACES:
		{
			STRACE(("ServerWindow %s: Message Set_Workspaces unimplemented\n", Title()));
			uint32 newWorkspaces;
			link.Read<uint32>(&newWorkspaces);

			fDesktop->SetWindowWorkspaces(fWindowLayer, newWorkspaces);
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			float xResizeBy;
			float yResizeBy;
			link.Read<float>(&xResizeBy);
			link.Read<float>(&yResizeBy);

			STRACE(("ServerWindow %s: Message AS_WINDOW_RESIZE %.1f, %.1f\n", Title(), xResizeBy, yResizeBy));

			fDesktop->ResizeWindowBy(fWindowLayer, xResizeBy, yResizeBy);
			break;
		}
		case AS_WINDOW_MOVE:
		{
			float xMoveBy;
			float yMoveBy;
			link.Read<float>(&xMoveBy);
			link.Read<float>(&yMoveBy);

			STRACE(("ServerWindow %s: Message AS_WINDOW_MOVE: %.1f, %.1f\n", Title(), xMoveBy, yMoveBy));

			fDesktop->MoveWindowBy(fWindowLayer, xMoveBy, yMoveBy);
			break;
		}
		case AS_SET_SIZE_LIMITS:
		{
			// Attached Data:
			// 1) float minimum width
			// 2) float maximum width
			// 3) float minimum height
			// 4) float maximum height

			float minWidth;
			float maxWidth;
			float minHeight;
			float maxHeight;

			link.Read<float>(&minWidth);
			link.Read<float>(&maxWidth);
			link.Read<float>(&minHeight);
			link.Read<float>(&maxHeight);

			// TODO: setting size limits can change the window size, and therefore,
			//	it should be done by the Desktop class as well.
			fWindowLayer->SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

			// and now, sync the client to the limits that we were able to enforce
			fWindowLayer->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

			fLink.StartMessage(B_OK);
			fLink.Attach<BRect>(fWindowLayer->Frame());
			fLink.Attach<float>(minWidth);
			fLink.Attach<float>(maxWidth);
			fLink.Attach<float>(minHeight);
			fLink.Attach<float>(maxHeight);

			fLink.Flush();
			break;
		}
		// Some BView drawing messages, but which don't need clipping
		case AS_LAYER_SET_HIGH_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_HIGH_COLOR: Layer: %s\n", Title(), fCurrentLayer->Name()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			fCurrentLayer->CurrentState()->SetHighColor(RGBColor(c));

			break;
		}
		case AS_LAYER_SET_LOW_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_LOW_COLOR: Layer: %s\n", Title(), fCurrentLayer->Name()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			fCurrentLayer->CurrentState()->SetLowColor(RGBColor(c));
			
			break;
		}
		case AS_LAYER_SET_PATTERN:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PATTERN: Layer: %s\n", fTitle, fCurrentLayer->Name()));
			pattern pat;
			
			link.Read(&pat, sizeof(pattern));
			
			fCurrentLayer->CurrentState()->SetPattern(Pattern(pat));
			
			break;
		}	
		case AS_MOVEPENTO:
		{
			DTRACE(("ServerWindow %s: Message AS_MOVEPENTO\n", Title()));
			
			float x,y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);
			if (fCurrentLayer && fCurrentLayer->CurrentState())
				fCurrentLayer->CurrentState()->SetPenLocation(BPoint(x, y));
			
			break;
		}
		case AS_SETPENSIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_SETPENSIZE\n", Title()));
			float size;
			
			link.Read<float>(&size);
			if (fCurrentLayer && fCurrentLayer->CurrentState())
				fCurrentLayer->CurrentState()->SetPenSize(size);
			
			break;
		}
		case AS_SET_FONT:
		{
			DTRACE(("ServerWindow %s: Message AS_SET_FONT\n", Title()));
			// TODO: Implement AS_SET_FONT?
			// Confusing!! But it works already!
			break;
		}
		case AS_SET_FONT_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_SET_FONT_SIZE\n", Title()));
			// TODO: Implement AS_SET_FONT_SIZE?
			break;
		}
		case AS_AREA_MESSAGE:
		{
			STRACE(("ServerWindow %s: Message AS_AREA_MESSAGE\n", Title()));
			// This occurs in only one kind of case: a message is too big to send over a port. This
			// is really an edge case, so this shouldn't happen *too* often
			
			// Attached Data:
			// 1) area_id id of an area already owned by the server containing the message
			// 2) size_t offset of the pointer in the area
			// 3) size_t size of the message
			
			area_id area;
			size_t offset;
			size_t msgsize;
			area_info ai;
			int8 *msgpointer;
			
			link.Read<area_id>(&area);
			link.Read<size_t>(&offset);
			link.Read<size_t>(&msgsize);
			
			// Part sanity check, part get base pointer :)
			if(get_area_info(area,&ai)!=B_OK)
				break;

			msgpointer = (int8*)ai.address + offset;

			RAMLinkMsgReader mlink(msgpointer);
			_DispatchMessage(mlink.Code(), mlink);

			// This is a very special case in the sense that when ServerMemIO is used for this 
			// purpose, it will be set to NOT automatically free the memory which it had 
			// requested. This is the server's job once the message has been dispatched.
			fServerApp->AppAreaPool()->ReleaseBuffer(msgpointer);
			break;
		}
		case AS_SYNC:
		{
			// TODO: AS_SYNC is a no-op for now, just to get things working
			fLink.StartMessage(B_OK);
			fLink.Flush();
			break;
		}
		case AS_LAYER_DRAG_IMAGE:
		{
			// TODO: Implement AS_LAYER_DRAG_IMAGE
			STRACE(("ServerWindow %s: Message AS_DRAG_IMAGE unimplemented\n", Title()));
			DTRACE(("ServerWindow %s: Message AS_DRAG_IMAGE unimplemented\n", Title()));
			break;
		}
		case AS_LAYER_DRAG_RECT:
		{
			// TODO: Implement AS_LAYER_DRAG_RECT
			STRACE(("ServerWindow %s: Message AS_DRAG_RECT unimplemented\n", Title()));
			DTRACE(("ServerWindow %s: Message AS_DRAG_RECT unimplemented\n", Title()));
			break;
		}
		case AS_GET_MOUSE:
		{
			DTRACE(("ServerWindow %s: Message AS_GET_MOUSE\n", fTitle));

			// Returns
			// 1) BPoint mouse location
			// 2) int32 button state
			rootLayer->Unlock();

			BPoint where;
			int32 buttons;
			fDesktop->EventDispatcher().GetMouse(where, buttons);

			fLink.StartMessage(B_OK);
			fLink.Attach<BPoint>(where);
			fLink.Attach<int32>(buttons);
			fLink.Flush();

			rootLayer->Lock();
			break;
		}

		case AS_DW_GET_SYNC_DATA:
		{
			// TODO: Use token or get rid of it.
			int32 serverToken;
			link.Read<int32>(&serverToken);
			
			if (_EnableDirectWindowMode() == B_OK) {
				fLink.StartMessage(B_OK);
				struct dw_sync_data syncData = { 
					fDirectWindowData->direct_area,
					fDirectWindowData->direct_sem,
					fDirectWindowData->direct_sem_ack
				};
				
				fLink.Attach(&syncData, sizeof(syncData));
				 
			} else
				fLink.StartMessage(B_ERROR);
			
			fLink.Flush();
			
			break;
		}
		
		default:
			_DispatchGraphicsMessage(code, link);
			break;
	}

	if (rootLayer)
		rootLayer->Unlock();
}

// -------------------- Graphics messages ----------------------------------

void
ServerWindow::_DispatchGraphicsMessage(int32 code, BPrivate::LinkReceiver &link)
{
	// NOTE: fCurrentLayer and fCurrentLayer->fLayerData cannot be NULL,
	// _DispatchGraphicsMessage() is called from _DispatchMessage() which
	// checks both these conditions
	BRegion rreg(fCurrentLayer->DrawingRegion());

	if (fWindowLayer->InUpdate())
		rreg.IntersectWith(&fWindowLayer->RegionToBeUpdated());

	DrawingEngine* driver = fWindowLayer->GetDrawingEngine();
	if (!driver) {
		// ?!?
		DTRACE(("ServerWindow %s: no display driver!!\n", Title()));
		return;
	}

	driver->ConstrainClippingRegion(&rreg);
//	rgb_color  rrr = fCurrentLayer->CurrentState()->viewcolor.GetColor32();
//	RGBColor c(rand()%255,rand()%255,rand()%255);
//	driver->FillRect(BRect(0,0,639,479), c);

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
			fCurrentLayer->ConvertToScreenForDrawing(&p1);
			fCurrentLayer->ConvertToScreenForDrawing(&p2);
			driver->StrokeLine(p1, p2, fCurrentLayer->CurrentState());
			
			// We update the pen here because many DrawingEngine calls which do not update the
			// pen position actually call StrokeLine

			// TODO: Decide where to put this, for example, it cannot be done
			// for DrawString(), also there needs to be a decision, if penlocation
			// is in View coordinates (I think it should be) or in screen coordinates.
			fCurrentLayer->CurrentState()->SetPenLocation(penPos);
			break;
		}
		case AS_LAYER_INVERT_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_INVERT_RECT\n", Title()));
			
			BRect rect;
			link.Read<BRect>(&rect);
			
			fCurrentLayer->ConvertToScreenForDrawing(&rect);
			driver->InvertRect(rect);
			break;
		}
		case AS_STROKE_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_RECT\n", Title()));
			
			float left, top, right, bottom;
			link.Read<float>(&left);
			link.Read<float>(&top);
			link.Read<float>(&right);
			link.Read<float>(&bottom);
			BRect rect(left,top,right,bottom);
			
			fCurrentLayer->ConvertToScreenForDrawing(&rect);
			driver->StrokeRect(rect, fCurrentLayer->CurrentState());

			break;
		}
		case AS_FILL_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_RECT\n", Title()));
			
			BRect rect;
			link.Read<BRect>(&rect);

			fCurrentLayer->ConvertToScreenForDrawing(&rect);
			driver->FillRect(rect, fCurrentLayer->CurrentState());
			break;
		}
		case AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT:
		case AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP_(A)SYNC_AT_POINT: Layer name: %s\n", fTitle, fCurrentLayer->Name()));
			int32 bitmapToken;
			BPoint point;
			link.Read<int32>(&bitmapToken);
			link.Read<BPoint>(&point);

			ServerBitmap* bitmap = fServerApp->FindBitmap(bitmapToken);
			if (bitmap) {
				BRect src = bitmap->Bounds();
				BRect dst = src.OffsetToCopy(point);
				fCurrentLayer->ConvertToScreenForDrawing(&dst);

				driver->DrawBitmap(bitmap, src, dst, fCurrentLayer->CurrentState());
			}

			// TODO: how should AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT sync with the client?
			// It Doesn't have to. Sync means: force a sync of the view/link, so that
			// the bitmap is already drawn when the "BView::DrawBitmap()" call returns.
			// If this is ever possible.
			break;
		}
		case AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT:
		case AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP_(A)SYNC_IN_RECT: Layer name: %s\n", fTitle, fCurrentLayer->Name()));
			int32 bitmapToken;
			BRect srcRect, dstRect;

			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&dstRect);
			link.Read<BRect>(&srcRect);

			ServerBitmap* bitmap = fServerApp->FindBitmap(bitmapToken);
			if (bitmap) {
				fCurrentLayer->ConvertToScreenForDrawing(&dstRect);

				driver->DrawBitmap(bitmap, srcRect, dstRect, fCurrentLayer->CurrentState());
			}

			// TODO: how should AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT sync with the client?
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

			fCurrentLayer->ConvertToScreenForDrawing(&r);
			driver->DrawArc(r, angle, span, fCurrentLayer->CurrentState(), code == AS_FILL_ARC);

			break;
		}
		case AS_STROKE_BEZIER:
		case AS_FILL_BEZIER:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_BEZIER\n", Title()));
			
			BPoint pts[4];
			for (int32 i = 0; i < 4; i++) {
				link.Read<BPoint>(&(pts[i]));
				fCurrentLayer->ConvertToScreenForDrawing(&pts[i]);
			}
				
			driver->DrawBezier(pts, fCurrentLayer->CurrentState(), code == AS_FILL_BEZIER);

			break;
		}
		case AS_STROKE_ELLIPSE:
		case AS_FILL_ELLIPSE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_ELLIPSE\n", Title()));
			
			BRect rect;
			link.Read<BRect>(&rect);

			fCurrentLayer->ConvertToScreenForDrawing(&rect);
			driver->DrawEllipse(rect, fCurrentLayer->CurrentState(), code == AS_FILL_ELLIPSE);

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

			fCurrentLayer->ConvertToScreenForDrawing(&rect);
			driver->DrawRoundRect(rect, xrad, yrad, fCurrentLayer->CurrentState(), code == AS_FILL_ROUNDRECT);

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
				fCurrentLayer->ConvertToScreenForDrawing(&pts[i]);
			}

			link.Read<BRect>(&rect);

			fCurrentLayer->ConvertToScreenForDrawing(&rect);
			driver->DrawTriangle(pts, rect, fCurrentLayer->CurrentState(), code == AS_FILL_TRIANGLE);

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
					fCurrentLayer->ConvertToScreenForDrawing(&pointList[i]);
	
				driver->DrawPolygon(pointList, pointCount, polyFrame,
									fCurrentLayer->CurrentState(), code == AS_FILL_POLYGON,
									isClosed && pointCount > 2);
	
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

				for (int32 i = 0; i < ptCount; i++)
					fCurrentLayer->ConvertToScreenForDrawing(&ptList[i]);
	
				driver->DrawShape(shapeFrame, opCount, opList, ptCount, ptList,
								  fCurrentLayer->CurrentState(), code == AS_FILL_SHAPE);
			}

			delete[] opList;
			delete[] ptList;

			break;
		}
		case AS_FILL_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_REGION\n", Title()));
			
			int32 count;
			link.Read<int32>(&count);

			BRect* rects = new(nothrow) BRect[count];
			if (link.Read(rects, sizeof(BRect) * count) < B_OK) {
				delete[] rects;
				break;
			}

			// Between the client-side conversion to BRects from clipping_rects to the overhead
			// in repeatedly calling FillRect(), this is definitely in need of optimization. At
			// least it works for now. :)
			BRegion region;
			for (int32 i = 0; i < count; i++) {
				region.Include(rects[i]);
			}

			fCurrentLayer->ConvertToScreenForDrawing(&region);
			driver->FillRegion(region, fCurrentLayer->CurrentState());

			delete[] rects;

			// TODO: create support for clipping_rect usage for faster BRegion display.
			// Tweaks to DrawingEngine are necessary along with conversion routines in Layer
			break;
		}
		case AS_STROKE_LINEARRAY:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_LINEARRAY\n", Title()));
			
			// Attached Data:
			// 1) int32 Number of lines in the array
			// 2) array of struct _array_data_ objects, as defined in ViewAux.h
			
			int32 linecount;
			
			link.Read<int32>(&linecount);
			if (linecount > 0) {
				LineArrayData linedata[linecount];
				LineArrayData* index;

				for (int32 i = 0; i < linecount; i++) {
					index = &linedata[i];

					link.Read<float>(&(index->pt1.x));
					link.Read<float>(&(index->pt1.y));
					link.Read<float>(&(index->pt2.x));
					link.Read<float>(&(index->pt2.y));
					link.Read<rgb_color>(&(index->color));

					fCurrentLayer->ConvertToScreenForDrawing(&index->pt1);
					fCurrentLayer->ConvertToScreenForDrawing(&index->pt2);
				}
				driver->StrokeLineArray(linecount, linedata, fCurrentLayer->CurrentState());
			}
			break;
		}
		case AS_DRAW_STRING:
		{
			DTRACE(("ServerWindow %s: Message AS_DRAW_STRING\n", Title()));
			char *string;
			int32 length;
			BPoint location;
			escapement_delta delta;
			
			link.Read<int32>(&length);
			link.Read<BPoint>(&location);
			link.Read<escapement_delta>(&delta);
			link.ReadString(&string);
			
			fCurrentLayer->ConvertToScreenForDrawing(&location);
			BPoint penLocation = driver->DrawString(string, length, location,
													fCurrentLayer->CurrentState(), &delta);
			fCurrentLayer->ConvertFromScreenForDrawing(&penLocation);
			fCurrentLayer->CurrentState()->SetPenLocation(penLocation);
			
			free(string);
			break;
		}
		case AS_LAYER_BEGIN_PICTURE:
			CRITICAL("AS_LAYER_BEGIN_PICTURE not implemented\n");
			break;
		case AS_LAYER_APPEND_TO_PICTURE:
			CRITICAL("AS_LAYER_APPEND_TO_PICTURE not implemented\n");
			break;
		case AS_LAYER_END_PICTURE:
			CRITICAL("AS_LAYER_END_PICTURE not implemented\n");
			break;

		default:
			printf("ServerWindow %s received unexpected code - message offset %ld\n",
				Title(), code - B_OK);

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
	}

	driver->ConstrainClippingRegion(NULL);
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

		switch (code) {
			case AS_DELETE_WINDOW:
			case kMsgQuitLooper:
				// this means the client has been killed
				STRACE(("ServerWindow %s received 'AS_DELETE_WINDOW' message code\n",
					Title()));

				quitLoop = true;

				// ToDo: what's this?
				//RootLayer *rootLayer = fWindowLayer->GetRootLayer();

				// we are preparing to delete a ServerWindow, RootLayer should be aware
				// of that and stop for a moment.
				// also we must wait a bit for the associated WindowLayer to become hidden
				//while(1) {
				//	rootLayer->Lock();
				//	if (IsHidden())
				//		break;
				//	else
				//		rootLayer->Unlock();
				//}

				// ServerWindow's destructor takes care of pulling this object off the desktop.
				if (!fWindowLayer->IsHidden())
					CRITICAL("ServerWindow: a window must be hidden before it's deleted\n");
				break;

			// TODO: these are here temporarily, as they don't lock the root layer
			case AS_SHOW_WINDOW:
				STRACE(("ServerWindow %s: Message AS_SHOW_WINDOW\n", Title()));
				Show();
				break;

			case AS_HIDE_WINDOW:
				STRACE(("ServerWindow %s: Message AS_HIDE_WINDOW\n", Title()));		
				Hide();
				break;

			case AS_ACTIVATE_WINDOW:
			{
				DTRACE(("ServerWindow %s: Message AS_ACTIVATE_WINDOW: Layer: %s\n", Title(), fCurrentLayer->Name()));
				bool activate = true;
	
				receiver.Read<bool>(&activate);
	
				if (activate)
					fDesktop->ActivateWindow(fWindowLayer);
				else
					fDesktop->SendBehindWindow(fWindowLayer, NULL);
				break;
			}
			case AS_SEND_BEHIND:
			{
				STRACE(("ServerWindow %s: Message  Send_Behind unimplemented\n", Title()));
				int32 token;
				team_id teamID;
				status_t status;
		
				receiver.Read<int32>(&token);
				receiver.Read<team_id>(&teamID);
		
				WindowLayer *behindOf;
				if ((behindOf = fDesktop->FindWindowLayerByClientToken(token, teamID)) != NULL) {
					fDesktop->SendBehindWindow(fWindowLayer, behindOf);
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

			default:
				_DispatchMessage(code, receiver);
				break;
		}

#ifdef PROFILE_MESSAGE_LOOP
		if (code >= 0 && code < AS_LAST_CODE) {
			diff = system_time() - start;
			atomic_add(&sMessageProfile[code].count, 1);
#ifdef __HAIKU__
			atomic_add64(&sMessageProfile[code].time, diff);
#else
			sMessageProfile[code].time += diff;
#endif
			if (diff > 10000)
				printf("ServerWindow %s: message %ld took %Ld usecs\n", Title(), code, diff);
		}
#endif

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

#ifndef USING_MESSAGE4
	ssize_t size = msg->FlattenedSize();
	char* buffer = new(nothrow) char[size];
	status_t ret;

	if ((ret = msg->Flatten(buffer, size)) == B_OK) {
		ret = BMessage::Private::SendFlattenedMessage(buffer, size,
			fClientLooperPort, target, 100000);
		if (ret < B_OK) {
			fprintf(stderr, "ServerWindow(\"%s\")::SendMessageToClient('%.4s'): %s\n",
				Title(), (char*)&msg->what, strerror(ret));
		}
	} else
		printf("PANIC: ServerWindow %s: can't flatten message in 'SendMessageToClient()'\n", fTitle);

	delete[] buffer;
	return ret;
#else
	BMessenger reply;
	BMessage::Private messagePrivate((BMessage *)msg);
	return messagePrivate.SendMessage(fClientLooperPort, target, 100000,
		false, reply);
#endif
}


WindowLayer*
ServerWindow::MakeWindowLayer(BRect frame, const char* name,
	uint32 look, uint32 feel, uint32 flags, uint32 workspace)
{
	// The non-offscreen ServerWindow uses the DrawingEngine instance from the desktop.
	return new(nothrow) WindowLayer(frame, name, look, feel, flags,
		workspace, this, fDesktop->GetDrawingEngine());
}


status_t
ServerWindow::_EnableDirectWindowMode()
{
	if (fDirectWindowData != NULL)
		return B_ERROR; // already in direct window mode
	
	fDirectWindowData = new (nothrow) dw_data;
	if (fDirectWindowData == NULL)
		return B_NO_MEMORY;
		
	if (!fDirectWindowData->IsValid()) {
		delete fDirectWindowData;
		fDirectWindowData = NULL;
		return B_ERROR;
	}
	
	return B_OK;
}


void
ServerWindow::HandleDirectConnection(int bufferState, int driverState)
{
	if (fDirectWindowData == NULL)
		return;
	
	if (bufferState != -1)
		fDirectWindowData->direct_info->buffer_state = (direct_buffer_state)bufferState;
	
	if (driverState != -1)
		fDirectWindowData->direct_info->driver_state = (direct_driver_state)driverState;
	
	if ((bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_STOP) {
		// TODO: Locking ?
		RenderingBuffer *buffer = fDesktop->HWInterface()->FrontBuffer();
		fDirectWindowData->direct_info->bits = buffer->Bits();
		fDirectWindowData->direct_info->pci_bits = NULL; // TODO	
		fDirectWindowData->direct_info->bytes_per_row = buffer->BytesPerRow();
		fDirectWindowData->direct_info->bits_per_pixel = buffer->BytesPerRow() / buffer->Width() * 8;
		fDirectWindowData->direct_info->pixel_format = buffer->ColorSpace();
		fDirectWindowData->direct_info->layout = B_BUFFER_NONINTERLEAVED;
		fDirectWindowData->direct_info->orientation = B_BUFFER_TOP_TO_BOTTOM; // TODO
		
		WindowLayer *layer = const_cast<WindowLayer *>(GetWindowLayer());
		fDirectWindowData->direct_info->window_bounds = to_clipping_rect(layer->Frame());

		// TODO: Review this
		const int32 kMaxClipRectsCount = (B_PAGE_SIZE - sizeof(direct_buffer_info))
			/ sizeof(clipping_rect);

		// TODO: Is there a simpler way to obtain this result ?
		// We just want the region INSIDE the window, border excluded.
		BRegion clipRegion = const_cast<BRegion &>(layer->FullVisible());
		BRegion exclude = const_cast<BRegion &>(layer->VisibleRegion());
		clipRegion.Exclude(&exclude);

		fDirectWindowData->direct_info->clip_list_count = min_c(clipRegion.CountRects(),
			kMaxClipRectsCount);
		fDirectWindowData->direct_info->clip_bounds = clipRegion.FrameInt();

		for (uint32 i = 0; i < fDirectWindowData->direct_info->clip_list_count; i++)
			fDirectWindowData->direct_info->clip_list[i] = clipRegion.RectAtInt(i);
	}

	// Releasing this sem causes the client to call BDirectWindow::DirectConnected()
	release_sem(fDirectWindowData->direct_sem);

	// TODO: Waiting half a second in this thread is not a problem,
	// but since we are called from the RootLayer's thread too, very bad things could happen.
	// Find some way to call this method only within ServerWindow's thread (messaging ?)
	status_t status;
	do {
		// TODO: The timeout is 3000000 usecs (3 seconds) on beos.
		// Test, but I think half a second is enough.
		status = acquire_sem_etc(fDirectWindowData->direct_sem_ack, 1, B_TIMEOUT, 500000);
	} while (status == B_INTERRUPTED);

	if (status < B_OK) {
		// The client application didn't release the semaphore
		// within the given timeout. Or something else went wrong.
		// Deleting this member should make it crash.
		delete fDirectWindowData;
		fDirectWindowData = NULL;
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
