//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <OS.h>

#include <KPPPInterface.h>
#include <KPPPConfigurePacket.h>
#include <KPPPDevice.h>
#include <KPPPLCPExtension.h>
#include <KPPPOptionHandler.h>

#include <net/if.h>
#include <core_funcs.h>


#ifdef _KERNEL_MODE
	#define spawn_thread spawn_kernel_thread
	#define printf dprintf
#elif DEBUG
	#include <cstdio>
#endif


#define PPP_STATE_MACHINE_TIMEOUT			3000000
	// 3 seconds


PPPStateMachine::PPPStateMachine(PPPInterface& interface)
	: fInterface(interface),
	fLCP(interface.LCP()),
	fState(PPP_INITIAL_STATE),
	fPhase(PPP_DOWN_PHASE),
	fID(system_time() & 0xFF),
	fMagicNumber(0),
	fLocalAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fPeerAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fLocalAuthenticationName(NULL),
	fPeerAuthenticationName(NULL),
	fMaxRequest(10),
	fMaxTerminate(2),
	fMaxNak(5),
	fRequestID(0),
	fTerminateID(0),
	fEchoID(0),
	fNextTimeout(0)
{
}


PPPStateMachine::~PPPStateMachine()
{
	free(fLocalAuthenticationName);
	free(fPeerAuthenticationName);
}


uint8
PPPStateMachine::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


// remember: NewState() must always be called _after_ IllegalEvent()
// because IllegalEvent() also looks at the current state.
void
PPPStateMachine::NewState(ppp_state next)
{
#if DEBUG
	printf("PPPSM: NewState(%d) state=%d\n", next, State());
#endif
	
	// maybe we do not need the timer anymore
	if(next < PPP_CLOSING_STATE || next == PPP_OPENED_STATE)
		fNextTimeout = 0;
	
	if(State() == PPP_OPENED_STATE && next != State())
		ResetLCPHandlers();
	
	fState = next;
}


void
PPPStateMachine::NewPhase(ppp_phase next)
{
#if DEBUG
	if(next <= PPP_ESTABLISHMENT_PHASE || next == PPP_ESTABLISHED_PHASE)
		printf("PPPSM: NewPhase(%d) phase=%d\n", next, Phase());
#endif
	
	// there is nothing after established phase and nothing before down phase
	if(next > PPP_ESTABLISHED_PHASE)
		next = PPP_ESTABLISHED_PHASE;
	else if(next < PPP_DOWN_PHASE)
		next = PPP_DOWN_PHASE;
	
	// Report a down event to parent if we are not usable anymore.
	// The report threads get their notification later.
	if(Phase() == PPP_ESTABLISHED_PHASE && next != Phase()) {
		if(!Interface().DoesDialOnDemand())
			Interface().UnregisterInterface();
		
		if(Interface().Ifnet()) {
			Interface().Ifnet()->if_flags &= ~IFF_RUNNING;
			
			if(!Interface().DoesDialOnDemand())
				Interface().Ifnet()->if_flags &= ~IFF_UP;
		}
		
		if(Interface().Parent())
			Interface().Parent()->StateMachine().DownEvent(Interface());
	}
	
	fPhase = next;
	
	if(Phase() == PPP_ESTABLISHED_PHASE) {
		if(Interface().Ifnet())
			Interface().Ifnet()->if_flags |= IFF_UP | IFF_RUNNING;
		
		Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_UP_SUCCESSFUL, NULL, 0);
	}
}


