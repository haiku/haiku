//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _PPP_MANAGER__H
#define _PPP_MANAGER__H

#include <KPPPInterface.h>
#include <KPPPManager.h>


typedef struct interface_entry {
	PPPInterface *interface;
	vint32 accessing;
	bool deleting;
} interface_entry;


class PPPManager {
	public:
		PPPManager();
		~PPPManager();
		
		interface_id CreateInterface(const driver_settings *settings,
			interface_id parentID);
		void DeleteInterface(interface_id ID);
		void RemoveInterface(interface_id ID);
		
		ifnet *RegisterInterface(interface_id ID);
		bool UnregisterInterface(interface_id ID);
		
		status_t Control(uint32 op, void *data, size_t length);
		status_t ControlInterface(interface_id ID, uint32 op, void *data,
			size_t length);
		
		int32 GetInterfaces(interface_id *interfaces, int32 count,
			ppp_interface_filter filter = PPP_REGISTERED_INTERFACES);
		int32 CountInterfaces(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES);
		
		PPPReportManager& ReportManager()
			{ return fReportManager; }
		bool Report(ppp_report_type type, int32 code, void *data, int32 length)
			{ return ReportManager().Report(type, code, data, length); }
			// returns false if reply was bad (or an error occured)
		
		interface_entry *EntryFor(interface_id ID, int32 *start = NULL) const;
		
		interface_id NextID()
			{ return (interface_id) atomic_add((vint32*) &fNextID, 1); }

	private:
		BLocker fLock;
		PPPReportManager fReportManager;
		List<interface_entry*> fEntries;
		interface_id fNextID;
};


#endif
