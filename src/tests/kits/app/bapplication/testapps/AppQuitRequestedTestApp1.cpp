// AppQuitRequestedTestApp1.cpp

#include <stdio.h>

#include <OS.h>

#include "CommonTestApp.h"

int
main()
{
// R5: doesn't set the error variable in case of success
#ifdef TEST_R5
	status_t error = B_OK;
#else
	status_t error = B_ERROR;
#endif
	CommonTestApp *app = new CommonTestApp(
		"application/x-vnd.obos-app-quit-testapp1", &error);
	init_connection();
	report("error: %lx\n", error);
	report("InitCheck(): %lx\n", app->InitCheck());
	app->SetReportDestruction(true);
	if (error == B_OK) {
		app->SetQuittingPolicy(true);
		app->PostMessage(B_QUIT_REQUESTED, app);
		app->PostMessage(B_QUIT_REQUESTED, app);
		app->Run();
	}
	delete app;
	return 0;
}

