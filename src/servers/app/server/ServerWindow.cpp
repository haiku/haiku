//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
#include <View.h>	// for B_XXXXX_MOUSE_BUTTON defines
#include <Message.h>
#include <GraphicsDefs.h>
#include <PortLink.h>
#include <Session.h>
#include "Desktop.h"
#include "AppServer.h"
#include "Layer.h"
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

#define DEBUG_SERVERWINDOW
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

	fSession = new BPortLink(fClientWinPort, fMessagePort);
	
	// Send a reply to our window - it is expecting fMessagePort port.
	fSession->StartMessage(SERVER_TRUE);
	fSession->Attach<port_id>(fMessagePort);
	fSession->Flush();

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
	
	if(fSession)
	{
		delete fSession;
		fSession = NULL;
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
		STRACE(("ServerWindow(%s)::Show() - General lock acquired\n", fWinBorder->GetName()));

		rl->fMainLock.Lock();
		STRACE(("ServerWindow(%s)::Show() - Main lock acquired\n", fWinBorder->GetName()));

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
		STRACE(("ServerWindow(%s)::Show() - Main lock released\n", fWinBorder->GetName()));
		
		desktop->fGeneralLock.Unlock();
		STRACE(("ServerWindow(%s)::Show() - General lock released\n", fWinBorder->GetName()));
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
		STRACE(("ServerWindow(%s)::Hide() - General lock acquired\n", fWinBorder->GetName()));
		
		rl->fMainLock.Lock();
		STRACE(("ServerWindow(%s)::Hide() - Main lock acquired\n", fWinBorder->GetName()));
		
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
		STRACE(("ServerWindow(%s)::Hide() - Main lock released\n", fWinBorder->GetName()));

		desktop->fGeneralLock.Unlock();
		STRACE(("ServerWindow(%s)::Hide() - General lock released\n", fWinBorder->GetName()));
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
void ServerWindow::SetLayerFontState(Layer *layer)
{
	// NOTE: no need to check for a lock. This is a private method.
	uint16 mask;

	fSession->Read<uint16>(&mask);
			
	if (mask & B_FONT_FAMILY_AND_STYLE)
	{
		uint32		fontID;
		fSession->Read<int32>((int32*)&fontID);
		
		// TODO: Implement. ServerFont::SetFamilyAndStyle(uint32) is needed
		//layer->fLayerData->font->
	}

	if (mask & B_FONT_SIZE)
	{
		float size;
		fSession->Read<float>(&size);
		layer->fLayerData->font.SetSize(size);
	}
	
	if (mask & B_FONT_SHEAR)
	{
		float shear;
		fSession->Read<float>(&shear);
		layer->fLayerData->font.SetShear(shear);
	}

	if (mask & B_FONT_ROTATION)
	{
		float rotation;
		fSession->Read<float>(&rotation);
		layer->fLayerData->font.SetRotation(rotation);
	}

	if (mask & B_FONT_SPACING)
	{
		uint8 spacing;
		fSession->Read<uint8>(&spacing);
		layer->fLayerData->font.SetSpacing(spacing);
	}

	if (mask & B_FONT_ENCODING)
	{
		uint8 encoding;
		fSession->Read<uint8>((uint8*)&encoding);
		layer->fLayerData->font.SetEncoding(encoding);
	}

	if (mask & B_FONT_FACE)
	{
		uint16 face;
		fSession->Read<uint16>(&face);
		layer->fLayerData->font.SetFace(face);
	}

	if (mask & B_FONT_FLAGS)
	{
		uint32 flags;
		fSession->Read<uint32>(&flags);
		layer->fLayerData->font.SetFlags(flags);
	}
	STRACE(("DONE: ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer: %s\n",
			fTitle.String(), layer->fName->String()));
}
//------------------------------------------------------------------------------
void ServerWindow::SetLayerState(Layer *layer)
{
	// NOTE: no need to check for a lock. This is a private method.
	rgb_color highColor, lowColor, viewColor;
	pattern patt;
	int32 clipRegRects;

	fSession->Read<BPoint>(		&(layer->fLayerData->penlocation));
	fSession->Read<float>(		&(layer->fLayerData->pensize));
	fSession->Read(			&highColor, sizeof(rgb_color));
	fSession->Read(			&lowColor, sizeof(rgb_color));
	fSession->Read(			&viewColor, sizeof(rgb_color));
	fSession->Read(			&patt, sizeof(pattern));
	fSession->Read<int8>((int8*)	&(layer->fLayerData->draw_mode));
	fSession->Read<BPoint>(		&(layer->fLayerData->coordOrigin));
	fSession->Read<int8>((int8*)	&(layer->fLayerData->lineJoin));
	fSession->Read<int8>((int8*)	&(layer->fLayerData->lineCap));
	fSession->Read<float>(		&(layer->fLayerData->miterLimit));
	fSession->Read<int8>((int8*)	&(layer->fLayerData->alphaSrcMode));
	fSession->Read<int8>((int8*)	&(layer->fLayerData->alphaFncMode));
	fSession->Read<float>(		&(layer->fLayerData->scale));
	fSession->Read<bool>(			&(layer->fLayerData->fontAliasing));
	fSession->Read<int32>(		&clipRegRects);

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
			fSession->Read<BRect>(&rect);
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
Layer * ServerWindow::CreateLayerTree(Layer *localRoot)
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
		
	fSession->Read<int32>(&token);
	fSession->ReadString(&name);
	fSession->Read<BRect>(&frame);
	fSession->Read<uint32>(&resizeMask);
	fSession->Read<uint32>(&flags);
	fSession->Read<bool>(&hidden);
	fSession->Read<int32>(&childCount);
			
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
void ServerWindow::DispatchMessage(int32 code)
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
			
			fSession->Read<int32>(&bitmapToken);
			fSession->Read<BPoint>(&point);
			
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
			
			fSession->Read<int32>(&bitmapToken);
			fSession->Read<BPoint>(&point);
			
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
			
			fSession->Read<int32>(&bitmapToken);
			fSession->Read<BRect>(&dstRect);
			fSession->Read<BRect>(&srcRect);
			
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
			
			fSession->Read<int32>(&bitmapToken);
			fSession->Read<BRect>(&dstRect);
			fSession->Read<BRect>(&srcRect);
			
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
			
			fSession->Read<int32>(&token);
			
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
			
			fWinBorder->fTopLayer = CreateLayerTree(NULL);
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

			newLayer	= CreateLayerTree(NULL);
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
			SetLayerState(cl);
			cl->RebuildFullRegion();
			break;
		}
		case AS_LAYER_SET_FONT_STATE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			SetLayerFontState(cl);
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
			fSession->StartMessage(SERVER_TRUE);
			
			// Attach font state
			fSession->Attach<uint32>(0UL /*uint32 ld->font.GetFamAndStyle()*/);
			fSession->Attach<float>(ld->font.Size());
			fSession->Attach<float>(ld->font.Shear());
			fSession->Attach<float>(ld->font.Rotation());
			fSession->Attach<uint8>(ld->font.Spacing());
			fSession->Attach<uint8>(ld->font.Encoding());
			fSession->Attach<uint16>(ld->font.Face());
			fSession->Attach<uint32>(ld->font.Flags());
			
			// Attach view state
			fSession->Attach<BPoint>(ld->penlocation);
			fSession->Attach<float>(ld->pensize);
			fSession->Attach(&hc, sizeof(rgb_color));
			fSession->Attach(&lc, sizeof(rgb_color));
			fSession->Attach(&vc, sizeof(rgb_color));
			fSession->Attach<uint64>(patt);
			fSession->Attach<BPoint>(ld->coordOrigin);
			fSession->Attach<uint8>((uint8)(ld->draw_mode));
			fSession->Attach<uint8>((uint8)(ld->lineCap));
			fSession->Attach<uint8>((uint8)(ld->lineJoin));
			fSession->Attach<float>(ld->miterLimit);
			fSession->Attach<uint8>((uint8)(ld->alphaSrcMode));
			fSession->Attach<uint8>((uint8)(ld->alphaFncMode));
			fSession->Attach<float>(ld->scale);
			fSession->Attach<float>(ld->fontAliasing);
			
			int32 noOfRects = 0;
			if (ld->clipReg)
				noOfRects = ld->clipReg->CountRects();
			
			fSession->Attach<int32>(noOfRects);
			
			for(int i = 0; i < noOfRects; i++)
				fSession->Attach<BRect>(ld->clipReg->RectAt(i));
			
			fSession->Attach<float>(cl->fFrame.left);
			fSession->Attach<float>(cl->fFrame.top);
			fSession->Attach<BRect>(cl->fFrame.OffsetToCopy(cl->fBoundsLeftTop));
			fSession->Flush();

			break;
		}
		case AS_LAYER_MOVETO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_MOVETO: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			float x, y;
			
			fSession->Read<float>(&x);
			fSession->Read<float>(&y);
			
			cl->MoveBy(x, y);
			
			break;
		}
		case AS_LAYER_RESIZETO:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZETO: Layer name: %s\n", fTitle.String(), cl->fName->String()));
			float newWidth, newHeight;
			
			fSession->Read<float>(&newWidth);
			fSession->Read<float>(&newHeight);
			
			// TODO: Check for minimum size allowed. Need WinBorder::GetSizeLimits
			
			cl->ResizeBy(newWidth, newHeight);
			
			break;
		}
		case AS_LAYER_GET_COORD:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<float>(cl->fFrame.left);
			fSession->Attach<float>(cl->fFrame.top);
			fSession->Attach<BRect>(cl->fFrame.OffsetToCopy(cl->fBoundsLeftTop));
			fSession->Flush();

			break;
		}
		case AS_LAYER_SET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_ORIGIN: Layer: %s\n",fTitle.String(), cl->fName->String()));
			float x, y;
			
			fSession->Read<float>(&x);
			fSession->Read<float>(&y);
			
			cl->fLayerData->coordOrigin.Set(x, y);

			break;
		}
		case AS_LAYER_GET_ORIGIN:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_ORIGIN: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<BPoint>(cl->fLayerData->coordOrigin);
			fSession->Flush();

			break;
		}
		case AS_LAYER_RESIZE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->Read<uint32>(&(cl->fResizeMode));
			
			break;
		}
		case AS_LAYER_CURSOR:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CURSOR: Layer: %s\n",fTitle.String(), cl->fName->String()));
			int32 token;

			fSession->Read<int32>(&token);
			
			cursormanager->SetCursor(token);

			break;
		}
		case AS_LAYER_SET_FLAGS:
		{
			fSession->Read<uint32>(&(cl->fFlags));
			
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

			fSession->Read<int8>(&lineCap);
			fSession->Read<int8>(&lineJoin);
			fSession->Read<float>(&(cl->fLayerData->miterLimit));
			
			cl->fLayerData->lineCap	= (cap_mode)lineCap;
			cl->fLayerData->lineJoin = (join_mode)lineJoin;
		
			break;
		}
		case AS_LAYER_GET_LINE_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_LINE_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<int8>((int8)(cl->fLayerData->lineCap));
			fSession->Attach<int8>((int8)(cl->fLayerData->lineJoin));
			fSession->Attach<float>(cl->fLayerData->miterLimit);
			fSession->Flush();
		
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
			fSession->Read<float>(&(cl->fLayerData->scale));
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_SCALE: Layer: %s\n",fTitle.String(), cl->fName->String()));		
			LayerData		*ld = cl->fLayerData;

			float			scale = ld->scale;
			
			while((ld = ld->prevState))
				scale		*= ld->scale;
			
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<float>(scale);
			fSession->Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_LOC:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_LOC: Layer: %s\n",fTitle.String(), cl->fName->String()));
			float		x, y;
			
			fSession->Read<float>(&x);
			fSession->Read<float>(&y);

			cl->fLayerData->penlocation.Set(x, y);

			break;
		}
		case AS_LAYER_GET_PEN_LOC:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_LOC: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<BPoint>(cl->fLayerData->penlocation);
			fSession->Flush();
		
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_SIZE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->Read<float>(&(cl->fLayerData->pensize));
		
			break;
		}
		case AS_LAYER_GET_PEN_SIZE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_SIZE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<float>(cl->fLayerData->pensize);
			fSession->Flush();
		
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: Layer: %s\n",fTitle.String(), cl->fName->String()));
			rgb_color c;
			
			fSession->Read(&c, sizeof(rgb_color));
			
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
			
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach(&highColor, sizeof(rgb_color));
			fSession->Attach(&lowColor, sizeof(rgb_color));
			fSession->Attach(&viewColor, sizeof(rgb_color));
			fSession->Flush();

			break;
		}
		case AS_LAYER_SET_BLEND_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_BLEND_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			int8 srcAlpha, alphaFunc;
			
			fSession->Read<int8>(&srcAlpha);
			fSession->Read<int8>(&alphaFunc);
			
			cl->fLayerData->alphaSrcMode = (source_alpha)srcAlpha;
			cl->fLayerData->alphaFncMode = (alpha_function)alphaFunc;

			break;
		}
		case AS_LAYER_GET_BLEND_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_BLEND_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<int8>((int8)(cl->fLayerData->alphaSrcMode));
			fSession->Attach<int8>((int8)(cl->fLayerData->alphaFncMode));
			fSession->Flush();

			break;
		}
		case AS_LAYER_SET_DRAW_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_DRAW_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			int8 drawingMode;
			
			fSession->Read<int8>(&drawingMode);
			
			cl->fLayerData->draw_mode = (drawing_mode)drawingMode;
			
			break;
		}
		case AS_LAYER_GET_DRAW_MODE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_DRAW_MODE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->StartMessage(SERVER_TRUE);
			fSession->Attach<int8>((int8)(cl->fLayerData->draw_mode));
			fSession->Flush();
		
			break;
		}
		case AS_LAYER_PRINT_ALIASING:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_PRINT_ALIASING: Layer: %s\n",fTitle.String(), cl->fName->String()));
			fSession->Read<bool>(&(cl->fLayerData->fontAliasing));
			
			break;
		}
		case AS_LAYER_CLIP_TO_PICTURE:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_PICTURE: Layer: %s\n",fTitle.String(), cl->fName->String()));
			
			// TODO: Watch out for the coordinate system in AS_LAYER_CLIP_TO_PICTURE
			int32 pictureToken;
			BPoint where;
			
			fSession->Read<int32>(&pictureToken);
			fSession->Read<BPoint>(&where);
			
			
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
			
			fSession->Read<int32>(&pictureToken);
			fSession->Read<BPoint>(&where);
			
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
				fSession->StartMessage(SERVER_TRUE);
				fSession->Attach<int32>(0L);
				fSession->Flush();
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
				fSession->StartMessage(SERVER_TRUE);
				fSession->Attach<int32>(noOfRects);

				for(int i = 0; i < noOfRects; i++)
					fSession->Attach<BRect>(reg.RectAt(i));
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
			
			fSession->Read<int32>(&noOfRects);
			
			for(int i = 0; i < noOfRects; i++)
			{
				fSession->Read<BRect>(&r);
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
			
			fSession->Read<BRect>(&invalRect);
			
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
			
			fSession->Read<int32>(&noOfRects);
			
			for(int i = 0; i < noOfRects; i++)
			{
				fSession->Read<BRect>(&rect);
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
			WinBorder *wb;
			int32 mainToken;
			team_id	teamID;
			
			fSession->Read<int32>(&mainToken);
			fSession->Read(&teamID, sizeof(team_id));
			
			wb = desktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if(wb)
			{
				fSession->StartMessage(SERVER_TRUE);
				fSession->Flush();
				
				fWinBorder->AddToSubsetOf(wb);
			}
			else
			{
				fSession->StartMessage(SERVER_FALSE);
				fSession->Flush();
			}
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			WinBorder *wb;
			int32 mainToken;
			team_id teamID;
			
			fSession->Read<int32>(&mainToken);
			fSession->Read(&teamID, sizeof(team_id));
			
			wb = desktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if(wb)
			{
				fSession->StartMessage(SERVER_TRUE);
				fSession->Flush();
				
				fWinBorder->RemoveFromSubsetOf(wb);
			}
			else
			{
				fSession->StartMessage(SERVER_FALSE);
				fSession->Flush();
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
			float xResizeBy;
			float yResizeBy;
			
			fSession->Read<float>(&xResizeBy);
			fSession->Read<float>(&yResizeBy);
			
			fWinBorder->ResizeBy(xResizeBy, yResizeBy);
			
			break;
		}
		case AS_WINDOW_MOVE:
		{
			float xMoveBy;
			float yMoveBy;
			
			fSession->Read<float>(&xMoveBy);
			fSession->Read<float>(&yMoveBy);

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
			
			fSession->Read<float>(&wmin);
			fSession->Read<float>(&wmax);
			fSession->Read<float>(&hmin);
			fSession->Read<float>(&hmax);
			
			fWinBorder->SetSizeLimits(wmin,wmax,hmin,hmax);
			
			fSession->StartMessage(SERVER_TRUE);
			fSession->Flush();
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
			
			fSession->Read(&c, sizeof(rgb_color));
			
			cl->fLayerData->highcolor.SetColor(c);

			break;
		}
		case AS_LAYER_SET_LOW_COLOR:
		{
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_LOW_COLOR: Layer: %s\n",fTitle.String(), cl->fName->String()));
			rgb_color c;
			
			fSession->Read(&c, sizeof(rgb_color));
			
			cl->fLayerData->lowcolor.SetColor(c);
			
			break;
		}
		case AS_STROKE_LINE:
		{
			// TODO: Add clipping TO AS_STROKE_LINE
			float x1, y1, x2, y2;
			
			fSession->Read<float>(&x1);
			fSession->Read<float>(&y1);
			fSession->Read<float>(&x2);
			fSession->Read<float>(&y2);
			
			if (cl && cl->fLayerData)
			{
				BPoint p1(x1,y1);
				BPoint p2(x2,y2);
				desktop->GetDisplayDriver()->StrokeLine(cl->ConvertToTop(p1),cl->ConvertToTop(p2),
						cl->fLayerData);
			}
			break;
		}
		case AS_STROKE_RECT:
		{
			// TODO: Add clipping TO AS_STROKE_RECT
			float left, top, right, bottom;
			fSession->Read<float>(&left);
			fSession->Read<float>(&top);
			fSession->Read<float>(&right);
			fSession->Read<float>(&bottom);
			BRect rect(left,top,right,bottom);

			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeRect(cl->ConvertToTop(rect),cl->fLayerData);
		}
		case AS_FILL_RECT:
		{
			// TODO: Add clipping TO AS_STROKE_RECT
			BRect rect;
			fSession->Read<BRect>(&rect);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillRect(cl->ConvertToTop(rect),cl->fLayerData);
		}
		case AS_STROKE_ARC:
		{
			// TODO: Add clipping to AS_STROKE_ARC
			float angle, span;
			BRect r;
			
			fSession->Read<BRect>(&r);
			fSession->Read<float>(&angle);
			fSession->Read<float>(&span);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeArc(cl->ConvertToTop(r),angle,span,cl->fLayerData);
			break;
		}
		case AS_FILL_ARC:
		{
			// TODO: Add clipping to AS_STROKE_ARC
			float angle, span;
			BRect r;
			
			fSession->Read<BRect>(&r);
			fSession->Read<float>(&angle);
			fSession->Read<float>(&span);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillArc(cl->ConvertToTop(r),angle,span,cl->fLayerData);
			break;
		}
		case AS_STROKE_BEZIER:
		{
			// TODO: Add clipping to AS_STROKE_BEZIER
			BPoint *pts;
			int i;
			pts = new BPoint[4];
			
			for (i=0; i<4; i++)
				fSession->Read<BPoint>(&(pts[i]));
			
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
			// TODO: Add clipping to AS_STROKE_BEZIER
			BPoint *pts;
			int i;
			pts = new BPoint[4];
			
			for (i=0; i<4; i++)
				fSession->Read<BPoint>(&(pts[i]));
			
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
			// TODO: Add clipping AS_STROKE_ELLIPSE
			BRect rect;
			fSession->Read<BRect>(&rect);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeEllipse(cl->ConvertToTop(rect),cl->fLayerData);
			break;
		}
		case AS_FILL_ELLIPSE:
		{
			// TODO: Add clipping AS_STROKE_ELLIPSE
			BRect rect;
			fSession->Read<BRect>(&rect);
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillEllipse(cl->ConvertToTop(rect),cl->fLayerData);
			break;
		}
		case AS_STROKE_ROUNDRECT:
		{
			// TODO: Add clipping AS_STROKE_ROUNDRECT
			BRect rect;
			float xrad,yrad;
			fSession->Read<BRect>(&rect);
			fSession->Read<float>(&xrad);
			fSession->Read<float>(&yrad);
			
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->StrokeRoundRect(cl->ConvertToTop(rect),xrad,yrad,cl->fLayerData);
			break;
		}
		case AS_FILL_ROUNDRECT:
		{
			// TODO: Add clipping AS_STROKE_ROUNDRECT
			BRect rect;
			float xrad,yrad;
			fSession->Read<BRect>(&rect);
			fSession->Read<float>(&xrad);
			fSession->Read<float>(&yrad);
			
			if (cl && cl->fLayerData)
				desktop->GetDisplayDriver()->FillRoundRect(cl->ConvertToTop(rect),xrad,yrad,cl->fLayerData);
			break;
		}
		case AS_STROKE_TRIANGLE:
		{
			// TODO:: Add clipping to AS_STROKE_TRIANGLE
			BPoint pts[3];
			BRect rect;
			
			for (int i=0; i<3; i++)
				fSession->Read<BPoint>(&(pts[i]));
			
			fSession->Read<BRect>(&rect);
			
			if (cl && cl->fLayerData)
			{
				for(int i=0;i<3;i++)
					pts[i]=cl->ConvertToTop(pts[i]);
				
				// TODO: modify DisplayDriver::StrokeTriangle to utilize a boundary BRect
				desktop->GetDisplayDriver()->StrokeTriangle(pts,cl->fLayerData);
//				desktop->GetDisplayDriver()->StrokeTriangle(pts,cl->ConvertToTop(rect),cl->fLayerData);
			}
			break;
		}
		case AS_FILL_TRIANGLE:
		{
			// TODO:: Add clipping to AS_FILL_TRIANGLE
			BPoint pts[3];
			BRect rect;
			
			for (int i=0; i<3; i++)
				fSession->Read<BPoint>(&(pts[i]));
			
			fSession->Read<BRect>(&rect);
			
			if (cl && cl->fLayerData)
			{
				for(int i=0;i<3;i++)
					pts[i]=cl->ConvertToTop(pts[i]);
				
				// TODO: modify DisplayDriver::FillTriangle to utilize a boundary BRect
				desktop->GetDisplayDriver()->FillTriangle(pts,cl->fLayerData);
//				desktop->GetDisplayDriver()->FillTriangle(pts,cl->ConvertToTop(rect),cl->fLayerData);
			}
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
	\brief Iterator for graphics update messages
	\param msgsize Size of the buffer containing the graphics messages
	\param msgbuffer Buffer containing the graphics message
*/
void ServerWindow::DispatchGraphicsMessage(int32 msgsize, int8 *msgbuffer)
{
/*	Layer *layer;
	LayerData *layerdata;
	int32 code;
	int32 view_token;
	uint32 sizeRemaining = (uint32)msgsize;
	BRegion WindowClipRegion;
	BRegion LayerClipRegion;
//	Layer *sibling;
	int32 numRects = 0;
	
	if (!msgsize || !msgbuffer)
		return;
	if (IsHidden())
		return;
		
	// TODO: fix sibling-related clipping calculations in DispatchGraphicsMessage
//	WindowClipRegion.Set(fWinBorder->Frame());
//	sibling = fWinBorder->UpperSibling();
//	while (sibling)
//	{
//		WindowClipRegion.Exclude(sibling->Frame());
//		sibling = sibling->UpperSibling();
//	}


	if (!WindowClipRegion.Frame().IsValid())
		return;
	
	// We need to decide whether coordinates are specified in view or root coordinates.
	// For now, we assume root level coordinates.
	code = AS_BEGIN_UPDATE;
	while ((sizeRemaining > 2*sizeof(int32)) && (code != AS_END_UPDATE))
	{
		code = read_from_buffer<int32>(&msgbuffer);
		view_token = read_from_buffer<int32>(&msgbuffer);
		
		//TODO: fix code to find a layer based on a view token in DispatchGraphicsMessage
		layer = NULL;//fWorkspace->GetRoot()->FindLayer(view_token);
		
		if (layer)
		{
			layerdata = layer->fLayerData;
			LayerClipRegion.Set(layer->Frame());
			LayerClipRegion.IntersectWith(&WindowClipRegion);
			numRects = LayerClipRegion.CountRects();
		}
		else
		{
			layerdata = NULL;
			STRACE(("ServerWindow %s received invalid view token %lx",fTitle.String(),view_token));
		}
		
		switch (code)
		{
			case AS_STROKE_LINEARRAY:
			{
				// TODO: Implement AS_STROKE_LINEARRAY
				break;
			}
			case AS_STROKE_POLYGON:
			{
				// TODO: Implement AS_STROKE_POLYGON
				break;
			}
			case AS_STROKE_SHAPE:
			{
				// TODO: Implement AS_STROKE_SHAPE
				break;
			}
			case AS_FILL_POLYGON:
			{
				// TODO: Implement AS_FILL_POLYGON
				break;
			}
			case AS_FILL_REGION:
			{
				// TODO: Implement AS_FILL_REGION
				break;
			}
			case AS_FILL_SHAPE:
			{
				// TODO: Implement AS_FILL_SHAPE
				break;
			}
			case AS_MOVEPENBY:
			{
				// TODO: Implement AS_MOVEPENBY
				break;
			}
			case AS_MOVEPENTO:
			{
				// TODO: Implement AS_MOVEPENTO
				break;
			}
			case AS_SETPENSIZE:
			{
				// TODO: Implement AS_SETPENSIZE
				break;
			}
			case AS_DRAW_STRING:
			{
				// TODO: Implement AS_DRAW_STRING
				break;
			}
			case AS_SET_FONT:
			{
				// TODO: Implement AS_SET_FONT
				break;
			}
			case AS_SET_FONT_SIZE:
			{
				// TODO: Implement AS_SET_FONT_SIZE
				break;
			}
			default:
			{
				sizeRemaining -= sizeof(int32);
				printf("ServerWindow %s received unexpected graphics code %lx",fTitle.String(),code);
				break;
			}
		}
	}
*/
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
	BPortLink	*ses = win->fSession;
	bool			quitting = false;
	int32			code;
	status_t		err = B_OK;
	
	while(!quitting)
	{
		printf("info: ServerWindow::MonitorWin listening on port %ld.\n", win->fMessagePort);
		code = AS_CLIENT_DEAD;
		err = ses->GetNextReply(&code);
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
				win->DispatchMessage(code);
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

