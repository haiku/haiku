#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Locker.h>
#include <List.h>
#include <Application.h>
#include <Window.h>
#include <PortQueue.h>
#include <String.h>
#include "Decorator.h"
#include "ServerConfig.h"

class Layer;
class BMessage;
class ServerApp;
class DisplayDriver;
class CursorManager;
class BitmapManager;
class PortMessage;

/*!
	\class AppServer AppServer.h
	\brief main manager object for the app_server
	
	File for the main app_server thread. This particular thread monitors for
	application start and quit messages. It also starts the housekeeping threads
	and initializes most of the server's globals.
*/
#if DISPLAYDRIVER == HWDRIVER
class AppServer
#else
class AppServer : public BApplication
#endif
{
public:
	AppServer(void);
	~AppServer(void);

	static	int32 PollerThread(void *data);
	static	int32 PicassoThread(void *data);
	thread_id			Run(void);
	void				MainLoop(void);
	
	bool				LoadDecorator(const char *path);
	void				InitDecorators(void);
	
	void				DispatchMessage(PortMessage *msg);
	void				Broadcast(int32 code);

	// TODO: remove shortly!
	void				HandleKeyMessage(int32 code, int8 *buffer);
	
	ServerApp*			FindApp(const char *sig);

private:
	friend	Decorator*	new_decorator(BRect rect, const char *title,
				int32 wlook, int32 wfeel, int32 wflags, DisplayDriver *ddriver);

	// global function pointer
	create_decorator	*make_decorator;
	
	port_id	_messageport,
			_mouseport;
	
	image_id _decorator_id;
	
	BString decorator_name;
	
	bool _quitting_server,
		_exit_poller;
	
	BList *_applist;
	thread_id _poller_id,
			  _picasso_id;
	
	sem_id 	_active_lock,
			_applist_lock,
			_decor_lock;
	
	DisplayDriver *_driver;
	
	int32 _ssindex;
};

Decorator *new_decorator(BRect rect, const char *title, int32 wlook, int32 wfeel,
	int32 wflags, DisplayDriver *ddriver);

extern CursorManager *cursormanager;
extern BitmapManager *bitmapmanager;
extern ColorSet gui_colorset;

#endif
