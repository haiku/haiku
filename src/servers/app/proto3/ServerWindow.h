#ifndef _SERVERWIN_H_
#define _SERVERWIN_H_

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <Locker.h>

class BString;
class BMessenger;
class BRect;
class BPoint;
class ServerApp;
class ServerBitmap;
class WindowDecorator;

class ServerWindow
{
public:
ServerWindow(const char *Title, uint32 Flags, uint32 Desktop,
	const BRect &Rect, ServerApp *App, port_id SendPort);
ServerWindow(ServerApp *App, ServerBitmap *Bitmap);
virtual ~ServerWindow(void);

void PostMessage(BMessage *msg);
void ReplaceDecorator(void);
virtual void Quit(void);
const char *GetTitle(void);
ServerApp *GetApp(void);
BMessenger *GetAppTarget(void);
void Show(void);
void Hide(void);
void SetFocus(void);
bool HasFocus(void);

void DesktopActivated(int32 NewDesktop, const BPoint Resolution, color_space CSpace);
void WindowActivated(bool Active);
void ScreenModeChanged(const BPoint Resolustion, color_space CSpace);

void SetFrame(const BRect &Frame);
BRect Frame(void);

thread_id Run(void);
status_t Lock(void);
status_t Unlock(void);
bool IsLocked(void);

bool DispatchMessage(BMessage *msg);
bool DispatchMessage(const void *msg, int nCode);

void Loop(void);

const char *title;
int32 flags;
int32 desktop;

ServerBitmap *userbitmap;
ServerApp *app;

WindowDecorator *decorator;
bigtime_t lasthit; // Time of last mouse click

thread_id thread;
port_id rcvport;	// Messages from application
port_id sendport; // Messages to application
BLocker mutex;
BMessenger *apptarget;
};

#endif
