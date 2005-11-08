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
	coordinating and linking a window's WinBorder half with its messaging half, dispatching 
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

#include "AppServer.h"
#include "BGet++.h"
#include "DebugInfoManager.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "Layer.h"
#include "MessagePrivate.h"
#include "RAMLinkMsgReader.h"
#include "RenderingBuffer.h"
#include "RootLayer.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "ServerProtocol.h"
#include "TokenHandler.h"
#include "Utils.h"
#include "WinBorder.h"
#include "Workspace.h"
#include "WorkspacesLayer.h"

#include "ServerWindow.h"

#include "clipping.h"

//#define DEBUG_SERVERWINDOW
//#define DEBUG_SERVERWINDOW_GRAPHICS


#ifdef DEBUG_SERVERWINDOW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_SERVERWINDOW_GRAPHICS
#	include <stdio.h>
#	define DTRACE(x) printf x
#else
#	define DTRACE(x) ;
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
	\brief Constructor
	
	Does a lot of stuff to set up for the window - new decorator, new winborder, spawn a 
	monitor thread.
*/
ServerWindow::ServerWindow(const char *title, ServerApp *app,
						   port_id clientPort, port_id looperPort, int32 handlerID)
	: MessageLooper(title && *title ? title : "Unnamed Window"),
	fTitle(title),
	fDesktop(app->GetDesktop()),
	fServerApp(app),
	fWinBorder(NULL),
	fClientTeam(app->ClientTeam()),
	fMessagePort(-1),
	fClientReplyPort(clientPort),
	fClientLooperPort(looperPort),
	fClientViewsWithInvalidCoords(B_VIEW_RESIZED),
	fHandlerToken(handlerID),
	fCurrentLayer(NULL),
	fDirectWindowData(NULL)
{
	STRACE(("ServerWindow(%s)::ServerWindow()\n", title));

	fServerToken = BPrivate::gDefaultTokens.NewToken(B_SERVER_TOKEN, this);
}


//!Tears down all connections the main app_server objects, and deletes some internals.
ServerWindow::~ServerWindow()
{
	STRACE(("*ServerWindow(%s@%p):~ServerWindow()\n", fTitle, this));

	if (!fWinBorder->IsOffscreenWindow())
		fDesktop->RemoveWinBorder(fWinBorder);

	delete fWinBorder;

	free(const_cast<char *>(fTitle));
	delete_port(fMessagePort);

	BPrivate::gDefaultTokens.RemoveToken(fServerToken);

	delete fDirectWindowData;
	STRACE(("#ServerWindow(%p) will exit NOW\n", this));
}


status_t
ServerWindow::Init(BRect frame, uint32 look, uint32 feel, uint32 flags, uint32 workspace)
{
	if (fTitle == NULL)
		fTitle = strdup("Unnamed Window");
	if (fTitle == NULL)
		return B_NO_MEMORY;

	// fMessagePort is the port to which the app sends messages for the server
	fMessagePort = create_port(100, fTitle);
	if (fMessagePort < B_OK)
		return fMessagePort;

	fLink.SetSenderPort(fClientReplyPort);
	fLink.SetReceiverPort(fMessagePort);

	// We cannot call MakeWinBorder in the constructor, since it 
	fWinBorder = MakeWinBorder(frame, fTitle, look, feel, flags, workspace);
	if (!fWinBorder)
		return B_NO_MEMORY;

	if (!fWinBorder->IsOffscreenWindow())
		fDesktop->AddWinBorder(fWinBorder);

	return B_OK;
}


bool
ServerWindow::Run()
{
	if (!MessageLooper::Run())
		return false;

	// Send a reply to our window - it is expecting fMessagePort
	// port and some other info

	fLink.StartMessage(SERVER_TRUE);
	fLink.Attach<port_id>(fMessagePort);

	float minWidth, maxWidth, minHeight, maxHeight;
	fWinBorder->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	fLink.Attach<BRect>(fWinBorder->Frame());
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
	snprintf(name, length, "w:%ld:%s", ClientTeam(), Title());
}


//! Forces the window border to update its decorator
void
ServerWindow::ReplaceDecorator()
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::ReplaceDecorator()\n");

	STRACE(("ServerWindow %s: Replace Decorator\n", fTitle));
	fWinBorder->UpdateDecorator();
}


//! Shows the window's WinBorder
void
ServerWindow::Show()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Show\n", Title()));

	if (fQuitting || !fWinBorder->IsHidden())
		return;

	RootLayer* rootLayer = fWinBorder->GetRootLayer();
	if (rootLayer && rootLayer->Lock()) {
		rootLayer->ShowWinBorder(fWinBorder);
		rootLayer->Unlock();
	}
	
	if (fDirectWindowData != NULL)
		_HandleDirectConnection(B_DIRECT_START|B_BUFFER_RESET);
}


