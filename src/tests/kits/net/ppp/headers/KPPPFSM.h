#ifndef _K_PPP_FSM__H
#define _K_PPP_FSM__H

#include "KPPPLCP.h"

#include "Locker.h"


class PPPFSM {
		friend class PPPInterface;
		friend class PPPLCP;

	private:
		// may only be constructed/destructed by PPPInterface
		PPPFSM(PPPInterface& interface);
		~PPPFSM();
		
		// copies are not allowed!
		PPPFSM(const PPPFSM& copy);
		PPPFSM& operator= (const PPPFSM& copy);

	public:
		PPPInterface *Interface() const
			{ return fInterface; }
		
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
		
		// private FSM methods
		void NewState(PPP_STATE next);
		
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
		void RUCEvent(mbuf *packet);
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
		void SendCodeReject(mbuf *packet);
		void SendEchoReply(mbuf *request);

	private:
		PPPInterface *fInterface;
		
		PPP_PHASE fPhase;
		PPP_STATE fState;
		
		vint32 fID;
		
		PPP_AUTHENTICATION_STATUS fAuthenticationStatus,
			fPeerAuthenticationStatus;
		int32 fAuthenticatorIndex, fPeerAuthenticatorIndex;
		
		BLocker fLock;
};


#endif
