// AppRunTestApp1.cpp

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
	CommonTestApp app("application/x-vnd.obos-app-run-testapp1", &error);
	init_connection();
	report("error: %lx\n", error);
	report("InitCheck(): %lx\n", app.InitCheck());
	if (error == B_OK)
		app.Run();
	return 0;
}

