//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_INTERFACE__H
#define _K_PPP_INTERFACE__H

#include <driver_settings.h>

#include <KPPPDefs.h>

#ifndef _K_PPP_LCP__H
#include <KPPPLCP.h>
#endif

#include <KPPPReportManager.h>

#ifndef _K_PPP_STATE_MACHINE__H
#include <KPPPStateMachine.h>
#endif

#include <List.h>
#include <LockerHelper.h>

class PPPDevice;
class PPPProtocol;
class PPPEncapsulator;
class PPPOptionHandler;

struct ppp_manager_info;
struct ppp_module_info;


class PPPInterface {
		friend class PPPStateMachine;
		friend class PPPManager;

	private:
		// copies are not allowed!
		PPPInterface(const PPPInterface& copy);
		PPPInterface& operator= (const PPPInterface& copy);
		
		// only PPPManager may construct us!
		PPPInterface(interface_id ID, driver_settings *settings,
			PPPInterface *parent = NULL);
		~PPPInterface();

	public:
		void Delete();
		
		status_t InitCheck() const;
		
		interface_id ID() const
			{ return fID; }
		
		driver_settings* Settings() const
			{ return fSettings; }
		
		PPPStateMachine& StateMachine()
			{ return fStateMachine; }
		PPPLCP& LCP()
			{ return fLCP; }
		
		struct ifnet *Ifnet() const
			{ return fIfnet; }
		
		struct ifq *InQueue() const
			{ return fInQueue; }
		
		// delays
		uint32 DialRetryDelay() const
			{ return fDialRetryDelay; }
		uint32 RedialDelay() const
			{ return fRedialDelay; }
		
		// idle handling
		uint32 IdleSince() const
			{ return fIdleSince; }
		uint32 DisconnectAfterIdleSince() const
			{ return fDisconnectAfterIdleSince; }
		
		bool SetMRU(uint32 MRU);
		uint32 MRU() const
			{ return fMRU; }
				// this is the smallest MRU that we and the peer have
		bool SetInterfaceMTU(uint32 interfaceMTU)
			{ return SetMRU(interfaceMTU - fHeaderLength); }
		uint32 InterfaceMTU() const
			{ return fInterfaceMTU; }
				// this is the MRU including encapsulator overhead
		
		status_t Control(uint32 op, void *data, size_t length);
		
		bool SetDevice(PPPDevice *device);
		PPPDevice *Device() const
			{ return fDevice; }
		
		bool AddProtocol(PPPProtocol *protocol);
		bool RemoveProtocol(PPPProtocol *protocol);
		int32 CountProtocols() const
			{ return fProtocols.CountItems(); }
		PPPProtocol *ProtocolAt(int32 index) const;
		PPPProtocol *ProtocolFor(uint16 protocol, int32 *start = NULL) const;
		int32 IndexOfProtocol(PPPProtocol *protocol) const
			{ return fProtocols.IndexOf(protocol); }
		
		bool AddEncapsulator(PPPEncapsulator *encapsulator);
		bool RemoveEncapsulator(PPPEncapsulator *encapsulator);
		int32 CountEncapsulators() const;
		PPPEncapsulator *EncapsulatorAt(int32 index) const;
		PPPEncapsulator *FirstEncapsulator() const
			{ return fFirstEncapsulator; }
		PPPEncapsulator *EncapsulatorFor(uint16 protocol,
			PPPEncapsulator *start = NULL) const;
		
		// multilink methods
		bool AddChild(PPPInterface *child);
		bool RemoveChild(PPPInterface *child);
		int32 CountChildren() const
			{ return fChildren.CountItems(); }
		PPPInterface *ChildAt(int32 index) const;
		PPPInterface *Parent() const
			{ return fParent; }
		bool IsMultilink() const
			{ return fIsMultilink; }
		
		void SetAutoRedial(bool autoRedial = true);
		bool DoesAutoRedial() const
			{ return fAutoRedial; }
		
