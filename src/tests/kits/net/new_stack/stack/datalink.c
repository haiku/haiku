/* core.c */

/* This the heart of network stack
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>

#include <KernelExport.h>
#include <OS.h>

#include "datalink.h"
#include "data.h"

/* Defines we need */
#define NETWORK_INTERFACES	"network/interfaces"
#define NETWORK_PROTOCOLS	"network/protocols"

struct interfaces_list {
	ifnet_t 			*first;
	sem_id				lock;
};

static struct interfaces_list g_interfaces;

status_t enable_interface(ifnet_t *iface, bool enable);
status_t interface_reader(void *args);


//	#pragma mark [Start/Stop functions]


// --------------------------------------------------
status_t start_datalink_layer()
{
	void 	*module_list;
	ifnet_t *iface;

	g_interfaces.first = NULL;
	
	g_interfaces.lock = create_sem(1, "net_interfaces list lock");
	if (g_interfaces.lock < B_OK)
		return B_ERROR;

#ifdef _KERNEL_MODE
	set_sem_owner(g_interfaces.lock, B_SYSTEM_TEAM);
#endif

	// Load all network/interfaces/* modules and let them
	// register any interface they may support by calling init()
	module_list = open_module_list(NET_INTERFACE_MODULE_ROOT);
	if (module_list) {
		size_t sz;
		char module_name[256];
		struct net_interface_module_info *nimi;
		
		sz = sizeof(module_name);
		while(read_next_module_name(module_list, module_name, &sz) == B_OK) {
			if (strlen(module_name) && get_module(module_name, (module_info **) &nimi) == B_OK) {
				printf("datalink: initing %s interface module...\n", module_name);
				// allow this module to register one or more interfaces
				nimi->init(NULL);
			};
			sz = sizeof(module_name);
		};
		close_module_list(module_list);
	};

	acquire_sem(g_interfaces.lock);
	iface = g_interfaces.first;
	while (iface) {
		enable_interface(iface, true);
		iface = iface->if_next;
	};
	release_sem(g_interfaces.lock);

	puts("net datalink layer started.");
	return B_OK;
}

// --------------------------------------------------
status_t stop_datalink_layer()
{
	ifnet_t *iface, *next;

	delete_sem(g_interfaces.lock);
	g_interfaces.lock = -1;

	// free the remaining interfaces entries
	iface = g_interfaces.first;
	while (iface) {
		printf("datalink: uninit interface %s\n", iface->if_name);
		// down the interface if currently up
		enable_interface(iface, false);
			
		iface->module->uninit(iface);
		iface = iface->if_next;
	};

	// unload all interface modules... and free iface structs 
	iface = g_interfaces.first;
	while (iface) {
		printf("datalink: unloading %s interface module\n", iface->module->info.name);
		next = iface->if_next;
		put_module(iface->module->info.name);
		free(iface);
		iface = next;
	};

	puts("net datalink layer stopped.");

	return B_OK;
}



// #pragma mark [Public functions]

// --------------------------------------------------
status_t register_interface(ifnet_t *iface)
{
	status_t status;

	if (!iface)
		return B_ERROR;
		
	iface->if_reader_thread = -1;
	
	status = acquire_sem(g_interfaces.lock);
	if (status != B_OK)
		return status;

	iface->if_next = g_interfaces.first;
	g_interfaces.first = iface;
	
	release_sem(g_interfaces.lock);
		
	printf("datalink: register_interface(%s)\n", iface->if_name);
	return B_OK;
}

// --------------------------------------------------
status_t unregister_interface(ifnet_t *iface)
{
	status_t status;

	if (!iface)
		return B_ERROR;
		
	status = acquire_sem(g_interfaces.lock);
	if (status != B_OK)
		return status;

	if (g_interfaces.first == iface)
		g_interfaces.first = iface->if_next;
	else {
		ifnet_t * p = g_interfaces.first;
		while (p && p->if_next != iface)
			p = p->if_next;
			
		if (!p)
			printf("datalink: unregister_interface(): %p iface not found in list!\n", iface);
		else
			p->if_next = iface->if_next;
	};
	
	release_sem(g_interfaces.lock);

	printf("datalink: unregister_interface(%s)\n", iface->if_name);
	return iface->module->uninit(iface);
}


// ----------------------------------------------------
status_t enable_interface(ifnet_t *iface, bool enable)
{
	if (enable) {
		thread_id tid;

		if (iface->if_flags & IFF_UP)
			// already up
			return B_OK;
		
		iface->module->up(iface);
		
		tid = spawn_kernel_thread(interface_reader, iface->if_name, 
		                              B_NORMAL_PRIORITY, iface);
		if (tid < 0) {
			printf("datalink: enable_interface(%s): failed to start reader thread -> %d [%s]\n",
				iface->if_name, (int) tid, strerror(tid));
			return tid;
		};

		iface->if_reader_thread = tid;
		iface->if_flags |= IFF_UP;
		printf("datalink: starting interface %s...\n", iface->if_name);
		return resume_thread(tid);
	} else {
		if (iface->if_reader_thread) {
			kill_thread(iface->if_reader_thread);
			iface->if_reader_thread = -1;
		};
		iface->if_flags &= ~IFF_UP;
		return iface->module->down(iface);
	};
}


// ----------------------------------------------------
status_t interface_reader(void *args)
{
	ifnet_t 	*iface = args;
	net_data 	*nd;
	status_t status;
	
	if (!iface || iface->module == NULL)
		return B_ERROR;
		
	while(iface->if_flags & IFF_UP) {
		status = iface->module->receive_data(iface, &nd);
		if (status < B_OK || nd == NULL)
			continue;
		dump_data(nd);
		delete_data(nd, false);
	};
	
	return B_OK;
}

