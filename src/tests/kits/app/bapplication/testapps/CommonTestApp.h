// CommonTestApp.h

#ifndef COMMON_TEST_APP_H
#define COMMON_TEST_APP_H

#include <stdarg.h>

#include <Application.h>

class CommonTestApp : public BApplication {
public:
	CommonTestApp(const char *signature);
	CommonTestApp(const char *signature, status_t *result);
	virtual ~CommonTestApp();

	virtual void ArgvReceived(int32 argc, char **argv);
	virtual bool QuitRequested();
	virtual void ReadyToRun();
	thread_id Run();

	void SetQuittingPolicy(bool onSecondTry);

	status_t RunQuitterThread(bigtime_t delay, int32 tries);

private:
	static int32 _QuitterEntry(void *data);
	int32 _QuitterLoop();

private:
	bool		fQuitOnSecondTry;
	thread_id	fQuitter;
	bigtime_t	fQuittingDelay;
	int32		fQuittingTries;
};

status_t init_connection();
void report(const char *format,...);
void vreport(const char *format, va_list args);

#endif	// COMMON_TEST_APP_H