// public actions
bool
PPPStateMachine::Reconfigure()
{
#if DEBUG
	printf("PPPSM: Reconfigure() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(State() < PPP_REQ_SENT_STATE)
		return false;
	
	NewState(PPP_REQ_SENT_STATE);
	NewPhase(PPP_ESTABLISHMENT_PHASE);
		// indicates to handlers that we are reconfiguring
	
	DownProtocols();
	ResetLCPHandlers();
	
	locker.UnlockNow();
	
	return SendConfigureRequest();
}


bool
PPPStateMachine::SendEchoRequest()
{
#if DEBUG
	printf("PPPSM: SendEchoRequest() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if(State() != PPP_OPENED_STATE)
		return false;
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if(!packet)
		return false;
	
	packet->m_data += LCP().AdditionalOverhead();
	packet->m_pkthdr.len = packet->m_len = 8;
		// echo requests are at least eight bytes long
	
	ppp_lcp_packet *request = mtod(packet, ppp_lcp_packet*);
	request->code = PPP_ECHO_REQUEST;
	request->id = NextID();
	fEchoID = request->id;
	request->length = htons(packet->m_len);
	memcpy(request->data, &fMagicNumber, sizeof(fMagicNumber));
	
	return LCP().Send(packet) == B_OK;
}


bool
PPPStateMachine::SendDiscardRequest()
{
#if DEBUG
	printf("PPPSM: SendDiscardRequest() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if(State() != PPP_OPENED_STATE)
		return false;
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if(!packet)
		return false;
	
	packet->m_data += LCP().AdditionalOverhead();
	packet->m_pkthdr.len = packet->m_len = 8;
		// discard requests are at least eight bytes long
	
	ppp_lcp_packet *request = mtod(packet, ppp_lcp_packet*);
	request->code = PPP_DISCARD_REQUEST;
	request->id = NextID();
	request->length = htons(packet->m_len);
	memcpy(request->data, &fMagicNumber, sizeof(fMagicNumber));
	
	return LCP().Send(packet) == B_OK;
}


// authentication events
void
PPPStateMachine::LocalAuthenticationRequested()
{
#if DEBUG
	printf("PPPSM: LocalAuthenticationRequested() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	fLocalAuthenticationStatus = PPP_AUTHENTICATING;
	free(fLocalAuthenticationName);
	fLocalAuthenticationName = NULL;
}


void
PPPStateMachine::LocalAuthenticationAccepted(const char *name)
{
#if DEBUG
	printf("PPPSM: LocalAuthenticationAccepted() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	fLocalAuthenticationStatus = PPP_AUTHENTICATION_SUCCESSFUL;
	free(fLocalAuthenticationName);
	if(name)
		fLocalAuthenticationName = strdup(name);
	else
		fLocalAuthenticationName = NULL;
	
	Interface().Report(PPP_CONNECTION_REPORT,
		PPP_REPORT_LOCAL_AUTHENTICATION_SUCCESSFUL, NULL, 0);
}


void
PPPStateMachine::LocalAuthenticationDenied(const char *name)
{
#if DEBUG
	printf("PPPSM: LocalAuthenticationDenied() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	fLocalAuthenticationStatus = PPP_AUTHENTICATION_FAILED;
	free(fLocalAuthenticationName);
	if(name)
		fLocalAuthenticationName = strdup(name);
	else
		fLocalAuthenticationName = NULL;
}


void
PPPStateMachine::PeerAuthenticationRequested()
{
#if DEBUG
	printf("PPPSM: PeerAuthenticationRequested() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATING;
	free(fPeerAuthenticationName);
	fPeerAuthenticationName = NULL;
}


void
PPPStateMachine::PeerAuthenticationAccepted(const char *name)
{
#if DEBUG
	printf("PPPSM: PeerAuthenticationAccepted() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATION_SUCCESSFUL;
	free(fPeerAuthenticationName);
	if(name)
		fPeerAuthenticationName = strdup(name);
	else
		fPeerAuthenticationName = NULL;
	
	Interface().Report(PPP_CONNECTION_REPORT,
		PPP_REPORT_PEER_AUTHENTICATION_SUCCESSFUL, NULL, 0);
}


void
PPPStateMachine::PeerAuthenticationDenied(const char *name)
{
#if DEBUG
	printf("PPPSM: PeerAuthenticationDenied() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATION_FAILED;
	free(fPeerAuthenticationName);
	if(name)
		fPeerAuthenticationName = strdup(name);
	else
		fPeerAuthenticationName = NULL;
	
	CloseEvent();
}


void
PPPStateMachine::UpFailedEvent(PPPInterface& interface)
{
#if DEBUG
	printf("PPPSM: UpFailedEvent(interface) state=%d phase=%d\n",
		State(), Phase());
#endif
	
	// TODO:
	// log that an interface did not go up
}


void
PPPStateMachine::UpEvent(PPPInterface& interface)
{
#if DEBUG
	printf("PPPSM: UpEvent(interface) state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(Phase() <= PPP_TERMINATION_PHASE) {
		interface.StateMachine().CloseEvent();
		return;
	}
	
	Interface().CalculateBaudRate();
	
	if(Phase() == PPP_ESTABLISHMENT_PHASE) {
		// this is the first interface that went up
		Interface().SetMRU(interface.MRU());
		locker.UnlockNow();
		ThisLayerUp();
	} else if(Interface().MRU() > interface.MRU())
		Interface().SetMRU(interface.MRU());
			// MRU should always be the smallest value of all children
	
	NewState(PPP_OPENED_STATE);
}


void
PPPStateMachine::DownEvent(PPPInterface& interface)
{
#if DEBUG
	printf("PPPSM: DownEvent(interface) state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	uint32 MRU = 0;
		// the new MRU
	
	Interface().CalculateBaudRate();
	
	// when all children are down we should not be running
	if(Interface().IsMultilink() && !Interface().Parent()) {
		uint32 count = 0;
		PPPInterface *child;
		for(int32 index = 0; index < Interface().CountChildren(); index++) {
			child = Interface().ChildAt(index);
			
			if(child && child->IsUp()) {
				// set MRU to the smallest value of all children
				if(MRU == 0)
					MRU = child->MRU();
				else if(MRU > child->MRU())
					MRU = child->MRU();
				
				++count;
			}
		}
		
		if(MRU == 0)
			Interface().SetMRU(1500);
		else
			Interface().SetMRU(MRU);
		
		if(count == 0) {
			locker.UnlockNow();
			DownEvent();
		}
	}
}


void
PPPStateMachine::UpFailedEvent(PPPProtocol *protocol)
{
#if DEBUG
	printf("PPPSM: UpFailedEvent(protocol) state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if((protocol->Flags() & PPP_NOT_IMPORTANT) == 0) {
		if(Interface().Mode() == PPP_CLIENT_MODE) {
			// pretend we lost connection
			if(Interface().IsMultilink() && !Interface().Parent())
				for(int32 index = 0; index < Interface().CountChildren(); index++)
					Interface().ChildAt(index)->StateMachine().CloseEvent();
			else if(Interface().Device())
				Interface().Device()->Down();
			else
				CloseEvent();
					// just to be on the secure side ;)
		} else
			CloseEvent();
	}
}


void
PPPStateMachine::UpEvent(PPPProtocol *protocol)
{
#if DEBUG
	printf("PPPSM: UpEvent(protocol) state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(Phase() >= PPP_ESTABLISHMENT_PHASE)
		BringProtocolsUp();
}


void
PPPStateMachine::DownEvent(PPPProtocol *protocol)
{
#if DEBUG
	printf("PPPSM: DownEvent(protocol) state=%d phase=%d\n",
		State(), Phase());
#endif
}


// This is called by the device to tell us that it entered establishment
// phase. We can use Device::Down() to abort establishment until UpEvent()
// is called.
// The return value says if we are waiting for an UpEvent(). If false is
// returned the device should immediately abort its attempt to connect.
bool
PPPStateMachine::TLSNotify()
{
#if DEBUG
	printf("PPPSM: TLSNotify() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(State() == PPP_STARTING_STATE) {
			if(Phase() == PPP_DOWN_PHASE)
				NewPhase(PPP_ESTABLISHMENT_PHASE);
					// this says that the device is going up
			return true;
	}
	
	return false;
}


// This is called by the device to tell us that it entered termination phase.
// A Device::Up() should wait until the device went down.
// If false is returned we want to stay connected, though we called
// Device::Down().
bool
PPPStateMachine::TLFNotify()
{
#if DEBUG
	printf("PPPSM: TLFNotify() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	NewPhase(PPP_TERMINATION_PHASE);
		// tell DownEvent() that it may create a connection-lost-report
	
	return true;
}


void
PPPStateMachine::UpFailedEvent()
{
#if DEBUG
	printf("PPPSM: UpFailedEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_STARTING_STATE:
#if DEBUG
			printf("PPPSM::UpFailedEvent(): Reporting...\n");
#endif
			Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_DEVICE_UP_FAILED,
				NULL, 0);
			if(Interface().Parent())
				Interface().Parent()->StateMachine().UpFailedEvent(Interface());
			
			NewPhase(PPP_DOWN_PHASE);
				// tell DownEvent() that it should not create a connection-lost-report
#if DEBUG
			printf("PPPSM::UpFailedEvent(): Calling DownEvent()\n");
#endif
			DownEvent();
		break;
		
		default:
			IllegalEvent(PPP_UP_FAILED_EVENT);
	}
}


void
PPPStateMachine::UpEvent()
{
#if DEBUG
	printf("PPPSM: UpEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	// This call is public, thus, it might not only be called by the device.
	// We must recognize these attempts to fool us and handle them correctly.
	
	LockerHelper locker(fLock);
	
	if(!Interface().Device() || !Interface().Device()->IsUp())
		return;
			// it is not our device that went up...
	
	Interface().CalculateBaudRate();
	
	switch(State()) {
		case PPP_INITIAL_STATE:
			if(Interface().Mode() != PPP_SERVER_MODE
					|| Phase() != PPP_ESTABLISHMENT_PHASE) {
				// we are a client or we do not listen for an incoming
				// connection, so this is an illegal event
				IllegalEvent(PPP_UP_EVENT);
				NewState(PPP_CLOSED_STATE);
				locker.UnlockNow();
				ThisLayerFinished();
				
				return;
			}
			
			// TODO: handle server-up! (maybe already done correctly)
			NewState(PPP_REQ_SENT_STATE);
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_STARTING_STATE:
			// we must have called TLS() which sets establishment phase
			if(Phase() != PPP_ESTABLISHMENT_PHASE) {
				// there must be a BUG in the device add-on or someone is trying to
				// fool us (UpEvent() is public) as we did not request the device
				// to go up
				IllegalEvent(PPP_UP_EVENT);
				NewState(PPP_CLOSED_STATE);
				locker.UnlockNow();
				ThisLayerFinished();
				break;
			}
			
			NewState(PPP_REQ_SENT_STATE);
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		default:
			IllegalEvent(PPP_UP_EVENT);
	}
}


void
PPPStateMachine::DownEvent()
{
#if DEBUG
	printf("PPPSM: DownEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(Interface().Device() && Interface().Device()->IsUp())
		return;
			// it is not our device that went up...
	
	Interface().CalculateBaudRate();
	
	// reset IdleSince
	Interface().fIdleSince = 0;
	
	switch(State()) {
		// XXX: this does not belong to the standard, but may happen in our
		// implementation
		case PPP_STARTING_STATE:
		break;
		
		case PPP_CLOSED_STATE:
		case PPP_CLOSING_STATE:
			NewState(PPP_INITIAL_STATE);
		break;
		
		case PPP_STOPPED_STATE:
			// The RFC says we should reconnect, but our implementation
			// will only do this if auto-redial is enabled (only clients).
			NewState(PPP_STARTING_STATE);
		break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
		case PPP_OPENED_STATE:
			NewState(PPP_STARTING_STATE);
		break;
		
		default:
			IllegalEvent(PPP_DOWN_EVENT);
	}
	
	ppp_phase oldPhase = Phase();
	NewPhase(PPP_DOWN_PHASE);
	
	DownProtocols();
	
	fLocalAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	fPeerAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	
	// maybe we need to redial
	if(State() == PPP_STARTING_STATE) {
		bool needsRedial = false;
		
		// we do not try to redial if authentication failed
		if(fLocalAuthenticationStatus == PPP_AUTHENTICATION_FAILED
				|| fLocalAuthenticationStatus == PPP_AUTHENTICATING)
			Interface().Report(PPP_CONNECTION_REPORT,
				PPP_REPORT_LOCAL_AUTHENTICATION_FAILED, NULL, 0);
		else if(fPeerAuthenticationStatus == PPP_AUTHENTICATION_FAILED
				|| fPeerAuthenticationStatus == PPP_AUTHENTICATING)
			Interface().Report(PPP_CONNECTION_REPORT,
				PPP_REPORT_PEER_AUTHENTICATION_FAILED, NULL, 0);
		else {
			// if we are going up and lost connection the redial attempt becomes
			// a dial retry which is managed by the main thread in Interface::Up()
			if(Interface().fUpThread == -1)
				needsRedial = true;
			
			// test if UpFailedEvent() was not called
			if(oldPhase != PPP_DOWN_PHASE)
				Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_CONNECTION_LOST,
					NULL, 0);
		}
		
		if(Interface().Parent())
			Interface().Parent()->StateMachine().UpFailedEvent(Interface());
		
		NewState(PPP_INITIAL_STATE);
		
		if(Interface().DoesAutoRedial()) {
			if(needsRedial)
				Interface().Redial(Interface().RedialDelay());
		} else if(!Interface().DoesDialOnDemand())
			Interface().Delete();
	} else {
		Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_DOWN_SUCCESSFUL, NULL, 0);
		
		if(!Interface().DoesDialOnDemand())
			Interface().Delete();
	}
}


// private events
void
PPPStateMachine::OpenEvent()
{
#if DEBUG
	printf("PPPSM: OpenEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	// reset all handlers
	if(Phase() != PPP_ESTABLISHED_PHASE) {
		DownProtocols();
		ResetLCPHandlers();
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
			if(!Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_GOING_UP, NULL, 0))
				return;
			
			if(Interface().Mode() == PPP_SERVER_MODE) {
				NewPhase(PPP_ESTABLISHMENT_PHASE);
				
				if(Interface().Device() && !Interface().Device()->Up()) {
					Interface().Device()->UpFailedEvent();
					return;
				}
			} else
				NewState(PPP_STARTING_STATE);
			
			if(Interface().IsMultilink() && !Interface().Parent()) {
				NewPhase(PPP_ESTABLISHMENT_PHASE);
				for(int32 index = 0; index < Interface().CountChildren(); index++)
					if(Interface().ChildAt(index)->Mode() == Interface().Mode())
						Interface().ChildAt(index)->StateMachine().OpenEvent();
			} else {
				locker.UnlockNow();
				ThisLayerStarted();
			}
		break;
		
		case PPP_CLOSED_STATE:
			if(Phase() == PPP_DOWN_PHASE) {
				// the device is already going down
				return;
			}
			
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_STOPPING_STATE);
		break;
		
		default:
			;
	}
}


void
PPPStateMachine::CloseEvent()
{
#if DEBUG
	printf("PPPSM: CloseEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(Interface().IsMultilink() && !Interface().Parent()) {
		NewState(PPP_INITIAL_STATE);
		
		if(Phase() != PPP_DOWN_PHASE)
			NewPhase(PPP_TERMINATION_PHASE);
		
		ThisLayerDown();
		
		for(int32 index = 0; index < Interface().CountChildren(); index++)
			Interface().ChildAt(index)->StateMachine().CloseEvent();
		
		return;
	}
	
	switch(State()) {
		case PPP_OPENED_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_CLOSING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			InitializeRestartCount();
			locker.UnlockNow();
			if(State() == PPP_OPENED_STATE)
				ThisLayerDown();
			SendTerminateRequest();
		break;
		
		case PPP_STARTING_STATE:
			NewState(PPP_INITIAL_STATE);
			
			// TLSNotify() will know that we were faster because we
			// are in PPP_INITIAL_STATE now
			if(Phase() == PPP_ESTABLISHMENT_PHASE) {
				// the device is already up
				NewPhase(PPP_DOWN_PHASE);
					// this says the following DownEvent() was not caused by
					// a connection fault
				locker.UnlockNow();
				ThisLayerFinished();
			}
		break;
		
		case PPP_STOPPING_STATE:
			NewState(PPP_CLOSING_STATE);
		break;
		
		case PPP_STOPPED_STATE:
			NewState(PPP_STOPPED_STATE);
		break;
		
		default:
			;
	}
}


// timeout (restart counters are > 0)
void
PPPStateMachine::TOGoodEvent()
{
#if DEBUG
	printf("PPPSM: TOGoodEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_CLOSING_STATE:
		case PPP_STOPPING_STATE:
			locker.UnlockNow();
			SendTerminateRequest();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		default:
			IllegalEvent(PPP_TO_GOOD_EVENT);
	}
}


// timeout (restart counters are <= 0)
void
PPPStateMachine::TOBadEvent()
{
#if DEBUG
	printf("PPPSM: TOBadEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		default:
			IllegalEvent(PPP_TO_BAD_EVENT);
	}
}


// receive configure request (acceptable request)
void
PPPStateMachine::RCRGoodEvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RCRGoodEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCR_GOOD_EVENT);
			m_freem(packet);
		break;
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			SendTerminateAck();
			m_freem(packet);
		break;
		
		case PPP_STOPPED_STATE:
			// irc,scr,sca/8
			// XXX: should we do nothing and wait for DownEvent()?
			m_freem(packet);
		break;
		
		case PPP_REQ_SENT_STATE:
			NewState(PPP_ACK_SENT_STATE);
		
		case PPP_ACK_SENT_STATE:
			locker.UnlockNow();
			SendConfigureAck(packet);
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_OPENED_STATE);
			locker.UnlockNow();
			SendConfigureAck(packet);
			ThisLayerUp();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_ACK_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
			SendConfigureAck(packet);
		break;
		
		default:
			m_freem(packet);
	}
}


// receive configure request (unacceptable request)
void
PPPStateMachine::RCRBadEvent(struct mbuf *nak, struct mbuf *reject)
{
#if DEBUG
	printf("PPPSM: RCRBadEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCR_BAD_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			SendTerminateAck();
		break;
		
		case PPP_STOPPED_STATE:
			// irc,scr,scn/6
			// XXX: should we do nothing and wait for DownEvent()?
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		
		case PPP_ACK_SENT_STATE:
			if(State() == PPP_ACK_SENT_STATE)
				NewState(PPP_REQ_SENT_STATE);
					// OPENED_STATE might have set this already
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
			locker.UnlockNow();
			if(nak && ntohs(mtod(nak, ppp_lcp_packet*)->length) > 3)
				SendConfigureNak(nak);
			else if(reject && ntohs(mtod(reject, ppp_lcp_packet*)->length) > 3)
				SendConfigureNak(reject);
		return;
			// prevents the nak/reject from being m_freem()'d
		
		default:
			;
	}
	
	if(nak)
		m_freem(nak);
	if(reject)
		m_freem(reject);
}


// receive configure ack
void
PPPStateMachine::RCAEvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RCAEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		m_freem(packet);
		return;
	}
	
	// let the option handlers parse this ack
	PPPConfigurePacket ack(packet);
	PPPOptionHandler *optionHandler;
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		optionHandler = LCP().OptionHandlerAt(index);
		if(optionHandler->ParseAck(ack) != B_OK) {
			m_freem(packet);
			locker.UnlockNow();
			CloseEvent();
			return;
		}
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCA_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			SendTerminateAck();
		break;
		
		case PPP_REQ_SENT_STATE:
			NewState(PPP_ACK_RCVD_STATE);
			InitializeRestartCount();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_ACK_SENT_STATE:
			NewState(PPP_OPENED_STATE);
			InitializeRestartCount();
			locker.UnlockNow();
			ThisLayerUp();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
		
		default:
			;
	}
	
	m_freem(packet);
}


// receive configure nak/reject
void
PPPStateMachine::RCNEvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RCNEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		m_freem(packet);
		return;
	}
	
	// let the option handlers parse this nak/reject
	PPPConfigurePacket nak_reject(packet);
	PPPOptionHandler *optionHandler;
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		optionHandler = LCP().OptionHandlerAt(index);
		
		if(nak_reject.Code() == PPP_CONFIGURE_NAK) {
			if(optionHandler->ParseNak(nak_reject) != B_OK) {
				m_freem(packet);
				locker.UnlockNow();
				CloseEvent();
				return;
			}
		} else if(nak_reject.Code() == PPP_CONFIGURE_REJECT) {
			if(optionHandler->ParseReject(nak_reject) != B_OK) {
				m_freem(packet);
				locker.UnlockNow();
				CloseEvent();
				return;
			}
		}
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCN_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			SendTerminateAck();
		break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			InitializeRestartCount();
		
		case PPP_ACK_RCVD_STATE:
			if(State() == PPP_ACK_RCVD_STATE)
				NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
		
		default:
			;
	}
	
	m_freem(packet);
}


