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

//! TokenHandler object used to provide IDs for all windows in the system
TokenHandler win_token_handler;

//! Active winborder - used for tracking windows during moves, resizes, and tab slides
WinBorder *active_winborder=NULL;

template<class Type> Type
read_from_buffer(int8 **_buffer)
{
	Type *typedBuffer = (Type *)(*_buffer);
	Type value = *typedBuffer;

	typedBuffer++;
	*_buffer = (int8 *)(typedBuffer);

	return value;
}

static int8 *read_pattern_from_buffer(int8 **_buffer)
{
	int8 *pattern = *_buffer;

	*_buffer += AS_PATTERN_SIZE;

	return pattern;
}

template<class Type> void
write_to_buffer(int8 **_buffer, Type value)
{
	Type *typedBuffer = (Type *)(*_buffer);

	*typedBuffer = value;
	typedBuffer++;

	*_buffer = (int8 *)(typedBuffer);
}

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
	fFrame			= rect;
	fFlags			= wflags;
	fLook			= wlook;
	fFeel			= wfeel;
	fHandlerToken	= handlerID;
	fClientLooperPort	= looperPort;
	fWorkspaces		= index;
	fClientTeamID	= winapp->ClientTeamID();
	fWorkspace		= NULL;
	fWinBorder		= NULL;
	fTopLayer		= NULL;
	// fClientWinPort is the port to which the app awaits messages from the server
	fClientWinPort	= winport;
	// fMessagePort is the port to which the app sends messages for the server
	fMessagePort	= create_port(30,fTitle.String());

	fSession		= new BSession(fMessagePort, fClientWinPort);
	
	// Send a reply to our window - it is expecting fMessagePort port.
	// Temporarily use winlink to save time and memory
	PortLink		winLink(replyport);
	winLink.SetOpCode(AS_CREATE_WINDOW);
	winLink.Attach<port_id>(fMessagePort);
	winLink.Flush();

	int32			vCode;

	// check the next 2 messages to make sure we receive top_view's attributes.
	fSession->ReadInt32(&vCode);
	if(vCode != AS_LAYER_CREATE_ROOT)
		debugger("SERVER ERROR: ServerWindow(xxx): NO top_view data received! - 1\n");

	fSession->ReadInt32(&vCode);
	if(vCode != AS_LAYER_CREATE)
		debugger("SERVER ERROR: ServerWindow(xxx): NO top_view data received! - 2\n");

	// start receiving top_view data. pass NULL as the parent view.
	// This should be the *only* place where this happens.
	fTopLayer		= CreateLayerTree(NULL);
	fTopLayer->SetAsTopLayer(true);
	cl = fTopLayer;

	// Create a WindoBorder object for our ServerWindow.
	fWinBorder		= new WinBorder( fFrame, fTitle.String(),
									 wlook, wfeel, wflags,
									 this, desktop->GetDisplayDriver());

	STRACE(("ServerWindow %s:\n",fTitle.String()));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom));
	STRACE(("\tPort: %ld\n",fMessagePort));
	STRACE(("\tWorkspace: %ld\n",index));
}

void ServerWindow::Init(){
	fWinBorder->AddChild(fTopLayer);

	// NOTE: this MUST be before the monitor thread is spawned!
	desktop->AddWinBorder(fWinBorder);

	// Spawn our message-monitoring fMonitorThreadID
	fMonitorThreadID	= spawn_thread(MonitorWin, fTitle.String(), B_NORMAL_PRIORITY, this);
	if(fMonitorThreadID != B_NO_MORE_THREADS && fMonitorThreadID != B_NO_MEMORY)
		resume_thread(fMonitorThreadID);
}

//!Tears down all connections with the user application, kills the monitoring thread.
ServerWindow::~ServerWindow(void)
{
STRACE(("*ServerWindow (%s):~ServerWindow()\n",fTitle.String()));

	desktop->fGeneralLock.Lock();

	desktop->RemoveWinBorder(fWinBorder);
	STRACE(("SW(%s) Successfuly removed from the desktop\n", fTitle.String()));
	if(fSession){
		delete fSession;
		fSession = NULL;
	}
	if (fWinBorder){
		delete fWinBorder;
		fWinBorder = NULL;
	}
	cl		= NULL;
	if (fTopLayer)
		delete fTopLayer;

	desktop->fGeneralLock.Unlock();
	STRACE(("#ServerWindow(%s) will exit NOW!!!\n", fTitle.String()));
}

/*!
	\brief Requests an update of the specified rectangle
	\param rect The area to update, in the parent's coordinates
	
	This could be considered equivalent to BView::Invalidate()
*/
void ServerWindow::RequestDraw(BRect rect)
{
STRACE(("ServerWindow %s: Request Draw\n",fTitle.String()));
	BMessage		msg;
	
	msg.what		= _UPDATE_;
	msg.AddRect("_rect", rect);
	
	SendMessageToClient(&msg);
}

//! Requests an update for the entire window
void ServerWindow::RequestDraw(void)
{
	RequestDraw(fFrame);
}

//! Forces the window border to update its decorator
void ServerWindow::ReplaceDecorator(void)
{
STRACE(("ServerWindow %s: Replace Decorator\n",fTitle.String()));
	fWinBorder->UpdateDecorator();
}

//! Requests that the ServerWindow's BWindow quit
void ServerWindow::Quit(void)
{
STRACE(("ServerWindow %s: Quit\n",fTitle.String()));
	BMessage		msg;
	
	msg.what		= B_QUIT_REQUESTED;
	
	SendMessageToClient(&msg);
}

//! Shows the window's WinBorder
void ServerWindow::Show(void)
{
// whaaat?
	if(!fWinBorder->IsHidden())
		return;

	STRACE(("ServerWindow %s: Show\n",fTitle.String()));
	if(fWinBorder)
	{
		RootLayer	*rl = fWinBorder->GetRootLayer();
		int32		wksCount;

		desktop->fGeneralLock.Lock();
		STRACE(("ServerWindow(%s)::Show() - General lock acquired\n", fWinBorder->GetName()));

		rl->fMainLock.Lock();
		STRACE(("ServerWindow(%s)::Show() - Main lock acquired\n", fWinBorder->GetName()));

		// manualy set fWinBorder->_hidden to false because Layer's version also calls FullInvalidate.
		fWinBorder->_hidden = false;
		
		if ((fFeel == B_FLOATING_SUBSET_WINDOW_FEEL || fFeel == B_MODAL_SUBSET_WINDOW_FEEL)
			 && fWinBorder->MainWinBorder() == NULL)
		{
			// This window hasn't been added to a normal window subset,
			//	so don't call placement or redrawing methods!
			goto goOut;
		}
		
		wksCount	= rl->WorkspaceCount();
		for(int32 i = 0; i < wksCount; i++){
			if (fWorkspaces & (0x00000001UL << i)){
				Workspace		*ws = rl->WorkspaceAt(i+1);
				ws->BringToFrontANormalWindow(fWinBorder);
				ws->SearchAndSetNewFront(fWinBorder);
				ws->SetFocusLayer(fWinBorder);
			}
		}

		goOut:

		rl->fMainLock.Unlock();
		STRACE(("ServerWindow(%s)::Show() - Main lock released\n", fWinBorder->GetName()));
		desktop->fGeneralLock.Unlock();
		STRACE(("ServerWindow(%s)::Show() - General lock released\n", fWinBorder->GetName()));
	}
}

