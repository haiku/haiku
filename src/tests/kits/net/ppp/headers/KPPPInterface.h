#ifndef _K_PPP_INTERFACE__H
#define _K_PPP_INTERFACE__H

#include <driver_settings.h>

#include "KPPPDefs.h"

#include "KPPPFSM.h"
#include "KPPPLCP.h"

#include "List.h"
#include "LockerHelper.h"

class PPPAuthenticator;
class PPPDevice;
class PPPEncapsulator;
class PPPOptionHandler;
class PPPProtocol;


class PPPInterface {
	public:
		PPPInterface(driver_settings *settings);
		~PPPInterface();
		
		status_t InitCheck() const;
		
		driver_settings* Settings()
			{ return fSettings; }
		
		PPPFSM &FSM() const
			{ return fFSM; }
		
		bool RegisterInterface();
			// adds us to the manager module and
			// saves the returned ifnet structure
		bool UnregisterInterface();
		ifnet *Ifnet() const
			{ return fIfnet; }
		
		void SetLinkMTU(uint32 linkMTU);
		uint32 LinkMTU() const
			{ return fLinkMTU; }
				// this is the smallest MTU that we and the peer have
		uint32 MRU() const
			{ return fMRU; }
		
		status_t Control(uint32 op, void *data, size_t length);
		
		bool SetDevice(PPPDevice *device);
		
		bool AddProtocol(PPPProtocol *protocol);
		bool RemoveProtocol(PPPProtocol *protocol);
		int32 CountProtocols() const
			{ return fProtocols.CountItems(); }
		PPPProtocol *ProtocolAt(int32 index) const;
		PPPProtocol *ProtocolFor(uint16 protocol, int32 start = 0) const;
		int32 IndexOfProtocol(const PPPProtocol *protocol) const
			{ return fProtocols.IndexOf(protocol); }
		
		bool AddEncapsulator(PPPEncapsulator *encapsulator);
		bool RemoveEncapsulator(PPPEncapsulator *encapsulator);
		PPPEncapsulator *FirstEncapsulator() const
			{ return fFirstEncapsulator; }
		PPPEncapsulator *EncapsulatorFor(uint16 protocol,
			PPPEncapsulator start = NULL) const;
		
		bool AddOptionHandler(PPPOptionHandler *handler);
		bool RemoveOptionHandler(PPPOptionHandler *handler);
		int32 CountOptionHandlers() const
			{ return fOptionHandlers.CountItems(); }
		PPPOptionHandler *OptionHandlerAt(int32 index) const;
		
		void SetAutoRedial(bool autoredial = true);
		bool DoesAutoRedial() const
			{ return fAutoRedial; }
		
		void SetDialOnDemand(bool dialondemand = true);
		bool DoesDialOnDemand() const
			{ return fDialOnDemand; }
		
		PPP_MODE Mode() const
			{ return fMode; }
			// client or server mode?
		PPP_STATE State() const
			{ return FSM().State(); }
		PPP_PHASE Phase() const
			{ return FSM().Phase(); }
		
		bool Up();
			// in server mode Up() listens for an incoming connection
		bool Down();
		bool IsUp() const;
		
/*		void EnableReports(PPP_REPORT type, thread_id thread,
				PPP_REPORT_FLAGS flags);
		void DisableReports(PPP_REPORT type, thread_id thread);
		bool DoesReport(PPP_REPORT type, thread_id thread); */
		
		bool LoadModules(const driver_settings *settings,
			int32 start, int32 count);
		bool LoadModule(const char *name, const driver_parameter *parameter);
		
		status_t Send(mbuf *packet, uint16 protocol);
		status_t Receive(mbuf *packet, uint16 protocol);
		
		status_t SendToDevice(mbuf *packet, uint16 protocol);
		status_t ReceiveFromDevice(mbuf *packet);

	private:
		PPPLCP &LCP() const
			{ return fLCP; }
		void CalculateMRU();
		
//		void Report(PPP_REPORT type, int32 code, void *data, int32 length);

	private:
		driver_parameter *fSettings;
		PPPFSM fFSM;
		PPPLCP fLCP;
		ifnet *fIfnet;
		
		uint32 fLinkMTU, fMRU, fHeaderLength;
		
		bool fAutoRedial, fDialOnDemand;
		
		vint32 fAccesing;
		
		PPP_MODE fMode;
		
		PPPDevice *fDevice;
		PPPEncapsulator *fFirstEncapsulator;
		
		List<PPPProtocol*> fProtocols;
		List<PPPOptionHandler*> fOptionHandlers;
		List<ppp_module_info*> fModules;
		
		BLocker &fGeneralLock;
};


#endif
