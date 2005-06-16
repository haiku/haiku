//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerWindow.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@cotty.iren.ro>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Shadow BWindow class
//
//------------------------------------------------------------------------------

#include <AppDefs.h>
#include <Rect.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <View.h>
#include <Message.h>
#include <GraphicsDefs.h>
#include <PortLink.h>
#include <ViewAux.h>
#include "AppServer.h"
#include "BGet++.h"
#include "DebugInfoManager.h"
#include "Desktop.h"
#include "Layer.h"
#include "RAMLinkMsgReader.h"
#include "RootLayer.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "ServerProtocol.h"
#include "ServerPicture.h"
#include "WinBorder.h"
#include "TokenHandler.h"
#include "Utils.h"
#include "DisplayDriver.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "Workspace.h"
#include "MessagePrivate.h"

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

//	#pragma mark -

/*!
	\brief Constructor
	
	Does a lot of stuff to set up for the window - new decorator, new winborder, spawn a 
	monitor thread.
*/
ServerWindow::ServerWindow(const char *string, ServerApp *winapp,
	port_id winport, port_id looperPort, int32 handlerID)
	: fServerApp(winapp),
	// fClientWinPort is the port to which the app awaits messages from the server
	fClientWinPort(winport),
	fClientLooperPort(looperPort),
	fClientViewsWithInvalidCoords(B_VIEW_RESIZED),
	fHandlerToken(handlerID)
{
	STRACE(("ServerWindow(%s)::ServerWindow()\n", string));

	if (string) {
		strncpy(fName, string, sizeof(fName) - 1);
		fName[sizeof(fName) - 1] = '\0';
	} else
		strcpy(fName, "Unnamed Window");

	fClientTeamID = winapp->ClientTeamID();
	fWinBorder = NULL;
	fCurrentLayer = NULL;

	// fMessagePort is the port to which the app sends messages for the server
	fMessagePort = create_port(30, fName);

	fMsgSender = new BPrivate::LinkSender(fClientWinPort);
	fMsgReceiver = new BPrivate::LinkReceiver(fMessagePort);

	// Send a reply to our window - it is expecting fMessagePort port.
	fMsgSender->StartMessage(SERVER_TRUE);
	fMsgSender->Attach<port_id>(fMessagePort);
	fMsgSender->Flush();

	STRACE(("ServerWindow %s Created\n", fName));
}


void
ServerWindow::Init(BRect frame, uint32 wlook, 
	uint32 wfeel, uint32 wflags, uint32 wwksindex)
{
	char name[60];
	snprintf(name, sizeof(name), "%ld: %s", fClientTeamID, fName);

	fWinBorder = new WinBorder(frame, name, wlook, wfeel, wflags,
		wwksindex, this, gDesktop->GetDisplayDriver());

	// Spawn our message-monitoring thread
	fMonitorThreadID = spawn_thread(MonitorWin, fName, B_NORMAL_PRIORITY, this);
	if (fMonitorThreadID >= B_OK)
		resume_thread(fMonitorThreadID);
}


//!Tears down all connections the main app_server objects, and deletes some internals.
ServerWindow::~ServerWindow(void)
{
	STRACE(("*ServerWindow (%s):~ServerWindow()\n", fName));

	delete fWinBorder;
	delete fMsgSender;
	delete fMsgReceiver;

	STRACE(("#ServerWindow(%s) will exit NOW\n", fName));
}

//! Forces the window border to update its decorator
void
ServerWindow::ReplaceDecorator(void)
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::ReplaceDecorator()\n");

	STRACE(("ServerWindow %s: Replace Decorator\n",fName));
	fWinBorder->UpdateDecorator();
}

//! Requests that the ServerWindow's BWindow quit
void
ServerWindow::Quit(void)
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Quit\n",fName));

	BMessage msg(B_QUIT_REQUESTED);
	SendMessageToClient(&msg);
}

//! Shows the window's WinBorder
void
ServerWindow::Show(void)
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Show\n",fName));

	if (!fWinBorder->IsHidden())
		return;

	fWinBorder->GetRootLayer()->ShowWinBorder(fWinBorder);
}

//! Hides the window's WinBorder
void
ServerWindow::Hide(void)
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	STRACE(("ServerWindow %s: Hide\n",fName));

	if (fWinBorder->IsHidden())
		return;

	fWinBorder->GetRootLayer()->HideWinBorder(fWinBorder);
}


void
ServerWindow::Minimize(bool status)
{
	// NOTE: if you do something else, other than sending a port message, PLEASE lock
	// This function doesn't need much -- check to make sure that we should and
	// send the message to the client. According to the BeBook, the BWindow hook function
	// does all the heavy lifting for us. :)
	bool sendMessages = false;

	if (status) {
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
		BMessage msg;
		msg.what = B_MINIMIZE;
		msg.AddInt64("when", real_time_clock_usecs());
		msg.AddBool("minimize", status);

		SendMessageToClient(&msg);
	}
}

//! Sends a message to the client to perform a Zoom
void
ServerWindow::Zoom()
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
ServerWindow::ScreenModeChanged(const BRect frame, const color_space colorSpace)
{
	STRACE(("ServerWindow %s: ScreenModeChanged\n", fName));

	BMessage msg(B_SCREEN_CHANGED);
	msg.AddRect("frame", frame);
	msg.AddInt32("mode", (int32)colorSpace);

	SendMessageToClient(&msg);
}

/*!
	\brief Locks the window
	\return B_OK if everything is ok, B_ERROR if something went wrong
*/
status_t
ServerWindow::Lock()
{
	STRACE(("\nServerWindow %s: Lock\n", fName));

	return fLocker.Lock() ? B_OK : B_ERROR;
}

//! Unlocks the window
void
ServerWindow::Unlock()
{
	STRACE(("ServerWindow %s: Unlock\n\n", fName));

	fLocker.Unlock();
}

/*!
	\brief Determines whether or not the window is locked
	\return True if locked, false if not.
*/
bool
ServerWindow::IsLocked() const
{
	return fLocker.IsLocked();
}

/*!
	\brief Sets the font state for a layer
	\param layer The layer to set the font
*/
inline void
ServerWindow::SetLayerFontState(Layer *layer, BPrivate::LinkReceiver &link)
{
	STRACE(("ServerWindow %s: SetLayerFontStateMessage for layer %s\n",
			fName, layer->fName->String()));
	// NOTE: no need to check for a lock. This is a private method.

	layer->fLayerData->ReadFontFromLink(link);
}


inline void
ServerWindow::SetLayerState(Layer *layer, BPrivate::LinkReceiver &link)
{
	STRACE(("ServerWindow %s: SetLayerState for layer %s\n",fName,
			 layer->fName->String()));
	// NOTE: no need to check for a lock. This is a private method.

	layer->fLayerData->ReadFromLink(link);
	// TODO: Rebuild clipping here?
}


