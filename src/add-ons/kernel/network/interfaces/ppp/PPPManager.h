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
#include <core_funcs.h>


// these functions are defined in ppp.cpp
extern int ppp_ifnet_stop(ifnet *ifp);
extern int ppp_ifnet_output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
	struct rtentry *rt0);
extern int ppp_ifnet_ioctl(ifnet *ifp, ulong cmd, caddr_t data);


class PPPManager {
	public:
		PPPManager();
		~PPPManager();
		
		int32 Stop(ifnet *ifp);
		int32 Output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
			struct rtentry *rt0);
		int32 Control(ifnet *ifp, ulong cmd, caddr_t data);
		
		interface_id CreateInterface(const driver_settings *settings,
			interface_id parentID = PPP_UNDEFINED_INTERFACE_ID);
		bool DeleteInterface(interface_id ID);
		bool RemoveInterface(interface_id ID);
		
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
		
		ppp_interface_entry *EntryFor(interface_id ID, int32 *saveIndex = NULL) const;
		ppp_interface_entry *EntryFor(ifnet *ifp, int32 *saveIndex = NULL) const;
		
		interface_id NextID()
			{ return (interface_id) atomic_add((int32*) &fNextID, 1); }
		
		void DeleterThreadEvent();
		void Pulse();

	private:
		int32 FindUnit() const;

	private:
		BLocker fLock, fReportLock;
		PPPReportManager fReportManager;
		TemplateList<ppp_interface_entry*> fEntries;
		interface_id fNextID;
		thread_id fDeleterThread, fPulseTimer;
};


#endif