// receive terminate request
void
PPPStateMachine::RTREvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RTREvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	// we should not use the same ID as the peer
	if(fID == mtod(packet, ppp_lcp_packet*)->id)
		fID -= 128;
	
	fLocalAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	fPeerAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RTR_EVENT);
			m_freem(packet);
		break;
		
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			locker.UnlockNow();
			SendTerminateAck(packet);
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			ZeroRestartCount();
			locker.UnlockNow();
			ThisLayerDown();
			SendTerminateAck(packet);
		break;
		
		default:
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			locker.UnlockNow();
			SendTerminateAck(packet);
	}
}


// receive terminate ack
void
PPPStateMachine::RTAEvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RTAEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	if(fTerminateID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		m_freem(packet);
		return;
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RTA_EVENT);
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_STOPPING_STATE:
			NewState(PPP_STOPPED_STATE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
		
		default:
			;
	}
	
	m_freem(packet);
}


// receive unknown code
void
PPPStateMachine::RUCEvent(struct mbuf *packet, uint16 protocolNumber,
	uint8 code = PPP_PROTOCOL_REJECT)
{
#if DEBUG
	printf("PPPSM: RUCEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RUC_EVENT);
			m_freem(packet);
		break;
		
		default:
			locker.UnlockNow();
			SendCodeReject(packet, protocolNumber, code);
	}
}