inline Layer*
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
		fName, name, token));

	Layer *newLayer = new Layer(frame, name, token, resizeMask, 
			flags, gDesktop->GetDisplayDriver());

	free(name);

	// there is no way of setting this, other than manually :-)
	newLayer->fViewColor = viewColor;
	newLayer->fHidden = hidden;
	newLayer->fEventMask = eventMask;
	newLayer->fEventOptions = eventOptions;
	newLayer->fOwner = fWinBorder;

	if (_parent) {
		Layer *parent = fWinBorder->FindLayer(parentToken);
		if (parent == NULL)
			CRITICAL("View token not found!\n");

		*_parent = parent;
	}

	return newLayer;
}


void
ServerWindow::DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
	if (fCurrentLayer == NULL && code != AS_LAYER_CREATE_ROOT && code != AS_LAYER_CREATE) {
		printf("ServerWindow %s received unexpected code - message offset %ld before top_view attached.\n",fName, code - SERVER_TRUE);
		return;
	}

	RootLayer *myRootLayer = fWinBorder->GetRootLayer();

	switch (code) {
		//--------- BView Messages -----------------
		case AS_LAYER_SCROLL:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SCROLL: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
			float dh;
			float dv;

			link.Read<float>(&dh);
			link.Read<float>(&dv);

			// scroll visually by using the CopyBits() implementation
			// this will also take care of invalidating previously invisible
			// areas (areas scrolled into view)
			BRect src = fCurrentLayer->Bounds();
			BRect dst = src;
			// NOTE: if we scroll down, the contents are moved *up*
			dst.OffsetBy(-dh, -dv);

			// TODO: Are origin and scale handled in this conversion?
			src = fCurrentLayer->ConvertToTop(src);
			dst = fCurrentLayer->ConvertToTop(dst);

			int32 xOffset = (int32)(dst.left - src.left);
			int32 yOffset = (int32)(dst.top - src.top);

			// this little detail is where it differs from CopyBits()
			// -> it will invalidate areas previously out of screen
			dst = dst | src;

			_CopyBits(myRootLayer, fCurrentLayer, src, dst, xOffset, yOffset);

			fCurrentLayer->fLayerData->OffsetOrigin(BPoint(dh, dv));

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

			_CopyBits(myRootLayer, fCurrentLayer, src, dst, xOffset, yOffset);

			break;
		}
		case AS_SET_CURRENT_LAYER:
		{
			int32 token;
			
			link.Read<int32>(&token);

			Layer *current = fWinBorder->FindLayer(token);
			if (current) {
				DTRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: %s, token %ld\n", fName, current->fName->String(), token));
			} else {
				DTRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: layer not found, token %ld\n", fName, token));
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
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE_ROOT\n", fName));

			// Start receiving top_view data -- pass NULL as the parent view.
			// This should be the *only* place where this happens.
			if (fCurrentLayer != NULL)
				break;

			fWinBorder->fTopLayer = CreateLayerTree(link, NULL);
			fWinBorder->fTopLayer->SetAsTopLayer(true);
			fCurrentLayer = fWinBorder->fTopLayer;

			// connect decorator and top layer.
			fWinBorder->AddChild(fWinBorder->fTopLayer, NULL);
			break;
		}

		case AS_LAYER_CREATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE: Layer name: %s\n", fName, fCurrentLayer->fName->String()));

			Layer* parent = NULL;
			Layer* newLayer = CreateLayerTree(link, &parent);
			if (parent != NULL)
				parent->AddChild(newLayer, this);

			if (!newLayer->IsHidden()) {
				myRootLayer->GoInvalidate(newLayer,
#ifndef NEW_CLIPPING
				newLayer->fFull
#else
				newLayer->Frame()
#endif
				);
			}
			break;
		}
		case AS_LAYER_DELETE:
		{
			// Received when a view is detached from a window. This is definitely
			// the less taxing operation - we call PruneTree() on the removed
			// layer, detach the layer itself, delete it, and invalidate the
			// area assuming that the view was visible when removed

			STRACE(("ServerWindow %s: AS_LAYER_DELETE(self)...\n", fName));			
			Layer *parent;
			parent = fCurrentLayer->fParent;
			
			// here we remove current layer from list.
			fCurrentLayer->RemoveSelf();
			fCurrentLayer->PruneTree();
			
			if (parent)
				myRootLayer->GoInvalidate(parent, BRegion(fCurrentLayer->Frame()));
			
			#ifdef DEBUG_SERVERWINDOW
			parent->PrintTree();
			#endif
			STRACE(("DONE: ServerWindow %s: Message AS_DELETE_LAYER: Parent: %s Layer: %s\n", fName, parent->fName->String(), fCurrentLayer->fName->String()));

			delete fCurrentLayer;

			fCurrentLayer = parent;
			break;
		}
		case AS_LAYER_SET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_STATE: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
//			SetLayerState(fCurrentLayer);
			SetLayerState(fCurrentLayer, link);
			// TODO: should this be moved into SetLayerState?
			// If it _always_ needs to be done afterwards, then yes!
#ifndef NEW_CLIPPING
			fCurrentLayer->RebuildFullRegion();
#endif
			break;
		}
		case AS_LAYER_SET_FONT_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
//			SetLayerFontState(fCurrentLayer);
			SetLayerFontState(fCurrentLayer, link);
#ifndef NEW_CLIPPING
			fCurrentLayer->RebuildFullRegion();
