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
	ServerWindow(BRect rect, const char *string, uint32 wlook, uint32 wfeel, 
			uint32 wflags, ServerApp *winapp,  port_id winport,
			port_id looperPort, port_id replyport, uint32 index,int32 handlerID);
	~ServerWindow(void);
	
	void ReplaceDecorator(void);
	void Quit(void);

	const char *GetTitle(void);
	ServerApp *GetApp(void);
	int32 GetLook(void) const { return _look; }
	int32 GetFeel(void) const { return _feel; }
	int32 GetFlags(void) const { return _flags; }
	int32 GetHandlerID(void) const { return _handlertoken; }
	//! Returns the index of the workspaces to which it belongs
	uint32 GetWorkspaces(void) const { return _workspaces; }
	
	team_id GetClientTeamID(void) const { return fClientTeamID; }
	FMWList *GetFMWList(void) { return &fWinFMWList; }

	void SetLook(int32 look);
	void SetFeel(int32 feel);
	void SetFlags(int32 flags);
	void SetWorkspaces(int32 workspaces);

	void Show(void);
	void Hide(void);
	bool IsHidden(void);
	void Minimize(bool status);
	void Zoom(void);
	void SetFocus(bool value);
	bool HasFocus(void);
	void RequestDraw(BRect rect);
	void RequestDraw(void);
	
	void WorkspaceActivated(int32 workspace, bool active);
	void WorkspacesChanged(int32 oldone,int32 newone);
	void WindowActivated(bool active);
	void ScreenModeChanged(const BRect frame, const color_space cspace);
	
	void SetFrame(const BRect &rect);
	BRect Frame(void);
	
	status_t Lock(void);
	void Unlock(void);
	bool IsLocked(void);
	
	void DispatchMessage(PortMessage msg);
	void DispatchGraphicsMessage(int32 msgsize, int8 *msgbuffer);

	static	int32 MonitorWin(void *data);
	static	void HandleMouseEvent(PortMessage *msg);
	static	void HandleKeyEvent(int32 code, int8 *buffer);
	
//	Workspace* GetWorkspace(void);
	void SetWorkspace(Workspace *wkspc);
	
	//! Returns the window's title
	const char *Title(void) { return _title->String(); }
	
	Layer* FindLayer(const Layer* start, int32 token) const;
	void SendMessageToClient( const BMessage* msg ) const;

protected:	
	friend class ServerApp;
	friend class WinBorder;
	friend class Screen;
	friend class Layer;
	
	BString *_title;
	int32 _look,
		  _feel,
		  _flags;
	uint32 _workspaces;
	Workspace *_workspace;
	bool _active;
	
	ServerApp *_app;
	WinBorder *_winborder;
	
	thread_id _monitorthread;

	port_id _receiver;	// Messages from window
	port_id _sender; // Messages to window

	PortLink *_winlink,
			 *_applink;
	BLocker _locker;
	BRect _frame;
	uint32 _token;
	int32 _handlertoken;
	
	port_id winLooperPort;
	Layer *top_layer;
	Layer *currentlayer;
	FMWList fWinFMWList;
	team_id fClientTeamID;
};

void ActivateWindow(ServerWindow *oldwin,ServerWindow *newwin);


#endif