//! Hides the window's WinBorder
void
ServerWindow::Hide()
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Hide\n", Title()));

	if (fWinBorder->IsHidden())
		return;

	if (fDirectWindowData != NULL)
		_HandleDirectConnection(B_DIRECT_STOP);
	
	RootLayer* rootLayer = fWinBorder->GetRootLayer();
	if (rootLayer && rootLayer->Lock()) {
		rootLayer->HideWinBorder(fWinBorder);
		rootLayer->Unlock();
	}
}


void
ServerWindow::SetTitle(const char* newTitle)
{
	const char* oldTitle = fTitle;

	if (newTitle == NULL || !newTitle[0])
		fTitle = strdup("Unnamed Window");
	else
		fTitle = strdup(newTitle);

	if (fTitle == NULL) {
		fTitle = oldTitle;
		return;
	}

	free(const_cast<char*>(oldTitle));

	if (Thread() >= B_OK) {
		char name[B_OS_NAME_LENGTH];
		snprintf(name, sizeof(name), "w:%ld:%s", ClientTeam(), fTitle);
		rename_thread(Thread(), name);
	}

	if (fWinBorder != NULL)
		fWinBorder->SetName(newTitle);
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
		if (!fWinBorder->IsHidden()) {
			Hide();
			sendMessages = true;
		}
	} else {
		if (fWinBorder->IsHidden()) {
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

/*!
	\brief Notifies window of a change in screen resolution
	\param frame Size of the new resolution
	\param color_space Color space of the new screen mode
*/
void
ServerWindow::NotifyScreenModeChanged(const BRect frame, const color_space colorSpace)
{
	STRACE(("ServerWindow %s: ScreenModeChanged\n", fTitle));

	BMessage msg(B_SCREEN_CHANGED);
	msg.AddRect("frame", frame);
	msg.AddInt32("mode", (int32)colorSpace);

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
	info.workspaces = fWinBorder->Workspaces();

	info.layer = 0; // ToDo: what is this???
	info.feel = fWinBorder->Feel();
	info.flags = fWinBorder->WindowFlags();
	info.window_left = (int)floor(fWinBorder->Frame().left);
	info.window_top = (int)floor(fWinBorder->Frame().top);
	info.window_right = (int)floor(fWinBorder->Frame().right);
	info.window_bottom = (int)floor(fWinBorder->Frame().bottom);

	info.show_hide_level = fWinBorder->IsHidden() ? 1 : -1; // ???
	info.is_mini = fWinBorder->IsHidden();
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
		&& (fWinBorder->WindowFlags() & kWorkspacesWindowFlag) != 0) {
		// this is a workspaces window!
		newLayer = new (nothrow) WorkspacesLayer(frame, name, token, resizeMask,
			flags, fWinBorder->GetDrawingEngine());
	} else {
		newLayer = new (nothrow) Layer(frame, name, token, resizeMask, flags,
			fWinBorder->GetDrawingEngine());
	}

	if (newLayer == NULL)
		return NULL;

	free(name);

	// there is no way of setting this, other than manually :-)
	newLayer->SetViewColor(viewColor);
	newLayer->fHidden = hidden;
	newLayer->fEventMask = eventMask;
	newLayer->fEventOptions = eventOptions;
	newLayer->fOwner = fWinBorder;

	DesktopSettings settings(fDesktop);
	ServerFont font;
	settings.GetDefaultPlainFont(font);
	newLayer->CurrentState()->SetFont(font);

// TODO: rework the clipping stuff to remove RootLayer dependency and then
// remove this hack:
if (fWinBorder->IsOffscreenWindow()) {
	newLayer->fVisible2.Set(newLayer->fFrame);
}

	if (_parent) {
		Layer *parent = fWinBorder->FindLayer(parentToken);
		if (parent == NULL)
			CRITICAL("View token not found!\n");

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

		printf("ServerWindow %s received unexpected code - message offset %ld before top_view attached.\n", Title(), code - SERVER_TRUE);
		return;
	}

	RootLayer *myRootLayer = fWinBorder->GetRootLayer();
	// NOTE: is NULL when fWinBorder is offscreen!
	if (myRootLayer)
		myRootLayer->Lock();

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

			// TODO: Are origin and scale handled in this conversion?
			src = fCurrentLayer->ConvertToTop(src);
			dst = fCurrentLayer->ConvertToTop(dst);
		
			int32 xOffset = (int32)(dst.left - src.left);
			int32 yOffset = (int32)(dst.top - src.top);

			fCurrentLayer->CopyBits(src, dst, xOffset, yOffset);

			break;
		}
		case AS_SET_CURRENT_LAYER:
		{
			int32 token;
			
			link.Read<int32>(&token);

			Layer *current = fWinBorder->FindLayer(token);
			if (current) {
				DTRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: %s, token %ld\n", fTitle, current->Name(), token));
			} else {
				DTRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: layer not found, token %ld\n", fTitle, token));
			}

			if (current)
				fCurrentLayer = current;
			else // hope this NEVER happens! :-)
				CRITICAL("Server PANIC: window cannot find Layer with ID\n");
			// ToDo: if this happens, we probably want to kill the app and clean up
			break;
		}

		case AS_LAYER_CREATE_ROOT:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE_ROOT\n", fTitle));

			// Start receiving top_view data -- pass NULL as the parent view.
			// This should be the *only* place where this happens.
			if (fCurrentLayer != NULL)
				break;

			fWinBorder->SetTopLayer(CreateLayerTree(link, NULL));
			fCurrentLayer = fWinBorder->TopLayer();
			break;
		}

		case AS_LAYER_CREATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE: Layer name: %s\n", fTitle, fCurrentLayer->Name()));

			Layer* parent = NULL;
			Layer* newLayer = CreateLayerTree(link, &parent);
			if (parent != NULL)
				parent->AddChild(newLayer, this);

			if (myRootLayer && !newLayer->IsHidden() && parent) {
				BRegion invalidRegion;
				newLayer->GetWantedRegion(invalidRegion);
				parent->MarkForRebuild(invalidRegion);
				parent->TriggerRebuild();
				if (newLayer->VisibleRegion().Frame().IsValid()) {
					myRootLayer->MarkForRedraw(newLayer->VisibleRegion());
					myRootLayer->TriggerRedraw();
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

			STRACE(("ServerWindow %s: AS_LAYER_DELETE(self)...\n", fTitle));			

			Layer *parent = fCurrentLayer->fParent;
//			BRegion *invalidRegion = NULL;

			if (!fCurrentLayer->IsHidden() && parent && myRootLayer) {
				if (fCurrentLayer->FullVisible().Frame().IsValid()) {
					parent->MarkForRebuild(fCurrentLayer->FullVisible());
					myRootLayer->MarkForRedraw(fCurrentLayer->FullVisible());
				}
			}

			// here we remove current layer from list.
			fCurrentLayer->RemoveSelf();
			fCurrentLayer->PruneTree();

			if (parent)
				parent->TriggerRebuild();			

			if (myRootLayer) {
				myRootLayer->LayerRemoved(fCurrentLayer);
				myRootLayer->TriggerRedraw();
			}
			
			#ifdef DEBUG_SERVERWINDOW
			parent->PrintTree();
			#endif
			STRACE(("DONE: ServerWindow %s: Message AS_DELETE_LAYER: Parent: %s Layer: %s\n", fTitle, parent->Name(), fCurrentLayer->Name()));

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

			fLink.StartMessage(SERVER_TRUE);

			// attach state data
			fCurrentLayer->CurrentState()->WriteToLink(fLink.Sender());
			fLink.Flush();
			break;
		}
		case AS_LAYER_SET_EVENT_MASK:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_MOUSE_EVENT_MASK: Layer name: %s\n", fTitle, fCurrentLayer->Name()));			

			uint32		mask;
			uint32		options;

			link.Read<uint32>(&mask);
			link.Read<uint32>(&options);

			fCurrentLayer->QuietlySetEventMask(mask);
			fCurrentLayer->QuietlySetEventOptions(options);

			if (myRootLayer)
				myRootLayer->AddToInputNotificationLists(fCurrentLayer, mask, options);
		}
		case AS_LAYER_SET_MOUSE_EVENT_MASK:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_MOUSE_EVENT_MASK: Layer name: %s\n", fTitle, fCurrentLayer->Name()));			

			uint32		mask;
			uint32		options;

			link.Read<uint32>(&mask);
			link.Read<uint32>(&options);

			if (myRootLayer)
				myRootLayer->SetNotifyLayer(fCurrentLayer, mask, options);
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
			// Need WinBorder::GetSizeLimits
			float deltaWidth = newWidth - fCurrentLayer->fFrame.Width();
			float deltaHeight = newHeight - fCurrentLayer->fFrame.Height();

			fCurrentLayer->ResizeBy(deltaWidth, deltaHeight);
			break;
		}
		case AS_LAYER_GET_COORD:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(SERVER_TRUE);
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
			
			fCurrentLayer->CurrentState()->SetOrigin(BPoint(x, y));
			break;
		}
		case AS_LAYER_GET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_ORIGIN: Layer: %s\n", Title(), fCurrentLayer->Name()));
			fLink.StartMessage(SERVER_TRUE);
			fLink.Attach<BPoint>(fCurrentLayer->CurrentState()->Origin());
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
			fLink.StartMessage(SERVER_TRUE);
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

			fCurrentLayer->CurrentState()->SetScale(scale);
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_SCALE: Layer: %s\n", Title(), fCurrentLayer->Name()));		

			fLink.StartMessage(SERVER_TRUE);
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
			fLink.StartMessage(SERVER_TRUE);
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
			fLink.StartMessage(SERVER_TRUE);
			fLink.Attach<float>(fCurrentLayer->CurrentState()->PenSize());
			fLink.Flush();
		
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: Layer: %s\n", Title(), fCurrentLayer->Name()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
if (myRootLayer)
	myRootLayer->Lock();			
			fCurrentLayer->SetViewColor(RGBColor(c));

			if (myRootLayer) {
				myRootLayer->MarkForRedraw(fCurrentLayer->VisibleRegion());
				myRootLayer->TriggerRedraw();
			}