#endif
			break;
		}
		case AS_LAYER_GET_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_STATE: Layer name: %s\n", fName, fCurrentLayer->fName->String()));

			fMsgSender->StartMessage(SERVER_TRUE);

			// attach state data
			fCurrentLayer->fLayerData->WriteToLink(*fMsgSender);

			fMsgSender->Attach<float>(fCurrentLayer->fFrame.left);
			fMsgSender->Attach<float>(fCurrentLayer->fFrame.top);
			fMsgSender->Attach<BRect>(fCurrentLayer->fFrame.OffsetToCopy(fCurrentLayer->BoundsOrigin()));

			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_SET_MOUSE_EVENT_MASK:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_MOUSE_EVENT_MASK: Layer name: %s\n", fName, fCurrentLayer->fName->String()));			

			uint32		mask;
			uint32		options;

			link.Read<uint32>(&mask);
			link.Read<uint32>(&options);

			myRootLayer->SetEventMaskLayer(fCurrentLayer, mask, options);

			break;
		}
		case AS_LAYER_MOVETO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_MOVETO: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
			float x, y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);

			float offsetX = x - fCurrentLayer->fFrame.left;
			float offsetY = y - fCurrentLayer->fFrame.top;

			fCurrentLayer->MoveBy(offsetX, offsetY);
			
			break;
		}
		case AS_LAYER_RESIZETO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZETO: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
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
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			// our offset in the parent -> will be originX and originY in BView
			fMsgSender->Attach<float>(fCurrentLayer->fFrame.left);
			fMsgSender->Attach<float>(fCurrentLayer->fFrame.top);
			// convert frame to bounds
			fMsgSender->Attach<BRect>(fCurrentLayer->fFrame.OffsetToCopy(fCurrentLayer->BoundsOrigin()));
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_SET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_ORIGIN: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			float x, y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);
			
			fCurrentLayer->fLayerData->SetOrigin(BPoint(x, y));

			break;
		}
		case AS_LAYER_GET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_ORIGIN: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<BPoint>(fCurrentLayer->fLayerData->Origin());
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_RESIZE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_MODE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			link.Read<uint32>(&(fCurrentLayer->fResizeMode));
			
			break;
		}
		case AS_LAYER_CURSOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_CURSOR: Layer: %s - NOT IMPLEMENTED\n",fName, fCurrentLayer->fName->String()));
			int32 token;

			link.Read<int32>(&token);

			// TODO: implement; I think each Layer should have a member pointing
			// to this requested cursor.
			
			break;
		}
		case AS_LAYER_SET_FLAGS:
		{
			link.Read<uint32>(&(fCurrentLayer->fFlags));
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FLAGS: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			break;
		}
		case AS_LAYER_HIDE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_HIDE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fCurrentLayer->Hide();
			break;
		}
		case AS_LAYER_SHOW:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SHOW: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fCurrentLayer->Show();
			break;
		}
		case AS_LAYER_SET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_LINE_MODE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			int8 lineCap, lineJoin;
			float miterLimit;

			// TODO: Look into locking scheme relating to Layers and modifying redraw-related members

			link.Read<int8>(&lineCap);
			link.Read<int8>(&lineJoin);
			link.Read<float>(&miterLimit);
			
			fCurrentLayer->fLayerData->SetLineCapMode((cap_mode)lineCap);
			fCurrentLayer->fLayerData->SetLineJoinMode((join_mode)lineJoin);
			fCurrentLayer->fLayerData->SetMiterLimit(miterLimit);
		
			break;
		}
		case AS_LAYER_GET_LINE_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_LINE_MODE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<int8>((int8)(fCurrentLayer->fLayerData->LineCapMode()));
			fMsgSender->Attach<int8>((int8)(fCurrentLayer->fLayerData->LineJoinMode()));
			fMsgSender->Attach<float>(fCurrentLayer->fLayerData->MiterLimit());
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_PUSH_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_PUSH_STATE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			
			fCurrentLayer->PushState();
#ifndef NEW_CLIPPING
			fCurrentLayer->RebuildFullRegion();
#endif
			break;
		}
		case AS_LAYER_POP_STATE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_POP_STATE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			
			fCurrentLayer->PopState();
#ifndef NEW_CLIPPING
			fCurrentLayer->RebuildFullRegion();
#endif
			break;
		}
		case AS_LAYER_SET_SCALE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			float scale;
			link.Read<float>(&scale);
			// TODO: The BeBook says, if you call SetScale() it will be
			// multiplied with the scale from all previous states on the stack
			fCurrentLayer->fLayerData->SetScale(scale);
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_SCALE: Layer: %s\n",fName, fCurrentLayer->fName->String()));		
			LayerData		*ld = fCurrentLayer->fLayerData;

			// TODO: And here, we're taking that into account, but not above
			// -> refactor put scale into Layer, or better yet, when the
			// state stack is within Layer, PushState() should multiply
			// by the previous last states scale. Would fix the problem above too.
			float			scale = ld->Scale();
			
			while ((ld = ld->prevState))
				scale *= ld->Scale();
			
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<float>(scale);
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_LOC:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_LOC: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			float		x, y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);

			fCurrentLayer->fLayerData->SetPenLocation(BPoint(x, y));

			break;
		}
		case AS_LAYER_GET_PEN_LOC:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_LOC: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<BPoint>(fCurrentLayer->fLayerData->PenLocation());
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_SIZE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			float penSize;
			link.Read<float>(&penSize);
			fCurrentLayer->fLayerData->SetPenSize(penSize);
		
			break;
		}
		case AS_LAYER_GET_PEN_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_SIZE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<float>(fCurrentLayer->fLayerData->PenSize());
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			fCurrentLayer->SetViewColor(RGBColor(c));

			// TODO: this should not trigger redraw, no?!?
#ifndef NEW_CLIPPING
			myRootLayer->GoRedraw(fCurrentLayer, fCurrentLayer->fVisible);
#endif
			break;
		}
		case AS_LAYER_GET_COLORS:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_COLORS: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			rgb_color highColor, lowColor, viewColor;
			
			highColor = fCurrentLayer->fLayerData->HighColor().GetColor32();
			lowColor = fCurrentLayer->fLayerData->LowColor().GetColor32();
			viewColor = fCurrentLayer->ViewColor().GetColor32();
			
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach(&highColor, sizeof(rgb_color));
			fMsgSender->Attach(&lowColor, sizeof(rgb_color));
			fMsgSender->Attach(&viewColor, sizeof(rgb_color));
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_SET_BLEND_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_BLEND_MODE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			int8 srcAlpha, alphaFunc;
			
			link.Read<int8>(&srcAlpha);
			link.Read<int8>(&alphaFunc);
			
			fCurrentLayer->fLayerData->SetBlendingMode((source_alpha)srcAlpha,
											(alpha_function)alphaFunc);

			break;
		}
		case AS_LAYER_GET_BLEND_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_BLEND_MODE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<int8>((int8)(fCurrentLayer->fLayerData->AlphaSrcMode()));
			fMsgSender->Attach<int8>((int8)(fCurrentLayer->fLayerData->AlphaFncMode()));
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_SET_DRAW_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_DRAW_MODE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			int8 drawingMode;
			
			link.Read<int8>(&drawingMode);
			
			fCurrentLayer->fLayerData->SetDrawingMode((drawing_mode)drawingMode);
			
			break;
		}
		case AS_LAYER_GET_DRAW_MODE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_DRAW_MODE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<int8>((int8)(fCurrentLayer->fLayerData->GetDrawingMode()));
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_PRINT_ALIASING:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_PRINT_ALIASING: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			bool fontAliasing;
			link.Read<bool>(&fontAliasing);
			fCurrentLayer->fLayerData->SetFontAntiAliasing(!fontAliasing);
			
			break;
		}
		case AS_LAYER_CLIP_TO_PICTURE:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_PICTURE: Layer: %s\n",fName, fCurrentLayer->fName->String()));
		// TODO: you are not allowed to use Layer regions here!!!
		// If there is no other way, then first lock RootLayer object first.
			
			// TODO: Watch out for the coordinate system in AS_LAYER_CLIP_TO_PICTURE
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
				
			fCurrentLayer->fLayerData->SetClippingRegion(region);

#ifndef NEW_CLIPPING
			fCurrentLayer->RebuildFullRegion();
