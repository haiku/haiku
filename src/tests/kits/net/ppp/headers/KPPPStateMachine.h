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
		PPPInterface& Interface() const
			{ return fInterface; }
		PPPLCP& LCP() const
			{ return fLCP; }
		
		ppp_state State() const
			{ return fState; }
		ppp_phase Phase() const
			{ return fPhase; }
		
		uint8 NextID();
			// return the next id for LCP packets
		
		void SetMagicNumber(uint32 magicNumber)
			{ fMagicNumber = magicNumber; }
		uint32 MagicNumber() const
			{ return fMagicNumber; }
		
		// public actions
		bool Reconfigure();
		bool SendEchoRequest();
		bool SendDiscardRequest();
		
		// public events
		void LocalAuthenticationRequested();
		void LocalAuthenticationAccepted(const char *name);
		void LocalAuthenticationDenied(const char *name);
		const char *LocalAuthenticationName() const
			{ return fLocalAuthenticationName; }
		ppp_authentication_status LocalAuthenticationStatus() const
			{ return fLocalAuthenticationStatus; }
		
		void PeerAuthenticationRequested();
		void PeerAuthenticationAccepted(const char *name);
		void PeerAuthenticationDenied(const char *name);
		const char *PeerAuthenticationName() const
			{ return fPeerAuthenticationName; }
		ppp_authentication_status PeerAuthenticationStatus() const
			{ return fPeerAuthenticationStatus; }
		
		// sub-interface events
		void UpFailedEvent(PPPInterface& interface);
		void UpEvent(PPPInterface& interface);
		void DownEvent(PPPInterface& interface);
		
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
		void NewState(ppp_state next);
		void NewPhase(ppp_phase next);
		
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
		void RUCEvent(struct mbuf *packet, uint16 protocol,
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
		bool SendCodeReject(struct mbuf *packet, uint16 protocol, uint8 code);
		bool SendEchoReply(struct mbuf *request);
		
		void BringHandlersUp();
		uint32 BringPhaseUp();
		
		void DownProtocols();
		void DownEncapsulators();
		void ResetLCPHandlers();

	private:
		PPPInterface& fInterface;
		PPPLCP& fLCP;
		
		ppp_phase fPhase;
		ppp_state fState;
		
		vint32 fID;
		uint32 fMagicNumber;
		
		ppp_authentication_status fLocalAuthenticationStatus,
			fPeerAuthenticationStatus;
		char *fLocalAuthenticationName, *fPeerAuthenticationName;
		
		BLocker fLock;
		
		// counters and timers
		int32 fMaxRequest, fMaxTerminate, fMaxNak;
		int32 fRequestCounter, fTerminateCounter, fNakCounter;
		uint8 fRequestID, fTerminateID, fEchoID;
			// the ID we used for the last configure/terminate/echo request
		bigtime_t fNextTimeout;
};


#endif
