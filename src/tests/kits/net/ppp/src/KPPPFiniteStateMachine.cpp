#include "KPPPFiniteStateMachine.h"


// TODO:
// - add support for multilink interfaces
//   (LCP packets may be received over MP encapsulators
//   so we should return the reply using the same encapsulator)
// - add Up(Failed)/DownEvent for child interfaces + protocols + encapsulators

PPPFiniteStateMachine::PPPFiniteStateMachine(PPPInterface& interface)
	: fInterface(&interface), fPhase(PPP_DOWN_PHASE),
	fState(PPP_INITIAL_STATE), fID(system_time() & 0xFF),
	fAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fPeerAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fAuthenticatorIndex(-1), fPeerAuthenticatorIndex(-1),
	fMaxTerminate(2), fMaxConfigure(10), fMaxNak(5),
	fTimeout(0)
{
}


PPPFiniteStateMachine::~PPPFiniteStateMachine()
{
}


uint8
PPPFiniteStateMachine::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


// remember: NewState() must always be called _after_ IllegalEvent()
// because IllegalEvent() also looks at the current state.
void
PPPFiniteStateMachine::NewState(PPP_STATE next)
{
//	if(State() == PPP_OPENED_STATE)
//		ResetOptionHandlers();
	
	fState = next;
}


// authentication events
void
PPPFiniteStateMachine::AuthenticationRequested()
{
	// IMPLEMENT ME!
}


void
PPPFiniteStateMachine::AuthenticationAccepted(const char *name)
{
	// IMPLEMENT ME!
}


void
PPPFiniteStateMachine::AuthenticationDenied(const char *name)
{
	// IMPLEMENT ME!
}


const char*
PPPFiniteStateMachine::AuthenticationName() const
{
	// IMPLEMENT ME!
}


void
PPPFiniteStateMachine::PeerAuthenticationRequested()
{
	// IMPLEMENT ME!
}


void
PPPFiniteStateMachine::PeerAuthenticationAccepted(const char *name)
{
	// IMPLEMENT ME!
}


void
PPPFiniteStateMachine::PeerAuthenticationDenied(const char *name)
{
	// IMPLEMENT ME!
}


const char*
PPPFiniteStateMachine::PeerAuthenticationName() const
{
	// IMPLEMENT ME!
}


// This is called by the device to tell us that it entered establishment
// phase. We can use Device::Down() to abort establishment until UpEvent()
// is called.
// The return value says if we are waiting for an UpEvent(). If false is
// returned the device should immediately abort its attempt to connect.
bool
PPPFiniteStateMachine::TLSNotify()
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
PPPFiniteStateMachine::TLFNotify()
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
PPPFiniteStateMachine::UpFailedEvent()
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
PPPFiniteStateMachine::UpEvent()
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
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		default:
			IllegalEvent(PPP_UP_EVENT);
	}
}


void
PPPFiniteStateMachine::DownEvent()
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
PPPFiniteStateMachine::OpenEvent()
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
			InitializeRestartCount();
			locker.UnlockNow();
			SendConfigureRequest();
		break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_STOPPING_STATE);
	}
}


void
PPPFiniteStateMachine::CloseEvent()
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
			InitializeRestartCount();
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
PPPFiniteStateMachine::TOGoodEvent()
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
PPPFiniteStateMachine::TOBadEvent()
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
PPPFiniteStateMachine::RCRGoodEvent(mbuf *packet)
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
PPPFiniteStateMachine::RCRBadEvent(mbuf *nak, mbuf *reject)
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
			if(!nak && mtod(nak, lcp_packet*)->length > 3)
				SendConfigureNak(nak);
			else if(!reject && mtod(reject, lcp_packet*)->length > 3)
				SendConfigureNak(reject);
		break;
	}
}


// receive configure ack
void
PPPFiniteStateMachine::RCAEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	PPPOptionHandler *handler;
	PPPConfigurePacket ack(packet);
	
	for(int32 i = 0; i < LCP().CountOptionHandlers(); i++) {
		handler = LCP().OptionHandlerAt(i);
		handler->ParseAck(ack);
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
			locker.UnlockNow();
			ThisLayerDown();
			SendConfigureRequest();
		break;
	}
}


// receive configure nak/reject
void
PPPFiniteStateMachine::RCNEvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	PPPOptionHandler *handler;
	PPPConfigurePacket nak_reject(packet);
	
	for(int32 i = 0; i < LCP().CountOptionHandlers(); i++) {
		handler = LCP().OptionHandlerAt(i);
		
		if(nak_reject.Code() == PPP_CONFIGURE_NAK)
			handler->ParseNak(nak_reject);
		else if(nak_reject.Code() == PPP_CONFIGURE_REJECT)
			handler->ParseReject(nak_reject);
	}
	
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
			InitializeRestartCount();
		
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
PPPFiniteStateMachine::RTREvent(mbuf *packet)
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
PPPFiniteStateMachine::RTAEvent(mbuf *packet)
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
PPPFiniteStateMachine::RUCEvent(mbuf *packet, uint16 protocol)
{
	LockerHelper locker(fLock);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RUC_EVENT);
		break;
		
		default:
			locker.UnlockNow();
			SendCodeReject(packet, protocol);
	}
}


// receive code/protocol reject (acceptable such as IPX reject)
void
PPPFiniteStateMachine::RXJGoodEvent(mbuf *packet)
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
PPPFiniteStateMachine::RXJBadEvent(mbuf *packet)
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
			InitializeRestartCount();
			locker.UnlockNow();
			ThisLayerDown();
			SendTerminateRequest();
		break;
	}
}


