// CommonTestApp.h

#ifndef COMMON_TEST_APP_H
#define COMMON_TEST_APP_H

#include <stdarg.h>

#include <Application.h>

class CommonTestApp;

class EventHandler {
public:
	EventHandler() {}
	virtual ~EventHandler() {}

	virtual void HandleEvent(CommonTestApp *app) = 0;
};

class CommonTestApp : public BApplication {
public:
	CommonTestApp(const char *signature);
	CommonTestApp(const char *signature, status_t *result);
	virtual ~CommonTestApp();

	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();
	virtual void ReadyToRun();
	thread_id Run();

	void SetQuittingPolicy(bool onSecondTry);
	void SetReportDestruction(bool reportDestruction);

	status_t RunEventThread(bigtime_t delay, int32 count,
							EventHandler *handler);

	void SetMessageHandler(BHandler *handler);

private:
	static int32 _EventThreadEntry(void *data);
	int32 _EventLoop();

private:
	bool			fQuitOnSecondTry;
	thread_id		fEventThread;
	bigtime_t		fEventDelay;
	int32			fEventCount;
	EventHandler	*fEventHandler;
	BHandler		*fMessageHandler;
	bool			fReportDestruction;
};

status_t init_connection();
void report(const char *format,...);
void vreport(const char *format, va_list args);

#endif	// COMMON_TEST_APP_H
