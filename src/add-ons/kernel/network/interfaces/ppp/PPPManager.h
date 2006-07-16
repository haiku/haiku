/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

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
		
		ppp_interface_id CreateInterface(const driver_settings *settings,
			ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID);
		ppp_interface_id CreateInterfaceWithName(const char *name,
			ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID);
		bool DeleteInterface(ppp_interface_id ID);
		bool RemoveInterface(ppp_interface_id ID);
		
		ifnet *RegisterInterface(ppp_interface_id ID);
		bool UnregisterInterface(ppp_interface_id ID);
		
		status_t Control(uint32 op, void *data, size_t length);
		status_t ControlInterface(ppp_interface_id ID, uint32 op, void *data,
			size_t length);
		
		int32 GetInterfaces(ppp_interface_id *interfaces, int32 count,
			ppp_interface_filter filter = PPP_REGISTERED_INTERFACES);
		int32 CountInterfaces(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES);
		
		KPPPReportManager& ReportManager()
			{ return fReportManager; }
		bool Report(ppp_report_type type, int32 code, void *data, int32 length)
			{ return ReportManager().Report(type, code, data, length); }
			// returns false if reply was bad (or an error occured)
		
		// these methods must be called from a locked section
		ppp_interface_entry *EntryFor(ppp_interface_id ID,
			int32 *saveIndex = NULL) const;
		ppp_interface_entry *EntryFor(ifnet *ifp, int32 *saveIndex = NULL) const;
		ppp_interface_entry *EntryFor(const char *name, int32 *saveIndex = NULL) const;
		ppp_interface_entry *EntryFor(const driver_settings *settings) const;
		
		void SettingsChanged();
		
		ppp_interface_id NextID()
			{ return (ppp_interface_id) atomic_add((int32*) &fNextID, 1); }
		
		void DeleterThreadEvent();
		void Pulse();

	private:
		ppp_interface_id _CreateInterface(const char *name,
			const driver_settings *settings, ppp_interface_id parentID);
		int32 FindUnit() const;

	private:
		BLocker fLock, fReportLock;
		char *fDefaultInterface;
		KPPPReportManager fReportManager;
		TemplateList<ppp_interface_entry*> fEntries;
		ppp_interface_id fNextID;
		thread_id fDeleterThread, fPulseTimer;
};


#endif
