#include <Application.h>
#include <InterfaceDefs.h>
#include <stdio.h>

int main() 
{
	BApplication app("application/x-vnd.idle-test");
	printf("idle_time() : %Ld\n", idle_time());
}