#endif
			if (!(fCurrentLayer->IsHidden()))
				myRootLayer->GoInvalidate(fCurrentLayer,
#ifndef NEW_CLIPPING
				fCurrentLayer->fFull
#else
				fCurrentLayer->Frame()
#endif
				);
				
			break;
		}
		
		case AS_LAYER_GET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_GET_CLIP_REGION: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			
			// if this Layer is hidden, it is clear that its visible region is void.
			if (fCurrentLayer->IsHidden())
			{
				fMsgSender->StartMessage(SERVER_TRUE);
				fMsgSender->Attach<int32>(0L);
				fMsgSender->Flush();
			}
			else
			{
				// TODO: Watch out for the coordinate system in AS_LAYER_GET_CLIP_REGION
				BRegion reg;
				LayerData *ld;
				int32 noOfRects;
			
				ld = fCurrentLayer->fLayerData;
#ifndef NEW_CLIPPING
				reg = fCurrentLayer->ConvertFromParent(&(fCurrentLayer->fVisible));
#endif
				if (ld->ClippingRegion())
					reg.IntersectWith(ld->ClippingRegion());
			
				// TODO: This could also be done more reliably in the Layer,
				// when the State stack is implemented there. There should be
				// DrawData::fCulmulatedClippingRegion...
				// TODO: the DrawData clipping region should be in local view coords.
				while ((ld = ld->prevState)) {
					if (ld->ClippingRegion())
						reg.IntersectWith(ld->ClippingRegion());
				}
			
				noOfRects = reg.CountRects();
				fMsgSender->StartMessage(SERVER_TRUE);
				fMsgSender->Attach<int32>(noOfRects);

				for(int i = 0; i < noOfRects; i++)
					fMsgSender->Attach<BRect>(reg.RectAt(i));
			}
			break;
		}
		case AS_LAYER_SET_CLIP_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_CLIP_REGION: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			
			// TODO: Watch out for the coordinate system in AS_LAYER_SET_CLIP_REGION
			int32 noOfRects;
			BRect r;
			
			link.Read<int32>(&noOfRects);

			BRegion region;
			for(int i = 0; i < noOfRects; i++)
			{
				link.Read<BRect>(&r);
				region.Include(r);
			}
			fCurrentLayer->fLayerData->SetClippingRegion(region);

#ifndef NEW_CLIPPING
			fCurrentLayer->RebuildFullRegion();
#endif
			if (!(fCurrentLayer->IsHidden()))
				myRootLayer->GoInvalidate(fCurrentLayer,
#ifndef NEW_CLIPPING
				fCurrentLayer->fFull
#else
				fCurrentLayer->Frame()
#endif
				);

			break;
		}
		case AS_LAYER_INVAL_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			
			// TODO: handle transformation (origin and scale) prior to converting to top
			BRect		invalRect;
			
			link.Read<BRect>(&invalRect);
			BRect converted(fCurrentLayer->ConvertToTop(invalRect.LeftTop()),
							fCurrentLayer->ConvertToTop(invalRect.RightBottom()));
			BRegion invalidRegion(converted);
//			invalidRegion.IntersectWith(&fCurrentLayer->fVisible);

			myRootLayer->GoRedraw(fWinBorder, invalidRegion);
