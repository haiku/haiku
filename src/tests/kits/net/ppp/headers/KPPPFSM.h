#ifndef _K_PPP_FSM__H
#define _K_PPP_FSM__H

#include "KPPPLCP.h"

#include "Locker.h"


class PPPFSM {
		friend class PPPInterface;
		friend class PPPLCP;

	private:
		// may only be constructed/destructed by PPPInterface
		PPPFSM(PPPInterface &interface);
		~PPPFSM();

	public:
		PPPInterface *Interface() const
			{ return fInterface; }
		
		PPP_STATE State() const
			{ return fState; }
		PPP_PHASE Phase() const
			{ return fPhase; }
		
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
		BLocker &Locker()
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
		void RCRGoodEvent(PPPConfigurePacket *packet);
		void RCRBadEvent(PPPConfigurePacket *packet);
		void RCAEvent(PPPConfigurePacket *packet);
		void RCNEvent(PPPConfigurePacket *packet);
		void RTREvent();
		void RTAEvent();
		void RUCEvent();
		void RJXGoodEvent();
		void RJXBadEvent();
		void RXREvent();
		
		// actions
		void IllegalEvent(PPP_EVENT event);
		void ThisLayerUp();
		void ThisLayerDown();
		void ThisLayerStarted();
		void ThisLayerFinished();
		void InitializeRestartCount();
		void ZeroRestartCount();
		void SendConfigureRequest();
		void SendConfigureAck(PPPConfigurePacket *packet);
		
		void SendConfigureNak(PPPConfigurePacket *packet);
			// is this needed?
		
		void SendTerminateRequest();
		void SendTerminateAck();
		void SendCodeReject();
		void SendEchoReply();

	private:
		PPPInterface *fInterface;
		
		PPP_PHASE fPhase;
		PPP_STATE fState;
		
		PPP_AUTHENTICATION_STATUS fAuthenticationStatus,
			fPeerAuthenticationStatus;
		int32 fAuthenticatorIndex, fPeerAuthenticatorIndex;
		
		BLocker fLock;
};


#endif