// receive code/protocol reject (acceptable such as IPX reject)
void
PPPStateMachine::RXJGoodEvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RXJGoodEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	// This method does not m_freem(packet) because the acceptable rejects are
	// also passed to the parent. RXJEvent() will m_freem(packet) when needed.
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXJ_GOOD_EVENT);
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		break;
		
		default:
			;
	}
}


// receive code/protocol reject (catastrophic such as LCP reject)
void
PPPStateMachine::RXJBadEvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RXJBadEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXJ_BAD_EVENT);
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
		
		case PPP_STOPPING_STATE:
			NewPhase(PPP_TERMINATION_PHASE);
		
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			InitializeRestartCount();
			locker.UnlockNow();
			ThisLayerDown();
			SendTerminateRequest();
		break;
	}
	
	m_freem(packet);
}


// receive echo request/reply, discard request
void
PPPStateMachine::RXREvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RXREvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	ppp_lcp_packet *echo = mtod(packet, ppp_lcp_packet*);
	
	if(echo->code == PPP_ECHO_REPLY && echo->id != fEchoID) {
		// TODO:
		// log that we got a reply, but no request was sent
	}
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXR_EVENT);
		break;
		
		case PPP_OPENED_STATE:
			if(echo->code == PPP_ECHO_REQUEST)
				SendEchoReply(packet);
		return;
			// this prevents the packet from being freed
		
		default:
			;
	}
	
	m_freem(packet);
}


