#ifndef _SERVERWIN_H_
#define _SERVERWIN_H_

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <Locker.h>
#include <Rect.h>

class BString;
class BMessenger;
class BPoint;
class ServerApp;
class WindowDecorator;
class PortLink;

class ServerWindow
{
public:
	ServerWindow(const char *Title, uint32 Flags, uint32 Desktop,
		const BRect &Rect, ServerApp *App, port_id SendPort);
	~ServerWindow(void);
	
	void PostMessage(BMessage *msg);
	void ReplaceDecorator(void);
	void Quit(void);
	const char *GetTitle(void);
	ServerApp *GetApp(void);
	void Show(void);
	void Hide(void);
	bool IsHidden(void);
	void SetFocus(bool value);
	bool HasFocus(void);
	
	void DesktopActivated(int32 NewDesktop, const BPoint Resolution, color_space CSpace);
	void WindowActivated(bool Active);
	void ScreenModeChanged(const BPoint Resolution, color_space CSpace);
	
	void SetFrame(const BRect &rect);
	BRect Frame(void);
	
	status_t Lock(void);
	void Unlock(void);
	bool IsLocked(void);
	
	void DispatchMessage(const void *msg, int nCode);
	static int32 MonitorWin(void *data);
	void Loop(void);
	
	const char *title;
	int32 flags;
	int32 desktop;
	bool active;
	
	ServerApp *app;
	
	WindowDecorator *decorator;
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