//			myRootLayer->RequestDraw(invalidRegion, fWinBorder);
			break;
		}
		case AS_LAYER_INVAL_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			
			// TODO: handle transformation (origin and scale) prior to converting to top
			// TODO: Handle conversion to top
			BRegion invalReg;
			int32 noOfRects;
			BRect rect;
			
			link.Read<int32>(&noOfRects);
			
			for(int i = 0; i < noOfRects; i++)
			{
				link.Read<BRect>(&rect);
				invalReg.Include(rect);
			}
			
			myRootLayer->GoRedraw(fCurrentLayer, invalReg);

			break;
		}
		case AS_BEGIN_UPDATE:
		{
			DTRACE(("ServerWindowo %s: AS_BEGIN_UPDATE\n",fName));
			fWinBorder->GetRootLayer()->Lock();
			fWinBorder->UpdateStart();
			fWinBorder->GetRootLayer()->Unlock();
			break;
		}
		case AS_END_UPDATE:
		{
			DTRACE(("ServerWindowo %s: AS_END_UPDATE\n",fName));
			fWinBorder->GetRootLayer()->Lock();
			fWinBorder->UpdateEnd();
			fWinBorder->GetRootLayer()->Unlock();
			break;
		}

		// ********** END: BView Messages ***********
		
		// ********* BWindow Messages ***********
		case AS_LAYER_DELETE_ROOT:
		{
			// Received when a window deletes its internal top view
			
			// TODO: Implement AS_LAYER_DELETE_ROOT
			STRACE(("ServerWindow %s: Message Delete_Layer_Root unimplemented\n",fName));
			break;
		}
		case AS_SHOW_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_SHOW_WINDOW\n",fName));
			Show();
			break;
		}
		case AS_HIDE_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_HIDE_WINDOW\n",fName));		
			Hide();
			break;
		}
		case AS_SEND_BEHIND:
		{
			// TODO: Implement AS_SEND_BEHIND
			STRACE(("ServerWindow %s: Message  Send_Behind unimplemented\n",fName));
			break;
		}
		case AS_ENABLE_UPDATES:
		{
			// TODO: Implement AS_ENABLE_UPDATES
			STRACE(("ServerWindow %s: Message Enable_Updates unimplemented\n",fName));
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			// TODO: Implement AS_DISABLE_UPDATES
			STRACE(("ServerWindow %s: Message Disable_Updates unimplemented\n",fName));
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			// TODO: Implement AS_NEEDS_UPDATE
			STRACE(("ServerWindow %s: Message Needs_Update unimplemented\n",fName));
			break;
		}
		case AS_WINDOW_TITLE:
		{
			// TODO: Implement AS_WINDOW_TITLE
			STRACE(("ServerWindow %s: Message Set_Title unimplemented\n",fName));
			break;
		}
		case AS_ADD_TO_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_ADD_TO_SUBSET\n",fName));
			WinBorder *wb;
			int32 mainToken;
			team_id	teamID;

			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));

			wb = gDesktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if (wb) {
				fMsgSender->StartMessage(SERVER_TRUE);
				fMsgSender->Flush();

				// ToDo: this is a pretty expensive and complicated way to send a message...
				BPrivate::PortLink msg(-1, -1);
				msg.StartMessage(AS_ROOTLAYER_ADD_TO_SUBSET);
				msg.Attach<WinBorder*>(fWinBorder);
				msg.Attach<WinBorder*>(wb);
				fWinBorder->GetRootLayer()->EnqueueMessage(msg);
			} else {
				fMsgSender->StartMessage(SERVER_FALSE);
				fMsgSender->Flush();
			}
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_REM_FROM_SUBSET\n",fName));
			WinBorder *wb;
			int32 mainToken;
			team_id teamID;

			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));
			
			wb = gDesktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if (wb) {
				fMsgSender->StartMessage(SERVER_TRUE);
				fMsgSender->Flush();

				BPrivate::PortLink msg(-1, -1);
				msg.StartMessage(AS_ROOTLAYER_REMOVE_FROM_SUBSET);
				msg.Attach<WinBorder*>(fWinBorder);
				msg.Attach<WinBorder*>(wb);
				fWinBorder->GetRootLayer()->EnqueueMessage(msg);
			} else {
				fMsgSender->StartMessage(SERVER_FALSE);
				fMsgSender->Flush();
			}
			break;
		}
		case AS_SET_LOOK:
		{
			// TODO: Implement AS_SET_LOOK
			STRACE(("ServerWindow %s: Message Set_Look unimplemented\n",fName));
			break;
		}
		case AS_SET_FLAGS:
		{
			// TODO: Implement AS_SET_FLAGS
			STRACE(("ServerWindow %s: Message Set_Flags unimplemented\n",fName));
			break;
		}
		case AS_SET_FEEL:
		{
			STRACE(("ServerWindow %s: Message AS_SET_FEEL\n",fName));
			int32 newFeel;
			link.Read<int32>(&newFeel);
			myRootLayer->GoChangeWinBorderFeel(fWinBorder, newFeel);
			break;
		}
		case AS_SET_ALIGNMENT:
		{
			// TODO: Implement AS_SET_ALIGNMENT
			STRACE(("ServerWindow %s: Message Set_Alignment unimplemented\n",fName));
			break;
		}
		case AS_GET_ALIGNMENT:
		{
			// TODO: Implement AS_GET_ALIGNMENT
			STRACE(("ServerWindow %s: Message Get_Alignment unimplemented\n",fName));
			break;
		}
		case AS_GET_WORKSPACES:
		{
			STRACE(("ServerWindow %s: Message Get_Workspaces unimplemented\n",fName));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<uint32>(fWinBorder->Workspaces());
			fMsgSender->Flush();
			break;
		}
		case AS_SET_WORKSPACES:
		{
			// TODO: Implement AS_SET_WORKSPACES
			STRACE(("ServerWindow %s: Message Set_Workspaces unimplemented\n",fName));
			uint32 newWorkspaces;
			link.Read<uint32>(&newWorkspaces);

			BPrivate::PortLink msg(-1, -1);
			msg.StartMessage(AS_ROOTLAYER_WINBORDER_SET_WORKSPACES);
			msg.Attach<WinBorder*>(fWinBorder);
			msg.Attach<uint32>(fWinBorder->Workspaces());
			msg.Attach<uint32>(newWorkspaces);
			fWinBorder->GetRootLayer()->EnqueueMessage(msg);
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			float xResizeBy;
			float yResizeBy;
			
			link.Read<float>(&xResizeBy);
			link.Read<float>(&yResizeBy);

			STRACE(("ServerWindow %s: Message AS_WINDOW_RESIZE %.1f, %.1f\n",fName, xResizeBy, yResizeBy));
			
			fWinBorder->ResizeBy(xResizeBy, yResizeBy);
			
			break;
		}
		case AS_WINDOW_MOVE:
		{
			float xMoveBy;
			float yMoveBy;
			
			link.Read<float>(&xMoveBy);
			link.Read<float>(&yMoveBy);

			STRACE(("ServerWindow %s: Message AS_WINDOW_MOVE: %.1f, %.1f\n",fName, xMoveBy, yMoveBy));

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

			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<float>(minWidth);
			fMsgSender->Attach<float>(maxWidth);
			fMsgSender->Attach<float>(minHeight);
			fMsgSender->Attach<float>(maxHeight);

			fMsgSender->Flush();

			break;
		}
		case B_MINIMIZE:
		{
			// TODO: Implement B_MINIMIZE
			STRACE(("ServerWindow %s: Message Minimize unimplemented\n",fName));
			break;
		}
		case B_WINDOW_ACTIVATED:
		{
			// TODO: Implement B_WINDOW_ACTIVATED
			STRACE(("ServerWindow %s: Message Window_Activated unimplemented\n",fName));
			break;
		}
		case B_ZOOM:
		{
			// TODO: Implement B_ZOOM
			STRACE(("ServerWindow %s: Message Zoom unimplemented\n",fName));
			break;
		}
		// Some BView drawing messages, but which don't need clipping
		case AS_LAYER_SET_HIGH_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_HIGH_COLOR: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			fCurrentLayer->fLayerData->SetHighColor(RGBColor(c));

			break;
		}
		case AS_LAYER_SET_LOW_COLOR:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_LOW_COLOR: Layer: %s\n",fName, fCurrentLayer->fName->String()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			fCurrentLayer->fLayerData->SetLowColor(RGBColor(c));
			
			break;
		}
		case AS_LAYER_SET_PATTERN:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_SET_PATTERN: Layer: %s\n", fName, fCurrentLayer->fName->String()));
			pattern pat;
			
			link.Read(&pat, sizeof(pattern));
			
			fCurrentLayer->fLayerData->SetPattern(Pattern(pat));
			
			break;
		}	
		case AS_MOVEPENTO:
		{
			DTRACE(("ServerWindow %s: Message AS_MOVEPENTO\n",fName));
			
			float x,y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				fCurrentLayer->fLayerData->SetPenLocation(BPoint(x, y));
			
			break;
		}
		case AS_SETPENSIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_SETPENSIZE\n",fName));
			float size;
			
			link.Read<float>(&size);
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				fCurrentLayer->fLayerData->SetPenSize(size);
			
			break;
		}
		case AS_SET_FONT:
		{
			DTRACE(("ServerWindow %s: Message AS_SET_FONT\n",fName));
			// TODO: Implement AS_SET_FONT?
			// Confusing!! But it works already!
			break;
		}
		case AS_SET_FONT_SIZE:
		{
			DTRACE(("ServerWindow %s: Message AS_SET_FONT_SIZE\n",fName));
			// TODO: Implement AS_SET_FONT_SIZE?
			break;
		}
		case AS_AREA_MESSAGE:
		{
			STRACE(("ServerWindow %s: Message AS_AREA_MESSAGE\n",fName));
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
			
			msgpointer=(int8*)ai.address + offset;
			
			RAMLinkMsgReader mlink(msgpointer);
			DispatchMessage(mlink.Code(),mlink);
			
			// This is a very special case in the sense that when ServerMemIO is used for this 
			// purpose, it will be set to NOT automatically free the memory which it had 
			// requested. This is the server's job once the message has been dispatched.
			fServerApp->AppAreaPool()->ReleaseBuffer(msgpointer);
			
			break;
		}
		case AS_SYNC:
		{
			// TODO: AS_SYNC is a no-op for now, just to get things working
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Flush();
			break;
		}
		case AS_LAYER_DRAG_IMAGE:
		{
			// TODO: Implement AS_LAYER_DRAG_IMAGE
			STRACE(("ServerWindow %s: Message AS_DRAG_IMAGE unimplemented\n",fName));
			DTRACE(("ServerWindow %s: Message AS_DRAG_IMAGE unimplemented\n",fName));
			break;
		}
		case AS_LAYER_DRAG_RECT:
		{
			// TODO: Implement AS_LAYER_DRAG_RECT
			STRACE(("ServerWindow %s: Message AS_DRAG_RECT unimplemented\n",fName));
			DTRACE(("ServerWindow %s: Message AS_DRAG_RECT unimplemented\n",fName));
			break;
		}
		case AS_LAYER_GET_MOUSE_COORDS:
		{
			DTRACE(("ServerWindow %s: Message AS_GET_MOUSE_COORDS\n", fName));

			fMsgSender->StartMessage(SERVER_TRUE);

			// Returns
			// 1) BPoint mouse location
			// 2) int32 button state

			fMsgSender->Attach<BPoint>(gDesktop->GetDisplayDriver()->GetCursorPosition());
			fMsgSender->Attach<int32>(gDesktop->ActiveRootLayer()->Buttons());

			fMsgSender->Flush();
			break;
		}

		default:
			DispatchGraphicsMessage(code, link);
			break;
	}
}

		// -------------------- Graphics messages ----------------------------------

