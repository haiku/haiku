#ifndef _OBOS_APPLICATION_H_
#define _OBOS_APPLICATION_H_

#include <SupportDefs.h>
#include <Application.h>

class PortLink;
class BList;
class OBWindow;

class OBApplication : private BApplication
{
public:

OBApplication(const char *signature);
OBApplication(const char *signature, status_t *error);
virtual ~OBApplication(void);

OBApplication(BMessage *data);
static BArchivable *Instantiate(BMessage *data);
virtual status_t Archive(BMessage *data, bool deep = true) const;

status_t InitCheck() const;

virtual thread_id Run(void);
virtual void Quit(void);
virtual bool QuitRequested(void);
virtual void Pulse(void);
virtual void ReadyToRun(void);
virtual void MessageReceived(BMessage *msg);
virtual void ArgvReceived(int32 argc, char **argv);
virtual void AppActivated(bool active);
virtual void RefsReceived(BMessage *a_message);
virtual void AboutRequested(void);

virtual BHandler *ResolveSpecifier(BMessage *msg,int32 index,
BMessage *specifier,int32 form,const char *property);

void ShowCursor(void);
void HideCursor(void);
void ObscureCursor(void);
bool IsCursorHidden(void) const;
void SetCursor(const void *cursor);
void SetCursor(const BCursor *cursor, bool sync=true);
int32 CountWindows(void) const;
BWindow *WindowAt(int32 index) const;
int32 CountLoopers(void) const;
BLooper *LooperAt(int32 index) const;
bool IsLaunching(void) const;
status_t GetAppInfo(app_info *info) const;
static BResources *AppResources(void);
virtual void DispatchMessage(BMessage *an_event,BHandler *handler);
void SetPulseRate(bigtime_t rate);
virtual status_t GetSupportedSuites(BMessage *data);
virtual status_t Perform(perform_code d, void *arg);

protected:
friend OBWindow;

void MainLoop(void);
void DispatchMessage(int32 code, int8 *buffer);

port_id messageport, serverport;
BString *signature;
PortLink *serverlink;
BList *windowlist;
status_t initcheckval;
};

extern OBApplication *obe_app;
#endif