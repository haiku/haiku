#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Locker.h>
#include <List.h>
#include <Application.h>
#include <Window.h>
#include <String.h>
#include "Decorator.h"
#include "ServerConfig.h"

class Layer;
class BMessage;
class ServerApp;
class DisplayDriver;
class BPortLink;
class CursorManager;
class BitmapManager;

class AppServer
{
public:
	AppServer(void);
	~AppServer(void);

	static	int32 PollerThread(void *data);
	static	int32 PicassoThread(void *data);
	thread_id Run(void);
	void MainLoop(void);
	
	void DispatchMessage(int32 code, BPortLink &link);

private:
	port_id	fMessagePort;
	bool fQuittingServer;
	
	BList *fAppList;
	thread_id fPicassoThreadID;
	
	sem_id 	fActiveAppLock, fAppListLock;
};

extern AppServer *app_server;

#endif
