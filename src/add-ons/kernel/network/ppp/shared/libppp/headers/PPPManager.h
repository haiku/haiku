//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _PPP_MANAGER__H
#define _PPP_MANAGER__H

#include <KPPPManager.h>


class PPPManager {
	private:
		// copies are not allowed/needed
		PPPManager(const PPPManager& copy);
		PPPManager& operator= (const PPPManager& copy);

	public:
		PPPManager();
		~PPPManager();
		
		status_t InitCheck() const;
		
		status_t Control(uint32 op, void *data, size_t length) const;
		
		ppp_interface_id CreateInterface(const driver_settings *settings,
			const driver_settings *profile = NULL) const;
		ppp_interface_id CreateInterfaceWithName(const char *name,
			const driver_settings *profile = NULL) const;
		bool DeleteInterface(ppp_interface_id ID) const;
		
		ppp_interface_id *Interfaces(int32 *count,
			ppp_interface_filter filter = PPP_REGISTERED_INTERFACES) const;
				// the user is responsible for deleting the returned array!
		int32 GetInterfaces(ppp_interface_id *interfaces, int32 count,
			ppp_interface_filter filter = PPP_REGISTERED_INTERFACES) const;
				// make sure interfaces has enough space for count items
		ppp_interface_id InterfaceWithSettings(const driver_settings *settings) const;
		ppp_interface_id InterfaceWithUnit(int32 if_unit) const;
		ppp_interface_id InterfaceWithName(const char *name) const;
		int32 CountInterfaces(ppp_interface_filter filter =
			PPP_REGISTERED_INTERFACES) const;
		
		bool EnableReports(ppp_report_type type, thread_id thread,
			int32 flags = PPP_NO_FLAGS) const;
		bool DisableReports(ppp_report_type type, thread_id thread) const;

	private:
		int fFD;
};


#endif
