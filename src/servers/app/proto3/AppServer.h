#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Application.h>
#include <List.h>

class BMessage;
class ServerApp;
class DisplayDriver;

class AppServer : public BApplication
{
public:
	AppServer(void);
	~AppServer(void);
	thread_id Run(void);
	void MainLoop(void);
	port_id	messageport,mouseport;
	void Poller(void);

private:
	void DispatchMessage(int32 code, int8 *buffer);
	static int32 PollerThread(void *data);

	bool quitting_server;
	BList *applist;
	thread_id poller_id;
	bool exit_poller;
	DisplayDriver *driver;
};

#endif