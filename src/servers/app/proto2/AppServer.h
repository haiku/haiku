#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Application.h>
#include <List.h>

class BMessage;
class ServerApp;

class AppServer : public BApplication
{
public:
	AppServer(void);
	~AppServer(void);
	thread_id Run(void);
	void MainLoop(void);
	port_id	messageport,mouseport;

private:
	void DispatchMessage(int32 code, int8 *buffer);
	thread_id DrawID, InputID;
	bool quitting_server;
	BList *applist;
};

#endif