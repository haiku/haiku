/* Network modules debug platform
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <app/Application.h>
#include <drivers/module.h>

#include <core_module.h>
#include <userland_ipc.h>

struct core_module_info * core = NULL;

int main(int argc, char **argv)
{
	char buffer[8];
	int ret = -1;
	
	new BApplication("application/x-vnd-OBOS-net_server");

	if (init_userland_ipc() < B_OK)
		goto exit;

	if (get_module(NET_CORE_MODULE_NAME, (module_info **) &core) != B_OK) {
		shutdown_userland_ipc();
		goto exit;
	}

	puts("Starting core module...");
	core->start();

	puts("Userland net stack (net_server) is running. Press <Return> to quit.");
	fgets(buffer,sizeof(buffer), stdin);

	put_module(NET_CORE_MODULE_NAME);;
	shutdown_userland_ipc();

	ret = 0;
	
exit:;
	delete be_app;
	return ret;
}