//! Hides the window's WinBorder
void ServerWindow::Hide(void)
{
	if(fWinBorder->IsHidden())
		return;

	STRACE(("ServerWindow %s: Hide\n",fTitle.String()));
	if(fWinBorder){
		RootLayer	*rl		= fWinBorder->GetRootLayer();
		Workspace	*ws		= NULL;

		desktop->fGeneralLock.Lock();
		STRACE(("ServerWindow(%s)::Hide() - General lock acquired\n", fWinBorder->GetName()));

		rl->fMainLock.Lock();
		STRACE(("ServerWindow(%s)::Hide() - Main lock acquired\n", fWinBorder->GetName()));

		fWinBorder->Hide();

		int32		wksCount= rl->WorkspaceCount();
		for(int32 i = 0; i < wksCount; i++){
			ws		= rl->WorkspaceAt(i+1);
			if (ws->FrontLayer() == fWinBorder){
				ws->HideSubsetWindows(fWinBorder);
				ws->SetFocusLayer(ws->FrontLayer());
			}
			else{
				if (ws->FocusLayer() == fWinBorder){
					ws->SetFocusLayer(fWinBorder);
				}
				else{
					ws->Invalidate();
				}
			}
		}
		rl->fMainLock.Unlock();
		STRACE(("ServerWindow(%s)::Hide() - Main lock released\n", fWinBorder->GetName()));

		desktop->fGeneralLock.Unlock();
		STRACE(("ServerWindow(%s)::Hide() - General lock released\n", fWinBorder->GetName()));
	}
}

/*
	\brief Determines whether the window is hidden or not
	\return true if hidden, false if not
*/
bool ServerWindow::IsHidden(void)
{
	if(fWinBorder)
		return fWinBorder->IsHidden();
	return true;
}

void ServerWindow::Minimize(bool status){
	bool		sendMessages = false;

	if (status){
		if (!IsHidden()){
			Hide();
			sendMessages	= true;
		}
	}
	else{
		if (IsHidden()){
			Show();
			sendMessages	= true;
		}
	}
	
	if (sendMessages){
		BMessage		msg;
		msg.what		= B_MINIMIZE;
		msg.AddInt64("when", real_time_clock_usecs());
		msg.AddBool("minimize", status);

		SendMessageToClient(&msg);
		
// TODO: notify tracker! how???
	}
}

void ServerWindow::Zoom(){
// TODO: implement;
}

/*!
	\brief Handles focus and redrawing when changing focus states
	
	The ServerWindow is set to (in)active and its decorator is redrawn based on its active status
*/
void ServerWindow::SetFocus(bool value)
{
STRACE(("ServerWindow %s: Set Focus to %s - UNIMPLEMENTED\n",fTitle.String(),value?"true":"false"));
/*
	if(fIsActive!=value)
	{
		fIsActive=value;
		fWinBorder->SetFocus(value);
//		fWinBorder->RequestDraw();
	}
*/
}

/*!
	\brief Determines whether or not the window is active
	\return true if active, false if not
*/
bool ServerWindow::HasFocus(void)
{
	return fIsActive;
}

/*!
	\brief Notifies window of workspace (de)activation
	\param workspace Index of the workspace changed
	\param active New active status of the workspace
*/
void ServerWindow::WorkspaceActivated(int32 workspace, bool active)
{
STRACE(("ServerWindow %s: WorkspaceActivated(%ld,%s)\n",fTitle.String(),workspace,(active)?"active":"inactive"));
	BMessage		msg;
	
	msg.what		= B_WORKSPACE_ACTIVATED;
	msg.AddInt32("workspace", workspace);
	msg.AddBool("active", active);
	
	SendMessageToClient(&msg);
}

/*!
	\brief Notifies window of a workspace switch
	\param oldone index of the previous workspace
	\param newone index of the new workspace
*/
void ServerWindow::WorkspacesChanged(int32 oldone,int32 newone)
{
STRACE(("ServerWindow %s: WorkspacesChanged(%ld,%ld)\n",fTitle.String(),oldone,newone));
	BMessage		msg;
	
	msg.what		= B_WORKSPACES_CHANGED;
	msg.AddInt32("old", oldone);
	msg.AddInt32("new", newone);
	
	SendMessageToClient(&msg);
}

/*!
	\brief Notifies window of a change in focus
	\param active New active status of the window
*/
void ServerWindow::WindowActivated(bool active)
{
STRACE(("ServerWindow %s: WindowActivated(%s)\n",fTitle.String(),(active)?"active":"inactive"));
	BMessage		msg;
	
	msg.what		= B_WINDOW_ACTIVATED;
	msg.AddBool("active", active);
	
	SendMessageToClient(&msg);
}

/*!
	\brief Notifies window of a change in screen resolution
	\param frame Size of the new resolution
	\param color_space Color space of the new screen mode
*/
void ServerWindow::ScreenModeChanged(const BRect frame, const color_space cspace)
{
STRACE(("ServerWindow %s: ScreenModeChanged\n",fTitle.String()));
	BMessage		msg;
	
	msg.what		= B_SCREEN_CHANGED;
	msg.AddRect("frame", frame);
	msg.AddInt32("mode", (int32)cspace);
	
	SendMessageToClient(&msg);
}

/*
	\brief Sets the frame size of the window
	\rect New window size
*/
void ServerWindow::SetFrame(const BRect &rect)
{
STRACE(("ServerWindow %s: Set Frame to (%.1f,%.1f,%.1f,%.1f)\n",fTitle.String(),
			rect.left,rect.top,rect.right,rect.bottom));
	fFrame=rect;
}

/*!
	\brief Returns the frame of the window in screen coordinates
	\return The frame of the window in screen coordinates
*/
BRect ServerWindow::Frame(void)
{
	return fFrame;
}

/*!
	\brief Locks the window
	\return B_OK if everything is ok, B_ERROR if something went wrong
*/
status_t ServerWindow::Lock(void)
{
STRACE(("ServerWindow %s: Lock\n",fTitle.String()));
	return (fLocker.Lock())?B_OK:B_ERROR;
}

//! Unlocks the window
void ServerWindow::Unlock(void)
{
STRACE(("ServerWindow %s: Unlock\n",fTitle.String()));
	fLocker.Unlock();
}

/*!
	\brief Determines whether or not the window is locked
	\return True if locked, false if not.
*/
bool ServerWindow::IsLocked(void)
{
	return fLocker.IsLocked();
}