if (myRootLayer)
	myRootLayer->Unlock();
			break;
		}

		case AS_LAYER_GET_HIGH_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_HIGH_COLOR: Layer: %s\n",
				Title(), fCurrentLayer->Name()));

			fLink.StartMessage(SERVER_TRUE);
			fLink.Attach<rgb_color>(fCurrentLayer->CurrentState()->HighColor().GetColor32());
			fLink.Flush();
			break;

		case AS_LAYER_GET_LOW_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_LOW_COLOR: Layer: %s\n",
				Title(), fCurrentLayer->Name()));

			fLink.StartMessage(SERVER_TRUE);
			fLink.Attach<rgb_color>(fCurrentLayer->CurrentState()->LowColor().GetColor32());
			fLink.Flush();
			break;

		case AS_LAYER_GET_VIEW_COLOR:
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_VIEW_COLOR: Layer: %s\n",
				Title(), fCurrentLayer->Name()));

			fLink.StartMessage(SERVER_TRUE);
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
			fLink.StartMessage(SERVER_TRUE);
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
			fLink.StartMessage(SERVER_TRUE);
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

			if (myRootLayer && !(fCurrentLayer->IsHidden()) && !fWinBorder->InUpdate()) {
				BRegion invalidRegion;
				fCurrentLayer->GetWantedRegion(invalidRegion);

				// TODO: this is broken! a smaller area may be invalidated!

				fCurrentLayer->fParent->MarkForRebuild(invalidRegion);
				fCurrentLayer->fParent->TriggerRebuild();
				myRootLayer->MarkForRedraw(invalidRegion);
				myRootLayer->TriggerRedraw();
			}
				
			break;
		}
		
		case AS_LAYER_GET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_CLIP_REGION: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			// if this Layer is hidden, it is clear that its visible region is void.
			if (fCurrentLayer->IsHidden()) {
				fLink.StartMessage(SERVER_TRUE);
				fLink.Attach<int32>(0L);
				fLink.Flush();
			} else {
				// TODO: Watch out for the coordinate system in AS_LAYER_GET_CLIP_REGION
				int32 rectCount = fCurrentLayer->fVisible2.CountRects();

				fLink.StartMessage(SERVER_TRUE);
				fLink.Attach<int32>(rectCount);

				for (int32 i = 0; i < rectCount; i++)
					fLink.Attach<BRect>(fCurrentLayer->ConvertFromTop(fCurrentLayer->fVisible2.RectAt(i)));

				fLink.Flush();
			}

			break;
		}
		case AS_LAYER_SET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_CLIP_REGION: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			// TODO: Watch out for the coordinate system in AS_LAYER_SET_CLIP_REGION
			int32 noOfRects;
			BRect r;
			
			link.Read<int32>(&noOfRects);

			BRegion region;
			for (int i = 0; i < noOfRects; i++) {
				link.Read<BRect>(&r);
				region.Include(fCurrentLayer->ConvertToTop(r));
			}
