#ifndef _SERVERWIN_H_
#define _SERVERWIN_H_

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <Locker.h>
#include <Rect.h>
#include <String.h>

class BString;
class BPoint;
class ServerApp;
class Decorator;
class PortLink;
class WindowBorder;

class ServerWindow
{
public:
	ServerWindow(BRect rect, const char *title, uint32 wlook, uint32 wfeel,
		uint32 wflags, int32 workspace, ServerApp *winapp,  port_id winport);
	~ServerWindow(void);
	
	void Show(void);
	void Hide(void);
	bool IsHidden(void) { return (_hidecount==0)?true:false; }
	void RequestDraw(BRect rect);
	void DispatchMessage(int32 code, int8 *msgbuffer);
	static int32 MonitorWin(void *data);
	void DoUpdate(uint32 size, int8 *buffer);

	int32 Flags(void) { return _flags; }
	int32 Look(void) { return _look; }
	int32 Feel(void) { return _feel; }
	int32 Workspace(void) { return _workspace; }
	const char *Title(void) { return _title->String(); }
	ServerApp *App(void) { return _app; }
	BRect Frame(void);

	void Quit(void);
	void SetFocus(bool value);
	bool HasFocus(void);
	
	void WorkspaceActivated(int32 NewDesktop, const BPoint Resolution, color_space CSpace);
	void WindowActivated(bool Active);
	void ScreenModeChanged(const BPoint Resolution, color_space CSpace);
	
	static void HandleMouseEvent(int32 code, int8 *buffer);

protected:	
	friend ServerApp;
	
	BString *_title;
	int32 _look, _feel, _flags;
	int32 _workspace;
	bool _active;

	ServerApp *_app;
	Decorator *_decorator;
	WindowBorder *_winborder;
	bigtime_t _lasthit;
	
	thread_id _monitorthread;
	port_id _receiver;
	port_id _sender;
	PortLink *_winlink,*_applink;
	BRect _frame;
	uint8 _hidecount;
};

void ActivateWindow(ServerWindow *win);


#endif
