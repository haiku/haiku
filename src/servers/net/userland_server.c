#include "core_funcs.h"
#include "userland_ipc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define CORE_MODULE_PATH       "modules/core"

struct core_module_info *core = NULL;
static image_id gCoreID;


static status_t
load_core(void)
{
	char path[PATH_MAX];
	status_t status = -1;
	
	getcwd(path, PATH_MAX);
	sprintf(path, "%s/%s", path, CORE_MODULE_PATH);
	
	printf("opening %s\n", path);
	gCoreID = load_add_on(path);
	if (gCoreID < B_OK) {
		fprintf(stderr,"Unable to load the core module: %s\n",strerror(gCoreID));
		return gCoreID;
	}
	status = get_image_symbol(gCoreID, "core_info",B_SYMBOL_TYPE_ANY,(void**)&core);
	if (status < B_OK) {
		unload_add_on(gCoreID);
		printf("status = %ld, %s\n", status, strerror(status));
		return status;
	}
	return B_OK;
}


static void
unload_core()
{
	core = NULL;
	unload_add_on(gCoreID);
}


int
main(void)
{
	char buffer[8];

	if (init_userland_ipc() < B_OK)
		return -1;

	if (load_core() < B_OK) {
		shutdown_userland_ipc();
		return -1;
	}

	core->start();

	puts("Userland Server - is running. Press <Return> to quit.");
	fgets(buffer,sizeof(buffer),stdin);

	unload_core();
	shutdown_userland_ipc();

	return 0;
}
