//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _K_PPP_MANAGER__H
#define _K_PPP_MANAGER__H

#include "net_module.h"
#include <PPPControl.h>
#include <PPPReportDefs.h>


#define PPP_INTERFACE_MODULE_NAME		 NETWORK_MODULES_ROOT "interfaces/ppp"

#define PPP_UNDEFINED_INTERFACE_ID	0
	// create_interface() returns this value on failure


class KPPPInterface;
typedef struct ppp_interface_entry {
	KPPPInterface *interface;
	vint32 accessing;
	bool deleting;
} ppp_interface_entry;


/*!	\brief Exported by the PPP interface manager kernel module.
	
	You should always create interfaces using either of the CreateInterface()
	functions. The manager keeps track of all running connections and automatically
	deletes them when they are not needed anymore.
*/
typedef struct ppp_interface_module_info {
	kernel_net_module_info knminfo;
		//!< Exports needed network module functions.
	
	ppp_interface_id (*CreateInterface)(const driver_settings *settings,
		const driver_settings *profile = NULL,
		ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID);
	ppp_interface_id (*CreateInterfaceWithName)(const char *name,
		const driver_settings *profile = NULL,
		ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID);
	bool (*DeleteInterface)(ppp_interface_id ID);
		// this marks the interface for deletion
	bool (*RemoveInterface)(ppp_interface_id ID);
		// remove the interface from database (make sure you can delete it yourself!)
	
	ifnet *(*RegisterInterface)(ppp_interface_id ID);
	bool (*UnregisterInterface)(ppp_interface_id ID);
	
	status_t (*ControlInterface)(ppp_interface_id ID, uint32 op, void *data,
		size_t length);
	
	int32 (*GetInterfaces)(ppp_interface_id *interfaces, int32 count,
		ppp_interface_filter filter = PPP_REGISTERED_INTERFACES);
			// make sure interfaces has enough space for count items
	int32 (*CountInterfaces)(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES);
	
	void (*EnableReports)(ppp_report_type type, thread_id thread,
		int32 flags = PPP_NO_FLAGS);
	void (*DisableReports)(ppp_report_type type, thread_id thread);
	bool (*DoesReport)(ppp_report_type type, thread_id thread);
} ppp_interface_module_info;


#endif
