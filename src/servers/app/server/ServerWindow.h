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
//	File Name:		ServerWindow.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BWindow class
//  
//------------------------------------------------------------------------------
#ifndef _SERVERWIN_H_
#define _SERVERWIN_H_

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <Locker.h>
#include <Rect.h>
#include <String.h>
#include <Window.h>
#include <PortMessage.h>
#include "FMWList.h"

class BString;
class BMessenger;
class BPoint;
class BMessage;
class ServerApp;
class Decorator;
class PortLink;
class WinBorder;
class Workspace;
class BSession;
class Layer;

#define AS_UPDATE_DECORATOR 'asud'
#define AS_UPDATE_COLORS 'asuc'
#define AS_UPDATE_FONTS 'asuf'

/*!
	\class ServerWindow ServerWindow.h
	\brief Shadow BWindow class
	
	A ServerWindow handles all the intraserver tasks required of it by its BWindow. There are 
	too many tasks to list as being done by them, but they include handling View transactions, 
	coordinating and linking a window's WinBorder half with its messaging half, dispatching 
	mouse and key events from the server to its window, and other such things.
*/
class ServerWindow
{
public:
	ServerWindow(BRect rect, const char *string, uint32 wlook, uint32 wfeel, uint32 wflags,
			ServerApp *winapp, port_id winport,	port_id looperPort,	port_id replyport,
			uint32 index,int32 handlerID);
	virtual ~ServerWindow(void);
	
	void Init();

	// ServerWindow must be locked for these ones.	
	void ReplaceDecorator(void);
	void Quit(void);
	void Show(void);
	void Hide(void);

	// methods for sending various messages to client.
	bool IsHidden(void) const;
	void Minimize(bool status);
	void Zoom(void);
	void WorkspaceActivated(int32 workspace, bool active);
	void WorkspacesChanged(int32 oldone,int32 newone);
	void WindowActivated(bool active);
	void ScreenModeChanged(const BRect frame, const color_space cspace);
	
	status_t Lock(void);
	void Unlock(void);
	bool IsLocked(void) const;
	
	//! Returns the index of the workspaces to which it belongs
	int32 GetWorkspaceIndex(void) { return fWorkspaces; }

	// util methods.	
	Layer *FindLayer(const Layer *start, int32 token) const;
	void SendMessageToClient( const BMessage *msg ) const;

	// a few, not that important methods returning some internal settings.	
	int32 Look(void) const { return fLook; }
	int32 Feel(void) const { return fFeel; }
	uint32 Flags(void) const { return fFlags; }
	uint32 Workspaces(void) const { return fWorkspaces; }

	// to who we belong. who do we own. our title.
	ServerApp *App(void) const { return fServerApp; }
	WinBorder *GetWinBorder(void) const { return fWinBorder; }
	const char *Title(void) const { return fTitle.String(); }

	// related thread/team_id(s).
	team_id ClientTeamID(void) const { return fClientTeamID; }
	thread_id ThreadID(void) const { return fMonitorThreadID;}

	// server "private" - try not to use.
	void QuietlySetWorkspaces(uint32 wks) { fWorkspaces = wks; }
	void QuietlySetFeel(int32 feel) { fFeel = feel; }
	int32 ClientToken(void) const { return fHandlerToken; }
	
	FMWList fWinFMWList;

private:
	// methods for retrieving and creating a tree strcture of Layers.
	Layer *CreateLayerTree(Layer *localRoot);
	void SetLayerState(Layer *layer);
	void SetLayerFontState(Layer *layer);

	// message handle methods.
	void DispatchMessage(int32 code);
	void DispatchGraphicsMessage(int32 msgsize, int8 *msgbuffer);
	static int32 MonitorWin(void *data);


protected:	
	friend class ServerApp;
	friend class WinBorder;
	friend class Screen; 
	friend class Layer;
	
	BString fTitle;
	int32 fLook;
	int32 fFeel;
	int32 fFlags;
	uint32 fWorkspaces;
	
	ServerApp *fServerApp;
	WinBorder *fWinBorder;
	
	team_id fClientTeamID;
	thread_id fMonitorThreadID;
	
	port_id fMessagePort;
	port_id fClientWinPort;
	port_id fClientLooperPort;
	
	BLocker fLocker;
	BRect fFrame;
	uint32 fToken;
	int32 fHandlerToken;
	
	BSession *fSession;

	// cl is short for currentLayer. We'll use it a lot, that's why it's short :-)
	Layer *cl;
};

#endif
