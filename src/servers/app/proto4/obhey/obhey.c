#include <SupportDefs.h>
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
/*
	Sends a message to the OpenBeOS app_server's message port
*/

int main(int argc, char **argv)
{
	int32 msgcode;
	
	if(argc>2)
	{
		port_id serverport=atoi(argv[1]);
		msgcode=atoi(argv[2]);

		write_port(serverport,msgcode,NULL,0);
		printf("Sent %ld to port %ld\n",msgcode,serverport);
		return 1;
	}

	printf("obhey port <message code>\n");
	return 0;
}