inline
void
ServerWindow::DispatchGraphicsMessage(int32 code, BPrivate::LinkReceiver &link)
{
	fWinBorder->GetRootLayer()->Lock();
	BRegion rreg
#ifndef NEW_CLIPPING
	(fCurrentLayer->fVisible)
#endif
	;
	if (fWinBorder->fInUpdate)
		rreg.IntersectWith(&fWinBorder->yUpdateReg);

	gDesktop->GetDisplayDriver()->ConstrainClippingRegion(&rreg);
//	rgb_color  rrr = fCurrentLayer->fLayerData->viewcolor.GetColor32();
//	RGBColor c(rand()%255,rand()%255,rand()%255);
//	gDesktop->GetDisplayDriver()->FillRect(BRect(0,0,639,479), c);

	switch (code) {
		case AS_STROKE_LINE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_LINE\n",fName));

			float x1, y1, x2, y2;

			link.Read<float>(&x1);
			link.Read<float>(&y1);
			link.Read<float>(&x2);
			link.Read<float>(&y2);

			if (fCurrentLayer && fCurrentLayer->fLayerData) {
				BPoint p1(x1,y1);
				BPoint p2(x2,y2);
				gDesktop->GetDisplayDriver()->StrokeLine(fCurrentLayer->ConvertToTop(p1),
														fCurrentLayer->ConvertToTop(p2),
														fCurrentLayer->fLayerData);
				
				// We update the pen here because many DisplayDriver calls which do not update the
				// pen position actually call StrokeLine

				// TODO: Decide where to put this, for example, it cannot be done
				// for DrawString(), also there needs to be a decision, if penlocation
				// is in View coordinates (I think it should be) or in screen coordinates.
				fCurrentLayer->fLayerData->SetPenLocation(p2);
			}
			break;
		}
		case AS_LAYER_INVERT_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_INVERT_RECT\n",fName));
			
			BRect rect;
			link.Read<BRect>(&rect);
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->InvertRect(fCurrentLayer->ConvertToTop(rect));
			break;
		}
		case AS_STROKE_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_RECT\n",fName));
			
			float left, top, right, bottom;
			link.Read<float>(&left);
			link.Read<float>(&top);
			link.Read<float>(&right);
			link.Read<float>(&bottom);
			BRect rect(left,top,right,bottom);
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->StrokeRect(fCurrentLayer->ConvertToTop(rect),fCurrentLayer->fLayerData);
			break;
		}
		case AS_FILL_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_RECT\n",fName));
			
			BRect rect;
			link.Read<BRect>(&rect);
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->FillRect(fCurrentLayer->ConvertToTop(rect),fCurrentLayer->fLayerData);
			break;
		}
		case AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
			int32		bitmapToken;
			BPoint 		point;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BPoint>(&point);
			
			ServerBitmap* sbmp = fServerApp->FindBitmap(bitmapToken);
			if (sbmp) {
				BRect src = sbmp->Bounds();
				BRect dst = src.OffsetToCopy(point);
				dst = fCurrentLayer->ConvertToTop(dst);

				fCurrentLayer->GetDisplayDriver()->DrawBitmap(sbmp, src, dst, fCurrentLayer->fLayerData);
			}
			
			// TODO: Adi -- shouldn't AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT sync with the client?
			break;
		}
		case AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
			int32		bitmapToken;
			BPoint 		point;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BPoint>(&point);
			
			ServerBitmap* sbmp = fServerApp->FindBitmap(bitmapToken);
			if (sbmp) {
				BRect src = sbmp->Bounds();
				BRect dst = src.OffsetToCopy(point);
				dst = fCurrentLayer->ConvertToTop(dst);

				fCurrentLayer->GetDisplayDriver()->DrawBitmap(sbmp, src, dst, fCurrentLayer->fLayerData);
			}
			break;
		}
		case AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
			int32 bitmapToken;
			BRect srcRect, dstRect;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&dstRect);
			link.Read<BRect>(&srcRect);
			
			ServerBitmap* sbmp = fServerApp->FindBitmap(bitmapToken);
			if (sbmp) {
				dstRect = fCurrentLayer->ConvertToTop(dstRect);

				fCurrentLayer->GetDisplayDriver()->DrawBitmap(sbmp, srcRect, dstRect, fCurrentLayer->fLayerData);
			}
			
			// TODO: Adi -- shouldn't AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT sync with the client?
			break;
		}
		case AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT:
		{
			DTRACE(("ServerWindow %s: Message AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT: Layer name: %s\n", fName, fCurrentLayer->fName->String()));
			int32 bitmapToken;
			BRect srcRect, dstRect;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&dstRect);
			link.Read<BRect>(&srcRect);
			
			ServerBitmap* sbmp = fServerApp->FindBitmap(bitmapToken);
			if (sbmp) {
				dstRect = fCurrentLayer->ConvertToTop(dstRect);

				fCurrentLayer->GetDisplayDriver()->DrawBitmap(sbmp, srcRect, dstRect, fCurrentLayer->fLayerData);
			}
			break;
		}
		case AS_STROKE_ARC:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_ARC\n",fName));
			
			float angle, span;
			BRect r;
			
			link.Read<BRect>(&r);
			link.Read<float>(&angle);
			link.Read<float>(&span);
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->StrokeArc(fCurrentLayer->ConvertToTop(r),angle,span,fCurrentLayer->fLayerData);
			break;
		}
		case AS_FILL_ARC:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_ARC\n",fName));
			
			float angle, span;
			BRect r;
			
			link.Read<BRect>(&r);
			link.Read<float>(&angle);
			link.Read<float>(&span);
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->FillArc(fCurrentLayer->ConvertToTop(r),angle,span,fCurrentLayer->fLayerData);
			break;
		}
		case AS_STROKE_BEZIER:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_BEZIER\n",fName));
			
			BPoint *pts;
			int i;
			pts = new BPoint[4];
			
			for (i=0; i<4; i++)
				link.Read<BPoint>(&(pts[i]));
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
			{
				for (i=0; i<4; i++)
					pts[i]=fCurrentLayer->ConvertToTop(pts[i]);
				
				gDesktop->GetDisplayDriver()->StrokeBezier(pts,fCurrentLayer->fLayerData);
			}
			delete [] pts;
			break;
		}
		case AS_FILL_BEZIER:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_BEZIER\n",fName));
			
			BPoint *pts;
			int i;
			pts = new BPoint[4];
			
			for (i=0; i<4; i++)
				link.Read<BPoint>(&(pts[i]));
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
			{
				for (i=0; i<4; i++)
					pts[i]=fCurrentLayer->ConvertToTop(pts[i]);
				
				gDesktop->GetDisplayDriver()->FillBezier(pts,fCurrentLayer->fLayerData);
			}
			delete [] pts;
			break;
		}
		case AS_STROKE_ELLIPSE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_ELLIPSE\n",fName));
			
			BRect rect;
			link.Read<BRect>(&rect);
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->StrokeEllipse(fCurrentLayer->ConvertToTop(rect),fCurrentLayer->fLayerData);
			break;
		}
		case AS_FILL_ELLIPSE:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_ELLIPSE\n",fName));
			
			BRect rect;
			link.Read<BRect>(&rect);
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->FillEllipse(fCurrentLayer->ConvertToTop(rect),fCurrentLayer->fLayerData);
			break;
		}
		case AS_STROKE_ROUNDRECT:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_ROUNDRECT\n",fName));
			
			BRect rect;
			float xrad,yrad;
			link.Read<BRect>(&rect);
			link.Read<float>(&xrad);
			link.Read<float>(&yrad);
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->StrokeRoundRect(fCurrentLayer->ConvertToTop(rect),xrad,yrad,fCurrentLayer->fLayerData);
			break;
		}
		case AS_FILL_ROUNDRECT:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_ROUNDRECT\n",fName));
			
			BRect rect;
			float xrad,yrad;
			link.Read<BRect>(&rect);
			link.Read<float>(&xrad);
			link.Read<float>(&yrad);
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->FillRoundRect(fCurrentLayer->ConvertToTop(rect),xrad,yrad,fCurrentLayer->fLayerData);
			break;
		}
		case AS_STROKE_TRIANGLE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_TRIANGLE\n",fName));
			
			BPoint pts[3];
			BRect rect;
			
			for (int i=0; i<3; i++)
				link.Read<BPoint>(&(pts[i]));
			
			link.Read<BRect>(&rect);
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
			{
				for(int i=0;i<3;i++)
					pts[i]=fCurrentLayer->ConvertToTop(pts[i]);
				
				gDesktop->GetDisplayDriver()->StrokeTriangle(pts,fCurrentLayer->ConvertToTop(rect),fCurrentLayer->fLayerData);
			}
			break;
		}
		case AS_FILL_TRIANGLE:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_TRIANGLE\n",fName));
			
			BPoint pts[3];
			BRect rect;
			
			for (int i=0; i<3; i++)
				link.Read<BPoint>(&(pts[i]));
			
			link.Read<BRect>(&rect);
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
			{
				for(int i=0;i<3;i++)
					pts[i]=fCurrentLayer->ConvertToTop(pts[i]);
				
				gDesktop->GetDisplayDriver()->FillTriangle(pts,fCurrentLayer->ConvertToTop(rect),fCurrentLayer->fLayerData);
			}
			break;
		}
