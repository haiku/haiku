// BApplicationTestApp3a.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	BApplication app("image/gif", NULL);
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