// receive echo request/reply, discard request
void
PPPFiniteStateMachine::RXREvent(mbuf *packet)
{
	LockerHelper locker(fLock);
	
	lcp_packet *echo = mtod(mbuf, lcp_packet*);
	
	switch(State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXR_EVENT);
		break;
		
		case PPP_OPENED_STATE:
			if(echo->code == PPP_ECHO_REQUEST)
				SendEchoReply(packet);
		break;
	}
}


// general events (for Good/Bad events)
void
PPPFiniteStateMachine::TimerEvent()
{
	// TODO:
	// check which counter expired
	// call TOGood/TOBad
}


// ReceiveConfigureRequest
// Here we get a configure-request packet from LCP and aks all OptionHandlers
// if its values are acceptable. From here we call our Good/Bad counterparts.
void
PPPFiniteStateMachine::RCREvent(mbuf *packet)
{
	PPPConfigurePacket request(packet), nak(PPP_CONFIGURE_NAK),
		reject(PPP_CONFIGURE_REJECT);
	PPPOptionHandler *handler;
	
	nak.SetID(request.ID());
	reject.SetID(request.ID());
	
	// each handler should add unacceptable values for each item
	bool handled;
	int32 error;
	for(int32 item = 0; item <= request.CountItems(); item++) {
		// if we sent too many naks we should not append additional values
		if(fNakCounter == 0 && item == request.CountItems())
			break;
		
		handled = false;
		
		for(int32 index = 0; index < LCP().CountOptionHandlers();
			index++) {
			handler = LCP().OptionHandlerAt(i);
			error = handler->ParseRequest(&request, item, &nak, &reject);
			if(error == PPP_UNHANLED)
				continue;
			else if(error == B_ERROR) {
				// the request contains a value that has been sent more than
				// once or the value is corrupted
				CloseEvent();
				return;
			} else if(error = B_OK)
				handled = true;
		}
		
		if(!handled && item < request.CountItems()) {
			// unhandled items should be added to reject
			reject.AddItem(request.ItemAt(item));
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
PPPFiniteStateMachine::RXJEvent(mbuf *packet)
{
	lcp_packet *reject mtod(packet, lcp_packet*);
	
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
		
		for(index = 0; index < LCP().CountProtocols(); index++) {
			protocol_handler = LCP().ProtocolAt(index);
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
PPPFiniteStateMachine::IllegalEvent(PPP_EVENT event)
{
	// TODO:
	// update error statistics
}


void
PPPFiniteStateMachine::ThisLayerUp()
{
	// TODO:
	// bring all P/Es up
	// increase until we reach PPP_ESTABLISHED_PHASE or until we actually
	// find P/Es that need an Up().
	// In this case we continue to increase the phase in UpEvent(P/E).
}


void
PPPFiniteStateMachine::ThisLayerDown()
{
	// TODO:
	// DownProtocols();
	// DownEncapsulators();
	// ResetProtocols();
	// ResetEncapsulators();
	// If there are protocols/encapsulators with PPP_DOWN_NEEDED flag
	// wait until they report a DownEvent() (or until all of them are down).
}


void
PPPFiniteStateMachine::ThisLayerStarted()
{
	if(Interface()->Device())
		Interface()->Device()->Up();
}


void
PPPFiniteStateMachine::ThisLayerFinished()
{
	// TODO:
	// ResetProtocols();
	// ResetEncapsulators();
	
	if(Interface()->Device())
		Interface()->Device()->Down();
}


void
PPPFiniteStateMachine::InitializeRestartCount()
{
	fConfigureCounter = fMaxConfigure;
	fTerminateCounter = fMaxTerminate;
	fNakCounter = fMaxNak;
	
	// TODO:
	// start timer
}


void
PPPFiniteStateMachine::ZeroRestartCount()
{
	fConfigureCounter = 0;
	fTerminateCounter = 0;
	fNakCounter = 0;
	
	// TODO:
	// start timer
}


void
PPPFiniteStateMachine::SendConfigureRequest()
{
	if(fConfigureCounter == 0) {
		CloseEvent();
		return;
	}
	
	--fConfigureCounter;
	
	PPPConfigurePacket request(PPP_CONFIGURE_REQUEST);
	request.SetID(NextID());
	
	for(int32 i = 0; i < LCP().CountOptionHandlers(); i++) {
		// add all items
		if(!LCP().OptionHandlerAt(i)->AddToRequest(&request)) {
			CloseEvent();
			return;
		}
	}
	
	LCP().Send(request.ToMbuf());
	
	fTimeout = system_time();
}


void
PPPFiniteStateMachine::SendConfigureAck(mbuf *packet)
{
	mtod(packet, lcp_packet*)->code = PPP_CONFIGURE_ACK;
	PPPConfigurePacket ack(packet);
	
	for(int32 i = 0; i < LCP().CountOptionHandlers; i++)
		LCP().OptionHandlerAt(i)->SendingAck(&ack);
	
	LCP().Send(packet);
}


void
PPPFiniteStateMachine::SendConfigureNak(mbuf *packet)
{
	lcp_packet *nak = mtod(packet, lcp_packet*);
	if(nak->code == PPP_CONFIGURE_NAK) {
		if(fNakCounter == 0) {
			// We sent enough naks. Let's try a reject.
			nak->code = PPP_CONFIGURE_REJECT;
		} else
			--fNakCounter;
	}
	
	LCP().Send(packet);
}


void
PPPFiniteStateMachine::SendTerminateRequest()
{
}


void
PPPFiniteStateMachine::SendTerminateAck(mbuf *request)
{
}


void
PPPFiniteStateMachine::SendCodeReject(mbuf *packet, uint16 protocol)
{
}


void
PPPFiniteStateMachine::SendEchoReply(mbuf *request)
{
}
