//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Shadow BWindow class
//  
//------------------------------------------------------------------------------
#include <AppDefs.h>
#include <Rect.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <View.h>	// for B_XXXXX_MOUSE_BUTTON defines
#include <Message.h>
#include <GraphicsDefs.h>
#include <PortLink.h>
#include <ViewAux.h>
#include "AppServer.h"
#include "BGet++.h"
#include "Desktop.h"
#include "Layer.h"
#include "RAMLinkMsgReader.h"
#include "RootLayer.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "ServerProtocol.h"
#include "WinBorder.h"
#include "Desktop.h"
#include "TokenHandler.h"
#include "Utils.h"
#include "DisplayDriver.h"
#include "ServerPicture.h"
#include "CursorManager.h"
#include "Workspace.h"

//#define DEBUG_SERVERWINDOW
//#define DEBUG_SERVERWINDOW_MOUSE
//#define DEBUG_SERVERWINDOW_KEYBOARD


#ifdef DEBUG_SERVERWINDOW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_SERVERWINDOW_KEYBOARD
#	include <stdio.h>
#	define STRACE_KEY(x) printf x
#else
#	define STRACE_KEY(x) ;
#endif

#ifdef DEBUG_SERVERWINDOW_MOUSE
#	include <stdio.h>
#	define STRACE_MOUSE(x) printf x
#else
#	define STRACE_MOUSE(x) ;
#endif

//------------------------------------------------------------------------------

template<class Type> Type
read_from_buffer(int8 **_buffer)
{
	Type *typedBuffer = (Type *)(*_buffer);
	Type value = *typedBuffer;

	typedBuffer++;
	*_buffer = (int8 *)(typedBuffer);

	return value;
}
//------------------------------------------------------------------------------
/*
static int8 *read_pattern_from_buffer(int8 **_buffer)
{
	int8 *pattern = *_buffer;

	*_buffer += AS_PATTERN_SIZE;

	return pattern;
}
*/
//------------------------------------------------------------------------------
template<class Type> void
write_to_buffer(int8 **_buffer, Type value)
{
	Type *typedBuffer = (Type *)(*_buffer);

	*typedBuffer = value;
	typedBuffer++;

	*_buffer = (int8 *)(typedBuffer);
}
//------------------------------------------------------------------------------
/*!
	\brief Contructor
	
	Does a lot of stuff to set up for the window - new decorator, new winborder, spawn a 
	monitor thread.
*/
ServerWindow::ServerWindow(BRect rect, const char *string, uint32 wlook,
	uint32 wfeel, uint32 wflags, ServerApp *winapp,  port_id winport,
	port_id looperPort, port_id replyport, uint32 index, int32 handlerID)
{
	STRACE(("ServerWindow(%s)::ServerWindow()\n",string? string: "NULL"));
	fServerApp			= winapp;

	if(string)
		fTitle.SetTo(string);
	else
		fTitle.SetTo(B_EMPTY_STRING);
		
	fFrame = rect;
	fFlags = wflags;
	fLook = wlook;
	fFeel = wfeel;
	
	fHandlerToken = handlerID;
	fClientLooperPort = looperPort;
	fClientTeamID = winapp->ClientTeamID();
	fWorkspaces = index;
	
	fWinBorder = NULL;
	cl = NULL;	//current layer
	
	// fClientWinPort is the port to which the app awaits messages from the server
	fClientWinPort = winport;

	// fMessagePort is the port to which the app sends messages for the server
	fMessagePort = create_port(30,fTitle.String());

	fMsgSender=new LinkMsgSender(fClientWinPort);
	fMsgReader=new LinkMsgReader(fMessagePort);
	
	// Send a reply to our window - it is expecting fMessagePort port.
	fMsgSender->StartMessage(SERVER_TRUE);
	fMsgSender->Attach<port_id>(fMessagePort);
	fMsgSender->Flush();

	STRACE(("ServerWindow %s:\n",fTitle.String()));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom));
	STRACE(("\tPort: %ld\n",fMessagePort));
	STRACE(("\tWorkspace: %ld\n",index));
}
//------------------------------------------------------------------------------
void ServerWindow::Init(void)
{
	fWinBorder = new WinBorder( fFrame, fTitle.String(), fLook, fFeel, 0UL,
			this, desktop->GetDisplayDriver());
	fWinBorder->RebuildFullRegion();

	// Spawn our message-monitoring thread
	fMonitorThreadID = spawn_thread(MonitorWin, fTitle.String(), B_NORMAL_PRIORITY, this);
	
	if(fMonitorThreadID != B_NO_MORE_THREADS && fMonitorThreadID != B_NO_MEMORY)
		resume_thread(fMonitorThreadID);
}
//------------------------------------------------------------------------------
//!Tears down all connections the main app_server objects, and deletes some internals.
ServerWindow::~ServerWindow(void)
{
	STRACE(("*ServerWindow (%s):~ServerWindow()\n",fTitle.String()));
	
	desktop->RemoveWinBorder(fWinBorder);
	STRACE(("SW(%s) Successfully removed from the desktop\n", fTitle.String()));
	
	if(fMsgSender)
	{
		delete fMsgSender;
		fMsgSender=NULL;
	}
		
	if(fMsgReader)
	{
		delete fMsgReader;
		fMsgReader=NULL;
	}
		
	if (fWinBorder)
	{
		delete fWinBorder;
		fWinBorder = NULL;
	}
	
	cl = NULL;
	
	STRACE(("#ServerWindow(%s) will exit NOW\n", fTitle.String()));
}

