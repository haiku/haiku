// BApplicationTestApp2a.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	BApplication app("no valid MIME string", NULL);
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