// TODO: Turned off user clipping for now (will probably not harm anything but performance right now)
// We need to integrate user clipping more, in Layer::PopState, the clipping needs to be
// restored too. "AS_LAYER_SET_CLIP_REGION" is irritating, as I think it should be
// "AS_LAYER_CONSTRAIN_CLIP_REGION", since it means to "add" to the current clipping, not "set" it.
//			fCurrentLayer->CurrentState()->SetClippingRegion(region);

			// TODO: rebuild clipping and redraw

			break;
		}
		case AS_LAYER_INVAL_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			// TODO: handle transformation (origin and scale) prior to converting to top
			BRect		invalRect;
			
			link.Read<BRect>(&invalRect);

			if (myRootLayer) {
				BRect converted(fCurrentLayer->ConvertToTop(invalRect.LeftTop()),
								fCurrentLayer->ConvertToTop(invalRect.RightBottom()));
				BRegion invalidRegion(converted);
				invalidRegion.IntersectWith(&fCurrentLayer->VisibleRegion());
				myRootLayer->MarkForRedraw(invalidRegion);
				myRootLayer->TriggerRedraw();
			}
			break;
		}
		case AS_LAYER_INVAL_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n", Title(), fCurrentLayer->Name()));
			
			// TODO: handle transformation (origin and scale) prior to converting to top
			// TODO: Handle conversion to top
			BRegion invalidReg;
			int32 noOfRects;
			BRect rect;
			
			link.Read<int32>(&noOfRects);
			
			for (int i = 0; i < noOfRects; i++) {
				link.Read<BRect>(&rect);
				invalidReg.Include(rect);
			}

			if (myRootLayer) {
				fCurrentLayer->ConvertToScreen2(&invalidReg);

				myRootLayer->MarkForRedraw(invalidReg);
				myRootLayer->TriggerRedraw();
			}

			break;
		}
		case AS_BEGIN_UPDATE:
		{
			DTRACE(("ServerWindowo %s: AS_BEGIN_UPDATE\n", Title()));
			fWinBorder->UpdateStart();
			break;
		}
		case AS_END_UPDATE:
		{
			DTRACE(("ServerWindowo %s: AS_END_UPDATE\n", Title()));
			fWinBorder->UpdateEnd();
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
		case AS_SHOW_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_SHOW_WINDOW\n", Title()));
			Show();
			break;
		}
		case AS_HIDE_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_HIDE_WINDOW\n", Title()));		
			Hide();
			break;
		}
		case AS_SEND_BEHIND:
		{
			STRACE(("ServerWindow %s: Message  Send_Behind unimplemented\n", Title()));
			int32 token;
			team_id teamID;
			status_t status = B_NAME_NOT_FOUND;

			link.Read<int32>(&token);
			link.Read<team_id>(&teamID);

			WinBorder *behindOf;
			if ((behindOf = fDesktop->FindWinBorderByClientToken(token, teamID)) != NULL) {
				fWinBorder->GetRootLayer()->Lock();
				// TODO: move to back ATM. Fix this later!
				fWinBorder->GetRootLayer()->SetActive(fWinBorder, false);
				fWinBorder->GetRootLayer()->Unlock();
				status = B_OK;
			}

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}
		case AS_ENABLE_UPDATES:
		{
			STRACE(("ServerWindow %s: Message Enable_Updates unimplemented\n", Title()));
			fWinBorder->EnableUpdateRequests();
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			STRACE(("ServerWindow %s: Message Disable_Updates unimplemented\n", Title()));
			fWinBorder->DisableUpdateRequests();
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			STRACE(("ServerWindow %s: Message Needs_Update unimplemented\n", Title()));
			if (fWinBorder->CulmulatedUpdateRegion().Frame().IsValid())
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
			WinBorder *windowBorder;
			int32 mainToken;
			team_id	teamID;

			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));

			windowBorder = fDesktop->FindWinBorderByClientToken(mainToken, teamID);
			if (windowBorder) {
				fLink.StartMessage(SERVER_TRUE);
				fLink.Flush();

				fWinBorder->GetRootLayer()->Lock();
				fDesktop->AddWinBorderToSubset(fWinBorder, windowBorder);
				fWinBorder->GetRootLayer()->Unlock();
			} else {
				fLink.StartMessage(SERVER_FALSE);
				fLink.Flush();
			}
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_REM_FROM_SUBSET\n", Title()));
			WinBorder *windowBorder;
			int32 mainToken;
			team_id teamID;

			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));
			
			windowBorder = fDesktop->FindWinBorderByClientToken(mainToken, teamID);
			if (windowBorder) {
				fLink.StartMessage(SERVER_TRUE);
				fLink.Flush();

				fWinBorder->GetRootLayer()->Lock();
				fDesktop->RemoveWinBorderFromSubset(fWinBorder, windowBorder);
				fWinBorder->GetRootLayer()->Unlock();
			} else {
				fLink.StartMessage(SERVER_FALSE);
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

			fWinBorder->GetRootLayer()->GoChangeWinBorderFeel(winBorder, newFeel);
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
			fLink.StartMessage(SERVER_TRUE);
			fLink.Attach<uint32>(fWinBorder->Workspaces());
			fLink.Flush();
			break;
		}
		case AS_SET_WORKSPACES:
		{
			STRACE(("ServerWindow %s: Message Set_Workspaces unimplemented\n", Title()));
			uint32 newWorkspaces;
			link.Read<uint32>(&newWorkspaces);

			fWinBorder->GetRootLayer()->Lock();
			fWinBorder->GetRootLayer()->SetWinBorderWorskpaces( fWinBorder,
																fWinBorder->Workspaces(),
																newWorkspaces);
			fWinBorder->GetRootLayer()->Unlock();
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			float xResizeBy;
			float yResizeBy;
			
			link.Read<float>(&xResizeBy);
			link.Read<float>(&yResizeBy);

			STRACE(("ServerWindow %s: Message AS_WINDOW_RESIZE %.1f, %.1f\n", Title(), xResizeBy, yResizeBy));
			
			fWinBorder->ResizeBy(xResizeBy, yResizeBy);
			break;
		}
		case AS_WINDOW_MOVE:
		{
			float xMoveBy;
			float yMoveBy;
			
			link.Read<float>(&xMoveBy);
			link.Read<float>(&yMoveBy);

			STRACE(("ServerWindow %s: Message AS_WINDOW_MOVE: %.1f, %.1f\n", Title(), xMoveBy, yMoveBy));

			fWinBorder->MoveBy(xMoveBy, yMoveBy);
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
			
			fWinBorder->SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

			// and now, sync the client to the limits that we were able to enforce
			fWinBorder->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

			fLink.StartMessage(SERVER_TRUE);
			fLink.Attach<BRect>(fWinBorder->Frame());
			fLink.Attach<float>(minWidth);
			fLink.Attach<float>(maxWidth);
			fLink.Attach<float>(minHeight);
			fLink.Attach<float>(maxHeight);

			fLink.Flush();
			break;
		}
		case AS_ACTIVATE_WINDOW:
		{
			DTRACE(("ServerWindow %s: Message AS_ACTIVATE_WINDOW: Layer: %s\n", Title(), fCurrentLayer->Name()));
			bool activate = true;

			link.Read<bool>(&activate);

			if (myRootLayer && myRootLayer->Lock()) {
				myRootLayer->SetActive(fWinBorder, activate);
				myRootLayer->Unlock();
			}
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
			fLink.StartMessage(SERVER_TRUE);
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
		case AS_LAYER_GET_MOUSE_COORDS:
		{
			DTRACE(("ServerWindow %s: Message AS_GET_MOUSE_COORDS\n", fTitle));

			fLink.StartMessage(SERVER_TRUE);

			// Returns
			// 1) BPoint mouse location
			// 2) int32 button state

			fLink.Attach<BPoint>(fDesktop->GetHWInterface()->GetCursorPosition());
			fLink.Attach<int32>(fDesktop->ActiveRootLayer()->Buttons());

			fLink.Flush();
			break;
		}
		
		case AS_DW_GET_SYNC_DATA:
		{
			// TODO: Use token or get rid of it.
			int32 serverToken;
			link.Read<int32>(&serverToken);
			
			if (_EnableDirectWindowMode() == B_OK) {
				fLink.StartMessage(SERVER_TRUE);
				struct dw_sync_data syncData = { 
					fDirectWindowData->direct_area,
					fDirectWindowData->direct_sem,
					fDirectWindowData->direct_sem_ack
				};
				
				fLink.Attach(&syncData, sizeof(syncData));
				 
			} else
				fLink.StartMessage(SERVER_FALSE);
			
			fLink.Flush();
			
			break;
		}
		
		default:
			_DispatchGraphicsMessage(code, link);
			break;
	}

	if (myRootLayer)
		myRootLayer->Unlock();
}

