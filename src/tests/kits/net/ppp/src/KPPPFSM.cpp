#include "KPPPFSM.h"


PPPFSM::PPPFSM(PPPInterface& interface)
	: fInterface(&interface), fPhase(PPP_CTOR_DTOR_PHASE),
	fState(PPP_INITIAL_STATE), fID(system_time() & 0xFF),
	fAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fPeerAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fAuthenticatorIndex(-1), fPeerAuthenticatorIndex(-1)
{
	
}


PPPFSM::~PPPFSM()
{
	
}


uint8
PPPFSM::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


// remember: NewState() must always be called _after_ IllegalEvent()
// because IllegalEvent() also looks at the current state.
void
PPPFSM::NewState(PPP_STATE next)
{
	fState = next;
	
	// TODO:
	// ? if next == OPENED_STATE we may want to reset all option handlers ?
}


void
PPPFSM::LeaveConstructionPhase()
{
	LockerHelper locker(fLock);
	fPhase = PPP_DOWN_PHASE;
}


PPPFSM::EnterDestructionPhase()
{
	LockerHelper locker(fLock);
	fPhase = PPP_CTOR_DTOR_PHASE;
}


// authentication events
void
PPPFSM::AuthenticationRequested()
{
	// IMPLEMENT ME!
}


void
PPPFSM::AuthenticationAccepted(const char *name)
{
	// IMPLEMENT ME!
}


void
PPPFSM::AuthenticationDenied(const char *name)
{
	// IMPLEMENT ME!
}


const char*
PPPFSM::AuthenticationName() const
{
	// IMPLEMENT ME!
}


void
PPPFSM::PeerAuthenticationRequested()
{
	// IMPLEMENT ME!
}


void
PPPFSM::PeerAuthenticationAccepted(const char *name)
{
	// IMPLEMENT ME!
}


void
PPPFSM::PeerAuthenticationDenied(const char *name)
{
	// IMPLEMENT ME!
}


const char*
PPPFSM::PeerAuthenticationName() const
{
	// IMPLEMENT ME!
}


// This is called by the device to tell us that it entered establishment
// phase. We can use Device::Down() to abort establishment until UpEvent()
// is called.
// The return value says if we are waiting for an UpEvent(). If false is
// returned the device should immediately abort its attempt to connect.
bool
PPPFSM::TLSNotify()
{
	LockerHelper locker(fLock);
	
	if(State() == PPP_STARTING_STATE) {
			if(Phase() == PPP_DOWN_PHASE)
				fPhase = PPP_ESTABLISHMENT_PHASE;
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
PPPFSM::TLFNotify()
{
	LockerHelper locker(fLock);
	
	if(Phase() == PPP_ESTABLISHMENT_PHASE) {
		// we may not go down because an OpenEvent indicates that the
		// user wants to connect
		return false;
	}
	
	// from now on no packets may be sent to the device
	fPhase = PPP_DOWN_PHASE;
	
	return true;
}


void
PPPFSM::UpFailedEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_STARTING_STATE:
			// we must have called TLS() which sets establishment phase
			if(Phase() != PPP_ESTABLISHMENT_PHASE) {
				// there must be a BUG in the device add-on or someone is trying to
				// fool us (UpEvent() is public) as we did not request the device
				// to go up
				IllegalEvent(PPP_UP_FAILED_EVENT);
				NewState(PPP_INITIAL_STATE);
				break;
			}
			
			// TODO:
			// report that up failed
			// Report(PPP_CONNECTION_REPORT, PPP_UP_FAILED, NULL, 0);
		break;
		
		default:
			IllegalEvent(PPP_UP_FAILED_EVENT);
	}
}


void
PPPFSM::UpEvent()
{
	// This call is public, thus, it might not only be called by the device.
	// We must recognize these attempts to fool us and handle them correctly.
	
	LockerHelper locker(fLock);
	
	if(!Device()->IsUp())
		return;
			// it is not our device that went up...
	
	switch(State()) {
		case PPP_INITIAL_STATE:
			if(Mode() != PPP_SERVER_MODE
				|| Phase() != PPP_ESTABLISHMENT_PHASE) {
				// we are a client or we do not listen for an incoming
				// connection, so this is an illegal event
				IllegalEvent(PPP_UP_EVENT);
				NewState(PPP_CLOSED_STATE);
				locker.UnlockNow();
				ThisLayerFinished();
			}
			
			// TODO: handle server-up!
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
			InitializeRestartCounter();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		default:
			IllegalEvent(PPP_UP_EVENT);
	}
}