//! Forces the window border to update its decorator
void ServerWindow::ReplaceDecorator(void)
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::ReplaceDecorator()\n");

	STRACE(("ServerWindow %s: Replace Decorator\n",fTitle.String()));
	fWinBorder->UpdateDecorator();
}
//------------------------------------------------------------------------------
//! Requests that the ServerWindow's BWindow quit
void ServerWindow::Quit(void)
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::Quit()\n");

	STRACE(("ServerWindow %s: Quit\n",fTitle.String()));
	BMessage msg;
	
	msg.what = B_QUIT_REQUESTED;
	
	SendMessageToClient(&msg);
}
//------------------------------------------------------------------------------
//! Shows the window's WinBorder
void ServerWindow::Show(void)
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::Show()\n");

	if(!fWinBorder->IsHidden())
		return;

	STRACE(("ServerWindow (%s): Show()\n",fTitle.String()));
	if(fWinBorder)
	{
		RootLayer *rl = fWinBorder->GetRootLayer();
		int32 wksCount;

		desktop->fGeneralLock.Lock();
		rl->fMainLock.Lock();

		// make WinBorder unhidden, but *do not* rebuild and redraw! We'll do that
		// after we bring it (its modal and floating windows also) in front.
		fWinBorder->Show(false);

		if ( (fFeel == B_FLOATING_SUBSET_WINDOW_FEEL || fFeel == B_MODAL_SUBSET_WINDOW_FEEL)
			 && fWinBorder->MainWinBorder() == NULL)
		{
			// This window hasn't been added to a normal window subset,	
			// so don't call placement or redrawing methods
		}
		else
		{
			wksCount	= rl->WorkspaceCount();
			for(int32 i = 0; i < wksCount; i++)
			{
				if (fWorkspaces & (0x00000001UL << i))
				{
					WinBorder		*previousFocus;
					BRegion			invalidRegion;
					Workspace		*ws;
					
					// TODO: can you unify this method with Desktop::MouseEventHandler::B_MOUSE_DOWN
					
					ws				= rl->WorkspaceAt(i+1);
					ws->BringToFrontANormalWindow(fWinBorder);
					ws->SearchAndSetNewFront(fWinBorder);
					previousFocus	= ws->FocusLayer();
					ws->SearchAndSetNewFocus(fWinBorder);
					
					// TODO: only do this in for the active workspace!
					
					// first redraw previous window's decorator. It has lost focus state.
					if (previousFocus)
						if (previousFocus->fDecorator)
							fWinBorder->fParent->Invalidate(previousFocus->fVisible);

					// we must build the rebuild region. we have nowhere to take it from.
					// include decorator's(if any) and fTopLayer's full regions.
					fWinBorder->fTopLayer->RebuildFullRegion();
					fWinBorder->RebuildFullRegion();
					invalidRegion.Include(&(fWinBorder->fTopLayer->fFull));
					if (fWinBorder->fDecorator)
						invalidRegion.Include(&(fWinBorder->fFull));

					fWinBorder->fParent->FullInvalidate(invalidRegion);
				}
			}
		}

		rl->fMainLock.Unlock();
		desktop->fGeneralLock.Unlock();
	}
	else
	{
		debugger("WARNING: NULL fWinBorder\n");
	}
}
//------------------------------------------------------------------------------
//! Hides the window's WinBorder
void ServerWindow::Hide(void)
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::Hide()\n");

	if(fWinBorder->IsHidden())
		return;

	STRACE(("ServerWindow %s: Hide\n",fTitle.String()));
	if(fWinBorder)
	{
		RootLayer *rl = fWinBorder->GetRootLayer();
		Workspace *ws = NULL;
		
		desktop->fGeneralLock.Lock();
		rl->fMainLock.Lock();
		
		fWinBorder->Hide();
		
		int32		wksCount= rl->WorkspaceCount();
		for(int32 i = 0; i < wksCount; i++)
		{
			ws = rl->WorkspaceAt(i+1);

			if (ws->FrontLayer() == fWinBorder)
			{
				ws->HideSubsetWindows(fWinBorder);
				ws->SearchAndSetNewFocus(ws->FrontLayer());
			}
			else
			{
				if (ws->FocusLayer() == fWinBorder)
					ws->SearchAndSetNewFocus(fWinBorder);
				else{
					// TODO: RootLayer or Desktop class should take care of invalidating
//					ws->Invalidate();
				}
			}
		}
		rl->fMainLock.Unlock();
		desktop->fGeneralLock.Unlock();
	}
}
//------------------------------------------------------------------------------
/*
	\brief Determines whether the window is hidden or not
	\return true if hidden, false if not
*/
bool ServerWindow::IsHidden(void) const
{
	if(fWinBorder)
		return fWinBorder->IsHidden();
	
	return true;
}
//------------------------------------------------------------------------------
void ServerWindow::Minimize(bool status)
{
	// This function doesn't need much -- check to make sure that we should and
	// send the message to the client. According to the BeBook, the BWindow hook function
	// does all the heavy lifting for us. :)
	
	bool sendMessages = false;

	if (status)
	{
		if (!IsHidden())
		{
			Hide();
			sendMessages = true;
		}
	}
	else
	{
		if (IsHidden())
		{
			Show();
			sendMessages = true;
		}
	}
	
	if (sendMessages)
	{
		BMessage msg;
		msg.what = B_MINIMIZE;
		msg.AddInt64("when", real_time_clock_usecs());
		msg.AddBool("minimize", status);
	
		SendMessageToClient(&msg);
	}
}
//------------------------------------------------------------------------------
// Sends a message to the client to perform a Zoom
void ServerWindow::Zoom(void)
{
	BMessage msg;
	msg.what=B_ZOOM;
	SendMessageToClient(&msg);
}
//------------------------------------------------------------------------------
/*!
	\brief Notifies window of workspace (de)activation
	\param workspace Index of the workspace changed
	\param active New active status of the workspace
*/
void ServerWindow::WorkspaceActivated(int32 workspace, bool active)
{
	STRACE(("ServerWindow %s: WorkspaceActivated(%ld,%s)\n",fTitle.String(),
			workspace,(active)?"active":"inactive"));
	
	BMessage msg;
	msg.what = B_WORKSPACE_ACTIVATED;
	msg.AddInt32("workspace", workspace);
	msg.AddBool("active", active);
	
	SendMessageToClient(&msg);
}
//------------------------------------------------------------------------------
/*!
	\brief Notifies window of a workspace switch
	\param oldone index of the previous workspace
	\param newone index of the new workspace
*/
void ServerWindow::WorkspacesChanged(int32 oldone,int32 newone)
{
	STRACE(("ServerWindow %s: WorkspacesChanged(%ld,%ld)\n",fTitle.String(),oldone,newone));

	BMessage msg;
	msg.what		= B_WORKSPACES_CHANGED;
	msg.AddInt32("old", oldone);
	msg.AddInt32("new", newone);
	
	SendMessageToClient(&msg);
}
//------------------------------------------------------------------------------
/*!
	\brief Notifies window of a change in focus
	\param active New active status of the window
*/
void ServerWindow::WindowActivated(bool active)
{
	STRACE(("ServerWindow %s: WindowActivated(%s)\n",fTitle.String(),(active)?"active":"inactive"));
	
	BMessage msg;
	msg.what = B_WINDOW_ACTIVATED;
	msg.AddBool("active", active);
	
	SendMessageToClient(&msg);
}
//------------------------------------------------------------------------------
/*!
	\brief Notifies window of a change in screen resolution
	\param frame Size of the new resolution
	\param color_space Color space of the new screen mode
*/
void ServerWindow::ScreenModeChanged(const BRect frame, const color_space cspace)
{
	STRACE(("ServerWindow %s: ScreenModeChanged\n",fTitle.String()));
	BMessage msg;
	
	msg.what = B_SCREEN_CHANGED;
	msg.AddRect("frame", frame);
	msg.AddInt32("mode", (int32)cspace);
	
	SendMessageToClient(&msg);
}
//------------------------------------------------------------------------------
/*!
	\brief Locks the window
	\return B_OK if everything is ok, B_ERROR if something went wrong
*/
status_t ServerWindow::Lock(void)
{
	STRACE(("ServerWindow %s: Lock\n",fTitle.String()));
	
	return (fLocker.Lock())?B_OK:B_ERROR;
}
//------------------------------------------------------------------------------
//! Unlocks the window
void ServerWindow::Unlock(void)
{
	STRACE(("ServerWindow %s: Unlock\n",fTitle.String()));
	
	fLocker.Unlock();
}
//------------------------------------------------------------------------------
/*!
	\brief Determines whether or not the window is locked
	\return True if locked, false if not.
*/
bool ServerWindow::IsLocked(void) const
{
	return fLocker.IsLocked();
}
//------------------------------------------------------------------------------
/*!
	\brief Sets the font state for a layer
	\param layer The layer to set the font
*/
void ServerWindow::SetLayerFontState(Layer *layer, LinkMsgReader &link)
{
	// NOTE: no need to check for a lock. This is a private method.
	uint16 mask;

	link.Read<uint16>(&mask);
			
	if (mask & B_FONT_FAMILY_AND_STYLE)
	{
		uint32		fontID;
		link.Read<int32>((int32*)&fontID);
		
		// TODO: Implement. ServerFont::SetFamilyAndStyle(uint32) is needed
		//layer->fLayerData->font->
	}

	if (mask & B_FONT_SIZE)
	{
		float size;
		link.Read<float>(&size);
		layer->fLayerData->font.SetSize(size);
	}
	
	if (mask & B_FONT_SHEAR)
	{
		float shear;
		link.Read<float>(&shear);
		layer->fLayerData->font.SetShear(shear);
	}

	if (mask & B_FONT_ROTATION)
	{
		float rotation;
		link.Read<float>(&rotation);
		layer->fLayerData->font.SetRotation(rotation);
	}

	if (mask & B_FONT_SPACING)
	{
		uint8 spacing;
		link.Read<uint8>(&spacing);
		layer->fLayerData->font.SetSpacing(spacing);
	}

	if (mask & B_FONT_ENCODING)
	{
		uint8 encoding;
		link.Read<uint8>((uint8*)&encoding);
		layer->fLayerData->font.SetEncoding(encoding);
	}

	if (mask & B_FONT_FACE)
	{
		uint16 face;
		link.Read<uint16>(&face);
		layer->fLayerData->font.SetFace(face);
	}

	if (mask & B_FONT_FLAGS)
	{
		uint32 flags;
		link.Read<uint32>(&flags);
		layer->fLayerData->font.SetFlags(flags);
	}
	STRACE(("DONE: ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer: %s\n",
			fTitle.String(), layer->fName->String()));
}
//------------------------------------------------------------------------------
void ServerWindow::SetLayerState(Layer *layer, LinkMsgReader &link)
{
	// NOTE: no need to check for a lock. This is a private method.
	rgb_color highColor, lowColor, viewColor;
	pattern patt;
	int32 clipRegRects;

	link.Read<BPoint>(		&(layer->fLayerData->penlocation));
	link.Read<float>(		&(layer->fLayerData->pensize));
	link.Read(			&highColor, sizeof(rgb_color));
	link.Read(			&lowColor, sizeof(rgb_color));
	link.Read(			&viewColor, sizeof(rgb_color));
	link.Read(			&patt, sizeof(pattern));
	link.Read<int8>((int8*)	&(layer->fLayerData->draw_mode));
	link.Read<BPoint>(		&(layer->fLayerData->coordOrigin));
	link.Read<int8>((int8*)	&(layer->fLayerData->lineJoin));
	link.Read<int8>((int8*)	&(layer->fLayerData->lineCap));
	link.Read<float>(		&(layer->fLayerData->miterLimit));
	link.Read<int8>((int8*)	&(layer->fLayerData->alphaSrcMode));
	link.Read<int8>((int8*)	&(layer->fLayerData->alphaFncMode));
	link.Read<float>(		&(layer->fLayerData->scale));
	link.Read<bool>(			&(layer->fLayerData->fontAliasing));
	link.Read<int32>(		&clipRegRects);

	layer->fLayerData->patt.Set(*((uint64*)&patt));
	layer->fLayerData->highcolor.SetColor(highColor);
	layer->fLayerData->lowcolor.SetColor(lowColor);
	layer->fLayerData->viewcolor.SetColor(viewColor);

	if(clipRegRects != 0)
	{
		if(layer->fLayerData->clipReg == NULL)
			layer->fLayerData->clipReg = new BRegion();
		else
			layer->fLayerData->clipReg->MakeEmpty();

		BRect rect;
				
		for(int32 i = 0; i < clipRegRects; i++)
		{
			link.Read<BRect>(&rect);
			layer->fLayerData->clipReg->Include(rect);
		}
	}
	else
	{
		if (layer->fLayerData->clipReg)
		{
			delete layer->fLayerData->clipReg;
			layer->fLayerData->clipReg = NULL;
		}
	}
	STRACE(("DONE: ServerWindow %s: Message AS_LAYER_SET_STATE: Layer: %s\n",fTitle.String(),
			 layer->fName->String()));
}
//------------------------------------------------------------------------------
Layer * ServerWindow::CreateLayerTree(Layer *localRoot, LinkMsgReader &link)
{
	// NOTE: no need to check for a lock. This is a private method.
	STRACE(("ServerWindow(%s)::CreateLayerTree()\n", fTitle.String()));

	int32 token;
	BRect frame;
	uint32 resizeMask;
	uint32 flags;
	bool hidden;
	int32 childCount;
	char *name = NULL;
		
	link.Read<int32>(&token);
	link.ReadString(&name);
	link.Read<BRect>(&frame);
	link.Read<uint32>(&resizeMask);
	link.Read<uint32>(&flags);
	link.Read<bool>(&hidden);
	link.Read<int32>(&childCount);
			
	Layer *newLayer;
	newLayer = new Layer(frame, name, token, resizeMask, 
			flags, desktop->GetDisplayDriver());
	if (name)
		free(name);

	// there is no way of setting this, other than manually :-)
	newLayer->fHidden = hidden;

	// add the new Layer to the tree structure.
	if(localRoot)
		localRoot->AddChild(newLayer, NULL);

	STRACE(("DONE: ServerWindow %s: Message AS_LAYER_CREATE: Parent: %s, Child: %s\n", fTitle.String(), 
			localRoot? localRoot->fName->String(): "NULL", newLayer->fName->String()));

	return newLayer;
}
//------------------------------------------------------------------------------
void ServerWindow::DispatchMessage(int32 code, LinkMsgReader &link)
{
	if (cl == NULL && code != AS_LAYER_CREATE_ROOT)
	{
		printf("ServerWindow %s received unexpected code - message offset %lx before top_view attached.\n",fTitle.String(), code - SERVER_TRUE);
		return;
	}
	
	switch(code)
	{
		//--------- BView Messages -----------------
		case AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT:
		{
			int32		bitmapToken;
			BPoint 		point;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BPoint>(&point);
			
			ServerBitmap *sbmp = fServerApp->FindBitmap(bitmapToken);
			if(sbmp)
			{
				BRect src, dst;
				BRegion region;
				
				src = sbmp->Bounds();
				dst = cl->fParent->ConvertFromParent(cl->fFull.Frame());
				region = cl->fParent->ConvertFromParent(&(cl->fFull));
				dst.OffsetBy(point);
				
				cl->fDriver->DrawBitmap(&region, sbmp, src, dst, cl->fLayerData);
			}
			
			// TODO: Adi -- shouldn't AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT sync with the client?
			break;
		}
		case AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT:
		{
			int32		bitmapToken;
			BPoint 		point;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BPoint>(&point);
			
			ServerBitmap *sbmp = fServerApp->FindBitmap(bitmapToken);
			if(sbmp)
			{
				BRect src, dst;
				BRegion region;
				
				src = sbmp->Bounds();
				dst = cl->fParent->ConvertFromParent(cl->fFull.Frame());
				region = cl->fParent->ConvertFromParent(&(cl->fFull));
				dst.OffsetBy(point);
				
				cl->fDriver->DrawBitmap(&region, sbmp, src, dst, cl->fLayerData);
			}
			break;
		}
		case AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT:
		{
			int32 bitmapToken;
			BRect srcRect, dstRect;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&dstRect);
			link.Read<BRect>(&srcRect);
			
			ServerBitmap *sbmp = fServerApp->FindBitmap(bitmapToken);
			if(sbmp)
			{
				BRegion region;
				BRect dst;
				region = cl->fParent->ConvertFromParent(&(cl->fFull));
				dst = cl->fParent->ConvertFromParent(cl->fFull.Frame());
				dstRect.OffsetBy(dst.left, dst.top);
				
				cl->fDriver->DrawBitmap(&region, sbmp, srcRect, dstRect, cl->fLayerData);
			}
			
			// TODO: Adi -- shouldn't AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT sync with the client?
			break;
		}
		case AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT:
		{
			int32 bitmapToken;
			BRect srcRect, dstRect;
			
			link.Read<int32>(&bitmapToken);
			link.Read<BRect>(&dstRect);
			link.Read<BRect>(&srcRect);
			
			ServerBitmap *sbmp = fServerApp->FindBitmap(bitmapToken);
			if(sbmp)
			{
				BRegion region;
				BRect dst;
				region = cl->fParent->ConvertFromParent(&(cl->fFull));
				dst = cl->fParent->ConvertFromParent(cl->fFull.Frame());
				dstRect.OffsetBy(dst.left, dst.top);
				
				cl->fDriver->DrawBitmap(&region, sbmp, srcRect, dstRect, cl->fLayerData);
			}
			break;
		}
		case AS_SET_CURRENT_LAYER:
		{
			STRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			int32 token;
			
			link.Read<int32>(&token);
			
			Layer *current = FindLayer(fWinBorder->fTopLayer, token);

			if (current)
				cl=current;
			else // hope this NEVER happens! :-)
				debugger("Server PANIC: window cannot find Layer with ID\n");
			break;
		}

		case AS_LAYER_CREATE_ROOT:
		{
			// Start receiving top_view data -- pass NULL as the parent view.
			// This should be the *only* place where this happens.
			if (cl != NULL)
				break;
			
//			fWinBorder->fTopLayer = CreateLayerTree(NULL);
			fWinBorder->fTopLayer = CreateLayerTree(NULL, link);
			fWinBorder->fTopLayer->SetAsTopLayer(true);
			cl = fWinBorder->fTopLayer;

			// connect decorator and top layer.
			fWinBorder->AddChild(fWinBorder->fTopLayer, NULL);
			desktop->AddWinBorder(fWinBorder);

			break;
		}

		case AS_LAYER_CREATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CREATE: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			Layer		*newLayer;
			
			if (cl == NULL)
				break;

//			newLayer	= CreateLayerTree(NULL);
			newLayer	= CreateLayerTree(NULL, link);
			cl->AddChild(newLayer, this);

			if (!(newLayer->IsHidden())){
				// cl is the parent of newLayer, so this call is OK.
				cl->FullInvalidate(BRegion(newLayer->fFull));
			}

			break;
		}
		case AS_LAYER_DELETE:
		{
			// Received when a view is detached from a window. This is definitely
			// the less taxing operation - we call PruneTree() on the removed
			// layer, detach the layer itself, delete it, and invalidate the
			// area assuming that the view was visible when removed

			STRACE(("ServerWindow %s: AS_LAYER_DELETE(self)...\n", fTitle.String()));			
			Layer *parent;
			parent = cl->fParent;
			
			// here we remove current layer from list.
			cl->RemoveSelf();
			cl->PruneTree();
			
			parent->Invalidate(cl->Frame());
			
			#ifdef DEBUG_SERVERWINDOW
			parent->PrintTree();
			#endif
			STRACE(("DONE: ServerWindow %s: Message AS_DELETE_LAYER: Parent: %s Layer: %s\n", fTitle.String(), parent->fName->String(), cl->fName->String()));

			delete cl;

			cl=parent;
			break;
		}
		case AS_LAYER_SET_STATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_STATE: Layer name: %s\n", fTitle.String(), cl->fName->String()));
