// BApplicationTestApp1b.cpp

#include <stdio.h>

#include <Application.h>

int
main()
{
	status_t error = B_OK;
	BApplication app((const char*)NULL, &error);
	printf("error: %lx\n", error);
	printf("InitCheck(): %lx\n", app.InitCheck());
	return 0;
}