// general events (for Good/Bad events)
void
PPPStateMachine::TimerEvent()
{
#if DEBUG
	if(fNextTimeout != 0)
		printf("PPPSM: TimerEvent()\n");
#endif
	
	LockerHelper locker(fLock);
	if(fNextTimeout == 0 || fNextTimeout > system_time())
		return;
	fNextTimeout = 0;
	locker.UnlockNow();
	
	switch(State()) {
		case PPP_CLOSING_STATE:
		case PPP_STOPPING_STATE:
			if(fTerminateCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
		break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			if(fRequestCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
		break;
		
		default:
			;
	}
}


// ReceiveConfigureRequest
// Here we get a configure-request packet from LCP and aks all OptionHandlers
// if its values are acceptable. From here we call our Good/Bad counterparts.
void
PPPStateMachine::RCREvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RCREvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	PPPConfigurePacket request(packet);
	PPPConfigurePacket nak(PPP_CONFIGURE_NAK);
	PPPConfigurePacket reject(PPP_CONFIGURE_REJECT);
	
	// we should not use the same id as the peer
	if(fID == mtod(packet, ppp_lcp_packet*)->id)
		fID -= 128;
	
	nak.SetID(request.ID());
	reject.SetID(request.ID());
	
	// each handler should add unacceptable values for each item
	status_t result;
		// the return value of ParseRequest()
	PPPOptionHandler *optionHandler;
	for(int32 index = 0; index < request.CountItems(); index++) {
		optionHandler = LCP().OptionHandlerFor(request.ItemAt(index)->type);
		
		if(!optionHandler || !optionHandler->IsEnabled()) {
			printf("PPPSM::RCREvent(): unknown type:%d\n", request.ItemAt(index)->type);
			// unhandled items should be added to the reject
			reject.AddItem(request.ItemAt(index));
			continue;
		}
		
#if DEBUG
		printf("PPPSM::RCREvent(): OH=%s\n", optionHandler->Name());
#endif
		result = optionHandler->ParseRequest(request, index, nak, reject);
		
		if(result == PPP_UNHANDLED) {
			// unhandled items should be added to the reject
			reject.AddItem(request.ItemAt(index));
			continue;
		} else if(result != B_OK) {
			// the request contains a value that has been sent more than
			// once or the value is corrupted
			printf("PPPSM::RCREvent(): OptionHandler returned parse error!\n");
			m_freem(packet);
			CloseEvent();
			return;
		}
	}
	
	// Additional values may be appended.
	// If we sent too many naks we should not append additional values.
	if(fNakCounter > 0) {
		for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
			optionHandler = LCP().OptionHandlerAt(index);
			if(optionHandler && optionHandler->IsEnabled()) {
				result = optionHandler->ParseRequest(request, request.CountItems(),
					nak, reject);
				
				if(result != B_OK) {
					// the request contains a value that has been sent more than
					// once or the value is corrupted
					printf("PPPSM::RCREvent(): OptionHandler returned append error!\n");
					m_freem(packet);
					CloseEvent();
					return;
				}
			}
		}
	}
	
	if(nak.CountItems() > 0) {
		RCRBadEvent(nak.ToMbuf(Interface().MRU(), LCP().AdditionalOverhead()), NULL);
		m_freem(packet);
	} else if(reject.CountItems() > 0) {
		RCRBadEvent(NULL, reject.ToMbuf(Interface().MRU(), LCP().AdditionalOverhead()));
		m_freem(packet);
	} else
		RCRGoodEvent(packet);
}