// -------------------- Graphics messages ----------------------------------

void
ServerWindow::_DispatchGraphicsMessage(int32 code, BPrivate::LinkReceiver &link)
{
	// NOTE: fCurrentLayer and fCurrentLayer->fLayerData cannot be NULL,
	// _DispatchGraphicsMessage() is called from _DispatchMessage() which
	// checks both these conditions
	BRegion rreg(fCurrentLayer->VisibleRegion());

	if (fWinBorder->InUpdate())
		rreg.IntersectWith(&fWinBorder->RegionToBeUpdated());

	DrawingEngine* driver = fWinBorder->GetDrawingEngine();
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

			BPoint p1(x1,y1);
			BPoint p2(x2,y2);
			driver->StrokeLine(fCurrentLayer->ConvertToTop(p1),
							   fCurrentLayer->ConvertToTop(p2),
							   fCurrentLayer->CurrentState());
			
			// We update the pen here because many DrawingEngine calls which do not update the
			// pen position actually call StrokeLine

			// TODO: Decide where to put this, for example, it cannot be done
			// for DrawString(), also there needs to be a decision, if penlocation
			// is in View coordinates (I think it should be) or in screen coordinates.
			fCurrentLayer->CurrentState()->SetPenLocation(p2);
			break;
		}
		case AS_LAYER_INVERT_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_INVERT_RECT\n", Title()));
			
			BRect rect;
			link.Read<BRect>(&rect);
			
			driver->InvertRect(fCurrentLayer->ConvertToTop(rect));
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
			
			driver->StrokeRect(fCurrentLayer->ConvertToTop(rect), fCurrentLayer->CurrentState());

			break;
		}
		case AS_FILL_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_RECT\n", Title()));
			
			BRect rect;
			link.Read<BRect>(&rect);
			driver->FillRect(fCurrentLayer->ConvertToTop(rect), fCurrentLayer->CurrentState());
			break;
		}
		case AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT:
		case AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP_(A)SYNC_AT_POINT: Layer name: %s\n", fTitle, fCurrentLayer->Name()));
			int32		bitmapToken;
			BPoint 		point;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BPoint>(&point);
			
			ServerBitmap* sbmp = fServerApp->FindBitmap(bitmapToken);
			if (sbmp) {
				BRect src = sbmp->Bounds();
				BRect dst = src.OffsetToCopy(point);
				dst = fCurrentLayer->ConvertToTop(dst);

				driver->DrawBitmap(sbmp, src, dst, fCurrentLayer->CurrentState());
			}
			
			// TODO: how should AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT sync with the client?
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
			
			ServerBitmap* sbmp = fServerApp->FindBitmap(bitmapToken);
			if (sbmp) {
				dstRect = fCurrentLayer->ConvertToTop(dstRect);

				driver->DrawBitmap(sbmp, srcRect, dstRect, fCurrentLayer->CurrentState());
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

			driver->DrawArc(fCurrentLayer->ConvertToTop(r), angle, span,
							fCurrentLayer->CurrentState(), code == AS_FILL_ARC);

			break;
		}
		case AS_STROKE_BEZIER:
		case AS_FILL_BEZIER:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_BEZIER\n", Title()));
			
			BPoint pts[4];
			for (int32 i = 0; i < 4; i++) {
				link.Read<BPoint>(&(pts[i]));
				pts[i] = fCurrentLayer->ConvertToTop(pts[i]);
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

			driver->DrawEllipse(fCurrentLayer->ConvertToTop(rect),
								fCurrentLayer->CurrentState(), code == AS_FILL_ELLIPSE);

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

			driver->DrawRoundRect(fCurrentLayer->ConvertToTop(rect), xrad, yrad,
								  fCurrentLayer->CurrentState(), code == AS_FILL_ROUNDRECT);

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
				pts[i] = fCurrentLayer->ConvertToTop(pts[i]);
			}

			link.Read<BRect>(&rect);

			driver->DrawTriangle(pts, fCurrentLayer->ConvertToTop(rect),
								 fCurrentLayer->CurrentState(), code == AS_FILL_TRIANGLE);

			break;
		}
		case AS_STROKE_POLYGON:
		case AS_FILL_POLYGON:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_POLYGON\n", Title()));

			BRect polyframe;
			bool isclosed = true;
			int32 pointcount;
			BPoint *pointlist;

			link.Read<BRect>(&polyframe);
			if (code == AS_STROKE_POLYGON)
				link.Read<bool>(&isclosed);
			link.Read<int32>(&pointcount);

			pointlist = new BPoint[pointcount];
			
			link.Read(pointlist, sizeof(BPoint)*pointcount);
			
			for (int32 i = 0; i < pointcount; i++)
				pointlist[i] = fCurrentLayer->ConvertToTop(pointlist[i]);

			driver->DrawPolygon(pointlist, pointcount, polyframe,
								fCurrentLayer->CurrentState(), code == AS_FILL_POLYGON,
								isclosed);

			delete [] pointlist;
			break;
		}
		case AS_STROKE_SHAPE:
		case AS_FILL_SHAPE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE/FILL_SHAPE\n", Title()));
			
			BRect shaperect;
			int32 opcount;
			int32 ptcount;
			
			link.Read<BRect>(&shaperect);
			link.Read<int32>(&opcount);
			link.Read<int32>(&ptcount);

			uint32* oplist = new uint32[opcount];
			BPoint* ptlist = new BPoint[ptcount];

			link.Read(oplist, opcount * sizeof(uint32));
			link.Read(ptlist, ptcount * sizeof(BPoint));

			for (int32 i = 0; i < ptcount; i++)
				ptlist[i] = fCurrentLayer->ConvertToTop(ptlist[i]);

			driver->DrawShape(shaperect, opcount, oplist, ptcount, ptlist,
							  fCurrentLayer->CurrentState(), code == AS_FILL_SHAPE);

			delete[] oplist;
			delete[] ptlist;
			break;
		}
		case AS_FILL_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_REGION\n", Title()));
			
			int32 count;
			link.Read<int32>(&count);

			BRect *rects = new BRect[count];
			if (link.Read(rects, sizeof(BRect) * count) != B_OK) {
				delete[] rects;
				break;
			}

			// Between the client-side conversion to BRects from clipping_rects to the overhead
			// in repeatedly calling FillRect(), this is definitely in need of optimization. At
			// least it works for now. :)
			BRegion region;
			for (int32 i = 0; i < count; i++) {
				region.Include(fCurrentLayer->ConvertToTop(rects[i]));
			}
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
				LineArrayData linedata[linecount], *index;

				for (int32 i = 0; i < linecount; i++) {
					index = &linedata[i];

					link.Read<float>(&(index->pt1.x));
					link.Read<float>(&(index->pt1.y));
					link.Read<float>(&(index->pt2.x));
					link.Read<float>(&(index->pt2.y));
					link.Read<rgb_color>(&(index->color));

					index->pt1 = fCurrentLayer->ConvertToTop(index->pt1);
					index->pt2 = fCurrentLayer->ConvertToTop(index->pt2);
				}
				driver->StrokeLineArray(linecount,linedata,fCurrentLayer->CurrentState());
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
			
			driver->DrawString(string, length,
							   fCurrentLayer->ConvertToTop(location),
							   fCurrentLayer->CurrentState(), &delta);
			
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
				Title(), code - SERVER_TRUE);

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(SERVER_FALSE);
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

		Lock();

		switch (code) {
			case AS_DELETE_WINDOW:
			case kMsgQuitLooper:
				// this means the client has been killed
				STRACE(("ServerWindow %s received 'AS_DELETE_WINDOW' message code\n",
					Title()));

				quitLoop = true;

				// ToDo: what's this?
				//RootLayer *rootLayer = fWinBorder->GetRootLayer();

				// we are preparing to delete a ServerWindow, RootLayer should be aware
				// of that and stop for a moment.
				// also we must wait a bit for the associated WinBorder to become hidden
				//while(1) {
				//	myRootLayer->Lock();
				//	if (IsHidden())
				//		break;
				//	else
				//		rootLayer->Unlock();
				//}

				// ServerWindow's destructor takes care of pulling this object off the desktop.
				if (!fWinBorder->IsHidden())
					CRITICAL("ServerWindow: a window must be hidden before it's deleted\n");
				break;

			case B_QUIT_REQUESTED:
				STRACE(("ServerWindow %s received quit request\n", Title()));
				NotifyQuitRequested();
				break;

			default:
				_DispatchMessage(code, receiver);
				break;
		}

		Unlock();
	}

	// we were asked to quit the message loop - either on request or because of an error
	Quit();
		// does not return
}