		void SetDialOnDemand(bool dialOnDemand = true);
		bool DoesDialOnDemand() const
			{ return fDialOnDemand; }
		
		ppp_mode Mode() const
			{ return fMode; }
			// client or server mode?
		ppp_state State() const
			{ return fStateMachine.State(); }
		ppp_phase Phase() const
			{ return fStateMachine.Phase(); }
		
		// Protocol-Field-Compression
		bool SetPFCOptions(uint8 pfcOptions);
		uint8 PFCOptions() const
			{ return fPFCOptions; }
		ppp_pfc_state LocalPFCState() const
			{ return fLocalPFCState; }
				// the local PFC state says if we accepted a request from the peer
				// i.e.: we may use PFC in outgoing packets
		ppp_pfc_state PeerPFCState() const
			{ return fPeerPFCState; }
				// the peer PFC state says if the peer accepted a request us
				// i.e.: the peer might send PFC-compressed packets to us
		bool UseLocalPFC() const
			{ return LocalPFCState() & PPP_PFC_ACCEPTED; }
		
		bool Up();
			// in server mode Up() listens for an incoming connection
		bool Down();
		bool IsUp() const;
		
		PPPReportManager& ReportManager()
			{ return fReportManager; }
		bool Report(ppp_report_type type, int32 code, void *data, int32 length)
			{ return fReportManager.Report(type, code, data, length); }
			// returns false if reply was bad (or an error occured)
		
		bool LoadModules(driver_settings *settings,
			int32 start, int32 count);
		bool LoadModule(const char *name, driver_parameter *parameter,
			ppp_module_key_type type);
		
		status_t Send(struct mbuf *packet, uint16 protocol);
		status_t Receive(struct mbuf *packet, uint16 protocol);
		
		status_t SendToDevice(struct mbuf *packet, uint16 protocol);
		status_t ReceiveFromDevice(struct mbuf *packet);
			// This is called by the receive-thread.
			// Only call this if it does not block Send() or
			// SendToDevice()!
		
		void Pulse();
			// this manages all timeouts, etc.

	private:
		bool RegisterInterface();
			// adds us to the manager module and
			// saves the returned ifnet structure
		bool UnregisterInterface();
		
		status_t StackControl(uint32 op, void *data);
			// stack routes ioctls to interface
		status_t StackControlEachHandler(uint32 op, void *data);
			// this calls StackControl() with the given parameters for each handler
		
		void CalculateInterfaceMTU();
		void CalculateBaudRate();
		
		void Redial(uint32 delay);
		
		// multilink methods
		void SetParent(PPPInterface *parent)
			{ fParent = parent; }

	private:
		interface_id fID;
			// the manager assigns an ID to every interface
		driver_settings *fSettings;
		PPPStateMachine fStateMachine;
		PPPLCP fLCP;
		PPPReportManager fReportManager;
		struct ifnet *fIfnet;
		
		thread_id fUpThread, fInQueueThread;
		struct ifq *fInQueue;
		
		thread_id fRedialThread;
		uint32 fDialRetry, fDialRetriesLimit;
		uint32 fDialRetryDelay, fRedialDelay;
		
		ppp_manager_info *fManager;
		
		uint32 fIdleSince, fDisconnectAfterIdleSince;
		uint32 fMRU, fInterfaceMTU, fHeaderLength;
		
		PPPInterface *fParent;
		List<PPPInterface*> fChildren;
		bool fIsMultilink;
		
		bool fAutoRedial, fDialOnDemand;
		
		ppp_mode fMode;
		ppp_pfc_state fLocalPFCState, fPeerPFCState;
		uint8 fPFCOptions;
		
		PPPDevice *fDevice;
		PPPEncapsulator *fFirstEncapsulator;
		List<PPPProtocol*> fProtocols;
		List<char*> fModules;
		
		BLocker& fLock;
		
		status_t fInitStatus;
		int32 fDeleteCounter;
};


#endif
