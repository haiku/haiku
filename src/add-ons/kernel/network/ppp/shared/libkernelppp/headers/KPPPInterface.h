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

#include <TemplateList.h>
#include <LockerHelper.h>

class PPPDevice;
class PPPProtocol;
class PPPOptionHandler;

struct ppp_interface_module_info;
struct ppp_module_info;
struct ppp_interface_entry;


class PPPInterface : public PPPLayer {
		friend class PPPStateMachine;
		friend class PPPManager;
		friend class PPPInterfaceAccess;

	private:
		// copies are not allowed!
		PPPInterface(const PPPInterface& copy);
		PPPInterface& operator= (const PPPInterface& copy);
		
		// only PPPManager may construct us!
		PPPInterface(ppp_interface_entry *entry, interface_id ID,
			const driver_settings *settings, PPPInterface *parent = NULL);
		~PPPInterface();

	public:
		void Delete();
		
		virtual status_t InitCheck() const;
		
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
				// this is the MRU including protocol overhead
		uint32 PacketOverhead() const;
			// including device and encapsulator headers
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		bool SetDevice(PPPDevice *device);
		PPPDevice *Device() const
			{ return fDevice; }
		
		bool AddProtocol(PPPProtocol *protocol);
		bool RemoveProtocol(PPPProtocol *protocol);
		int32 CountProtocols() const;
		PPPProtocol *ProtocolAt(int32 index) const;
		PPPProtocol *FirstProtocol() const
			{ return fFirstProtocol; }
		PPPProtocol *ProtocolFor(uint16 protocolNumber, PPPProtocol *start = NULL) const;
		
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
			{ return LocalPFCState() == PPP_PFC_ACCEPTED; }
		
		virtual bool Up();
			// in server mode Up() listens for an incoming connection
		virtual bool Down();
		bool IsUp() const;
		
		PPPReportManager& ReportManager()
			{ return fReportManager; }
		bool Report(ppp_report_type type, int32 code, void *data, int32 length)
			{ return ReportManager().Report(type, code, data, length); }
			// returns false if reply was bad (or an error occured)
		
		bool LoadModules(driver_settings *settings,
			int32 start, int32 count);
		bool LoadModule(const char *name, driver_parameter *parameter,
			ppp_module_key_type type);
		
		virtual bool IsAllowedToSend() const;
			// always returns true
		
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber);
			// sends the packet to the next handler (normally the device)
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber);
		
		status_t ReceiveFromDevice(struct mbuf *packet);
			// This must not block PPPDevice::Send()!
		
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
		
		// these two methods are used by the open/close_event_threads
		void CallOpenEvent()
			{ StateMachine().OpenEvent(); }
		void CallCloseEvent()
			{ StateMachine().CloseEvent(); }
		
		void Redial(uint32 delay);
		
		// multilink methods
		void SetParent(PPPInterface *parent)
			{ fParent = parent; }

	private:
		interface_id fID;
			// the manager assigns an ID to every interface
		driver_settings *fSettings;
		struct ifnet *fIfnet;
		
		thread_id fUpThread, fOpenEventThread, fCloseEventThread;
		
		thread_id fRedialThread;
		uint32 fDialRetry, fDialRetriesLimit;
		uint32 fDialRetryDelay, fRedialDelay;
		
		ppp_interface_module_info *fManager;
		
		uint32 fIdleSince, fDisconnectAfterIdleSince;
		uint32 fMRU, fInterfaceMTU, fHeaderLength;
		
		PPPInterface *fParent;
		TemplateList<PPPInterface*> fChildren;
		bool fIsMultilink;
		
		bool fAutoRedial, fDialOnDemand;
		
		ppp_mode fMode;
		ppp_pfc_state fLocalPFCState, fPeerPFCState;
		uint8 fPFCOptions;
		
		PPPDevice *fDevice;
		PPPProtocol *fFirstProtocol;
		TemplateList<char*> fModules;
		
		PPPStateMachine fStateMachine;
		PPPLCP fLCP;
		PPPReportManager fReportManager;
		BLocker& fLock;
		int32 fDeleteCounter;
};


#endif