void ServerWindow::SetLayerFontState(Layer *layer){
	uint16			mask;

	fSession->ReadUInt16(&mask);
			
	if (mask & B_FONT_FAMILY_AND_STYLE){
		uint32		fontID;
		fSession->ReadInt32((int32*)&fontID);
		// TODO: implement later. Currently there is no SetFamAndStyle(uint32)
		//   in ServerFont class. DW, could you add one?
		//layer->_layerdata->font->
	}

	if (mask & B_FONT_SIZE){
		float		size;
		fSession->ReadFloat(&size);
		layer->_layerdata->font.SetSize(size);
	}

	if (mask & B_FONT_SHEAR){
		float		shear;
		fSession->ReadFloat(&shear);
		layer->_layerdata->font.SetShear(shear);
	}

	if (mask & B_FONT_ROTATION){
		float		rotation;
		fSession->ReadFloat(&rotation);
		layer->_layerdata->font.SetRotation(rotation);
	}

	if (mask & B_FONT_SPACING){
		uint8		spacing;
		fSession->ReadUInt8(&spacing);	// uint8
		layer->_layerdata->font.SetSpacing(spacing);
	}

	if (mask & B_FONT_ENCODING){
		uint8		encoding;
		fSession->ReadUInt8((uint8*)&encoding); // uint8
		layer->_layerdata->font.SetEncoding(encoding);
	}

	if (mask & B_FONT_FACE){
		uint16		face;
		fSession->ReadUInt16(&face);	// uint16
		layer->_layerdata->font.SetFace(face);
	}

	if (mask & B_FONT_FLAGS){
		uint32		flags;
		fSession->ReadUInt32(&flags); // uint32
		layer->_layerdata->font.SetFlags(flags);
	}
STRACE(("DONE: ServerWindow %s: Message AS_LAYER_SET_FONT_STATE: Layer: %s\n",fTitle.String(), layer->_name->String()));
}

void ServerWindow::SetLayerState(Layer *layer){
	rgb_color		highColor,
					lowColor,
					viewColor;
	pattern			patt;
	int32			clippRegRects;

	fSession->ReadPoint(		&(layer->_layerdata->penlocation));
	fSession->ReadFloat(		&(layer->_layerdata->pensize));
	fSession->ReadData(			&highColor, sizeof(rgb_color));
	fSession->ReadData(			&lowColor, sizeof(rgb_color));
	fSession->ReadData(			&viewColor, sizeof(rgb_color));
	fSession->ReadData(			&patt, sizeof(pattern));
	fSession->ReadInt8((int8*)	&(layer->_layerdata->draw_mode));
	fSession->ReadPoint(		&(layer->_layerdata->coordOrigin));
	fSession->ReadInt8((int8*)	&(layer->_layerdata->lineJoin));
	fSession->ReadInt8((int8*)	&(layer->_layerdata->lineCap));
	fSession->ReadFloat(		&(layer->_layerdata->miterLimit));
	fSession->ReadInt8((int8*)	&(layer->_layerdata->alphaSrcMode));
	fSession->ReadInt8((int8*)	&(layer->_layerdata->alphaFncMode));
	fSession->ReadFloat(		&(layer->_layerdata->scale));
	fSession->ReadBool(			&(layer->_layerdata->fontAliasing));
	fSession->ReadInt32(		&clippRegRects);

	layer->_layerdata->patt.Set(*((uint64*)&patt));
	layer->_layerdata->highcolor.SetColor(highColor);
	layer->_layerdata->lowcolor.SetColor(lowColor);
	layer->_layerdata->viewcolor.SetColor(viewColor);

	if(clippRegRects != 0){
		if(layer->_layerdata->clippReg == NULL)
			layer->_layerdata->clippReg = new BRegion();
		else
			layer->_layerdata->clippReg->MakeEmpty();

		BRect		rect;
				
		for(int32 i = 0; i < clippRegRects; i++){
			fSession->ReadRect(&rect);
			layer->_layerdata->clippReg->Include(rect);
		}
	}
	else{
		if (layer->_layerdata->clippReg){
			delete layer->_layerdata->clippReg;
			layer->_layerdata->clippReg = NULL;
		}
	}
STRACE(("DONE: ServerWindow %s: Message AS_LAYER_SET_STATE: Layer: %s\n",fTitle.String(), layer->_name->String()));
}

Layer* ServerWindow::CreateLayerTree(Layer *localRoot){
STRACE(("ServerWindow(%s)::CreateLayerTree()\n", fTitle.String()));

	int32		token;
	BRect		frame;
	uint32		resizeMask;
	uint32		flags;
	bool		hidden;
	int32		childCount;
	char*		name;
		
	fSession->ReadInt32(&token);
	name		= fSession->ReadString();
	fSession->ReadRect(&frame);
	fSession->ReadUInt32(&resizeMask);
	fSession->ReadUInt32(&flags);
	fSession->ReadBool(&hidden);
	fSession->ReadInt32(&childCount);
			
	Layer		*newLayer;
	newLayer	= new Layer(frame, name, token, resizeMask, flags, desktop->GetDisplayDriver());
	delete name;

	// there is no way of setting this, other than manual. :-)
	newLayer->_hidden	= hidden;

	int32		dummyMsg;
			
	//next comes BView's font state
	fSession->ReadInt32(&dummyMsg);
	if (dummyMsg == AS_LAYER_SET_FONT_STATE)
		SetLayerFontState(newLayer);
	else
		debugger("ServerWindow(%s) - AS_LAYER_SET_FONT_STATE Expected!\n");

	//lastly, the BView's graphic state
	fSession->ReadInt32(&dummyMsg);
	if (dummyMsg == AS_LAYER_SET_STATE)
		SetLayerState(newLayer);
	else
		debugger("ServerWindow(%s) - AS_LAYER_SET_STATE Expected!\n");

	// add the new Layer to the tree structure.
	if (localRoot)
		localRoot->AddChild(newLayer);
			
	// attach newLayer's children...
	for(int i = 0; i < childCount; i++)
	{
		fSession->ReadInt32(&dummyMsg);
		if (dummyMsg == AS_LAYER_CREATE)
			CreateLayerTree(newLayer);
		else
			debugger("ServerWindow(%s) - AS_LAYER_CREATE Expected!\n");
	}
STRACE(("DONE: ServerWindow %s: Message AS_CREATE_LAYER: Parent: %s, Child: %s\n", fTitle.String(), newLayer->_name->String(), name));
	return newLayer;
}