// ReceiveCodeReject
// LCP received a code/protocol-reject packet and we look if it is acceptable.
// From here we call our Good/Bad counterparts.
void
PPPStateMachine::RXJEvent(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: RXJEvent() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	ppp_lcp_packet *reject = mtod(packet, ppp_lcp_packet*);
	
	if(reject->code == PPP_CODE_REJECT) {
		uint8 rejectedCode = reject->data[0];
		
		// test if the rejected code belongs to the minimum LCP requirements
		if(rejectedCode >= PPP_MIN_LCP_CODE && rejectedCode <= PPP_MAX_LCP_CODE) {
			if(Interface().IsMultilink() && !Interface().Parent()) {
				// Main interfaces do not have states between STARTING and OPENED.
				// An RXJBadEvent() would enter one of those states which is bad.
				m_freem(packet);
				CloseEvent();
			} else
				RXJBadEvent(packet);
			
			return;
		}
		
		// find the LCP extension and disable it
		PPPLCPExtension *lcpExtension;
		for(int32 index = 0; index < LCP().CountLCPExtensions(); index++) {
			lcpExtension = LCP().LCPExtensionAt(index);
			
			if(lcpExtension->Code() == rejectedCode)
				lcpExtension->SetEnabled(false);
		}
		
		m_freem(packet);
	} else if(reject->code == PPP_PROTOCOL_REJECT) {
		// disable all handlers for rejected protocol type
		uint16 rejected = *((uint16*) reject->data);
			// rejected protocol number
		
		if(rejected == PPP_LCP_PROTOCOL) {
			// LCP must not be rejected!
			RXJBadEvent(packet);
			return;
		}
		
		// disable protocols with the rejected protocol number
		PPPProtocol *protocol = Interface().FirstProtocol();
		for(; protocol; protocol = protocol->NextProtocol()) {
			if(protocol->ProtocolNumber() == rejected)
				protocol->SetEnabled(false);
					// disable protocol
		}
		
		RXJGoodEvent(packet);
			// this event handler does not m_freem(packet)!!!
		
		// notify parent, too
		if(Interface().Parent())
			Interface().Parent()->StateMachine().RXJEvent(packet);
		else
			m_freem(packet);
	}
}