void
PPPFSM::DownEvent()
{
	LockerHelper locker(fLock);
	
	// TODO:
	// ResetProtocols();
	// ResetEncapsulators();
	// ResetOptionHandlers();
	
	switch(State()) {
		case PPP_CLOSED_STATE:
		case PPP_CLOSING_STATE:
			NewState(PPP_INITIAL_STATE);
			fPhase = PPP_DOWN_PHASE;
		break;
		
		case PPP_STOPPED_STATE:
			// The RFC says we should reconnect, but our implementation
			// will only do this if auto-redial is enabled (only clients).
			NewStatus(PPP_STARTING_STATE);
			fPhase = PPP_DOWN_PHASE;
			
			// TODO:
			// report that we lost the connection
			// if fAuthStatus == FAILED || fPeerAuthStatus == FAILED
			//  Report(PPP_CONNECTION_REPORT, PPP_AUTH_FAILED, NULL, 0);
			// else
			//  Report(PPP_CONNECTION_REPORT, PPP_CONNECTION_LOST, NULL, 0);
		break;
		
		case PPP_STOPPING_STATE:
			NewStatus(PPP_STARTING_STATE);
			fPhase = PPP_DOWN_PHASE;
			
			// TODO:
			// report that we lost the connection
			// Report(PPP_CONNECTION_REPORT, PPP_CONNECTION_LOST, NULL, 0);
		break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewStatus(PPP_STARTING_STATE);
			fPhase = PPP_DOWN_PHASE;
			
			// TODO:
			// report that we lost the connection
			// Report(PPP_CONNECTION_REPORT, PPP_CONNECTION_LOST, NULL, 0);
		break;
		
		case PPP_OPENED_STATE:
			// tld/1
			NewStatus(PPP_STARTING_STATE);
			fPhase = PPP_DOWN_PHASE;
			
			// TODO:
			// report that we lost the connection
			// Report(PPP_CONNECTION_REPORT, PPP_CONNECTION_LOST, NULL, 0);
		break;
		
		default:
			IllegalEvent(PPP_DOWN_EVENT);
	}
	
	// maybe we need to redial
	if(State() == PPP_STARTING_STATE) {
		if(Interface()->DoesAutoRedial()) {
			// TODO:
			// redial using a new thread (maybe interface manager will help us)
		}
	}
}


// private events
void
PPPFSM::OpenEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
			NewState(PPP_STARTING_STATE);
			locker.UnlockNow();
			ThisLayerStarted();
		break;
		
		case PPP_CLOSED_STATE:
			if(Phase() == PPP_DOWN_PHASE) {
				// the device is already going down
				return;
			}
			
			NewState(PPP_REQ_SENT_STATE);
			fPhase = PPP_ESTABLISHMENT_PHASE;
			InitializeRestartCounter();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_STOPPING_STATE);
	}
}


void
PPPFSM::CloseEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_OPENED_STATE:
			ThisLayerDown();
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_CLOSING_STATE);
			fPhase = PPP_TERMINATION_PHASE;
			InitializeRestartCounter();
			locker.UnlockNow();
			SendTerminateRequest();
		break;
		
		case PPP_STARTING_STATE:
			NewState(PPP_INITIAL_STATE);
			
			// if Device()->TLS() has not been called
			// TLSNotify() will know that we were faster because we
			// are in PPP_INITIAL_STATE now
			if(Phase() == PPP_ESTABLISHMENT_PHASE) {
				// the device is already up
				fPhase = PPP_DOWN_PHASE;
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
	}
}


// timeout (restart counters are > 0)
void
PPPFSM::TOGoodEvent()
{
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
PPPFSM::TOBadEvent()
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
			fPhase = PPP_TERMINATION_PHASE;
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
			fPhase = PPP_TERMINATION_PHASE;
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		default:
			IllegalEvent(PPP_TO_BAD_EVENT);
	}
}


// receive configure request (acceptable request)
void
PPPFSM::RCRGoodEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCR_GOOD_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			SendTerminateAck();
		break;
		
		case PPP_STOPPED_STATE:
			// irc,scr,sca/8
			// XXX: should we do nothing and wait for DownEvent()?
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
			// tld,scr,sca/8
			NewState(PPP_ACK_SENT_STATE);
			locker.UnlockNow();
			SendConfigureRequest();
			SendConfigureAck(packet);
			ThisLayerDown();
		break;
	}
}


// receive configure request (unacceptable request)
void
PPPFSM::RCRBadEvent(mbuf *nak, mbuf *reject)
{
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
			if( !nak && ((lcp_packet*)nak)->length > 3)
				SendConfigureNak(nak);
			else if( !reject && ((lcp_packet*)reject)->length > 3)
				SendConfigureNak(reject);
		break;
	}
}


// receive configure ack
void
PPPFSM::RCAEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
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
			InitializeRestartCounter();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_ACK_SENT_STATE:
			NewState(PPP_OPENED_STATE);
			InitializeRestartCounter();
			locker.UnlockNow();
			ThisLayerUp();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
	}
}


// receive configure nak/reject
void
PPPFSM::RCNEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCN_EVENT);
		break;
		
		case PPP_CLOSED_STATE:
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			SendTermintateAck(packet);
		break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			InitializeRestartCounter();
		
		case PPP_ACK_RCVD_STATE:
			if(State() == PPP_ACK_RCVD_STATE)
				NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
	}
}


// receive terminate request
void
PPPFSM::RTREvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RTR_EVENT);
		break;
		
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			SendTerminateAck(packet);
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			ZeroRestartCount();
			locker.UnlockNow();
			ThisLayerDown();
			SendTerminateAck(packet);
		break;
		
		default:
			locker.UnlockNow();
			SendTerminateAck(packet);
	}
}