//			SetLayerState(cl);
			SetLayerState(cl,link);
			cl->RebuildFullRegion();
			break;
		}
		case AS_LAYER_SET_FONT_STATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer name: %s\n", fTitle.String(), cl->fName->String()));
//			SetLayerFontState(cl);
			SetLayerFontState(cl,link);
			cl->RebuildFullRegion();
			break;
		}
		case AS_LAYER_GET_STATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_STATE: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			LayerData	*ld;
			
			// these 4 are here because of a compiler warning. Maybe he's right... :-)
			rgb_color	hc, lc, vc; // high, low and view colors
			uint64		patt;
			
			ld = cl->fLayerData; // now we write fewer characters. :-)
			hc = ld->highcolor.GetColor32();
			lc = ld->lowcolor.GetColor32();
			vc = ld->viewcolor.GetColor32();
			patt = ld->patt.GetInt64();
			
			// TODO: Implement when ServerFont::SetfamilyAndStyle(int32) exists
			fMsgSender->StartMessage(SERVER_TRUE);
			
			// Attach font state
			
			// TODO: need a ServerFont::GetFamAndStyle
//			fMsgSender->Attach<uint32>(0UL uint32 ld->font.GetFamAndStyle());
			fMsgSender->Attach<uint32>(0UL);
			fMsgSender->Attach<float>(ld->font.Size());
			fMsgSender->Attach<float>(ld->font.Shear());
			fMsgSender->Attach<float>(ld->font.Rotation());
			fMsgSender->Attach<uint8>(ld->font.Spacing());
			fMsgSender->Attach<uint8>(ld->font.Encoding());
			fMsgSender->Attach<uint16>(ld->font.Face());
			fMsgSender->Attach<uint32>(ld->font.Flags());
			
			// Attach view state
			fMsgSender->Attach<BPoint>(ld->penlocation);
			fMsgSender->Attach<float>(ld->pensize);
			fMsgSender->Attach(&hc, sizeof(rgb_color));
			fMsgSender->Attach(&lc, sizeof(rgb_color));
			fMsgSender->Attach(&vc, sizeof(rgb_color));
			fMsgSender->Attach<uint64>(patt);
			fMsgSender->Attach<BPoint>(ld->coordOrigin);
			fMsgSender->Attach<uint8>((uint8)(ld->draw_mode));
			fMsgSender->Attach<uint8>((uint8)(ld->lineCap));
			fMsgSender->Attach<uint8>((uint8)(ld->lineJoin));
			fMsgSender->Attach<float>(ld->miterLimit);
			fMsgSender->Attach<uint8>((uint8)(ld->alphaSrcMode));
			fMsgSender->Attach<uint8>((uint8)(ld->alphaFncMode));
			fMsgSender->Attach<float>(ld->scale);
			fMsgSender->Attach<float>(ld->fontAliasing);
			
			int32 noOfRects = 0;
			if (ld->clipReg)
				noOfRects = ld->clipReg->CountRects();
			
			fMsgSender->Attach<int32>(noOfRects);
			
			for(int i = 0; i < noOfRects; i++)
				fMsgSender->Attach<BRect>(ld->clipReg->RectAt(i));
			
			fMsgSender->Attach<float>(cl->fFrame.left);
			fMsgSender->Attach<float>(cl->fFrame.top);
			fMsgSender->Attach<BRect>(cl->fFrame.OffsetToCopy(cl->fBoundsLeftTop));
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_MOVETO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_MOVETO: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			float x, y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);
			
			cl->MoveBy(x, y);
			
			break;
		}
		case AS_LAYER_RESIZETO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZETO: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			float newWidth, newHeight;
			
			link.Read<float>(&newWidth);
			link.Read<float>(&newHeight);
			
			// TODO: Check for minimum size allowed. Need WinBorder::GetSizeLimits
			
			cl->ResizeBy(newWidth, newHeight);
			
			break;
		}
		case AS_LAYER_GET_COORD:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<float>(cl->fFrame.left);
			fMsgSender->Attach<float>(cl->fFrame.top);
			fMsgSender->Attach<BRect>(cl->fFrame.OffsetToCopy(cl->fBoundsLeftTop));
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_SET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_ORIGIN: Layer: %s\n",fTitle.String(), cl->fName->String()));
			float x, y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);
			
			cl->fLayerData->coordOrigin.Set(x, y);

			break;
		}
		case AS_LAYER_GET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_ORIGIN: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<BPoint>(cl->fLayerData->coordOrigin);
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_RESIZE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			link.Read<uint32>(&(cl->fResizeMode));
			
			break;
		}
		case AS_LAYER_CURSOR:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CURSOR: Layer: %s\n",fTitle.String(), cl->fName->String()));
			int32 token;

			link.Read<int32>(&token);
			
			cursormanager->SetCursor(token);

			break;
		}
		case AS_LAYER_SET_FLAGS:
		{
			link.Read<uint32>(&(cl->fFlags));
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FLAGS: Layer: %s\n",fTitle.String(), cl->fName->String()));
			break;
		}
		case AS_LAYER_HIDE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_HIDE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			cl->Hide();
			break;
		}
		case AS_LAYER_SHOW:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SHOW: Layer: %s\n",fTitle.String(), cl->fName->String()));
			cl->Show();
			break;
		}
		case AS_LAYER_SET_LINE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_LINE_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			int8 lineCap, lineJoin;

			// TODO: Look into locking scheme relating to Layers and modifying redraw-related members

			link.Read<int8>(&lineCap);
			link.Read<int8>(&lineJoin);
			link.Read<float>(&(cl->fLayerData->miterLimit));
			
			cl->fLayerData->lineCap	= (cap_mode)lineCap;
			cl->fLayerData->lineJoin = (join_mode)lineJoin;
		
			break;
		}
		case AS_LAYER_GET_LINE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_LINE_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<int8>((int8)(cl->fLayerData->lineCap));
			fMsgSender->Attach<int8>((int8)(cl->fLayerData->lineJoin));
			fMsgSender->Attach<float>(cl->fLayerData->miterLimit);
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_PUSH_STATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_PUSH_STATE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			LayerData *ld = new LayerData();
			ld->prevState = cl->fLayerData;
			cl->fLayerData = ld;
			
			cl->RebuildFullRegion();
			
			break;
		}
		case AS_LAYER_POP_STATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_POP_STATE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			if (!(cl->fLayerData->prevState))
			{
				STRACE(("WARNING: SW(%s): User called BView(%s)::PopState(), but there is NO state on stack!\n", fTitle.String(), cl->fName->String()));
				break;
			}
			
			LayerData		*ld = cl->fLayerData;
			cl->fLayerData	= cl->fLayerData->prevState;
			delete ld;
			
			cl->RebuildFullRegion();

			break;
		}
		case AS_LAYER_SET_SCALE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: Layer: %s\n",fTitle.String(), cl->fName->String()));		
			link.Read<float>(&(cl->fLayerData->scale));
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_SCALE: Layer: %s\n",fTitle.String(), cl->fName->String()));		
			LayerData		*ld = cl->fLayerData;

			float			scale = ld->scale;
			
			while((ld = ld->prevState))
				scale		*= ld->scale;
			
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<float>(scale);
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_LOC:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_LOC: Layer: %s\n",fTitle.String(), cl->fName->String()));
			float		x, y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);

			cl->fLayerData->penlocation.Set(x, y);

			break;
		}
		case AS_LAYER_GET_PEN_LOC:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_LOC: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<BPoint>(cl->fLayerData->penlocation);
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_SIZE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			link.Read<float>(&(cl->fLayerData->pensize));
		
			break;
		}
		case AS_LAYER_GET_PEN_SIZE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_SIZE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<float>(cl->fLayerData->pensize);
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: Layer: %s\n",fTitle.String(), cl->fName->String()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			cl->fLayerData->viewcolor.SetColor(c);
			
			cl->Invalidate(cl->fVisible);
			
			break;
		}
		case AS_LAYER_GET_COLORS:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COLORS: Layer: %s\n",fTitle.String(), cl->fName->String()));
			rgb_color highColor, lowColor, viewColor;
			
			highColor = cl->fLayerData->highcolor.GetColor32();
			lowColor = cl->fLayerData->lowcolor.GetColor32();
			viewColor = cl->fLayerData->viewcolor.GetColor32();
			
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach(&highColor, sizeof(rgb_color));
			fMsgSender->Attach(&lowColor, sizeof(rgb_color));
			fMsgSender->Attach(&viewColor, sizeof(rgb_color));
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_SET_BLEND_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_BLEND_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			int8 srcAlpha, alphaFunc;
			
			link.Read<int8>(&srcAlpha);
			link.Read<int8>(&alphaFunc);
			
			cl->fLayerData->alphaSrcMode = (source_alpha)srcAlpha;
			cl->fLayerData->alphaFncMode = (alpha_function)alphaFunc;

			break;
		}
		case AS_LAYER_GET_BLEND_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_BLEND_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<int8>((int8)(cl->fLayerData->alphaSrcMode));
			fMsgSender->Attach<int8>((int8)(cl->fLayerData->alphaFncMode));
			fMsgSender->Flush();

			break;
		}
		case AS_LAYER_SET_DRAW_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_DRAW_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			int8 drawingMode;
			
			link.Read<int8>(&drawingMode);
			
			cl->fLayerData->draw_mode = (drawing_mode)drawingMode;
			
			break;
		}
		case AS_LAYER_GET_DRAW_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_DRAW_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Attach<int8>((int8)(cl->fLayerData->draw_mode));
			fMsgSender->Flush();
		
			break;
		}
		case AS_LAYER_PRINT_ALIASING:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_PRINT_ALIASING: Layer: %s\n",fTitle.String(), cl->fName->String()));
			link.Read<bool>(&(cl->fLayerData->fontAliasing));
			
			break;
		}
		case AS_LAYER_CLIP_TO_PICTURE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_PICTURE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			
			// TODO: Watch out for the coordinate system in AS_LAYER_CLIP_TO_PICTURE
			int32 pictureToken;
			BPoint where;
			
			link.Read<int32>(&pictureToken);
			link.Read<BPoint>(&where);
			
			
			BRegion reg;
			bool redraw = false;
			
			// if we had a picture to clip to, include the FULL visible region(if any) in the area to be redrawn
			// in other words: invalidate what ever is visible for this layer and his children.
			if (cl->clipToPicture && cl->fFullVisible.CountRects() > 0)
			{
				reg.Include(&cl->fFullVisible);
				redraw		= true;
			}
			
			// search for a picture with the specified token.
			ServerPicture *sp = NULL;
			int32 i = 0;
			while(1)
			{
				sp=static_cast<ServerPicture*>(cl->fServerWin->fServerApp->fPictureList->ItemAt(i++));
				if (!sp)
					break;
				
				if(sp->GetToken() == pictureToken)
				{
					//cl->clipToPicture	= sp;
					
					// TODO: Increase that picture's reference count.(~ allocate a picture)
					break;
				}
			}
			
			// avoid compiler warning
			i = 0;
			
			// we have a new picture to clip to, so rebuild our full region
			if(cl->clipToPicture) 
			{
				cl->clipToPictureInverse = false;
				cl->RebuildFullRegion();
			}
			
			// we need to rebuild the visible region, we may have a valid one.
			if (cl->fParent && !(cl->fHidden))
			{
				//cl->fParent->RebuildChildRegions(cl->fFull.Frame(), cl);
			}
			else
			{
				// will this happen? Maybe...
				//cl->RebuildRegions(cl->fFull.Frame());
			}
			
			// include our full visible region in the region to be redrawn
			if (!(cl->fHidden) && (cl->fFullVisible.CountRects() > 0))
			{
				reg.Include(&(cl->fFullVisible));
				redraw		= true;
			}
			
			// redraw if we previously had or if we have acquired a picture to clip to.
			if (redraw)
				cl->Invalidate(reg);

			break;
		}
		case AS_LAYER_CLIP_TO_INVERSE_PICTURE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_INVERSE_PICTURE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			
			// TODO: Watch out for the coordinate system in AS_LAYER_CLIP_TO_INVERSE_PICTURE
			int32 pictureToken;
			BPoint where;
			
			link.Read<int32>(&pictureToken);
			link.Read<BPoint>(&where);
			
			ServerPicture *sp = NULL;
			int32 i = 0;
			
			while(1)
			{
				sp= static_cast<ServerPicture*>(cl->fServerWin->fServerApp->fPictureList->ItemAt(i++));
				if (!sp)
					break;
					
				if(sp->GetToken() == pictureToken)
				{
					//cl->clipToPicture	= sp;
					
					// TODO: Increase that picture's reference count.(~ allocate a picture)
					break;
				}
			}
			// avoid compiler warning
			i = 0;
			
			// if a picture has been found...
			if(cl->clipToPicture) 
			{
				cl->clipToPictureInverse	= true;
				cl->RebuildFullRegion();
				//cl->RequestDraw(cl->clipToPicture->Frame());
			}
			break;
		}
		case AS_LAYER_GET_CLIP_REGION:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_CLIP_REGION: Layer: %s\n",fTitle.String(), cl->fName->String()));
			
			// if this Layer is hidden, it is clear that its visible region is void.
			if (cl->IsHidden())
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
			
				ld = cl->fLayerData;
				reg = cl->ConvertFromParent(&(cl->fVisible));
			
				if(ld->clipReg)
					reg.IntersectWith(ld->clipReg);
			
				while((ld = ld->prevState))
				{
					if(ld->clipReg)
						reg.IntersectWith(ld->clipReg);
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
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_CLIP_REGION: Layer: %s\n",fTitle.String(), cl->fName->String()));
			
			// TODO: Watch out for the coordinate system in AS_LAYER_SET_CLIP_REGION
			int32 noOfRects;
			BRect r;
			
			if(cl->fLayerData->clipReg)
				cl->fLayerData->clipReg->MakeEmpty();
			else
				cl->fLayerData->clipReg = new BRegion();
			
			link.Read<int32>(&noOfRects);
			
			for(int i = 0; i < noOfRects; i++)
			{
				link.Read<BRect>(&r);
				cl->fLayerData->clipReg->Include(r);
			}

			cl->RebuildFullRegion();
			if (!(cl->IsHidden()))
			{
				if (cl->fParent)
					cl->fParent->FullInvalidate(BRegion(cl->fFull));
				else
					cl->FullInvalidate(BRegion(cl->fFull));
			}

			break;
		}
		case AS_LAYER_INVAL_RECT:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n",fTitle.String(), cl->fName->String()));
			
			// TODO: Watch out for the coordinate system in AS_LAYER_INVAL_RECT
			BRect		invalRect;
			
			link.Read<BRect>(&invalRect);
			
			cl->Invalidate(invalRect);
			
			break;
		}
		case AS_LAYER_INVAL_REGION:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n",fTitle.String(), cl->fName->String()));
			
			// TODO: Watch out for the coordinate system AS_LAYER_INVAL_REGION
			BRegion invalReg;
			int32 noOfRects;
			BRect rect;
			
			link.Read<int32>(&noOfRects);
			
			for(int i = 0; i < noOfRects; i++)
			{
				link.Read<BRect>(&rect);
				invalReg.Include(rect);
			}
			
			cl->Invalidate(invalReg);

			break;
		}
		case AS_BEGIN_UPDATE:
		{
			STRACE(("ServerWindowo %s: AS_BEGIN_UPDATE\n",fTitle.String()));
			cl->UpdateStart();
			break;
		}
		case AS_END_UPDATE:
		{
			STRACE(("ServerWindowo %s: AS_END_UPDATE\n",fTitle.String()));
			cl->UpdateEnd();
			break;
		}

		// ********** END: BView Messages ***********
		
		// ********* BWindow Messages ***********
		case AS_LAYER_DELETE_ROOT:
		{
			// Received when a window deletes its internal top view
			
			// TODO: Implement AS_LAYER_DELETE_ROOT
			STRACE(("ServerWindow %s: Message Delete_Layer_Root unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SHOW_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_SHOW_WINDOW\n",fTitle.String()));
			Show();
			break;
		}
		case AS_HIDE_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_HIDE_WINDOW\n",fTitle.String()));		
			Hide();
			break;
		}
		case AS_SEND_BEHIND:
		{
			// TODO: Implement AS_SEND_BEHIND
			STRACE(("ServerWindow %s: Message  Send_Behind unimplemented\n",fTitle.String()));
			break;
		}
		case AS_ENABLE_UPDATES:
		{
			// TODO: Implement AS_ENABLE_UPDATES
			STRACE(("ServerWindow %s: Message Enable_Updates unimplemented\n",fTitle.String()));
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			// TODO: Implement AS_DISABLE_UPDATES
			STRACE(("ServerWindow %s: Message Disable_Updates unimplemented\n",fTitle.String()));
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			// TODO: Implement AS_NEEDS_UPDATE
			STRACE(("ServerWindow %s: Message Needs_Update unimplemented\n",fTitle.String()));
			break;
		}
		case AS_WINDOW_TITLE:
		{
			// TODO: Implement AS_WINDOW_TITLE
			STRACE(("ServerWindow %s: Message Set_Title unimplemented\n",fTitle.String()));
			break;
		}
		case AS_ADD_TO_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_ADD_TO_SUBSET\n",fTitle.String()));
			WinBorder *wb;
			int32 mainToken;
			team_id	teamID;
			
			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));
			
			wb = desktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if(wb)
			{
				fMsgSender->StartMessage(SERVER_TRUE);
				fMsgSender->Flush();
				
				fWinBorder->AddToSubsetOf(wb);
			}
			else
			{
				fMsgSender->StartMessage(SERVER_FALSE);
				fMsgSender->Flush();
			}
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			STRACE(("ServerWindow %s: Message AS_REM_FROM_SUBSET\n",fTitle.String()));
			WinBorder *wb;
			int32 mainToken;
			team_id teamID;
			
			link.Read<int32>(&mainToken);
			link.Read(&teamID, sizeof(team_id));
			
			wb = desktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if(wb)
			{
				fMsgSender->StartMessage(SERVER_TRUE);
				fMsgSender->Flush();
				
				fWinBorder->RemoveFromSubsetOf(wb);
			}
			else
			{
				fMsgSender->StartMessage(SERVER_FALSE);
				fMsgSender->Flush();
			}
			break;
		}
		case AS_SET_LOOK:
		{
			// TODO: Implement AS_SET_LOOK
			STRACE(("ServerWindow %s: Message Set_Look unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_FLAGS:
		{
			// TODO: Implement AS_SET_FLAGS
			STRACE(("ServerWindow %s: Message Set_Flags unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_FEEL:
		{
			// TODO: Implement AS_SET_FEEL
			STRACE(("ServerWindow %s: Message Set_Feel unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_ALIGNMENT:
		{
			// TODO: Implement AS_SET_ALIGNMENT
			STRACE(("ServerWindow %s: Message Set_Alignment unimplemented\n",fTitle.String()));
			break;
		}
		case AS_GET_ALIGNMENT:
		{
			// TODO: Implement AS_GET_ALIGNMENT
			STRACE(("ServerWindow %s: Message Get_Alignment unimplemented\n",fTitle.String()));
			break;
		}
		case AS_GET_WORKSPACES:
		{
			// TODO: Implement AS_GET_WORKSPACES
			STRACE(("ServerWindow %s: Message Get_Workspaces unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_WORKSPACES:
		{
			// TODO: Implement AS_SET_WORKSPACES
			STRACE(("ServerWindow %s: Message Set_Workspaces unimplemented\n",fTitle.String()));
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			STRACE(("ServerWindow %s: Message AS_WINDOW_RESIZE\n",fTitle.String()));
			float xResizeBy;
			float yResizeBy;
			
			link.Read<float>(&xResizeBy);
			link.Read<float>(&yResizeBy);
			
			fWinBorder->ResizeBy(xResizeBy, yResizeBy);
			
			break;
		}
		case AS_WINDOW_MOVE:
		{
			STRACE(("ServerWindow %s: Message AS_WINDOW_MOVE\n",fTitle.String()));
			float xMoveBy;
			float yMoveBy;
			
			link.Read<float>(&xMoveBy);
			link.Read<float>(&yMoveBy);

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
			
			float wmin,wmax,hmin,hmax;
			
			link.Read<float>(&wmin);
			link.Read<float>(&wmax);
			link.Read<float>(&hmin);
			link.Read<float>(&hmax);
			
			fWinBorder->SetSizeLimits(wmin,wmax,hmin,hmax);
			
			fMsgSender->StartMessage(SERVER_TRUE);
			fMsgSender->Flush();
			break;
		}
		case B_MINIMIZE:
		{
			// TODO: Implement B_MINIMIZE
			STRACE(("ServerWindow %s: Message Minimize unimplemented\n",fTitle.String()));
			break;
		}
		case B_WINDOW_ACTIVATED:
		{
			// TODO: Implement B_WINDOW_ACTIVATED
			STRACE(("ServerWindow %s: Message Window_Activated unimplemented\n",fTitle.String()));
			break;
		}
		case B_ZOOM:
		{
			// TODO: Implement B_ZOOM
			STRACE(("ServerWindow %s: Message Zoom unimplemented\n",fTitle.String()));
			break;
		}
		
		// -------------------- Graphics messages ----------------------------------
		
		
		case AS_LAYER_SET_HIGH_COLOR:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_HIGH_COLOR: Layer: %s\n",fTitle.String(), cl->fName->String()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			cl->fLayerData->highcolor.SetColor(c);

			break;
		}
		case AS_LAYER_SET_LOW_COLOR:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_LOW_COLOR: Layer: %s\n",fTitle.String(), cl->fName->String()));
			rgb_color c;
			
			link.Read(&c, sizeof(rgb_color));
			
			cl->fLayerData->lowcolor.SetColor(c);
			
			break;
		}
		case AS_STROKE_LINE:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_LINE\n",fTitle.String()));
			
			// TODO: Add clipping TO AS_STROKE_LINE
			float x1, y1, x2, y2;
			
			link.Read<float>(&x1);
			link.Read<float>(&y1);
			link.Read<float>(&x2);
			link.Read<float>(&y2);
			
			if (cl && cl->fLayerData)
			{
				BPoint p1(x1,y1);
				BPoint p2(x2,y2);
				desktop->GetDisplayDriver()->StrokeLine(cl->ConvertToTop(p1),cl->ConvertToTop(p2),
						cl->fLayerData);
				
				// We update the pen here because many DisplayDriver calls which do not update the
				// pen position actually call StrokeLine
				cl->fLayerData->penlocation=p2;
			}
			break;
		}
		case AS_STROKE_RECT:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_RECT\n",fTitle.String()));
			
			// TODO: Add clipping TO AS_STROKE_RECT
			float left, top, right, bottom;
			link.Read<float>(&left);
			link.Read<float>(&top);
			link.Read<float>(&right);
			link.Read<float>(&bottom);
			BRect rect(left,top,right,bottom);
debugger("STROKERECT");
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeRect(cl->ConvertToTop(rect),cl->fLayerData);
			break;
		}
		case AS_FILL_RECT:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_RECT\n",fTitle.String()));
			
			// TODO: Add clipping TO AS_FILL_RECT
			BRect rect;
			link.Read<BRect>(&rect);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillRect(cl->ConvertToTop(rect),cl->fLayerData);
			break;
		}
		case AS_STROKE_ARC:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_ARC\n",fTitle.String()));
			
			// TODO: Add clipping to AS_STROKE_ARC
			float angle, span;
			BRect r;
			
			link.Read<BRect>(&r);
			link.Read<float>(&angle);
			link.Read<float>(&span);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeArc(cl->ConvertToTop(r),angle,span,cl->fLayerData);
			break;
		}
		case AS_FILL_ARC:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_ARC\n",fTitle.String()));
			
			// TODO: Add clipping to AS_FILL_ARC
			float angle, span;
			BRect r;
			
			link.Read<BRect>(&r);
			link.Read<float>(&angle);
			link.Read<float>(&span);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillArc(cl->ConvertToTop(r),angle,span,cl->fLayerData);
			break;
		}
		case AS_STROKE_BEZIER:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_BEZIER\n",fTitle.String()));
			
			// TODO: Add clipping to AS_STROKE_BEZIER
			BPoint *pts;
			int i;
			pts = new BPoint[4];
			
			for (i=0; i<4; i++)
				link.Read<BPoint>(&(pts[i]));
			
			if (cl && cl->fLayerData)
			{
				for (i=0; i<4; i++)
					pts[i]=cl->ConvertToTop(pts[i]);
				
				desktop->GetDisplayDriver()->StrokeBezier(pts,cl->fLayerData);
			}
			delete [] pts;
			break;
		}
		case AS_FILL_BEZIER:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_BEZIER\n",fTitle.String()));
			
			// TODO: Add clipping to AS_STROKE_BEZIER
			BPoint *pts;
			int i;
			pts = new BPoint[4];
			
			for (i=0; i<4; i++)
				link.Read<BPoint>(&(pts[i]));
			
			if (cl && cl->fLayerData)
			{
				for (i=0; i<4; i++)
					pts[i]=cl->ConvertToTop(pts[i]);
				
				desktop->GetDisplayDriver()->FillBezier(pts,cl->fLayerData);
			}
			delete [] pts;
			break;
		}
		case AS_STROKE_ELLIPSE:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_ELLIPSE\n",fTitle.String()));
			
			// TODO: Add clipping AS_STROKE_ELLIPSE
			BRect rect;
			link.Read<BRect>(&rect);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeEllipse(cl->ConvertToTop(rect),cl->fLayerData);
			break;
		}
		case AS_FILL_ELLIPSE:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_ELLIPSE\n",fTitle.String()));
			
			// TODO: Add clipping AS_STROKE_ELLIPSE
			BRect rect;
			link.Read<BRect>(&rect);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillEllipse(cl->ConvertToTop(rect),cl->fLayerData);
			break;
		}
		case AS_STROKE_ROUNDRECT:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_ROUNDRECT\n",fTitle.String()));
			
			// TODO: Add clipping AS_STROKE_ROUNDRECT
			BRect rect;
			float xrad,yrad;
			link.Read<BRect>(&rect);
			link.Read<float>(&xrad);
			link.Read<float>(&yrad);
			
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeRoundRect(cl->ConvertToTop(rect),xrad,yrad,cl->fLayerData);
			break;
		}
		case AS_FILL_ROUNDRECT:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_ROUNDRECT\n",fTitle.String()));
			
			// TODO: Add clipping AS_STROKE_ROUNDRECT
			BRect rect;
			float xrad,yrad;
			link.Read<BRect>(&rect);
			link.Read<float>(&xrad);
			link.Read<float>(&yrad);
			
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillRoundRect(cl->ConvertToTop(rect),xrad,yrad,cl->fLayerData);
			break;
		}
		case AS_STROKE_TRIANGLE:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_TRIANGLE\n",fTitle.String()));
			
			// TODO:: Add clipping to AS_STROKE_TRIANGLE
			BPoint pts[3];
			BRect rect;
			
			for (int i=0; i<3; i++)
				link.Read<BPoint>(&(pts[i]));
			
			link.Read<BRect>(&rect);
			
			if (cl && cl->fLayerData)
			{
				for(int i=0;i<3;i++)
					pts[i]=cl->ConvertToTop(pts[i]);
				
				desktop->GetDisplayDriver()->StrokeTriangle(pts,cl->ConvertToTop(rect),cl->fLayerData);
			}
			break;
		}
		case AS_FILL_TRIANGLE:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_TRIANGLE\n",fTitle.String()));
			
			// TODO:: Add clipping to AS_FILL_TRIANGLE
			BPoint pts[3];
			BRect rect;
			
			for (int i=0; i<3; i++)
				link.Read<BPoint>(&(pts[i]));
			
			link.Read<BRect>(&rect);
			
			if (cl && cl->fLayerData)
			{
				for(int i=0;i<3;i++)
					pts[i]=cl->ConvertToTop(pts[i]);
				
				desktop->GetDisplayDriver()->FillTriangle(pts,cl->ConvertToTop(rect),cl->fLayerData);
			}
			break;
		}
		case AS_STROKE_POLYGON:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_POLYGON\n",fTitle.String()));
			
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
				pointlist[i]=cl->ConvertToTop(pointlist[i]);
			
			desktop->GetDisplayDriver()->StrokePolygon(pointlist,pointcount,polyframe,
					cl->fLayerData,isclosed);
			
			delete [] pointlist;
			
			break;
		}
		case AS_FILL_POLYGON:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_POLYGON\n",fTitle.String()));
			
			BRect polyframe;
			int32 pointcount;
			BPoint *pointlist;
			
			link.Read<BRect>(&polyframe);
			link.Read<int32>(&pointcount);
			
			pointlist=new BPoint[pointcount];
			
			link.Read(pointlist, sizeof(BPoint)*pointcount);
			
			for(int32 i=0; i<pointcount; i++)
				pointlist[i]=cl->ConvertToTop(pointlist[i]);
			
			desktop->GetDisplayDriver()->FillPolygon(pointlist,pointcount,polyframe,cl->fLayerData);
			
			delete [] pointlist;
			
			break;
		}
		case AS_STROKE_SHAPE:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_SHAPE\n",fTitle.String()));
			
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
				ptlist[i]=cl->ConvertToTop(ptlist[i]);
			
			desktop->GetDisplayDriver()->StrokeShape(shaperect, opcount, oplist, ptcount, ptlist, cl->fLayerData);
			delete oplist;
			delete ptlist;
			
			break;
		}
		case AS_FILL_SHAPE:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_SHAPE\n",fTitle.String()));
			
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
				ptlist[i]=cl->ConvertToTop(ptlist[i]);
			
			desktop->GetDisplayDriver()->FillShape(shaperect, opcount, oplist, ptcount, ptlist, cl->fLayerData);

			delete oplist;
			delete ptlist;
			
			break;
		}
		case AS_FILL_REGION:
		{
			STRACE(("ServerWindow %s: Message AS_FILL_REGION\n",fTitle.String()));
			
			int32 rectcount;
			BRect *rectlist;
			
			link.Read<int32>(&rectcount);
			
			rectlist=new BRect[rectcount];
			
			link.Read(rectlist, sizeof(BRect)*rectcount);
			
			// Between the client-side conversion to BRects from clipping_rects to the overhead
			// in repeatedly calling FillRect(), this is definitely in need of optimization. At
			// least it works for now. :)
			for(int32 i=0; i<rectcount; i++)
				desktop->GetDisplayDriver()->FillRect(cl->ConvertToTop(rectlist[i]),cl->fLayerData);
			
			delete [] rectlist;
			
			// TODO: create support for clipping_rect usage for faster BRegion display.
			// Tweaks to DisplayDriver are necessary along with conversion routines in Layer
			
			break;
		}
		case AS_STROKE_LINEARRAY:
		{
			STRACE(("ServerWindow %s: Message AS_STROKE_LINEARRAY\n",fTitle.String()));
			
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
					
					index->pt1=cl->ConvertToTop(index->pt1);
					index->pt2=cl->ConvertToTop(index->pt2);
				}				
				desktop->GetDisplayDriver()->StrokeLineArray(linecount,linedata,cl->fLayerData);
			}
			break;
		}
		case AS_DRAW_STRING:
		{
			STRACE(("ServerWindow %s: Message AS_DRAW_STRING\n",fTitle.String()));
			
			char *string;
			int32 length;
			BPoint location;
			escapement_delta delta;
			
			link.Read<int32>(&length);
			link.Read<BPoint>(&location);
			link.Read<escapement_delta>(&delta);
			link.ReadString(&string);
			
			if(cl && cl->fLayerData)
				desktop->GetDisplayDriver()->DrawString(string,length,cl->ConvertToTop(location),
						cl->fLayerData);
			
			free(string);
			break;
		}
		case AS_MOVEPENTO:
		{
			STRACE(("ServerWindow %s: Message AS_MOVEPENTO\n",fTitle.String()));
			
			float x,y;
			
			link.Read<float>(&x);
			link.Read<float>(&y);
			if(cl && cl->fLayerData)
				cl->fLayerData->penlocation.Set(x,y);
			
			break;
		}
		case AS_SETPENSIZE:
		{
			STRACE(("ServerWindow %s: Message AS_SETPENSIZE\n",fTitle.String()));
			float size;
			
			link.Read<float>(&size);
			if(cl && cl->fLayerData)
				cl->fLayerData->pensize=size;
			
			break;
		}
		case AS_SET_FONT:
		{
			STRACE(("ServerWindow %s: Message AS_SET_FONT\n",fTitle.String()));
			// TODO: Implement AS_SET_FONT?
			break;
		}
		case AS_SET_FONT_SIZE:
		{
			STRACE(("ServerWindow %s: Message AS_SET_FONT_SIZE\n",fTitle.String()));
			// TODO: Implement AS_SET_FONT_SIZE?
			break;
		}
		case AS_AREA_MESSAGE:
		{
			STRACE(("ServerWindow %s: Message AS_AREA_MESSAGE\n",fTitle.String()));
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
			fServerApp->fSharedMem->ReleaseBuffer(msgpointer);
			
			break;
		}
		default:
		{
			printf("ServerWindow %s received unexpected code - message offset %lx\n",fTitle.String(), code - SERVER_TRUE);
			break;
		}
	}
}
//------------------------------------------------------------------------------
/*!
	\brief Message-dispatching loop for the ServerWindow

	MonitorWin() watches the ServerWindow's message port and dispatches as necessary
	\param data The thread's ServerWindow
	\return Throwaway code. Always 0.
*/
int32 ServerWindow::MonitorWin(void *data)
{
	ServerWindow 	*win = (ServerWindow *)data;
	LinkMsgReader *ses = win->fMsgReader;
	
	bool			quitting = false;
	int32			code;
	status_t		err = B_OK;
	
	while(!quitting)
	{
		printf("info: ServerWindow::MonitorWin listening on port %ld.\n", win->fMessagePort);
		code = AS_CLIENT_DEAD;
		err = ses->GetNextMessage(&code);
		if (err < B_OK)
			return err;

		win->Lock();

		switch(code)
		{
			case AS_DELETE_WINDOW:
			case AS_CLIENT_DEAD:
			{
				// this means the client has been killed
				STRACE(("ServerWindow %s received 'AS_CLIENT_DEAD/AS_DELETE_WINDOW' message code\n",win->Title()));
				quitting = true;
				
				// ServerWindow's destructor takes care of pulling this object off the desktop.
				delete win;
				break;
			}
			case B_QUIT_REQUESTED:
			{
				STRACE(("ServerWindow %s received Quit request\n",win->Title()));
				quitting = true;
				delete win;
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
//------------------------------------------------------------------------------
Layer* ServerWindow::FindLayer(const Layer* start, int32 token) const
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::FindLayer()\n");

	if(!start)
		return NULL;
	
	// see if we're looking for 'start'
	if(start->fViewToken == token)
		return const_cast<Layer*>(start);

	Layer *c = start->fTopChild; //c = short for: current
	if(c != NULL)
		while(true)
		{
			// action block
			{
				if(c->fViewToken == token)
					return c;
			}

				// go deep
			if(	c->fTopChild)
			{
				c = c->fTopChild;
			}
			// go right or up
			else
				// go right
				if(c->fLowerSibling)
				{
					c = c->fLowerSibling;
				}
				// go up
				else
				{
					while(!c->fParent->fLowerSibling && c->fParent != start)
					{
						c = c->fParent;
					}
					// that enough! We've reached the start layer.
					if(c->fParent == start)
						break;
						
					c = c->fParent->fLowerSibling;
				}
		}

	return NULL;
}
//------------------------------------------------------------------------------
void ServerWindow::SendMessageToClient(const BMessage* msg) const
{
	if (!IsLocked())
		debugger("you must lock a ServerWindow object before calling ::SendMessageToClient()\n");

	ssize_t		size;
	char		*buffer;
	
	size		= msg->FlattenedSize();
	buffer		= new char[size];

	if (msg->Flatten(buffer, size) == B_OK)
		write_port(fClientLooperPort, msg->what, buffer, size);
	else
		printf("PANIC: ServerWindow %s: can't flatten message in 'SendMessageToClient()'\n", fTitle.String());

	delete buffer;
}
//------------------------------------------------------------------------------