void ServerWindow::DispatchMessage(int32 code)
{
	switch(code)
	{
		//--------- BView Messages -----------------
		case AS_SET_CURRENT_LAYER:
		{
			int32 token;
			
			fSession->ReadInt32(&token);
			
			Layer *current = FindLayer(fTopLayer, token);
			if (current)
				cl=current;
			else // hope this NEVER happens! :-)
				debugger("Server PANIC: window cannot find Layer with ID\n");

			STRACE(("ServerWindow %s: Message AS_SET_CURRENT_LAYER: Layer name: %s\n", fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_CREATE:
		{
			CreateLayerTree(cl);
			break;
		}
		case AS_LAYER_DELETE:
		{
			// Received when a view is detached from a window. This is definitely
			// the less taxing operation - we call PruneTree() on the removed
			// layer, detach the layer itself, delete it, and invalidate the
			// area assuming that the view was visible when removed
			STRACE(("SW %s: AS_LAYER_DELETE(self)...\n", fTitle.String()));			
			Layer		*parent;
			parent		= cl->_parent;
			
			// here we remove current layer from list.
			cl->RemoveSelf();

			// TODO: invalidate the region occupied by this view.
			// Should be done in Layer::RemoveChild() though!
			cl->PruneTree();

			parent->PrintTree();
			STRACE(("DONE: ServerWindow %s: Message AS_DELETE_LAYER: Parent: %s Layer: %s\n", fTitle.String(), parent->_name->String(), cl->_name->String()));

			delete cl;

			cl=parent;
			break;
		}
		case AS_LAYER_SET_STATE:
		{
			SetLayerState(cl);
			cl->RebuildFullRegion();
			break;
		}
		case AS_LAYER_SET_FONT_STATE:
		{
			SetLayerFontState(cl);
			cl->RebuildFullRegion();
			break;
		}
		case AS_LAYER_GET_STATE:
		{
			LayerData	*ld;
			
			// these 4 are here because of a compiler warning. Maybe he's right... :-)
			rgb_color	hc, lc, vc; // high, low and view colors
			uint64		patt;		// current pattern as a uint64

			ld			= cl->_layerdata; // now we write fewer characters. :-)
			hc			= ld->highcolor.GetColor32();
			lc			= ld->lowcolor.GetColor32();
			vc			= ld->viewcolor.GetColor32();
			patt		= ld->patt.GetInt64();
			
			// TODO: DW implement such a method in ServerFont class!
			fSession->WriteUInt32(0UL /*uint32 ld->font.GetFamAndStyle()*/);
			fSession->WriteFloat(ld->font.Size());
			fSession->WriteFloat(ld->font.Shear());
			fSession->WriteFloat(ld->font.Rotation());
			fSession->WriteUInt8(ld->font.Spacing());
			fSession->WriteUInt8(ld->font.Encoding());
			fSession->WriteUInt16(ld->font.Face());
			fSession->WriteUInt32(ld->font.Flags());
			
			fSession->WritePoint(ld->penlocation);
			fSession->WriteFloat(ld->pensize);
			fSession->WriteData(&hc, sizeof(rgb_color));
			fSession->WriteData(&lc, sizeof(rgb_color));
			fSession->WriteData(&vc, sizeof(rgb_color));
			fSession->WriteData(&patt,sizeof(pattern));
			fSession->WritePoint(ld->coordOrigin);
			fSession->WriteUInt8((uint8)(ld->draw_mode));
			fSession->WriteUInt8((uint8)(ld->lineCap));
			fSession->WriteUInt8((uint8)(ld->lineJoin));
			fSession->WriteFloat(ld->miterLimit);
			fSession->WriteUInt8((uint8)(ld->alphaSrcMode));
			fSession->WriteUInt8((uint8)(ld->alphaFncMode));
			fSession->WriteFloat(ld->scale);
			fSession->WriteFloat(ld->fontAliasing);
			
			int32		noOfRects = 0;
			if (ld->clippReg)
				noOfRects = ld->clippReg->CountRects();

			fSession->WriteInt32(noOfRects);
			
			for(int i = 0; i < noOfRects; i++){
				fSession->WriteRect(ld->clippReg->RectAt(i));
			}
			
			fSession->WriteFloat(cl->_frame.left);
			fSession->WriteFloat(cl->_frame.top);
			fSession->WriteRect(cl->_frame.OffsetToCopy(cl->_boundsLeftTop));
			fSession->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_STATE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_MOVETO:
		{
			float		x, y;
			
			fSession->ReadFloat(&x);
			fSession->ReadFloat(&y);
			
			cl->MoveBy(x, y);

			STRACE(("ServerWindow %s: Message AS_LAYER_MOVETO: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_RESIZETO:
		{
			float		newWidth, newHeight;
			
			fSession->ReadFloat(&newWidth);
			fSession->ReadFloat(&newHeight);

			// TODO: check for minimum alowed. WinBorder should provide such
			// a method, based on its decorator.
			
			cl->ResizeBy(newWidth, newHeight);

			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZETO: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_COORD:
		{
			fSession->WriteFloat(cl->_frame.left);
			fSession->WriteFloat(cl->_frame.top);
			fSession->WriteRect(cl->_frame.OffsetToCopy(cl->_boundsLeftTop));
			fSession->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COORD: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_ORIGIN:
		{
			float		x, y;
			
			fSession->ReadFloat(&x);
			fSession->ReadFloat(&y);
			
			cl->_layerdata->coordOrigin.Set(x, y);
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_ORIGIN: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_ORIGIN:
		{
			fSession->WritePoint(cl->_layerdata->coordOrigin);
			fSession->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_ORIGIN: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_RESIZE_MODE:
		{
			fSession->ReadUInt32(&(cl->_resize_mode));

			STRACE(("ServerWindow %s: Message AS_LAYER_RESIZE_MODE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_CURSOR:
		{
			int32		token;

			fSession->ReadInt32(&token);
			
			cursormanager->SetCursor(token);

			STRACE(("ServerWindow %s: Message AS_LAYER_CURSOR: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_FLAGS:
		{
			fSession->ReadUInt32(&(cl->_flags));
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_FLAGS: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_HIDE:
		{
			cl->Hide();
			
			STRACE(("ServerWindow %s: Message AS_LAYER_HIDE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SHOW:
		{
			cl->Show();
			
			STRACE(("ServerWindow %s: Message AS_LAYER_SHOW: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_LINE_MODE:
		{
			int8		lineCap, lineJoin;

			// TODO: speak with DW! Shouldn't we lock before modifying certain memebers?
			// Because redraw code might use an updated value instead of one for witch
			// it was called. e.g.: different lineCap or lineJoin. Strange result
			// would appear.

			fSession->ReadInt8(&lineCap);
			fSession->ReadInt8(&lineJoin);
			fSession->ReadFloat(&(cl->_layerdata->miterLimit));
			
			cl->_layerdata->lineCap		= (cap_mode)lineCap;
			cl->_layerdata->lineJoin	= (join_mode)lineJoin;
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_LINE_MODE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_LINE_MODE:
		{
			fSession->WriteInt8((int8)(cl->_layerdata->lineCap));
			fSession->WriteInt8((int8)(cl->_layerdata->lineJoin));
			fSession->WriteFloat(cl->_layerdata->miterLimit);
			fSession->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_LINE_MODE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_PUSH_STATE:
		{
			LayerData		*ld = new LayerData();
			ld->prevState	= cl->_layerdata;
			cl->_layerdata	= ld;

			cl->RebuildFullRegion();
			
			STRACE(("ServerWindow %s: Message AS_LAYER_PUSH_STATE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_POP_STATE:
		{
			if (!(cl->_layerdata->prevState)) {
				STRACE(("WARNING: SW(%s): User called BView(%s)::PopState(), but there is NO state on stack!\n", fTitle.String(), cl->_name->String()));
				break;
			}

			LayerData		*ld = cl->_layerdata;
			cl->_layerdata	= cl->_layerdata->prevState;
			delete ld;

			cl->RebuildFullRegion();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_POP_STATE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_SCALE:
		{
			fSession->ReadFloat(&(cl->_layerdata->scale));
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: Layer: %s\n",fTitle.String(), cl->_name->String()));		
			break;
		}
		case AS_LAYER_GET_SCALE:
		{
			LayerData		*ld = cl->_layerdata;
			float			scale = ld->scale;

			while((ld = ld->prevState))
				scale		*= ld->scale;
			
			fSession->WriteFloat(scale);
			fSession->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_SCALE: Layer: %s\n",fTitle.String(), cl->_name->String()));		
			break;
		}
		case AS_LAYER_SET_PEN_LOC:
		{
			float		x, y;
			
			fSession->ReadFloat(&x);
			fSession->ReadFloat(&y);
			
			cl->_layerdata->penlocation.Set(x, y);
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_LOC: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_PEN_LOC:
		{
			fSession->WritePoint(cl->_layerdata->penlocation);
			fSession->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_LOC: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_PEN_SIZE:
		{
			fSession->ReadFloat(&(cl->_layerdata->pensize));
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_PEN_SIZE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_PEN_SIZE:
		{
			fSession->WriteFloat(cl->_layerdata->pensize);
			fSession->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_PEN_SIZE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_HIGH_COLOR:
		{
			rgb_color		c;
			
			fSession->ReadData(&c, sizeof(rgb_color));
			
			cl->_layerdata->highcolor.SetColor(c);
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_HIGH_COLOR: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_LOW_COLOR:
		{
			rgb_color		c;
			
			fSession->ReadData(&c, sizeof(rgb_color));
			
			cl->_layerdata->lowcolor.SetColor(c);
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_LOW_COLOR: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_VIEW_COLOR:
		{
			rgb_color		c;
			
			fSession->ReadData(&c, sizeof(rgb_color));
			
			cl->_layerdata->viewcolor.SetColor(c);
		
			STRACE(("ServerWindow %s: Message AS_LAYER_SET_VIEW_COLOR: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_COLORS:
		{
			rgb_color		highColor, lowColor, viewColor;
			
			highColor		= cl->_layerdata->highcolor.GetColor32();
			lowColor		= cl->_layerdata->lowcolor.GetColor32();
			viewColor		= cl->_layerdata->viewcolor.GetColor32();
			
			fSession->WriteData(&highColor, sizeof(rgb_color));
			fSession->WriteData(&lowColor, sizeof(rgb_color));
			fSession->WriteData(&viewColor, sizeof(rgb_color));
			fSession->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_COLORS: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_BLEND_MODE:
		{
			int8		srcAlpha, alphaFunc;
			
			fSession->ReadInt8(&srcAlpha);
			fSession->ReadInt8(&alphaFunc);
			
			cl->_layerdata->alphaSrcMode	= (source_alpha)srcAlpha;
			cl->_layerdata->alphaFncMode	= (alpha_function)alphaFunc;

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_BLEND_MODE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_BLEND_MODE:
		{
			fSession->WriteInt8((int8)(cl->_layerdata->alphaSrcMode));
			fSession->WriteInt8((int8)(cl->_layerdata->alphaFncMode));
			fSession->Sync();

			STRACE(("ServerWindow %s: Message AS_LAYER_GET_BLEND_MODE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_DRAW_MODE:
		{
			int8		drawingMode;
			
			fSession->ReadInt8(&drawingMode);
			
			cl->_layerdata->draw_mode	= (drawing_mode)drawingMode;

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_DRAW_MODE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_DRAW_MODE:
		{
			fSession->WriteInt8((int8)(cl->_layerdata->draw_mode));
			fSession->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_DRAW_MODE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_PRINT_ALIASING:
		{
			fSession->ReadBool(&(cl->_layerdata->fontAliasing));
		
			STRACE(("ServerWindow %s: Message AS_LAYER_PRINT_ALIASING: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_CLIP_TO_PICTURE:
		{
			// TODO: watch out for the coordinate system
			int32		pictureToken;
			BPoint		where;
			
			fSession->ReadInt32(&pictureToken);
			fSession->ReadPoint(&where);

		
			BRegion			reg;
			bool			redraw = false;

				// if we had a picture to clip to, include the FULL visible region(if any) in the area to be redrawn
				// in other words: invalidate what ever is visible for this layer and his children.
			if (cl->clipToPicture && cl->_fullVisible.CountRects() > 0){
				reg.Include(&cl->_fullVisible);
				redraw		= true;
			}

				// search for a picture with the specified token.
			ServerPicture	*sp = NULL;
			int32			i = 0;
			while(1)
			{
				sp=static_cast<ServerPicture*>(cl->_serverwin->fServerApp->fPictureList->ItemAt(i++));
				if (!sp)
					break;
					
				if(sp->GetToken() == pictureToken){
//					cl->clipToPicture	= sp;
					// TODO: increase that picture's reference count.(~ allocate a picture)
					break;
				}
			}
				// avoid compiler warning
			i = 0;

				// we have a new picture to clip to, so rebuild our full region
			if(cl->clipToPicture) {

				cl->clipToPictureInverse	= false;

				cl->RebuildFullRegion();
			}

				// we need to rebuild the visible region, we may have a valid one.
			if (cl->_parent && !(cl->_hidden)){
//				cl->_parent->RebuildChildRegions(cl->_full.Frame(), cl);
			}
			else{
					// will this happen? Maybe...
//				cl->RebuildRegions(cl->_full.Frame());
			}

				// include our full visible region in the region to be redrawn
			if (!(cl->_hidden) && (cl->_fullVisible.CountRects() > 0)){
				reg.Include(&(cl->_fullVisible));
				redraw		= true;
			}

				// redraw if: we had OR if we NOW have a picture to clip to.
			if (redraw){
				cl->Invalidate(reg);
			}

			STRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_PICTURE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_CLIP_TO_INVERSE_PICTURE:
		{
// TODO: watch out for the coordinate system
			int32		pictureToken;
			BPoint		where;
			
			fSession->ReadInt32(&pictureToken);
			fSession->ReadPoint(&where);

			ServerPicture	*sp = NULL;
			int32			i = 0;

			while(1)
			{
				sp= static_cast<ServerPicture*>(cl->_serverwin->fServerApp->fPictureList->ItemAt(i++));
				if (!sp)
					break;
					
				if(sp->GetToken() == pictureToken){
//					cl->clipToPicture	= sp;
// TODO: increase that picture's reference count.(~ allocate a picture)
					break;
				}
			}
				// avoid compiler warning
			i = 0;
			
				// if a picture has been found...
			if(cl->clipToPicture) {

				cl->clipToPictureInverse	= true;

				cl->RebuildFullRegion();

//				cl->RequestDraw(cl->clipToPicture->Frame());
			}

			STRACE(("ServerWindow %s: Message AS_LAYER_CLIP_TO_INVERSE_PICTURE: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_GET_CLIP_REGION:
		{
// TODO: watch out for the coordinate system
			BRegion		reg;
			LayerData	*ld;
			int32		noOfRects;

			ld			= cl->_layerdata;
			reg			= cl->ConvertFromParent(&(cl->_visible));

			if(ld->clippReg)
				reg.IntersectWith(ld->clippReg);

			while((ld = ld->prevState))
				if(ld->clippReg)
					reg.IntersectWith(ld->clippReg);

			noOfRects	= reg.CountRects();
			fSession->WriteInt32(noOfRects);

			for(int i = 0; i < noOfRects; i++){
				fSession->WriteRect(reg.RectAt(i));
			}

			fSession->Sync();
		
			STRACE(("ServerWindow %s: Message AS_LAYER_GET_CLIP_REGION: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_SET_CLIP_REGION:
		{
// TODO: watch out for the coordinate system
			int32		noOfRects;
			BRect		r;

			if(cl->_layerdata->clippReg)
				cl->_layerdata->clippReg->MakeEmpty();
			else
				cl->_layerdata->clippReg = new BRegion();
			
			fSession->ReadInt32(&noOfRects);
			
			for(int i = 0; i < noOfRects; i++){
				fSession->ReadRect(&r);
				cl->_layerdata->clippReg->Include(r);
			}

			cl->RebuildFullRegion();

//			cl->RequestDraw(cl->clipToPicture->Frame());

			STRACE(("ServerWindow %s: Message AS_LAYER_SET_CLIP_REGION: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		
		case AS_LAYER_INVAL_RECT:
		{
// TODO: watch out for the coordinate system
			BRect		invalRect;
			
			fSession->ReadRect(&invalRect);
			
			cl->Invalidate(invalRect);

//			cl->RequestDraw(invalRect);

			STRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}
		case AS_LAYER_INVAL_REGION:
		{
// TODO: watch out for the coordinate system
			BRegion			invalReg;
			int32			noOfRects;
			BRect			rect;
			
			fSession->ReadInt32(&noOfRects);
			
			for(int i = 0; i < noOfRects; i++){
				fSession->ReadRect(&rect);
				invalReg.Include(rect);
			}
			
			cl->Invalidate(invalReg);

//			cl->RequestDraw(invalReg.Frame());

			STRACE(("ServerWindow %s: Message AS_LAYER_INVAL_RECT: Layer: %s\n",fTitle.String(), cl->_name->String()));
			break;
		}

		
	/********** END: BView Messages ***********/
	
	/********** BWindow Messages ***********/
		case AS_LAYER_DELETE_ROOT:
		{
			// Received when a window deletes its internal top view
			
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Delete_Layer_Root unimplemented\n",fTitle.String()));

			break;
		}
		case AS_SHOW_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_SHOW\n",fTitle.String()));
			Show();
			break;
		}
		case AS_HIDE_WINDOW:
		{
			STRACE(("ServerWindow %s: Message AS_HIDE\n",fTitle.String()));		
			Hide();
			break;
		}
		case AS_SEND_BEHIND:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message  Send_Behind unimplemented\n",fTitle.String()));
			break;
		}
		case AS_ENABLE_UPDATES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Enable_Updates unimplemented\n",fTitle.String()));
			break;
		}
		case AS_DISABLE_UPDATES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Disable_Updates unimplemented\n",fTitle.String()));
			break;
		}
		case AS_NEEDS_UPDATE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Needs_Update unimplemented\n",fTitle.String()));
			break;
		}
		case AS_WINDOW_TITLE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Title unimplemented\n",fTitle.String()));
			break;
		}
		case AS_ADD_TO_SUBSET:
		{
			WinBorder	*wb;
			int32		mainToken;
			team_id		teamID;
			
			fSession->ReadInt32(&mainToken);
			fSession->ReadData(&teamID, sizeof(team_id));
			
			wb			= desktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if(wb){
				fSession->WriteInt32(SERVER_TRUE);
				fSession->Sync();
				
				fWinBorder->AddToSubsetOf(wb);
			}
			else{
				fSession->WriteInt32(SERVER_FALSE);
				fSession->Sync();
			}
			break;
		}
		case AS_REM_FROM_SUBSET:
		{
			WinBorder	*wb;
			int32		mainToken;
			team_id		teamID;
			
			fSession->ReadInt32(&mainToken);
			fSession->ReadData(&teamID, sizeof(team_id));
			
			wb			= desktop->FindWinBorderByServerWindowTokenAndTeamID(mainToken, teamID);
			if(wb){
				fSession->WriteInt32(SERVER_TRUE);
				fSession->Sync();
				
				fWinBorder->RemoveFromSubsetOf(wb);
			}
			else{
				fSession->WriteInt32(SERVER_FALSE);
				fSession->Sync();
			}
			break;
		}
		case AS_SET_LOOK:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Look unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_FLAGS:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Flags unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_FEEL:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Feel unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_ALIGNMENT:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Alignment unimplemented\n",fTitle.String()));
			break;
		}
		case AS_GET_ALIGNMENT:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Get_Alignment unimplemented\n",fTitle.String()));
			break;
		}
		case AS_GET_WORKSPACES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Get_Workspaces unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_WORKSPACES:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Workspaces unimplemented\n",fTitle.String()));
			break;
		}
		case AS_WINDOW_RESIZE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Resize unimplemented\n",fTitle.String()));
			break;
		}
		case B_MINIMIZE:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Minimize unimplemented\n",fTitle.String()));
			break;
		}
		case B_WINDOW_ACTIVATED:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Window_Activated unimplemented\n",fTitle.String()));
			break;
		}
		case B_ZOOM:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Zoom unimplemented\n",fTitle.String()));
			break;
		}
		case B_WINDOW_MOVE_TO:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Move_To unimplemented\n",fTitle.String()));
			break;
		}
		case B_WINDOW_MOVE_BY:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Move_By unimplemented\n",fTitle.String()));
			break;
		}
		default:
		{
			printf("ServerWindow %s received unexpected code - message offset %lx\n",fTitle.String(), code - SERVER_TRUE);
			break;
		}
	}
}

/*!
	\brief Iterator for graphics update messages
	\param msgsize Size of the buffer containing the graphics messages
	\param msgbuffer Buffer containing the graphics message
*/
void ServerWindow::DispatchGraphicsMessage(int32 msgsize, int8 *msgbuffer)
{
	Layer *layer;
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
		
// TODO: fix!
/*
	WindowClipRegion.Set(fWinBorder->Frame());
	sibling = fWinBorder->UpperSibling();
	while (sibling)
	{
		WindowClipRegion.Exclude(sibling->Frame());
		sibling = sibling->UpperSibling();
	}
*/
	if (!WindowClipRegion.Frame().IsValid())
		return;
	
	// We need to decide whether coordinates are specified in view or root coordinates.
	// For now, we assume root level coordinates.
	code = AS_BEGIN_UPDATE;
	while ((sizeRemaining > 2*sizeof(int32)) && (code != AS_END_UPDATE))
	{
		code = read_from_buffer<int32>(&msgbuffer);
		view_token = read_from_buffer<int32>(&msgbuffer);
//TODO: fix!
		layer = NULL;//fWorkspace->GetRoot()->FindLayer(view_token);
		if (layer)
		{
// TODO: fix!
//			layerdata = layer->GetLayerData();
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
			case AS_SET_HIGH_COLOR:
			{
				if (sizeRemaining >= AS_SET_COLOR_MSG_SIZE)
				{
					uint8 r, g, b, a;
					r = read_from_buffer<uint8>(&msgbuffer);
					g = read_from_buffer<uint8>(&msgbuffer);
					b = read_from_buffer<uint8>(&msgbuffer);
					a = read_from_buffer<uint8>(&msgbuffer);
					if (layerdata)
						layerdata->highcolor.SetColor(r,g,b,a);
					sizeRemaining -= AS_SET_COLOR_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_SET_LOW_COLOR:
			{
				if (sizeRemaining >= AS_SET_COLOR_MSG_SIZE)
				{
					uint8 r, g, b, a;
					r = read_from_buffer<uint8>(&msgbuffer);
					g = read_from_buffer<uint8>(&msgbuffer);
					b = read_from_buffer<uint8>(&msgbuffer);
					a = read_from_buffer<uint8>(&msgbuffer);
					if (layerdata)
						layerdata->lowcolor.SetColor(r,g,b,a);
					sizeRemaining -= AS_SET_COLOR_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_SET_VIEW_COLOR:
			{
				if (sizeRemaining >= AS_SET_COLOR_MSG_SIZE)
				{
					uint8 r, g, b, a;
					r = read_from_buffer<uint8>(&msgbuffer);
					g = read_from_buffer<uint8>(&msgbuffer);
					b = read_from_buffer<uint8>(&msgbuffer);
					a = read_from_buffer<uint8>(&msgbuffer);
					if (layerdata)
						layerdata->viewcolor.SetColor(r,g,b,a);
					sizeRemaining -= AS_SET_COLOR_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_ARC:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_STROKE_ARC_MSG_SIZE)
				{
					float left, top, right, bottom, angle, span;
					int8 *pattern;

					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					angle = read_from_buffer<float>(&msgbuffer);
					span = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->StrokeArc(rect,angle,span,layerdata,pattern);
					sizeRemaining -= AS_STROKE_ARC_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_BEZIER:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_STROKE_BEZIER_MSG_SIZE)
				{
					BPoint *pts;
					int8 *pattern;
					int i;
					pts = new BPoint[4];
					for (i=0; i<4; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					pattern = read_pattern_from_buffer(&msgbuffer);
					//if (layerdata)
						//fServerApp->_driver->StrokeBezier(pts,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_STROKE_BEZIER_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_ELLIPSE:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_STROKE_ELLIPSE_MSG_SIZE)
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->StrokeEllipse(rect,layerdata,pattern);
					sizeRemaining -= AS_STROKE_ELLIPSE_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_LINE:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_STROKE_LINE_MSG_SIZE)
				{
					float x1, y1, x2, y2;
					int8 *pattern;
					x1 = read_from_buffer<float>(&msgbuffer);
					y1 = read_from_buffer<float>(&msgbuffer);
					x2 = read_from_buffer<float>(&msgbuffer);
					y2 = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BPoint p1(x1,y1);
					//BPoint p2(x2,y2);
					//if (layerdata)
						//fServerApp->_driver->StrokeLine(p1,p2,layerdata,pattern);
					sizeRemaining -= AS_STROKE_LINE_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_LINEARRAY:
			{
				// TODO: Implement
				break;
			}
			case AS_STROKE_POLYGON:
			{
				// TODO: Implement
				break;
			}
			case AS_STROKE_RECT:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_STROKE_RECT_MSG_SIZE)
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->StrokeRect(rect,layerdata,pattern);
					sizeRemaining -= AS_STROKE_RECT_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_ROUNDRECT:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_STROKE_ROUNDRECT_MSG_SIZE)
				{
					float left, top, right, bottom, xrad, yrad;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					xrad = read_from_buffer<float>(&msgbuffer);
					yrad = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->StrokeRoundRect(rect,xrad,yrad,layerdata,pattern);
					sizeRemaining -= AS_STROKE_ROUNDRECT_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_STROKE_SHAPE:
			{
				// TODO: Implement
				break;
			}
			case AS_STROKE_TRIANGLE:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_STROKE_TRIANGLE_MSG_SIZE)
				{
					BPoint *pts;
					int8 *pattern;
					float left, top, right, bottom;
					int i;
					pts = new BPoint[3];
					for (i=0; i<3; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->StrokeTriangle(pts,rect,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_STROKE_TRIANGLE_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_ARC:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_FILL_ARC_MSG_SIZE)
				{
					float left, top, right, bottom, angle, span;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					angle = read_from_buffer<float>(&msgbuffer);
					span = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->FillArc(rect,angle,span,layerdata,pattern);
					sizeRemaining -= AS_FILL_ARC_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_BEZIER:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_FILL_BEZIER_MSG_SIZE)
				{
					BPoint *pts;
					int8 *pattern;
					int i;
					pts = new BPoint[4];
					for (i=0; i<4; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					pattern = read_pattern_from_buffer(&msgbuffer);
					//if (layerdata)
						//fServerApp->_driver->FillBezier(pts,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_FILL_BEZIER_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_ELLIPSE:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_FILL_ELLIPSE_MSG_SIZE)
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->FillEllipse(rect,layerdata,pattern);
					sizeRemaining -= AS_FILL_ELLIPSE_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_POLYGON:
			{
				// TODO: Implement
				break;
			}
			case AS_FILL_RECT:
			{
				if (sizeRemaining >= AS_FILL_RECT_MSG_SIZE)
				{
					float left, top, right, bottom;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					/*
					BRect rect(left,top,right,bottom);
					if (layerdata && numRects)
						if (numRects == 1)
							fServerApp->_driver->FillRect(rect,layerdata,pattern);
						else
						{
							int i;
							for (i=0; i<numRects; i++)
								fServerApp->_driver->FillRect(LayerClipRegion.RectAt(i),layerdata,pattern);
						}
						*/
					sizeRemaining -= AS_FILL_RECT_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_REGION:
			{
				// TODO: Implement
				break;
			}
			case AS_FILL_ROUNDRECT:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_FILL_ROUNDRECT_MSG_SIZE)
				{
					float left, top, right, bottom, xrad, yrad;
					int8 *pattern;
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					xrad = read_from_buffer<float>(&msgbuffer);
					yrad = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->FillRoundRect(rect,xrad,yrad,layerdata,pattern);
					sizeRemaining -= AS_FILL_ROUNDRECT_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_FILL_SHAPE:
			{
				// TODO: Implement
				break;
			}
			case AS_FILL_TRIANGLE:
			{
				// TODO:: Add clipping
				if (sizeRemaining >= AS_FILL_TRIANGLE_MSG_SIZE)
				{
					BPoint *pts;
					int8 *pattern;
					float left, top, right, bottom;
					int i;
					pts = new BPoint[3];
					for (i=0; i<3; i++)
					{
						pts[i].x = read_from_buffer<float>(&msgbuffer);
						pts[i].y = read_from_buffer<float>(&msgbuffer);
					}
					left = read_from_buffer<float>(&msgbuffer);
					top = read_from_buffer<float>(&msgbuffer);
					right = read_from_buffer<float>(&msgbuffer);
					bottom = read_from_buffer<float>(&msgbuffer);
					pattern = read_pattern_from_buffer(&msgbuffer);
					//BRect rect(left,top,right,bottom);
					//if (layerdata)
						//fServerApp->_driver->FillTriangle(pts,rect,layerdata,pattern);
					delete[] pts;
					sizeRemaining -= AS_FILL_TRIANGLE_MSG_SIZE;
				}
				else
				{
					STRACE(("ServerWindow %s received truncated graphics code %lx",fTitle.String(),code));
					sizeRemaining = 0;
				}
				break;
			}
			case AS_MOVEPENBY:
			{
				// TODO: Implement
				break;
			}
			case AS_MOVEPENTO:
			{
				// TODO: Implement
				break;
			}
			case AS_SETPENSIZE:
			{
				// TODO: Implement
				break;
			}
			case AS_DRAW_STRING:
			{
				// TODO: Implement
				break;
			}
			case AS_SET_FONT:
			{
				// TODO: Implement
				break;
			}
			case AS_SET_FONT_SIZE:
			{
				// TODO: Implement
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
}

/*!
	\brief Message-dispatching loop for the ServerWindow

	MonitorWin() watches the ServerWindow's message port and dispatches as necessary
	\param data The thread's ServerWindow
	\return Throwaway code. Always 0.
*/
int32 ServerWindow::MonitorWin(void *data)
{
	ServerWindow 	*win = (ServerWindow *)data;
	BSession		*ses = win->fSession;
	bool			quitting = false;
	int32			code;
	
	while(!quitting)
	{
		ses->ReadInt32(&code);

		switch(code)
		{
			case 0:
			{
				// this means the client has been killed

				// TODO: A message code should *never* be 0. The source sending this message
				// needs to be changed to something like AS_CLIENT_DEAD or something.
				
				STRACE(("ServerWindow %s received '0' message code\n",win->Title()));
				quitting = true;
				delete win;
				break;
			}
			case B_QUIT_REQUESTED:
			{
				STRACE(("ServerWindow %s received Quit request\n",win->Title()));
				quitting = true;
printf("Deleting window...\n");
				delete win;
				break;
			}
			default:
			{
				STRACE(("ServerWindow %s: got a message to dispatch\n",win->Title()));
				win->DispatchMessage(code);
				break;
			}
		}
	}
	return 0;
}

/*!
	\brief PasfSession mouse event messages to the appropriate window
	\param code Message code of the mouse message
	\param buffer Attachment buffer for the mouse message
*/
//void ServerWindow::HandleMouseEvent(int32 code, int8 *buffer)
void ServerWindow::HandleMouseEvent(PortMessage *msg)
{
/*	ServerWindow *mousewin=NULL;
//	int8 *index=buffer;
	
	// Find the window which will receive our mouse event.
	
//TODO: resolve
	Layer *root=NULL;//GetRootLayer(CurrentWorkspace(),ActiveScreen());
	WinBorder *fWinBorder;
	
	// activeborder is used to remember windows when resizing/moving windows
	// or sliding a tab
	if(!root)
	{
		printf("ERROR: HandleMouseEvent has NULL root layer!!!\n");
		return;
	}

	// Dispatch the mouse event to the proper window
//	switch(code)
	switch(msg->Code())
	{
		case B_MOUSE_DOWN:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down
			// 5) int32 - buttons down
			// 6) int32 - clicks

//			index+=sizeof(int64);
//			float x=*((float*)index); index+=sizeof(float);
//			float y=*((float*)index); index+=sizeof(float);
//			BPoint pt(x,y);
			float x;
			float y;
			int64 dummy;
			msg->Read<int64>(&dummy);
			msg->Read<float>(&x);
			msg->Read<float>(&y);
//TODO: resolve
//			BPoint pt(x,y);
			
			// If we have clicked on a window, 			
			active_winborder = fWinBorder = NULL;
			if(fWinBorder)
			{
				mousewin=fWinBorder->Window();
				fWinBorder->MouseDown((int8*)msg->Buffer());
			}
			break;
		}
		case B_MOUSE_UP:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down

//			index+=sizeof(int64);
//			float x=*((float*)index); index+=sizeof(float);
//			float y=*((float*)index); index+=sizeof(float);
//			BPoint pt(x,y);
			float x;
			float y;
			int64 dummy;
			msg->Read(&dummy);
			msg->Read(&x);
			msg->Read(&y);
//TODO: resolve
//			BPoint pt(x,y);
			
//			set_is_sliding_tab(false);
//			set_is_moving_window(false);
//			set_is_resizing_window(false);

//TODO: resolve
			fWinBorder	= NULL;//WindowContainsPoint(pt);
			active_winborder=NULL;
			if(fWinBorder)
			{
				mousewin=fWinBorder->Window();
				
				// Eventually, we will build in MouseUp messages with buttons specified
				// For now, we just "assume" no mouse specification with a 0.
				fWinBorder->MouseUp((int8*)msg->Buffer());
			}
			break;
		}
		case B_MOUSE_MOVED:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - buttons down
//			index+=sizeof(int64);
//			float x=*((float*)index); index+=sizeof(float);
//			float y=*((float*)index); index+=sizeof(float);
//			BPoint pt(x,y);
			float x;
			float y;
			int64 dummy;
			msg->Read(&dummy);
			msg->Read(&x);
			msg->Read(&y);
//			BPoint pt(x,y);
//
//			if(is_moving_window() || is_resizing_window() || is_sliding_tab())
//			{
//				active_winborder->MouseMoved((int8*)msg->Buffer());
//			}
//			else
//			{
//				fWinBorder = WindowContainsPoint(pt);
//				if(fWinBorder)
//				{
//					mousewin=fWinBorder->Window();
//					fWinBorder->MouseMoved((int8*)msg->Buffer());
//				}
//			}				
			break;
		}
		default:
		{
			break;
		}
	}
*/
}

/*!
	\brief PasfSession key event messages to the appropriate window
	\param code Message code of the key message
	\param buffer Attachment buffer for the key message
*/
void ServerWindow::HandleKeyEvent(int32 code, int8 *buffer)
{
/*
STRACE_KEY(("ServerWindow::HandleKeyEvent unimplemented\n"));
	ServerWindow *keywin=NULL;
	
	// Dispatch the key event to the active window
	keywin=GetActiveWindow();
	if(keywin)
	{
		keywin->Lock();
		keywin->fWinLink->SetOpCode(code);
		keywin->fWinLink
		keywin->Unlock();
	}
*/
}

/*!
	\brief Send a message to the ServerWindow
	\param code ID code of the message to post
	\param size size of the data buffer
	\param buffer Any attached data
*/
void ServerWindow::PostMessage(int32 code, size_t size, int8 *buffer)
{
	write_port(fMessagePort,code, buffer, size);
}

/*!
	\brief Returns the Workspace object to which the window belongs
	
	If the window belongs to all workspaces, it returns the current workspace
*/
Workspace *ServerWindow::GetWorkspace(void)
{
	//TODO: resolve
	if(fWorkspaces==B_ALL_WORKSPACES)
		return NULL;//fWorkspaces->GetScreen()->GetActiveWorkspace();

	return NULL;
}

/*!
	\brief Assign the window to a particular workspace object
	\param The ServerWindow's new workspace
*/
void ServerWindow::SetWorkspace(Workspace *wkspc)
{
STRACE(("ServerWindow %s: Set Workspace\n",fTitle.String()));
	fWorkspace=wkspc;
}

Layer* ServerWindow::FindLayer(const Layer* start, int32 token) const
{
	if(!start)
		return NULL;
	
	// see if we're looking for 'start'
	if(start->_view_token == token)
		return const_cast<Layer*>(start);

	Layer *c = start->_topchild; //c = short for: current
	if(c != NULL)
		while(true)
		{
			// action block
			{
				if(c->_view_token == token)
					return c;
			}

				// go deep
			if(	c->_topchild)
			{
				c = c->_topchild;
			}
			// go right or up
			else
				// go right
				if(c->_lowersibling)
				{
					c = c->_lowersibling;
				}
				// go up
				else
				{
					while(!c->_parent->_lowersibling && c->_parent != start)
					{
						c = c->_parent;
					}
					// that enough! We've reached the start layer.
					if(c->_parent == start)
						break;
						
					c = c->_parent->_lowersibling;
				}
		}

	return NULL;
}

void ServerWindow::SendMessageToClient(const BMessage* msg) const
{
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

/*!
	\brief Handles window activation stuff. Called by Desktop functions
*/
void ActivateWindow(ServerWindow *oldwin,ServerWindow *newwin)
{
STRACE(("ActivateWindow: old=%s, new=%s\n",oldwin?oldwin->Title():"NULL",newwin?newwin->Title():"NULL"));
	if(oldwin==newwin)
		return;

	if(oldwin)
		oldwin->SetFocus(false);

	if(newwin)
		newwin->SetFocus(true);
}

/*
 @log
 	*added handlers for AS_LAYER_(MOVE/RESIZE)TO messages
 	*added AS_LAYER_GET_COORD handler
 	*changed some methods to use BMessage class for sending messages to BWindow
*/
