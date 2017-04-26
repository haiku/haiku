/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_MANAGER__H
#define _K_PPP_MANAGER__H

#include <module.h>
#include <net_device.h>
#include <PPPControl.h>
#include <PPPReportDefs.h>

#define PPP_INTERFACE_MODULE_NAME	NETWORK_MODULES_ROOT	"/ppp/ppp_manager/v1"

#define PPP_UNDEFINED_INTERFACE_ID	0
	// CreateInterface() returns this value on failure


class KPPPInterface;
//!	Private structure shared by PPP manager and KPPPInterface.
typedef struct ppp_interface_entry {
	char *name;
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
        struct module_info info;
	
	ppp_interface_id (*CreateInterface)(const driver_settings *settings,
		ppp_interface_id parentID);
	ppp_interface_id (*CreateInterfaceWithName)(const char *name,
		ppp_interface_id parentID);
	bool (*DeleteInterface)(ppp_interface_id ID);
		// this marks the interface for deletion
	bool (*RemoveInterface)(ppp_interface_id ID);
		// remove the interface from database (make sure you can delete it yourself!)
	
	net_device *(*RegisterInterface)(ppp_interface_id ID);
	KPPPInterface *(*GetInterface)(ppp_interface_id ID);
		// for accessing KPPPInterface directly
	bool (*UnregisterInterface)(ppp_interface_id ID);
	
	status_t (*ControlInterface)(ppp_interface_id ID, uint32 op, void *data,
		size_t length);
	
	int32 (*GetInterfaces)(ppp_interface_id *interfaces, int32 count,
		ppp_interface_filter filter);
			// make sure interfaces has enough space for count items
	int32 (*CountInterfaces)(ppp_interface_filter filter);
	
	void (*EnableReports)(ppp_report_type type, thread_id thread,
		int32 flags);
	void (*DisableReports)(ppp_report_type type, thread_id thread);
	bool (*DoesReport)(ppp_report_type type, thread_id thread);
} ppp_interface_module_info;


#endif