// receive terminate ack
void
PPPFSM::RTAEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
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
		
		case PPP_CLOSING_STATE:
			NewState(PPP_STOPPED_STATE);
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(REQ_SENT_STATE);
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
	}
}


// receive unknown code
void
PPPFSM::RUCEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RUC_EVENT);
		break;
		
		default:
			locker.UnlockNow();
			SendCodeReject(packet);
	}
}


// receive code/protocol reject (acceptable such as IPX reject)
void
PPPFSM::RXJGoodEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXJ_GOOD_EVENT);
		break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		break;
	}
}


// receive code/protocol reject (catastrophic such as LCP reject)
void
PPPFSM::RXJBadEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RJX_BAD_EVENT);
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
		
		case PPP_CLOSED_STATE:
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
		
		case PPP_STOPPED_STATE:
			locker.UnlockNow();
			ThisLayerFinished();
		break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			InitializeRestartCounter();
			locker.UnlockNow();
			ThisLayerDown();
			SendTerminateRequest();
		break;
	}
}


// receive echo request/reply, discard request
void
PPPFSM::RXREvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXR_EVENT);
		break;
		
		case PPP_OPENED_STATE:
			SendEchoReply(packet);
		break;
	}
}


// general events (for Good/Bad events)
void
PPPFSM::TimerEvent()
{
	// TODO:
	// 
}


// ReceiveConfigureRequest
// Here we get a configure-request packet from LCP and aks all OptionHandlers
// if its values are acceptable. From here we call our Good/Bad counterparts.
void
PPPFSM::RCREvent(mbuf *packet)
{
	PPPConfigurePacket request(packet), nak(PPP_CONFIGURE_NAK),
		reject(PPP_CONFIGURE_REJECT);
	PPPOptionHandler *handler;
	
	// each handler should add unacceptable values for each item
	for(int32 item = 0; item < request.CountItems(); item++) {
		for(int32 index = 0; index < Interface()->CountOptionHandlers();
			index++) {
			handler = Interface()->OptionHandlerAt(i);
			if(!handler->ParseConfigureRequest(&request, item, &nak, &reject)) {
				// the request contains a value that has been sent more than
				// once or the value is corrupted
				CloseEvent();
			}
		}
	}
	
	if(nak.CountItems() > 0)
		RCRBadEvent(nak.ToMbuf(), NULL);
	else if(reject.CountItmes() > 0)
		RCRBadEvent(NULL, reject.ToMbuf());
	else
		RCRGoodEvent(packet);
}


// ReceiveCodeReject
// LCP received a code-reject packet and we look if it is acceptable.
// From here we call our Good/Bad counterparts.
void
PPPFSM::RXJEvent(mbuf *packet)
{
	lcp_packet *reject = (lcp_packet*) packet;
	
	if(reject->code == PPP_CODE_REJECT)
		RXJBadEvent(packet);
	else if(reject->code == PPP_PROTOCOL_REJECT) {
		// disable all handlers for rejected protocol type
		uint16 rejected = *((uint16*) reject->data);
			// rejected protocol number
		
		if(rejected == PPP_LCP_PROTOCOL) {
			// LCP must not be rejected!
			RXJBadEvent(packet);
			return;
		}
		
		int32 index;
		PPPProtocol *protocol_handler;
		PPPEncapsulator *encapsulator_handler = fFirstEncapsulator;
		
		for(index = 0; index < Interface()->CountProtocols(); index++) {
			protocol_handler = Interface()->ProtocolAt(index);
			if(protocol_handler && protocol_handler->Protocol() == rejected)
				protocol_handler->SetEnabled(false);
					// disable protocol
		}
		
		for(; encapsulator_handler;
			encapsulator_handler = encapsulator_handler->Next()) {
			if(encapsulator_handler->Protocol() == rejected)
				encapsulator_handler->SetEnabled(false);
					// disable encapsulator
		}
		
		RXJGoodEvent(packet);
	}
}


// actions (all private)
void
PPPFSM::IllegalEvent(PPP_EVENT event)
{
	// TODO:
	// update error statistics
}


void
PPPFSM::ThisLayerUp()
{
}


void
PPPFSM::ThisLayerDown()
{
			// DownProtocols();
			// DownEncapsulators();
			// ResetProtocols();
			// ResetEncapsulators();
}


void
PPPFSM::ThisLayerStarted()
{
}


void
PPPFSM::ThisLayerFinished()
{
}


void
PPPFSM::InitializeRestartCount()
{
}


void
PPPFSM::ZeroRestartCount()
{
}


void
PPPFSM::SendConfigureRequest()
{
}


void
PPPFSM::SendConfigureAck(mbuf *packet)
{
}


void
PPPFSM::SendConfigureNak(mbuf *packet)
{
}


void
PPPFSM::SendTerminateRequest()
{
}


void
PPPFSM::SendTerminateAck(mbuf *request)
{
}


void
PPPFSM::SendCodeReject(mbuf *packet)
{
}


void
PPPFSM::SendEchoReply(mbuf *request)
{
}
