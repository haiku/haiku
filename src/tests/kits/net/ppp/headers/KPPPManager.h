#ifndef _K_PPP_MANAGER__H
#define _K_PPP_MANAGER__H

#include <net_module.h>


#define PPP_MANAGER_MODULE_NAME "network/interfaces/ppp"


typedef struct ppp_manager_info {
	kernel_net_module_info knminfo;
	
	// when you want to iterate through interfaces you should use locking
	void (*lock)();
	void (*unlock)();
	
	ifnet* (*add_interface)(PPPInterface *interface);
	bool (*remove_interface)(PPPInterface *interface);
	
	int32 (*count_interfaces)(int32 index);
	
	PPPInterface *(*get_interface_at)(int32 index);
	void (*put_interface)(PPPInterface *interface);
	
	void (*delete_interface)(PPPInterface *interface);
		// this deletes the interface when it is not needed anymore
} ppp_manager;


#endif
