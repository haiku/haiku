//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_MANAGER__H
#define _K_PPP_MANAGER__H

#include "net_module.h"


#define PPP_INTERFACE_MODULE_NAME		 "network/interfaces/ppp"

#define PPP_UNDEFINED_INTERFACE_ID	0
	// create_interface() returns this value on failure

// this allows you to ask for specific interface_ids
enum ppp_interface_filter {
	PPP_ALL_INTERFACES,
	PPP_REGISTERED_INTERFACES,
	PPP_UNREGISTERED_INTERFACES
};

typedef struct ppp_interface_module_info {
	kernel_net_module_info knminfo;
	
	interface_id (*CreateInterface)(const driver_settings *settings,
		interface_id parent);
			// you should always create interfaces using this function
	void (*DeleteInterface)(interface_id ID);
		// this marks the interface for deletion
	void (*RemoveInterface)(interface_id ID);
		// remove the interface from database (make sure you can delete it yourself!)
	
	ifnet *(*RegisterInterface)(interface_id ID);
	bool (*UnregisterInterface)(interface_id ID);
	
	status_t (*Control)(interface_id ID, uint32 op, void *data, size_t length);
	
	int32 (*GetInterfaces)(interface_id *interfaces, int32 count,
		ppp_interface_filter filter = PPP_REGISTERED_INTERFACES);
			// make sure interfaces has enough space for count items
	int32 (*CountInterfaces)();
} ppp_interface_module_info;


#endif