// TODO: get rid of all this code duplication!!
		case AS_STROKE_POLYGON:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_POLYGON\n",fName));
			
			BRect polyframe;
			bool isclosed;
			int32 pointcount;
			BPoint *pointlist;
			
			link.Read<BRect>(&polyframe);
			link.Read<bool>(&isclosed);
			link.Read<int32>(&pointcount);
			
			pointlist=new BPoint[pointcount];
			
			link.Read(pointlist, sizeof(BPoint)*pointcount);
			
			for(int32 i=0; i<pointcount; i++)
				pointlist[i]=fCurrentLayer->ConvertToTop(pointlist[i]);
			
			gDesktop->GetDisplayDriver()->StrokePolygon(pointlist,pointcount,polyframe,
					fCurrentLayer->fLayerData,isclosed);
			
			delete [] pointlist;
			
			break;
		}
		case AS_FILL_POLYGON:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_POLYGON\n",fName));
			
			BRect polyframe;
			int32 pointcount;
			BPoint *pointlist;
			
			link.Read<BRect>(&polyframe);
			link.Read<int32>(&pointcount);
			
			pointlist=new BPoint[pointcount];
			
			link.Read(pointlist, sizeof(BPoint)*pointcount);
			
			for(int32 i=0; i<pointcount; i++)
				pointlist[i]=fCurrentLayer->ConvertToTop(pointlist[i]);
			
			gDesktop->GetDisplayDriver()->FillPolygon(pointlist,pointcount,polyframe,fCurrentLayer->fLayerData);
			
			delete [] pointlist;
			
			break;
		}
		case AS_STROKE_SHAPE:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_SHAPE\n",fName));
			
			BRect shaperect;
			int32 opcount;
			int32 ptcount;
			int32 *oplist;
			BPoint *ptlist;
			
			link.Read<BRect>(&shaperect);
			link.Read<int32>(&opcount);
			link.Read<int32>(&ptcount);
			
			oplist=new int32[opcount];
			ptlist=new BPoint[ptcount];
			
			link.Read(oplist,sizeof(int32)*opcount);
			link.Read(ptlist,sizeof(BPoint)*ptcount);
			
			for(int32 i=0; i<ptcount; i++)
				ptlist[i]=fCurrentLayer->ConvertToTop(ptlist[i]);
			
			gDesktop->GetDisplayDriver()->StrokeShape(shaperect, opcount, oplist, ptcount, ptlist, fCurrentLayer->fLayerData);
			delete oplist;
			delete ptlist;
			
			break;
		}
		case AS_FILL_SHAPE:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_SHAPE\n",fName));
			
			BRect shaperect;
			int32 opcount;
			int32 ptcount;
			int32 *oplist;
			BPoint *ptlist;
			
			link.Read<BRect>(&shaperect);
			link.Read<int32>(&opcount);
			link.Read<int32>(&ptcount);
			
			oplist=new int32[opcount];
			ptlist=new BPoint[ptcount];
			
			link.Read(oplist,sizeof(int32)*opcount);
			link.Read(ptlist,sizeof(BPoint)*ptcount);
			
			for(int32 i=0; i<ptcount; i++)
				ptlist[i]=fCurrentLayer->ConvertToTop(ptlist[i]);
			
			gDesktop->GetDisplayDriver()->FillShape(shaperect, opcount, oplist, ptcount, ptlist, fCurrentLayer->fLayerData);

			delete oplist;
			delete ptlist;
			
			break;
		}
		case AS_FILL_REGION:
		{
			DTRACE(("ServerWindow %s: Message AS_FILL_REGION\n",fName));
			
			int32 rectcount;
			BRect *rectlist;
			
			link.Read<int32>(&rectcount);
			
			rectlist=new BRect[rectcount];
			
			link.Read(rectlist, sizeof(BRect)*rectcount);
			
			// Between the client-side conversion to BRects from clipping_rects to the overhead
			// in repeatedly calling FillRect(), this is definitely in need of optimization. At
			// least it works for now. :)
			for(int32 i=0; i<rectcount; i++)
				gDesktop->GetDisplayDriver()->FillRect(fCurrentLayer->ConvertToTop(rectlist[i]),fCurrentLayer->fLayerData);
			
			delete [] rectlist;
			
			// TODO: create support for clipping_rect usage for faster BRegion display.
			// Tweaks to DisplayDriver are necessary along with conversion routines in Layer
			
			break;
		}
		case AS_STROKE_LINEARRAY:
		{
			DTRACE(("ServerWindow %s: Message AS_STROKE_LINEARRAY\n",fName));
			
			// Attached Data:
			// 1) int32 Number of lines in the array
			// 2) array of struct _array_data_ objects, as defined in ViewAux.h
			
			int32 linecount;
			
			link.Read<int32>(&linecount);
			if(linecount>0)
			{
				LineArrayData linedata[linecount], *index;
				
				for(int32 i=0; i<linecount; i++)
				{
					index=&linedata[i];
					
					link.Read<float>(&(index->pt1.x));
					link.Read<float>(&(index->pt1.y));
					link.Read<float>(&(index->pt2.x));
					link.Read<float>(&(index->pt2.y));
					link.Read<rgb_color>(&(index->color));
					
					index->pt1=fCurrentLayer->ConvertToTop(index->pt1);
					index->pt2=fCurrentLayer->ConvertToTop(index->pt2);
				}				
				gDesktop->GetDisplayDriver()->StrokeLineArray(linecount,linedata,fCurrentLayer->fLayerData);
			}
			break;
		}
		case AS_DRAW_STRING:
		{
			DTRACE(("ServerWindow %s: Message AS_DRAW_STRING\n",fName));
			char *string;
			int32 length;
			BPoint location;
			escapement_delta delta;
			
			link.Read<int32>(&length);
			link.Read<BPoint>(&location);
			link.Read<escapement_delta>(&delta);
			link.ReadString(&string);
			
			if (fCurrentLayer && fCurrentLayer->fLayerData)
				gDesktop->GetDisplayDriver()->DrawString(string, length,
														fCurrentLayer->ConvertToTop(location),
														fCurrentLayer->fLayerData);
			
			free(string);
			break;
		}
		default:
		{
			printf("ServerWindow %s received unexpected code - message offset %ld\n",fName, code - SERVER_TRUE);
			break;
		}
	}

	gDesktop->GetDisplayDriver()->ConstrainClippingRegion(NULL);
	fWinBorder->GetRootLayer()->Unlock();
}

