// BApplicationTestApp2.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	BApplication app("no valid MIME string");
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

