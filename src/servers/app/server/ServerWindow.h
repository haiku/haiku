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

class BString;
class BMessenger;
class BPoint;
class ServerApp;
class Decorator;
class PortLink;
class WindowBorder;

class ServerWindow
{
public:
	ServerWindow(BRect rect, const char *string, uint32 wlook, uint32 wfeel,
		uint32 wflags, ServerApp *winapp,  port_id winport, uint32 index);
	~ServerWindow(void);
	
	void ReplaceDecorator(void);
	void Quit(void);
	const char *GetTitle(void);
	ServerApp *GetApp(void);
	void Show(void);
	void Hide(void);
	bool IsHidden(void);
	void SetFocus(bool value);
	bool HasFocus(void);
	void RequestDraw(BRect rect);
	
	void WorkspaceActivated(int32 NewDesktop, const BPoint Resolution, color_space CSpace);
	void WindowActivated(bool Active);
	void ScreenModeChanged(const BPoint Resolution, color_space CSpace);
	
	void SetFrame(const BRect &rect);
	BRect Frame(void);
	
	status_t Lock(void);
	void Unlock(void);
	bool IsLocked(void);
	
	void DispatchMessage(int32 code, int8 *msgbuffer);
	static int32 MonitorWin(void *data);
	static void HandleMouseEvent(int32 code, int8 *buffer);
	static void HandleKeyEvent(int32 code, int8 *buffer);
	void Loop(void);

protected:	
	friend ServerApp;
	BString *_title;
	int32 _look, _feel, _flags;
	int32 _workspace;
	bool _active;
	
	ServerApp *_app;
	
	Decorator *_decorator;
	WindowBorder *_winborder;
	
	thread_id _monitorthread;
	port_id _receiver;	// Messages from window
	port_id _sender; // Messages to window
	PortLink *_winlink,*_applink;
	BLocker _locker;
	BRect _frame;
	uint8 _hidecount;
	uint32 _token;
};

void ActivateWindow(ServerWindow *win);


#endif
