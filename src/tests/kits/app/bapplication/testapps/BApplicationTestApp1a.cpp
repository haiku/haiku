// BApplicationTestApp1a.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	BApplication app((const char*)NULL, NULL);
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

