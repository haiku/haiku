#ifndef _SRVAPP_H_
#define _SRVAPP_H_

#include <OS.h>
class BMessage;
class PortLink;

class ServerApp
{
public:
	ServerApp(port_id msgport, char *signature);
	~ServerApp(void);

	bool Run(void);
	static int32 MonitorApp(void *data);	
	void Loop(void);
	void DispatchMessage(BMessage *msg);
	
	port_id sender,receiver;
	char *app_sig;
	thread_id monitor_thread;
	PortLink *applink;
};

#endif