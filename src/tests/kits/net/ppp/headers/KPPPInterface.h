#ifndef _K_PPP_INTERFACE__H
#define _K_PPP_INTERFACE__H

#include <driver_settings.h>

#include "KPPPDefs.h"

#include "KPPPStateMachine.h"
#include "KPPPLCP.h"
#include "KPPPReport.h"

#include "List.h"
#include "LockerHelper.h"

class PPPDevice;
class PPPProtocol;
class PPPEncapsulator;
class PPPOptionHandler;


class PPPInterface {
	private:
		// copies are not allowed!
		PPPInterface(const PPPInterface& copy);
		PPPInterface& operator= (const PPPInterface& copy);

	public:
		PPPInterface(driver_settings *settings, PPPInterface *parent = NULL);
		~PPPInterface();
		
		void Delete();
		
		status_t InitCheck() const;
		
		driver_settings* Settings()
			{ return fSettings; }
		
		PPPStateMachine& StateMachine() const
			{ return fStateMachine; }
		PPPLCP& LCP() const
			{ return fLCP; }
		
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
		
		// multilink methods
		void AddChild(PPPInterface *child);
		void RemoveChild(PPPInterface *child);
		int32 CountChildren() const
			{ return fChildren.CountItems(); }
		PPPInterface *Parent() const
			{ return fParent; }
		bool IsMultilink() const
			{ return fIsMultilink; }
		
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
			{ return StateMachine().State(); }
		PPP_PHASE Phase() const
			{ return StateMachine().Phase(); }
		
		bool Up();
			// in server mode Up() listens for an incoming connection
		bool Down();
		bool IsUp() const;
		
		void EnableReports(PPP_REPORT_TYPE type, thread_id thread,
				int32 flags = PPP_NO_REPORT_FLAGS);
		void DisableReports(PPP_REPORT_TYPE type, thread_id thread);
		bool DoesReport(PPP_REPORT_TYPE type, thread_id thread);
		bool Report(PPP_REPORT_TYPE type, int32 code, void *data, int32 length);
			// returns false if reply was bad (or an error occured)
		
		bool LoadModules(const driver_settings *settings,
			int32 start, int32 count);
		bool LoadModule(const char *name, const driver_parameter *parameter);
		
		status_t Send(mbuf *packet, uint16 protocol);
		status_t Receive(mbuf *packet, uint16 protocol);
		
		status_t SendToDevice(mbuf *packet, uint16 protocol);
		status_t ReceiveFromDevice(mbuf *packet);
			// This is called by the receive-thread.
			// Only call this if it does not block Send() or
			// SendToDevice()!

	private:
		void CalculateMRU();
		
		// multilink methods
		void SetParent(PPPInterface *parent)
			{ fParent = parent; }

	private:
		driver_parameter *fSettings;
		PPPStateMachine fStateMachine;
		PPPLCP fLCP;
		ifnet *fIfnet;
		
		ppp_manager_info *fManager;
		
		uint32 fLinkMTU, fMRU, fHeaderLength;
		
		PPPInterface *fParent;
		List<PPPInterface*> fChildren;
		bool fIsMultilink;
		
		bool fAutoRedial, fDialOnDemand;
		
		vint32 fAccesing;
		
		PPP_MODE fMode;
		
		PPPDevice *fDevice;
		PPPEncapsulator *fFirstEncapsulator;
		List<PPPProtocol*> fProtocols;
		List<ppp_module_info*> fModules;
		List<ppp_report_request> fReportRequests;
		
		BLocker& fLock;
};


#endif
