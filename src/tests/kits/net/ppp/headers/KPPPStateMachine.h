#ifndef _K_PPP_STATE_MACHINE__H
#define _K_PPP_STATE_MACHINE__H

#include "KPPPLCP.h"

#include "Locker.h"


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
			{ return Interface()->LCP(); }
		
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
		const char *AuthenticationName() const;
			// returns NULL if not authenticated
		PPP_AUTHENTICATION_STATUS AuthenticationStatus() const
			{ return fAuthenticationStatus; }
		
		void PeerAuthenticationRequested();
		void PeerAuthenticationAccepted(const char *name);
		void PeerAuthenticationDenied(const char *name);
		const char *PeerAuthenticationName() const;
			// returns NULL if not authenticated
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
		void LeaveConstructionPhase();
		void EnterDestructionPhase();
		
		// private StateMachine methods
		void NewState(PPP_STATE next);
		void NewPhase(PPP_PHASE next);
		
		// private events
		void OpenEvent();
		void CloseEvent();
		void TOGoodEvent();
		void TOBadEvent();
		void RCRGoodEvent(mbuf *packet);
		void RCRBadEvent(mbuf *nak, mbuf *reject);
		void RCAEvent(mbuf *packet);
		void RCNEvent(mbuf *packet);
		void RTREvent(mbuf *packet);
		void RTAEvent(mbuf *packet);
		void RUCEvent(mbuf *packet, uint16 protocol, uint8 type);
		void RXJGoodEvent(mbuf *packet);
		void RXJBadEvent(mbuf *packet);
		void RXREvent(mbuf *packet);
		
		// general events (for Good/Bad events)
		void TimerEvent();
		void RCREvent(mbuf *packet);
		void RXJEvent(mbuf *packet);
		
		// actions
		void IllegalEvent(PPP_EVENT event);
		void ThisLayerUp();
		void ThisLayerDown();
		void ThisLayerStarted();
		void ThisLayerFinished();
		void InitializeRestartCount();
		void ZeroRestartCount();
		void SendConfigureRequest();
		void SendConfigureAck(mbuf *packet);
		void SendConfigureNak(mbuf *packet);
		void SendTerminateRequest();
		void SendTerminateAck(mbuf *request);
		void SendCodeReject(mbuf *packet, uint16 protocol, uint8 type);
		void SendEchoReply(mbuf *request);
		
		void BringHandlersUp();
		uint32 BringPhaseUp();

	private:
		PPPInterface *fInterface;
		
		PPP_PHASE fPhase;
		PPP_STATE fState;
		
		vint32 fID;
		
		PPP_AUTHENTICATION_STATUS fAuthenticationStatus,
			fPeerAuthenticationStatus;
		int32 fAuthenticatorIndex, fPeerAuthenticatorIndex;
		
		BLocker fLock;
		
		// counters and timers
		int32 fMaxRequest, fMaxTerminate, fMaxNak;
		int32 fRequestCounter, fTerminateCounter, fNakCounter;
		uint8 fRequestID, fTerminateID;
			// the ID we used for the last configure/terminate request
		bigtime_t fTimeout;
			// last time we sent a packet
};


#endif