status_t
ServerWindow::SendMessageToClient(const BMessage* msg, int32 target, bool usePreferred) const
{
	ssize_t size = msg->FlattenedSize();
	char* buffer = new char[size];
	status_t ret;

	if ((ret = msg->Flatten(buffer, size)) == B_OK) {
		ret = BMessage::Private::SendFlattenedMessage(buffer, size,
					fClientLooperPort, target, usePreferred, 100000);
		if (ret < B_OK)
			fprintf(stderr, "ServerWindow::SendMessageToClient(): %s\n", strerror(ret));
	} else
		printf("PANIC: ServerWindow %s: can't flatten message in 'SendMessageToClient()'\n", fTitle);

	delete[] buffer;

	return ret;
}

// MakeWinBorder
WinBorder*
ServerWindow::MakeWinBorder(BRect frame, const char* name,
							uint32 look, uint32 feel, uint32 flags,
							uint32 workspace)
{
	// The non-offscreen ServerWindow uses the DrawingEngine instance from the desktop.
	return new(nothrow) WinBorder(frame, name, look, feel, flags,
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
ServerWindow::_HandleDirectConnection(int bufferState, int driverState)
{
	if (fDirectWindowData == NULL)
		return;
	
	if (bufferState != -1)
		fDirectWindowData->direct_info->buffer_state = (direct_buffer_state)bufferState;
	
	if (driverState != -1)
		fDirectWindowData->direct_info->driver_state = (direct_driver_state)driverState;
	
	if ((bufferState & B_DIRECT_MODE_MASK) != B_DIRECT_STOP) {
		// TODO: Locking ?
		RenderingBuffer *buffer = fDesktop->GetHWInterface()->FrontBuffer();
		fDirectWindowData->direct_info->bits = buffer->Bits();
		fDirectWindowData->direct_info->pci_bits = NULL; // TODO	
		fDirectWindowData->direct_info->bytes_per_row = buffer->BytesPerRow();
		fDirectWindowData->direct_info->bits_per_pixel = buffer->BytesPerRow() / buffer->Width() * 8;
		fDirectWindowData->direct_info->pixel_format = buffer->ColorSpace();
		fDirectWindowData->direct_info->layout = B_BUFFER_NONINTERLEAVED;
		fDirectWindowData->direct_info->orientation = B_BUFFER_TOP_TO_BOTTOM; // TODO
		
		WinBorder *border = const_cast<WinBorder *>(GetWinBorder());
		fDirectWindowData->direct_info->window_bounds = to_clipping_rect(border->Frame());
		
		// TODO: Review this
		const int32 kMaxClipRectsCount = (B_PAGE_SIZE - sizeof(direct_buffer_info)) / sizeof(clipping_rect);
	
		// TODO: Is there a simpler way to obtain this result ?
		// We just want the region INSIDE the window, border excluded.
		BRegion clipRegion = const_cast<BRegion &>(border->FullVisible());
		BRegion exclude = const_cast<BRegion &>(border->VisibleRegion());
		clipRegion.Exclude(&exclude);
		
		border->ConvertToTop(&clipRegion);
		fDirectWindowData->direct_info->clip_list_count = min_c(clipRegion.CountRects(), kMaxClipRectsCount);
		fDirectWindowData->direct_info->clip_bounds = clipRegion.FrameInt();
		
		for (uint32 i = 0; i < fDirectWindowData->direct_info->clip_list_count; i++)
			fDirectWindowData->direct_info->clip_list[i] = clipRegion.RectAtInt(i);
	}
	
	// Releasing this sem causes the client to call BDirectWindow::DirectConnected()
	release_sem(fDirectWindowData->direct_sem);
	
	// TODO: Waiting 3 seconds in this thread could cause weird things: test
	status_t status = acquire_sem_etc(fDirectWindowData->direct_sem_ack, 1, B_TIMEOUT, 3000000);
	if (status == B_TIMED_OUT) {
		// The client application didn't release the semaphore
		// within the given timeout. Deleting this member should make it crash.
		// TODO: Actually, it will not. At least, not always. Find a better way to 
		// crash the client application.
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
