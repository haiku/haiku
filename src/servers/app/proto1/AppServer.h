#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Application.h>
#include "Globals.h"

class BMessage;
class Desktop;

class AppServer : public BApplication
{
public:
	AppServer(void);
	~AppServer(void);
	void Initialize(void);
	void MainLoop(void);
	port_id	messageport,mouseport;
	static int32 Picasso(void *data);
	static int32 Poller(void *data);
//	void Run(void);

private:
	void DispatchMessage(BMessage *pcReq);
	thread_id DrawID, InputID;
};

#endif