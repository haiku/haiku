#ifndef _SRVAPP_H_
#define _SRVAPP_H_

#include <OS.h>
#include <String.h>
class BMessage;
class PortLink;
class BList;
class ServerCursor;
class DisplayDriver;

class ServerApp
{
public:
	ServerApp(port_id msgport, char *signature);
	~ServerApp(void);

	bool Run(void);
	static int32 MonitorApp(void *data);	
	void Loop(void);
	bool IsActive(void) const { return isactive; }
	void Activate(bool value) { isactive=value; }
	bool PingTarget(void);
	
//	char *app_sig;
	port_id sender,receiver;
	BString app_sig;
	thread_id monitor_thread;
	PortLink *applink;
	BList *winlist, *bmplist;
	ServerCursor *cursor;
	DisplayDriver *driver;
	rgb_color high, low;	// just for testing until layers are implemented

protected:
	void DispatchMessage(int32 code, int8 *buffer);

	bool isactive;
	team_id target_id;
};

#endif