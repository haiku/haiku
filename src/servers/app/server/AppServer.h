#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Locker.h>
#include <List.h>
#include <Window.h>
#include "Decorator.h"

class Layer;
class BMessage;
class ServerApp;
class DisplayDriver;

/*!
	\class AppServer AppServer.h
	\brief main manager object for the app_server
	
	File for the main app_server thread. This particular thread monitors for
	application start and quit messages. It also starts the housekeeping threads
	and initializes most of the server's globals.
*/
class AppServer
{
public:
	AppServer(void);
	~AppServer(void);
	static int32 PollerThread(void *data);
	static int32 PicassoThread(void *data);
	thread_id Run(void);
	void MainLoop(void);
	bool LoadDecorator(const char *path);
	void DispatchMessage(int32 code, int8 *buffer);
	void Broadcast(int32 code);
	void HandleKeyMessage(int32 code, int8 *buffer);

	create_decorator *make_decorator;	// global function pointer
private:
	friend Decorator *instantiate_decorator(Layer *lay, const char *title, uint32 dflags, uint32 wlook);

	port_id	messageport,mouseport;
	image_id decorator_id;
	bool quitting_server;
	BList *applist;
	int32 active_app;
//	ServerApp *p_active_app;
	thread_id poller_id, picasso_id;

	BLocker *active_lock, *applist_lock, *decor_lock;
	bool exit_poller;
	DisplayDriver *driver;
};

Decorator *instantiate_decorator(BRect rect, const char *title, int32 wlook, int32 wfeel,
	int32 wflags, DisplayDriver *ddriver);
#endif