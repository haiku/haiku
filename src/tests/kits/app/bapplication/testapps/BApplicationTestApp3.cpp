// BApplicationTestApp3.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	BApplication app("image/gif");
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

