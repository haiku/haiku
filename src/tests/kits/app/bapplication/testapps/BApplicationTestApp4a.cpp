// BApplicationTestApp4a.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	BApplication app("application/x-vnd.obos-bapplication-testapp4", NULL);
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

