#ifndef _SRVAPP_H_
#define _SRVAPP_H_

#include <OS.h>
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
	void DispatchMessage(int32 code, int8 *buffer);
	bool IsActive(void) { return isactive; }
	void Activate(bool value) { isactive=value; }
	
	port_id sender,receiver;
	char *app_sig;
	thread_id monitor_thread;
	PortLink *applink;
	BList *winlist, *bmplist;
	ServerCursor *cursor;
	DisplayDriver *driver;
	bool isactive;
	rgb_color high, low;	// just for testing until layers are implemented
};

#endif