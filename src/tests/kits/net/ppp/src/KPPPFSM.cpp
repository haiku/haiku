#include "KPPPFSM.h"


PPPFSM::PPPFSM(PPPInterface &interface)
	: fInterface(&interface), fPhase(PPP_CTOR_DTOR_PHASE),
	fState(PPP_INITIAL_STATE),
	fAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fPeerAuthenticationStatus(PPP_NOT_AUTHENTICATED),
	fAuthenticatorIndex(-1), fPeerAuthenticatorIndex(-1)
{
	
}


PPPFSM::~PPPFSM()
{
	
}


// remember: NewState() must always be called _after_ IllegalEvent()
// because IllegalEvent() also looks at the current state.
void
PPPFSM::NewState(PPP_STATE next)
{
	fState = next;
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


// public events
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
			
			InitializeRestartCounter();
			NewState(PPP_REQ_SENT_STATE);
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
			
			InitializeRestartCounter();
			NewState(PPP_REQ_SENT_STATE);
			fPhase = PPP_ESTABLISHMENT_PHASE;
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
			InitializeRestartCounter();
			NewState(PPP_CLOSING_STATE);
			fPhase = PPP_TERMINATION_PHASE;
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


void
PPPFSM::RCRGoodEvent(PPPConfigurePacket *packet)
{
	LockerHelper locker(fLock);
	
	
}


void
PPPFSM::RCRBadEvent(PPPConfigurePacket *packet)
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RCAEvent(PPPConfigurePacket *packet)
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RCNEvent(PPPConfigurePacket *packet)
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RTREvent()
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RTAEvent()
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RUCEvent()
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RXJGoodEvent()
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RXJBadEvent()
{
	LockerHelper locker(fLock);
	
}


void
PPPFSM::RXREvent()
{
	LockerHelper locker(fLock);
	
}


// actions (all private)
void
PPPFSM::IllegalEvent(PPP_EVENT event)
{
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
PPPFSM::SendConfigureAck(PPPConfigurePacket *packet)
{
}


void
PPPFSM::SendConfigureNak(PPPConfigurePacket *packet)
{
			// is this needed?
}


void
PPPFSM::SendTerminateRequest()
{
}


void
PPPFSM::SendTerminateAck()
{
}


void
PPPFSM::SendCodeReject()
{
}


void
PPPFSM::SendEchoReply()
{
}
