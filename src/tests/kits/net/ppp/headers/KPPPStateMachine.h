//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_STATE_MACHINE__H
#define _K_PPP_STATE_MACHINE__H

#include <KPPPDefs.h>

class PPPEncapsulator;
class PPPInterface;
class PPPProtocol;

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif

#include <Locker.h>


class PPPStateMachine {
		friend class PPPInterface;
		friend class PPPLCP;

	private:
		// may only be constructed/destructed by PPPInterface
		PPPStateMachine(PPPInterface& interface);
		~PPPStateMachine();
		
		// copies are not allowed!
		PPPStateMachine(const PPPStateMachine& copy);
		PPPStateMachine& operator= (const PPPStateMachine& copy);

	public:
		PPPInterface *Interface() const
			{ return fInterface; }
		PPPLCP& LCP() const
			{ return fLCP; }
		
		PPP_STATE State() const
			{ return fState; }
		PPP_PHASE Phase() const
			{ return fPhase; }
		
		uint8 NextID();
			// return the next id for lcp_packets
		
		// public events
		void AuthenticationRequested();
		void AuthenticationAccepted(const char *name);
		void AuthenticationDenied(const char *name);
		const char *AuthenticationName() const
			{ return fAuthenticationName; }
		PPP_AUTHENTICATION_STATUS AuthenticationStatus() const
			{ return fAuthenticationStatus; }
		
		void PeerAuthenticationRequested();
		void PeerAuthenticationAccepted(const char *name);
		void PeerAuthenticationDenied(const char *name);
		const char *PeerAuthenticationName() const
			{ return fPeerAuthenticationName; }
		PPP_AUTHENTICATION_STATUS PeerAuthenticationStatus() const
			{ return fPeerAuthenticationStatus; }
		
		// sub-interface events
		void UpFailedEvent(PPPInterface *interface);
		void UpEvent(PPPInterface *interface);
		void DownEvent(PPPInterface *interface);
		
		// protocol events
		void UpFailedEvent(PPPProtocol *protocol);
		void UpEvent(PPPProtocol *protocol);
		void DownEvent(PPPProtocol *protocol);
		
		// encapsulator events
		void UpFailedEvent(PPPEncapsulator *encapsulator);
		void UpEvent(PPPEncapsulator *encapsulator);
		void DownEvent(PPPEncapsulator *encapsulator);
		
		// device events
		bool TLSNotify();
		bool TLFNotify();
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();

	private:
		BLocker& Locker()
			{ return fLock; }
		
		// private StateMachine methods
		void NewState(PPP_STATE next);
		void NewPhase(PPP_PHASE next);
		
		// private events
		void OpenEvent();
		void CloseEvent();
		void TOGoodEvent();
		void TOBadEvent();
		void RCRGoodEvent(struct mbuf *packet);
		void RCRBadEvent(struct mbuf *nak, struct mbuf *reject);
		void RCAEvent(struct mbuf *packet);
		void RCNEvent(struct mbuf *packet);
		void RTREvent(struct mbuf *packet);
		void RTAEvent(struct mbuf *packet);
		void RUCEvent(struct mbuf *packet, uint16 protocol, uint8 type = PPP_PROTOCOL_REJECT);
		void RXJGoodEvent(struct mbuf *packet);
		void RXJBadEvent(struct mbuf *packet);
		void RXREvent(struct mbuf *packet);
		
		// general events (for Good/Bad events)
		void TimerEvent();
		void RCREvent(struct mbuf *packet);
		void RXJEvent(struct mbuf *packet);
		
		// actions
		void IllegalEvent(PPP_EVENT event);
		void ThisLayerUp();
		void ThisLayerDown();
		void ThisLayerStarted();
		void ThisLayerFinished();
		void InitializeRestartCount();
		void ZeroRestartCount();
		void SendConfigureRequest();
		void SendConfigureAck(struct mbuf *packet);
		void SendConfigureNak(struct mbuf *packet);
		void SendTerminateRequest();
		void SendTerminateAck(struct mbuf *request);
		void SendCodeReject(struct mbuf *packet, uint16 protocol, uint8 type);
		void SendEchoReply(struct mbuf *request);
		
		void BringHandlersUp();
		uint32 BringPhaseUp();
		
		void DownProtocols();
		void DownEncapsulators();
		void ResetOptionHandlers();

	private:
		PPPInterface *fInterface;
		PPPLCP& fLCP;
		
		PPP_PHASE fPhase;
		PPP_STATE fState;
		
		vint32 fID;
		
		PPP_AUTHENTICATION_STATUS fAuthenticationStatus,
			fPeerAuthenticationStatus;
		char *fAuthenticationName, *fPeerAuthenticationName;
		
		BLocker fLock;
		
		// counters and timers
		int32 fMaxRequest, fMaxTerminate, fMaxNak;
		int32 fRequestCounter, fTerminateCounter, fNakCounter;
		uint8 fRequestID, fTerminateID;
			// the ID we used for the last configure/terminate request
		bigtime_t fNextTimeout;
};


#endif
