#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Locker.h>
#include <Application.h>
#include <List.h>
//#include <Window.h>	// for window_look
#include "Decorator.h"

class Layer;
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
	void Picasso(void);

private:
	friend Decorator *instantiate_decorator(Layer *lay, uint32 dflags, uint32 wlook);

	void DispatchMessage(int32 code, int8 *buffer);
	static int32 PollerThread(void *data);
	static int32 PicassoThread(void *data);
	bool LoadDecorator(BString path);
	void LoadDefaultDecorator(void);

	image_id decorator_id;
	create_decorator *make_decorator;	// global function pointer
	bool quitting_server;
	BList *applist;
	int32 active_app;
	ServerApp *p_active_app;
	thread_id poller_id, picasso_id;

	BLocker *active_lock, *applist_lock, *decor_lock;
	bool exit_poller;
	DisplayDriver *driver;
};

Decorator *instantiate_decorator(Layer *lay, uint32 dflags, uint32 wlook);
#endif