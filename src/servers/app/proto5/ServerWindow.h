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
	ServerWindow(BRect rect, const char *string, uint32 winflags,
		ServerApp *winapp,  port_id winport, uint32 index);
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
	void Loop(void);
	
	BString *title;
	int32 flags;
	int32 workspace;
	bool active;
	
	ServerApp *app;
	
	Decorator *decorator;
	WindowBorder *winborder;
	bigtime_t lasthit; // Time of last mouse click
	
	thread_id thread;
	port_id receiver;	// Messages from window
	port_id sender; // Messages to window
	PortLink *winlink,*applink;
	BLocker locker;
	BRect frame;
	uint8 hidecount;
};

#endif
