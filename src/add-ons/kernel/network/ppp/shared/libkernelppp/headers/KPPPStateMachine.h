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

#include <lock.h>
#include <util/AutoLock.h>

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
		void RCRGoodEvent(net_buffer *packet);
		void RCRBadEvent(net_buffer *nak, net_buffer *reject);
		void RCAEvent(net_buffer *packet);
		void RCNEvent(net_buffer *packet);
		void RTREvent(net_buffer *packet);
		void RTAEvent(net_buffer *packet);
		void RUCEvent(net_buffer *packet, uint16 protocolNumber,
			uint8 code = PPP_PROTOCOL_REJECT);
		void RXJGoodEvent(net_buffer *packet);
		void RXJBadEvent(net_buffer *packet);
		void RXREvent(net_buffer *packet);

		// general events (for Good/Bad events)
		void TimerEvent();
		void RCREvent(net_buffer *packet);
		void RXJEvent(net_buffer *packet);

		// actions
		void IllegalEvent(ppp_event event);
		void ThisLayerUp();
		void ThisLayerDown();
		void ThisLayerStarted();
		void ThisLayerFinished();
		void InitializeRestartCount();
		void ZeroRestartCount();
		bool SendConfigureRequest();
		bool SendConfigureAck(net_buffer *packet);
		bool SendConfigureNak(net_buffer *packet);
		bool SendTerminateRequest();
		bool SendTerminateAck(net_buffer *request = NULL);
		bool SendCodeReject(net_buffer *packet, uint16 protocolNumber, uint8 code);
		bool SendEchoReply(net_buffer *request);

		void BringProtocolsUp();
		uint32 BringPhaseUp();

		void DownProtocols();
		void ResetLCPHandlers();

	private:
		KPPPInterface& fInterface;
		KPPPLCP& fLCP;

		ppp_state fState;
		ppp_phase fPhase;

		int32 fID;
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

		mutex fLock;
};


#endif