// actions (all private)
void
PPPStateMachine::IllegalEvent(ppp_event event)
{
	// TODO:
	// update error statistics
	printf("PPPSM: IllegalEvent(event=%d) state=%d phase=%d\n",
		event, State(), Phase());
}


void
PPPStateMachine::ThisLayerUp()
{
#if DEBUG
	printf("PPPSM: ThisLayerUp() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	
	// We begin with authentication phase and wait until each phase is done.
	// We stop when we reach established phase.
	
	// Do not forget to check if we are going down.
	if(Phase() != PPP_ESTABLISHMENT_PHASE)
		return;
	
	NewPhase(PPP_AUTHENTICATION_PHASE);
	
	locker.UnlockNow();
	
	BringProtocolsUp();
}


void
PPPStateMachine::ThisLayerDown()
{
#if DEBUG
	printf("PPPSM: ThisLayerDown() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	// PPPProtocol::Down() should block if needed.
	DownProtocols();
}


void
PPPStateMachine::ThisLayerStarted()
{
#if DEBUG
	printf("PPPSM: ThisLayerStarted() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if(Interface().Device() && !Interface().Device()->Up())
		Interface().Device()->UpFailedEvent();
}


void
PPPStateMachine::ThisLayerFinished()
{
#if DEBUG
	printf("PPPSM: ThisLayerFinished() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if(Interface().Device())
		Interface().Device()->Down();
}


void
PPPStateMachine::InitializeRestartCount()
{
	fRequestCounter = fMaxRequest;
	fTerminateCounter = fMaxTerminate;
	fNakCounter = fMaxNak;
}


void
PPPStateMachine::ZeroRestartCount()
{
	fRequestCounter = 0;
	fTerminateCounter = 0;
	fNakCounter = 0;
}


bool
PPPStateMachine::SendConfigureRequest()
{
#if DEBUG
	printf("PPPSM: SendConfigureRequest() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	--fRequestCounter;
	fNextTimeout = system_time() + PPP_STATE_MACHINE_TIMEOUT;
	locker.UnlockNow();
	
	PPPConfigurePacket request(PPP_CONFIGURE_REQUEST);
	request.SetID(NextID());
	fRequestID = request.ID();
	
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		// add all items
		if(LCP().OptionHandlerAt(index)->AddToRequest(request) != B_OK) {
			CloseEvent();
			return false;
		}
	}
	
	return LCP().Send(request.ToMbuf(Interface().MRU(),
		LCP().AdditionalOverhead())) == B_OK;
}


bool
PPPStateMachine::SendConfigureAck(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: SendConfigureAck() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if(!packet)
		return false;
	
	mtod(packet, ppp_lcp_packet*)->code = PPP_CONFIGURE_ACK;
	PPPConfigurePacket ack(packet);
	
	// notify all option handlers that we are sending an ack for each value
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		if(LCP().OptionHandlerAt(index)->SendingAck(ack) != B_OK) {
			m_freem(packet);
			CloseEvent();
			return false;
		}
	}
	
	return LCP().Send(packet) == B_OK;
}


bool
PPPStateMachine::SendConfigureNak(struct mbuf *packet)
{
#if DEBUG
	printf("PPPSM: SendConfigureNak() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if(!packet)
		return false;
	
	ppp_lcp_packet *nak = mtod(packet, ppp_lcp_packet*);
	if(nak->code == PPP_CONFIGURE_NAK) {
		if(fNakCounter == 0) {
			// We sent enough naks. Let's try a reject.
			nak->code = PPP_CONFIGURE_REJECT;
		} else
			--fNakCounter;
	}
	
	return LCP().Send(packet) == B_OK;
}


bool
PPPStateMachine::SendTerminateRequest()
{
#if DEBUG
	printf("PPPSM: SendTerminateRequest() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	LockerHelper locker(fLock);
	--fTerminateCounter;
	fNextTimeout = system_time() + PPP_STATE_MACHINE_TIMEOUT;
	locker.UnlockNow();
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if(!packet)
		return false;
	
	packet->m_pkthdr.len = packet->m_len = 4;
	
	// reserve some space for other protocols
	packet->m_data += LCP().AdditionalOverhead();
	
	ppp_lcp_packet *request = mtod(packet, ppp_lcp_packet*);
	request->code = PPP_TERMINATE_REQUEST;
	request->id = fTerminateID = NextID();
	request->length = htons(4);
	
	return LCP().Send(packet) == B_OK;
}


bool
PPPStateMachine::SendTerminateAck(struct mbuf *request = NULL)
{
#if DEBUG
	printf("PPPSM: SendTerminateAck() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	struct mbuf *reply = request;
	
	ppp_lcp_packet *ack;
	
	if(!reply) {
		reply = m_gethdr(MT_DATA);
		if(!reply)
			return false;
		
		reply->m_data += LCP().AdditionalOverhead();
		reply->m_pkthdr.len = reply->m_len = 4;
		
		ack = mtod(reply, ppp_lcp_packet*);
		ack->id = NextID();
	} else
		ack = mtod(reply, ppp_lcp_packet*);
	
	ack->code = PPP_TERMINATE_ACK;
	ack->length = htons(4);
	
	return LCP().Send(reply) == B_OK;
}


bool
PPPStateMachine::SendCodeReject(struct mbuf *packet, uint16 protocolNumber, uint8 code)
{
#if DEBUG
	printf("PPPSM: SendCodeReject(protocolNumber=%d;code=%d) state=%d phase=%d\n",
		protocolNumber, code, State(), Phase());
#endif
	
	if(!packet)
		return false;
	
	int32 length;
		// additional space needed for this reject
	if(code == PPP_PROTOCOL_REJECT)
		length = 6;
	else
		length = 4;
	
	M_PREPEND(packet, length);
		// add some space for the header
	
	// adjust packet if too big
	int32 adjust = Interface().MRU();
	if(packet->m_flags & M_PKTHDR) {
		adjust -= packet->m_pkthdr.len;
	} else
		adjust -= packet->m_len;
	
	if(adjust < 0)
		m_adj(packet, adjust);
	
	ppp_lcp_packet *reject = mtod(packet, ppp_lcp_packet*);
	reject->code = code;
	reject->id = NextID();
	if(packet->m_flags & M_PKTHDR)
		reject->length = htons(packet->m_pkthdr.len);
	else
		reject->length = htons(packet->m_len);
	
	protocolNumber = htons(protocolNumber);
	if(code == PPP_PROTOCOL_REJECT)
		memcpy(&reject->data, &protocolNumber, sizeof(protocolNumber));
	
	return LCP().Send(packet) == B_OK;
}


bool
PPPStateMachine::SendEchoReply(struct mbuf *request)
{
#if DEBUG
	printf("PPPSM: SendEchoReply() state=%d phase=%d\n",
		State(), Phase());
#endif
	
	if(!request)
		return false;
	
	ppp_lcp_packet *reply = mtod(request, ppp_lcp_packet*);
	reply->code = PPP_ECHO_REPLY;
		// the request becomes a reply
	
	if(request->m_flags & M_PKTHDR)
		request->m_pkthdr.len = 8;
	request->m_len = 8;
	
	memcpy(reply->data, &fMagicNumber, sizeof(fMagicNumber));
	
	return LCP().Send(request) == B_OK;
}


// methods for bringing protocols up
void
PPPStateMachine::BringProtocolsUp()
{
	// use a simple check for phase changes (e.g., caused by CloseEvent())
	while(Phase() <= PPP_ESTABLISHED_PHASE && Phase() >= PPP_AUTHENTICATION_PHASE) {
		if(BringPhaseUp() > 0)
			break;
		
		LockerHelper locker(fLock);
		
		if(Phase() < PPP_AUTHENTICATION_PHASE)
			return;
				// phase was changed by another event
		else if(Phase() == PPP_ESTABLISHED_PHASE) {
			if(Interface().Parent())
				Interface().Parent()->StateMachine().UpEvent(Interface());
		} else
			NewPhase((ppp_phase) (Phase() + 1));
	}
}


// this returns the number of handlers waiting to go up
uint32
PPPStateMachine::BringPhaseUp()
{
	// Servers do not need to bring all protocols up.
	// The client specifies which protocols he wants to go up.
	
	LockerHelper locker(fLock);
	
	// check for phase change
	if(Phase() < PPP_AUTHENTICATION_PHASE)
		return 0;
	
	uint32 count = 0;
	PPPProtocol *protocol = Interface().FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol()) {
		if(protocol->IsEnabled() && protocol->ActivationPhase() == Phase()) {
			if(protocol->IsGoingUp() && Interface().Mode() == PPP_CLIENT_MODE)
				++count;
			else if(protocol->IsDown() && protocol->IsUpRequested()) {
				if(Interface().Mode() == PPP_CLIENT_MODE)
					++count;
				
				protocol->Up();
			}
		}
	}
	
	// We only wait until authentication is complete.
	if(Interface().Mode() == PPP_SERVER_MODE
			&& (LocalAuthenticationStatus() == PPP_AUTHENTICATING
			|| PeerAuthenticationStatus() == PPP_AUTHENTICATING))
		++count;
	
	return count;
}


void
PPPStateMachine::DownProtocols()
{
	PPPProtocol *protocol = Interface().FirstProtocol();
	
	for(; protocol; protocol = protocol->NextProtocol())
		if(protocol->IsEnabled())
			protocol->Down();
}


void
PPPStateMachine::ResetLCPHandlers()
{
	for(int32 index = 0; index < LCP().CountOptionHandlers(); index++)
		LCP().OptionHandlerAt(index)->Reset();
	
	for(int32 index = 0; index < LCP().CountLCPExtensions(); index++)
		LCP().LCPExtensionAt(index)->Reset();
}
