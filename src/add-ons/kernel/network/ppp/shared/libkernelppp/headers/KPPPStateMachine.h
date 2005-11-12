/*
 * Copyright 2003-2004, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_STATE_MACHINE__H
#define _K_PPP_STATE_MACHINE__H

#include <KPPPDefs.h>

class KPPPProtocol;

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif

#include <Locker.h>

class PPPManager;
class KPPPInterface;
class KPPPLCP;


class KPPPStateMachine {
		friend class PPPManager;
		friend class KPPPInterface;
		friend class KPPPLCP;

	private:
		// may only be constructed/destructed by KPPPInterface
		KPPPStateMachine(KPPPInterface& interface);
		~KPPPStateMachine();
		
		// copies are not allowed!
		KPPPStateMachine(const KPPPStateMachine& copy);
		KPPPStateMachine& operator= (const KPPPStateMachine& copy);

	public:
		//!	Returns the interface that owns this state machine.
		KPPPInterface& Interface() const
			{ return fInterface; }
		//!	Returns the LCP protocol object belonging to this state machine.
		KPPPLCP& LCP() const
			{ return fLCP; }
		
		//!	Returns the current state as defined in RFC 1661.
		ppp_state State() const
			{ return fState; }
		//!	Returns the internal phase.
		ppp_phase Phase() const
			{ return fPhase; }
		
		uint8 NextID();
		
		//!	Sets our packets' magic number. Used by Link-Quality-Monitoring.
		void SetMagicNumber(uint32 magicNumber)
			{ fMagicNumber = magicNumber; }
		//!	Returns our packets' magic number.
		uint32 MagicNumber() const
			{ return fMagicNumber; }
		
		// public actions
		bool Reconfigure();
		bool SendEchoRequest();
		bool SendDiscardRequest();
		
		// public events:
		// NOTE: Local/PeerAuthenticationAccepted/Denied MUST be called before
		// Up(Failed)/DownEvent in order to allow changing the configuration of
		// the next phase's protocols before they are brought up.
		void LocalAuthenticationRequested();
		void LocalAuthenticationAccepted(const char *name);
		void LocalAuthenticationDenied(const char *name);
		//!	Returns the name/login string we used for authentication.
		const char *LocalAuthenticationName() const
			{ return fLocalAuthenticationName; }
		//!	Returns our local authentication status.
		ppp_authentication_status LocalAuthenticationStatus() const
			{ return fLocalAuthenticationStatus; }
		
		void PeerAuthenticationRequested();
		void PeerAuthenticationAccepted(const char *name);
		void PeerAuthenticationDenied(const char *name);
		//!	Returns the name/login string the peer used for authentication.
		const char *PeerAuthenticationName() const
			{ return fPeerAuthenticationName; }
		//!	Returns the peer's authentication status.
		ppp_authentication_status PeerAuthenticationStatus() const
			{ return fPeerAuthenticationStatus; }
		
		// sub-interface events
		void UpFailedEvent(KPPPInterface& interface);
		void UpEvent(KPPPInterface& interface);
		void DownEvent(KPPPInterface& interface);
		
		// protocol events
		void UpFailedEvent(KPPPProtocol *protocol);
		void UpEvent(KPPPProtocol *protocol);
		void DownEvent(KPPPProtocol *protocol);
		
		// device events
		bool TLSNotify();
		bool TLFNotify();
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();

	private:
		// private StateMachine methods
		void NewState(ppp_state next);
		void NewPhase(ppp_phase next);
		
		// private events
		void OpenEvent();
		void ContinueOpenEvent();
		void CloseEvent();
		void TOGoodEvent();
		void TOBadEvent();
		void RCRGoodEvent(struct mbuf *packet);
		void RCRBadEvent(struct mbuf *nak, struct mbuf *reject);
		void RCAEvent(struct mbuf *packet);
		void RCNEvent(struct mbuf *packet);
		void RTREvent(struct mbuf *packet);
		void RTAEvent(struct mbuf *packet);
		void RUCEvent(struct mbuf *packet, uint16 protocolNumber,
			uint8 code = PPP_PROTOCOL_REJECT);
		void RXJGoodEvent(struct mbuf *packet);
		void RXJBadEvent(struct mbuf *packet);
		void RXREvent(struct mbuf *packet);
		
		// general events (for Good/Bad events)
		void TimerEvent();
		void RCREvent(struct mbuf *packet);
		void RXJEvent(struct mbuf *packet);
		
		// actions
		void IllegalEvent(ppp_event event);
		void ThisLayerUp();
		void ThisLayerDown();
		void ThisLayerStarted();
		void ThisLayerFinished();
		void InitializeRestartCount();
		void ZeroRestartCount();
		bool SendConfigureRequest();
		bool SendConfigureAck(struct mbuf *packet);
		bool SendConfigureNak(struct mbuf *packet);
		bool SendTerminateRequest();
		bool SendTerminateAck(struct mbuf *request = NULL);
		bool SendCodeReject(struct mbuf *packet, uint16 protocolNumber, uint8 code);
		bool SendEchoReply(struct mbuf *request);
		
		void BringProtocolsUp();
		uint32 BringPhaseUp();
		
		void DownProtocols();
		void ResetLCPHandlers();

	private:
		KPPPInterface& fInterface;
		KPPPLCP& fLCP;
		
		ppp_state fState;
		ppp_phase fPhase;
		
		vint32 fID;
		uint32 fMagicNumber;
		int32 fLastConnectionReportCode;
		
		ppp_authentication_status fLocalAuthenticationStatus,
			fPeerAuthenticationStatus;
		char *fLocalAuthenticationName, *fPeerAuthenticationName;
		
		// counters and timers
		int32 fMaxRequest, fMaxTerminate, fMaxNak;
		int32 fRequestCounter, fTerminateCounter, fNakCounter;
		uint8 fRequestID, fTerminateID, fEchoID;
			// the ID we used for the last configure/terminate/echo request
		bigtime_t fNextTimeout;
		
		BLocker fLock;
};


#endif
