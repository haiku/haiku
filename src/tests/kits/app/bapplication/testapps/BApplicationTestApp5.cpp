// BApplicationTestApp5.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	BApplication app("application/x-vnd.obos-bapplication-testapp5");
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

