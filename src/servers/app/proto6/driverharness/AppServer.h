#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <OS.h>
#include <Locker.h>
#include <Application.h>
#include <List.h>
#include <Window.h>	// for window_look

class BMessage;
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

	void TestScreenStates(void);
	void TestArcs(void);
	void TestBeziers(void);
	void TestEllipses(void);
	void TestFonts(void);
	void TestLines(void);
	void TestPolygon(void);
	void TestRects(void);
	void TestRegions(void);
	void TestShape(void);
	void TestTriangle(void);
	void TestCursors(void);

private:
	void RunTests(void);
	void DispatchMessage(int32 code, int8 *buffer);
	static int32 PollerThread(void *data);
	bool quitting_server;
	BList *applist;
	int32 active_app;
	thread_id poller_id;

	BLocker *active_lock, *applist_lock, *decor_lock;
	bool exit_poller;
	DisplayDriver *driver;
};

#endif