/*!
	\brief Message-dispatching loop for the ServerWindow

	MonitorWin() watches the ServerWindow's message port and dispatches as necessary
	\param data The thread's ServerWindow
	\return Throwaway code. Always 0.
*/
int32
ServerWindow::MonitorWin(void *data)
{
	ServerWindow *win = (ServerWindow *)data;
	BPrivate::LinkReceiver *ses = win->fMsgReceiver;

	bool quitting = false;
	int32 code;
	status_t err = B_OK;

	while (!quitting) {
//		printf("info: ServerWindow::MonitorWin listening on port %ld.\n", win->fMessagePort);
		code = AS_CLIENT_DEAD;
		err = ses->GetNextMessage(code);
		if (err < B_OK)
			return err;

		win->Lock();

		switch (code) {
			case AS_DELETE_WINDOW:
			case AS_CLIENT_DEAD:
			{
				// this means the client has been killed
				STRACE(("ServerWindow %s received 'AS_CLIENT_DEAD/AS_DELETE_WINDOW' message code\n",win->Title()));

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
				if (!win->fWinBorder->IsHidden())
					CRITICAL("ServerWindow: a window must be hidden before it's deleted\n");

				delete win;
				//	rootLayer->Unlock();
				
				exit_thread(0);
				break;
			}
			case B_QUIT_REQUESTED:
			{
				STRACE(("ServerWindow %s received Quit request\n",win->Title()));
				win->Quit();
				break;
			}
			default:
			{
				win->DispatchMessage(code, *ses);
				break;
			}
		}

		win->Unlock();
	}
	return err;
}

// _CopyBits
void
ServerWindow::_CopyBits(RootLayer* rootLayer, Layer* layer,
						BRect& src, BRect& dst,
						int32 xOffset, int32 yOffset) const
{
	// NOTE: The correct behaviour is this:
	// * The region that is copied is the
	//   src rectangle, no matter if it fits
	//   into the dst rectangle. It is copied
	//   by the offset dst.LeftTop() - src.LeftTop()
	// * The dst rectangle is used for invalidation:
	//   Any area in the dst rectangle that could
	//   not be copied from src (because either the
	//   src rectangle was not big enough, or because there
	//   were parts cut off by the current layer clipping),
	//   are triggering BView::Draw() to be called
	//   and for these parts only.

#ifndef NEW_CLIPPING

	// the region that is going to be copied
	BRegion copyRegion(src);
	// apply the current clipping of the layer

	copyRegion.IntersectWith(&layer->fVisible);

	// offset the region to the destination
	// and apply the current clipping there as well
	copyRegion.OffsetBy(xOffset, yOffset);
	copyRegion.IntersectWith(&layer->fVisible);

	// the region at the destination that needs invalidation
	BRegion invalidRegion(dst);
	// exclude the region drawn by the copy operation
	invalidRegion.Exclude(&copyRegion);
	// apply the current clipping as well
	invalidRegion.IntersectWith(&layer->fVisible);

	// move the region back for the actual operation
	copyRegion.OffsetBy(-xOffset, -yOffset);

	layer->GetDisplayDriver()->CopyRegion(&copyRegion, xOffset, yOffset);

	// trigger the redraw			
//	rootLayer->GoRedraw(fWinBorder, invalidRegion);
rootLayer->RequestDraw(invalidRegion, fWinBorder);

#endif
}


void
ServerWindow::SendMessageToClient(const BMessage* msg, int32 target, bool usePreferred) const
{
	ssize_t size = msg->FlattenedSize();
	char* buffer = new char[size];

	if (msg->Flatten(buffer, size) == B_OK) {
//		BMessage::Private::SendFlattenedMessage(buffer, size,
//			fClientLooperPort, target, usePreferred, B_INFINITE_TIMEOUT);
		status_t ret = BMessage::Private::SendFlattenedMessage(buffer, size,
							fClientLooperPort, target, usePreferred, 100000);
		if (ret < B_OK)
			fprintf(stderr, "ServerWindow::SendMessageToClient(): %s\n", strerror(ret));
	} else
		printf("PANIC: ServerWindow %s: can't flatten message in 'SendMessageToClient()'\n", fName);

	delete[] buffer;
}


status_t
ServerWindow::PictureToRegion(ServerPicture *picture, BRegion &region,
							bool inverse, BPoint where)
{
	fprintf(stderr, "ServerWindow::PictureToRegion() not implemented\n");
	region.MakeEmpty();
	return B_ERROR;
}
