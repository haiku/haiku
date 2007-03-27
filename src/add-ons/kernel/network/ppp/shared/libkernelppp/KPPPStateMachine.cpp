/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPStateMachine
	\brief PPP state machine belonging to the LCP protocol
	
	The state machine is responsible for handling events and state changes.
	
	\sa KPPPLCP
*/

#include <OS.h>

#include <KPPPInterface.h>
#include <KPPPConfigurePacket.h>
#include <KPPPDevice.h>
#include <KPPPLCPExtension.h>
#include <KPPPOptionHandler.h>

#include <KPPPUtils.h>

#include <net/if.h>
#include <core_funcs.h>


static const bigtime_t kPPPStateMachineTimeout = 3000000;
	// 3 seconds


//!	Constructor.
KPPPStateMachine::KPPPStateMachine(KPPPInterface& interface)
	: fInterface(interface),
	fLCP(interface.LCP()),
	fState(PPP_INITIAL_STATE),
	fPhase(PPP_DOWN_PHASE),
	fID(system_time() & 0xFF),
	fMagicNumber(0),
	fLastConnectionReportCode(PPP_REPORT_DOWN_SUCCESSFUL),
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
	fNextTimeout(0),
	fLock("KPPPStateMachine")
{
}


//!	Destructor. Frees loaded memory.
KPPPStateMachine::~KPPPStateMachine()
{
	free(fLocalAuthenticationName);
	free(fPeerAuthenticationName);
}


//!	Creates an ID for the next LCP packet.
uint8
KPPPStateMachine::NextID()
{
	return (uint8) atomic_add(&fID, 1);
}


/*!	\brief Changes the current state.
	
	Remember: NewState() must always be called \e after IllegalEvent() because
	IllegalEvent() also looks at the current state.
*/
void
KPPPStateMachine::NewState(ppp_state next)
{
	TRACE("KPPPSM: NewState(%d) state=%d\n", next, State());
	
	// maybe we do not need the timer anymore
	if (next < PPP_CLOSING_STATE || next == PPP_OPENED_STATE)
		fNextTimeout = 0;
	
	if (State() == PPP_OPENED_STATE && next != State())
		ResetLCPHandlers();
	
	fState = next;
}


/*!	\brief Changes the current phase.
	
	This method is responsible for setting the \c if_flags and sending the
	\c PPP_REPORT_UP_SUCCESSFUL report message.
*/
void
KPPPStateMachine::NewPhase(ppp_phase next)
{
#if DEBUG
	if (next <= PPP_ESTABLISHMENT_PHASE || next == PPP_ESTABLISHED_PHASE)
		TRACE("KPPPSM: NewPhase(%d) phase=%d\n", next, Phase());
#endif
	
	// there is nothing after established phase and nothing before down phase
	if (next > PPP_ESTABLISHED_PHASE)
		next = PPP_ESTABLISHED_PHASE;
	else if (next < PPP_DOWN_PHASE)
		next = PPP_DOWN_PHASE;
	
	// Report a down event to parent if we are not usable anymore.
	// The report threads get their notification later.
	if (Phase() == PPP_ESTABLISHED_PHASE && next != Phase()) {
		if (Interface().Ifnet()) {
			Interface().Ifnet()->if_flags &= ~IFF_RUNNING;
			Interface().Ifnet()->if_flags &= ~IFF_UP;
		}
		
		if (Interface().Parent())
			Interface().Parent()->StateMachine().DownEvent(Interface());
	}
	
	fPhase = next;
	
	if (Phase() == PPP_ESTABLISHED_PHASE) {
		Interface().fConnectedSince = system_time();
		
		if (Interface().Ifnet())
			Interface().Ifnet()->if_flags |= IFF_UP | IFF_RUNNING;
		
		Interface().fConnectAttempt = 0;
			// when we Reconnect() this becomes 1 (the first connection attempt)
		send_data_with_timeout(Interface().fReconnectThread, 0, NULL, 0, 200);
			// abort possible reconnect attempt
		fLastConnectionReportCode = PPP_REPORT_UP_SUCCESSFUL;
		Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_UP_SUCCESSFUL,
			&fInterface.fID, sizeof(ppp_interface_id));
	}
}


// public actions

//!	Reconfigures the state machine. This initiates a new configure request handshake.
bool
KPPPStateMachine::Reconfigure()
{
	TRACE("KPPPSM: Reconfigure() state=%d phase=%d\n", State(), Phase());
	
	if (State() < PPP_REQ_SENT_STATE)
		return false;
	
	NewState(PPP_REQ_SENT_STATE);
	NewPhase(PPP_ESTABLISHMENT_PHASE);
		// indicates to handlers that we are reconfiguring
	
	DownProtocols();
	ResetLCPHandlers();
	
	return SendConfigureRequest();
}


//!	Sends an echo request packet.
bool
KPPPStateMachine::SendEchoRequest()
{
	TRACE("KPPPSM: SendEchoRequest() state=%d phase=%d\n", State(), Phase());
	
	if (State() != PPP_OPENED_STATE)
		return false;
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if (!packet)
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


//!	Sends a discard request packet.
bool
KPPPStateMachine::SendDiscardRequest()
{
	TRACE("KPPPSM: SendDiscardRequest() state=%d phase=%d\n", State(), Phase());
	
	if (State() != PPP_OPENED_STATE)
		return false;
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if (!packet)
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

/*!	Notification that local authentication is requested.
	
	NOTE: This must be called \e after \c KPPPProtocol::Up().
*/
void
KPPPStateMachine::LocalAuthenticationRequested()
{
	TRACE("KPPPSM: LocalAuthenticationRequested() state=%d phase=%d\n",
		State(), Phase());
	
	fLastConnectionReportCode = PPP_REPORT_AUTHENTICATION_REQUESTED;
	Interface().Report(PPP_CONNECTION_REPORT,
		PPP_REPORT_AUTHENTICATION_REQUESTED, &fInterface.fID,
		sizeof(ppp_interface_id));
	
	fLocalAuthenticationStatus = PPP_AUTHENTICATING;
	free(fLocalAuthenticationName);
	fLocalAuthenticationName = NULL;
}


/*!	\brief Notification that local authentication was accepted.
	
	NOTE: This must be called \e before \c UpEvent().
	
	\param name The username/login that was accepted.
*/
void
KPPPStateMachine::LocalAuthenticationAccepted(const char *name)
{
	TRACE("KPPPSM: LocalAuthenticationAccepted() state=%d phase=%d\n",
		State(), Phase());
	
	fLocalAuthenticationStatus = PPP_AUTHENTICATION_SUCCESSFUL;
	free(fLocalAuthenticationName);
	if (name)
		fLocalAuthenticationName = strdup(name);
	else
		fLocalAuthenticationName = NULL;
}


/*!	\brief Notification that local authentication was denied.
	
	NOTE: This must be called \e before \c UpFailedEvent().
	
	\param name The username/login that was denied.
*/
void
KPPPStateMachine::LocalAuthenticationDenied(const char *name)
{
	TRACE("KPPPSM: LocalAuthenticationDenied() state=%d phase=%d\n", State(), Phase());
	
	fLocalAuthenticationStatus = PPP_AUTHENTICATION_FAILED;
	free(fLocalAuthenticationName);
	if (name)
		fLocalAuthenticationName = strdup(name);
	else
		fLocalAuthenticationName = NULL;
	
	// the report will be sent in DownEvent()
}


//!	Notification that peer authentication is requested.
void
KPPPStateMachine::PeerAuthenticationRequested()
{
	TRACE("KPPPSM: PeerAuthenticationRequested() state=%d phase=%d\n",
		State(), Phase());
	
	fLastConnectionReportCode = PPP_REPORT_AUTHENTICATION_REQUESTED;
	Interface().Report(PPP_CONNECTION_REPORT,
		PPP_REPORT_AUTHENTICATION_REQUESTED, &fInterface.fID,
		sizeof(ppp_interface_id));
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATING;
	free(fPeerAuthenticationName);
	fPeerAuthenticationName = NULL;
}


/*!	\brief Notification that peer authentication was accepted.
	
	NOTE: This must be called \e before \c UpEvent().
	
	\param name The username/login that was accepted.
*/
void
KPPPStateMachine::PeerAuthenticationAccepted(const char *name)
{
	TRACE("KPPPSM: PeerAuthenticationAccepted() state=%d phase=%d\n",
		State(), Phase());
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATION_SUCCESSFUL;
	free(fPeerAuthenticationName);
	if (name)
		fPeerAuthenticationName = strdup(name);
	else
		fPeerAuthenticationName = NULL;
}


/*!	\brief Notification that peer authentication was denied.
	
	NOTE: This must be called \e before \c UpFailedEvent().
	
	\param name The username/login that was denied.
*/
void
KPPPStateMachine::PeerAuthenticationDenied(const char *name)
{
	TRACE("KPPPSM: PeerAuthenticationDenied() state=%d phase=%d\n", State(), Phase());
	
	fPeerAuthenticationStatus = PPP_AUTHENTICATION_FAILED;
	free(fPeerAuthenticationName);
	if (name)
		fPeerAuthenticationName = strdup(name);
	else
		fPeerAuthenticationName = NULL;
	
	CloseEvent();
	
	// the report will be sent in DownEvent()
}


//!	Notification that a child interface failed to go up.
void
KPPPStateMachine::UpFailedEvent(KPPPInterface& interface)
{
	TRACE("KPPPSM: UpFailedEvent(interface) state=%d phase=%d\n", State(), Phase());
	
	// TODO:
	// log that an interface did not go up
}


//!	Notification that a child interface went up successfully.
void
KPPPStateMachine::UpEvent(KPPPInterface& interface)
{
	TRACE("KPPPSM: UpEvent(interface) state=%d phase=%d\n", State(), Phase());
	
	if (Phase() <= PPP_TERMINATION_PHASE) {
		interface.StateMachine().CloseEvent();
		return;
	}
	
	Interface().CalculateBaudRate();
	
	if (Phase() == PPP_ESTABLISHMENT_PHASE) {
		// this is the first interface that went up
		Interface().SetMRU(interface.MRU());
		ThisLayerUp();
	} else if (Interface().MRU() > interface.MRU())
		Interface().SetMRU(interface.MRU());
			// MRU should always be the smallest value of all children
	
	NewState(PPP_OPENED_STATE);
}


//!	Notification that a child interface went down.
void
KPPPStateMachine::DownEvent(KPPPInterface& interface)
{
	TRACE("KPPPSM: DownEvent(interface) state=%d phase=%d\n", State(), Phase());
	
	uint32 MRU = 0;
		// the new MRU
	
	Interface().CalculateBaudRate();
	
	// when all children are down we should not be running
	if (Interface().IsMultilink() && !Interface().Parent()) {
		uint32 count = 0;
		KPPPInterface *child;
		for (int32 index = 0; index < Interface().CountChildren(); index++) {
			child = Interface().ChildAt(index);
			
			if (child && child->IsUp()) {
				// set MRU to the smallest value of all children
				if (MRU == 0)
					MRU = child->MRU();
				else if (MRU > child->MRU())
					MRU = child->MRU();
				
				++count;
			}
		}
		
		if (MRU == 0)
			Interface().SetMRU(1500);
		else
			Interface().SetMRU(MRU);
		
		if (count == 0)
			DownEvent();
	}
}


/*!	\brief Notification that a protocol failed to go up.
	
	NOTE FOR AUTHENTICATORS: This \e must be called \e after an authentication
	notification method like \c LocalAuthenticationFailed().
*/
void
KPPPStateMachine::UpFailedEvent(KPPPProtocol *protocol)
{
	TRACE("KPPPSM: UpFailedEvent(protocol) state=%d phase=%d\n", State(), Phase());
	
	if ((protocol->Flags() & PPP_NOT_IMPORTANT) == 0) {
		if (Interface().Mode() == PPP_CLIENT_MODE) {
			// pretend we lost connection
			if (Interface().IsMultilink() && !Interface().Parent())
				for (int32 index = 0; index < Interface().CountChildren(); index++)
					Interface().ChildAt(index)->StateMachine().CloseEvent();
			else if (Interface().Device())
				Interface().Device()->Down();
			else
				CloseEvent();
					// just to be on the secure side ;)
		} else
			CloseEvent();
	}
}


/*!	\brief Notification that a protocol went up successfully.
	
	NOTE FOR AUTHENTICATORS: This \e must be called \e after an authentication
	notification method like \c LocalAuthenticationSuccessful().
*/
void
KPPPStateMachine::UpEvent(KPPPProtocol *protocol)
{
	TRACE("KPPPSM: UpEvent(protocol) state=%d phase=%d\n", State(), Phase());
	
	if (Phase() >= PPP_ESTABLISHMENT_PHASE)
		BringProtocolsUp();
}


/*!	\brief Notification that a protocol went down.
	
	NOTE FOR AUTHENTICATORS: This \e must be called \e after an authentication
	notification method like \c LocalAuthenticationFailed().
*/
void
KPPPStateMachine::DownEvent(KPPPProtocol *protocol)
{
	TRACE("KPPPSM: DownEvent(protocol) state=%d phase=%d\n", State(), Phase());
}


/*!	\brief Notification that the device entered establishment phase.
	
	We can use \c Device::Down() to abort establishment until \c UpEvent() is called.
	
	\return
		- \c true: We are waiting for an \c UpEvent()
		- \c false: The device should immediately abort its attempt to connect.
*/
bool
KPPPStateMachine::TLSNotify()
{
	TRACE("KPPPSM: TLSNotify() state=%d phase=%d\n", State(), Phase());
	
	if (State() == PPP_STARTING_STATE) {
			if (Phase() == PPP_DOWN_PHASE)
				NewPhase(PPP_ESTABLISHMENT_PHASE);
					// this says that the device is going up
			return true;
	}
	
	return false;
}


/*!	\brief Notification that the device entered termination phase.
	
	A \c Device::Up() should wait until the device went down.
	
	\return
		- \c true: Continue terminating.
		- \c false: We want to stay connected, though we called \c Device::Down().
*/
bool
KPPPStateMachine::TLFNotify()
{
	TRACE("KPPPSM: TLFNotify() state=%d phase=%d\n", State(), Phase());
	
	NewPhase(PPP_TERMINATION_PHASE);
		// tell DownEvent() that it may create a connection-lost-report
	
	return true;
}


//!	Notification that the device failed to go up.
void
KPPPStateMachine::UpFailedEvent()
{
	TRACE("KPPPSM: UpFailedEvent() state=%d phase=%d\n", State(), Phase());
	
	switch (State()) {
		case PPP_STARTING_STATE:
			fLastConnectionReportCode = PPP_REPORT_DEVICE_UP_FAILED;
			Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_DEVICE_UP_FAILED,
				&fInterface.fID, sizeof(ppp_interface_id));
			if (Interface().Parent())
				Interface().Parent()->StateMachine().UpFailedEvent(Interface());
			
			NewPhase(PPP_DOWN_PHASE);
				// tell DownEvent() that it should not create a connection-lost-report
			DownEvent();
			break;
		
		default:
			IllegalEvent(PPP_UP_FAILED_EVENT);
	}
}


//!	Notification that the device went up successfully.
void
KPPPStateMachine::UpEvent()
{
	TRACE("KPPPSM: UpEvent() state=%d phase=%d\n", State(), Phase());
	
	// This call is public, thus, it might not only be called by the device.
	// We must recognize these attempts to fool us and handle them correctly.
	
	if (!Interface().Device() || !Interface().Device()->IsUp())
		return;
			// it is not our device that went up...
	
	Interface().CalculateBaudRate();
	
	switch (State()) {
		case PPP_INITIAL_STATE:
			if (Interface().Mode() != PPP_SERVER_MODE
					|| Phase() != PPP_ESTABLISHMENT_PHASE) {
				// we are a client or we do not listen for an incoming
				// connection, so this is an illegal event
				IllegalEvent(PPP_UP_EVENT);
				NewState(PPP_CLOSED_STATE);
				ThisLayerFinished();
				
				return;
			}
			
			// TODO: handle server-up! (maybe already done correctly)
			NewState(PPP_REQ_SENT_STATE);
			InitializeRestartCount();
			SendConfigureRequest();
			break;
		
		case PPP_STARTING_STATE:
			// we must have called TLS() which sets establishment phase
			if (Phase() != PPP_ESTABLISHMENT_PHASE) {
				// there must be a BUG in the device add-on or someone is trying to
				// fool us (UpEvent() is public) as we did not request the device
				// to go up
				IllegalEvent(PPP_UP_EVENT);
				NewState(PPP_CLOSED_STATE);
				ThisLayerFinished();
				break;
			}
			
			NewState(PPP_REQ_SENT_STATE);
			InitializeRestartCount();
			SendConfigureRequest();
			break;
		
		default:
			IllegalEvent(PPP_UP_EVENT);
	}
}


/*!	\brief Notification that the device went down.
	
	If this is called without a prior \c TLFNotify() the state machine will assume
	that we lost our connection.
*/
void
KPPPStateMachine::DownEvent()
{
	TRACE("KPPPSM: DownEvent() state=%d phase=%d\n", State(), Phase());
	
	if (Interface().Device() && Interface().Device()->IsUp())
		return;
			// it is not our device that went down...
	
	Interface().CalculateBaudRate();
	
	// reset IdleSince
	Interface().fIdleSince = 0;
	
	switch (State()) {
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
			// will only do this if auto-reconnect is enabled (only clients).
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
	
	// maybe we need to reconnect
	if (State() == PPP_STARTING_STATE) {
		bool deleteInterface = false, retry = false;
		
		// we do not try to reconnect if authentication failed
		if (fLocalAuthenticationStatus == PPP_AUTHENTICATION_FAILED
				|| fLocalAuthenticationStatus == PPP_AUTHENTICATING
				|| fPeerAuthenticationStatus == PPP_AUTHENTICATION_FAILED
				|| fPeerAuthenticationStatus == PPP_AUTHENTICATING) {
			fLastConnectionReportCode = PPP_REPORT_AUTHENTICATION_FAILED;
			Interface().Report(PPP_CONNECTION_REPORT,
				PPP_REPORT_AUTHENTICATION_FAILED, &fInterface.fID,
				sizeof(ppp_interface_id));
			deleteInterface = true;
		} else {
			if (Interface().fConnectAttempt > (Interface().fConnectRetriesLimit + 1))
				deleteInterface = true;
			
			if (oldPhase == PPP_DOWN_PHASE) {
				// failed to bring device up (UpFailedEvent() was called)
				retry = true;
					// this may have been overridden by "deleteInterface = true"
			} else {
				// lost connection (TLFNotify() was called)
				fLastConnectionReportCode = PPP_REPORT_CONNECTION_LOST;
				Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_CONNECTION_LOST,
					&fInterface.fID, sizeof(ppp_interface_id));
			}
		}
		
		if (Interface().Parent())
			Interface().Parent()->StateMachine().UpFailedEvent(Interface());
		
		NewState(PPP_INITIAL_STATE);
		
		if (!deleteInterface && (retry || Interface().DoesAutoReconnect()))
			Interface().Reconnect(Interface().ReconnectDelay());
		else
			Interface().Delete();
	} else {
		fLastConnectionReportCode = PPP_REPORT_DOWN_SUCCESSFUL;
		Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_DOWN_SUCCESSFUL,
			&fInterface.fID, sizeof(ppp_interface_id));
		Interface().Delete();
	}
	
	fLocalAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	fPeerAuthenticationStatus = PPP_NOT_AUTHENTICATED;
}


// private events
void
KPPPStateMachine::OpenEvent()
{
	TRACE("KPPPSM: OpenEvent() state=%d phase=%d\n", State(), Phase());
	
	// reset all handlers
	if (Phase() != PPP_ESTABLISHED_PHASE) {
		DownProtocols();
		ResetLCPHandlers();
	}
	
	switch (State()) {
		case PPP_INITIAL_STATE:
			fLastConnectionReportCode = PPP_REPORT_GOING_UP;
			Interface().Report(PPP_CONNECTION_REPORT, PPP_REPORT_GOING_UP,
					&fInterface.fID, sizeof(ppp_interface_id));
			
			if (Interface().Mode() == PPP_SERVER_MODE) {
				NewPhase(PPP_ESTABLISHMENT_PHASE);
				
				if (Interface().Device() && !Interface().Device()->Up()) {
					Interface().Device()->UpFailedEvent();
					return;
				}
			} else
				NewState(PPP_STARTING_STATE);
			
			if (Interface().fAskBeforeConnecting == false)
				ContinueOpenEvent();
			break;
		
		case PPP_CLOSED_STATE:
			if (Phase() == PPP_DOWN_PHASE) {
				// the device is already going down
				return;
			}
			
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
			InitializeRestartCount();
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
KPPPStateMachine::ContinueOpenEvent()
{
	TRACE("KPPPSM: ContinueOpenEvent() state=%d phase=%d\n", State(), Phase());
	
	if (Interface().IsMultilink() && !Interface().Parent()) {
		NewPhase(PPP_ESTABLISHMENT_PHASE);
		for (int32 index = 0; index < Interface().CountChildren(); index++)
			if (Interface().ChildAt(index)->Mode() == Interface().Mode())
				Interface().ChildAt(index)->StateMachine().OpenEvent();
	} else
		ThisLayerStarted();
}


void
KPPPStateMachine::CloseEvent()
{
	TRACE("KPPPSM: CloseEvent() state=%d phase=%d\n", State(), Phase());
	
	if (Interface().IsMultilink() && !Interface().Parent()) {
		NewState(PPP_INITIAL_STATE);
		
		if (Phase() != PPP_DOWN_PHASE)
			NewPhase(PPP_TERMINATION_PHASE);
		
		ThisLayerDown();
		
		for (int32 index = 0; index < Interface().CountChildren(); index++)
			Interface().ChildAt(index)->StateMachine().CloseEvent();
		
		return;
	}
	
	switch (State()) {
		case PPP_OPENED_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_CLOSING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			InitializeRestartCount();
			if (State() == PPP_OPENED_STATE)
				ThisLayerDown();
			SendTerminateRequest();
			break;
		
		case PPP_STARTING_STATE:
			NewState(PPP_INITIAL_STATE);
			
			// TLSNotify() will know that we were faster because we
			// are in PPP_INITIAL_STATE now
			if (Phase() == PPP_ESTABLISHMENT_PHASE) {
				// the device is already up
				NewPhase(PPP_DOWN_PHASE);
					// this says the following DownEvent() was not caused by
					// a connection fault
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
KPPPStateMachine::TOGoodEvent()
{
	TRACE("KPPPSM: TOGoodEvent() state=%d phase=%d\n", State(), Phase());
	
	switch (State()) {
		case PPP_CLOSING_STATE:
		case PPP_STOPPING_STATE:
			SendTerminateRequest();
			break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			SendConfigureRequest();
			break;
		
		default:
			IllegalEvent(PPP_TO_GOOD_EVENT);
	}
}


// timeout (restart counters are <= 0)
void
KPPPStateMachine::TOBadEvent()
{
	TRACE("KPPPSM: TOBadEvent() state=%d phase=%d\n", State(), Phase());
	
	switch (State()) {
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
			ThisLayerFinished();
			break;
		
		case PPP_STOPPING_STATE:
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
			ThisLayerFinished();
			break;
		
		default:
			IllegalEvent(PPP_TO_BAD_EVENT);
	}
}


// receive configure request (acceptable request)
void
KPPPStateMachine::RCRGoodEvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RCRGoodEvent() state=%d phase=%d\n", State(), Phase());
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCR_GOOD_EVENT);
			m_freem(packet);
			break;
		
		case PPP_CLOSED_STATE:
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
			SendConfigureAck(packet);
			break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_OPENED_STATE);
			SendConfigureAck(packet);
			ThisLayerUp();
			break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_ACK_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
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
KPPPStateMachine::RCRBadEvent(struct mbuf *nak, struct mbuf *reject)
{
	TRACE("KPPPSM: RCRBadEvent() state=%d phase=%d\n", State(), Phase());
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCR_BAD_EVENT);
			break;
		
		case PPP_CLOSED_STATE:
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
			ThisLayerDown();
			SendConfigureRequest();
		
		case PPP_ACK_SENT_STATE:
			if (State() == PPP_ACK_SENT_STATE)
				NewState(PPP_REQ_SENT_STATE);
					// OPENED_STATE might have set this already
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
			if (nak && ntohs(mtod(nak, ppp_lcp_packet*)->length) > 3)
				SendConfigureNak(nak);
			else if (reject && ntohs(mtod(reject, ppp_lcp_packet*)->length) > 3)
				SendConfigureNak(reject);
			return;
			// prevents the nak/reject from being m_freem()'d
		
		default:
			;
	}
	
	if (nak)
		m_freem(nak);
	if (reject)
		m_freem(reject);
}


// receive configure ack
void
KPPPStateMachine::RCAEvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RCAEvent() state=%d phase=%d\n", State(), Phase());
	
	if (fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		m_freem(packet);
		return;
	}
	
	// let the option handlers parse this ack
	KPPPConfigurePacket ack(packet);
	KPPPOptionHandler *optionHandler;
	for (int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		optionHandler = LCP().OptionHandlerAt(index);
		if (optionHandler->ParseAck(ack) != B_OK) {
			m_freem(packet);
			CloseEvent();
			return;
		}
	}
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCA_EVENT);
			break;
		
		case PPP_CLOSED_STATE:
		case PPP_STOPPED_STATE:
			SendTerminateAck();
			break;
		
		case PPP_REQ_SENT_STATE:
			NewState(PPP_ACK_RCVD_STATE);
			InitializeRestartCount();
			break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
			SendConfigureRequest();
			break;
		
		case PPP_ACK_SENT_STATE:
			NewState(PPP_OPENED_STATE);
			InitializeRestartCount();
			ThisLayerUp();
			break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
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
KPPPStateMachine::RCNEvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RCNEvent() state=%d phase=%d\n", State(), Phase());
	
	if (fRequestID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		m_freem(packet);
		return;
	}
	
	// let the option handlers parse this nak/reject
	KPPPConfigurePacket nak_reject(packet);
	KPPPOptionHandler *optionHandler;
	for (int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		optionHandler = LCP().OptionHandlerAt(index);
		
		if (nak_reject.Code() == PPP_CONFIGURE_NAK) {
			if (optionHandler->ParseNak(nak_reject) != B_OK) {
				m_freem(packet);
				CloseEvent();
				return;
			}
		} else if (nak_reject.Code() == PPP_CONFIGURE_REJECT) {
			if (optionHandler->ParseReject(nak_reject) != B_OK) {
				m_freem(packet);
				CloseEvent();
				return;
			}
		}
	}
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RCN_EVENT);
			break;
		
		case PPP_CLOSED_STATE:
		case PPP_STOPPED_STATE:
			SendTerminateAck();
			break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_SENT_STATE:
			InitializeRestartCount();
		
		case PPP_ACK_RCVD_STATE:
			if (State() == PPP_ACK_RCVD_STATE)
				NewState(PPP_REQ_SENT_STATE);
			SendConfigureRequest();
			break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
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
KPPPStateMachine::RTREvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RTREvent() state=%d phase=%d\n", State(), Phase());
	
	// we should not use the same ID as the peer
	if (fID == mtod(packet, ppp_lcp_packet*)->id)
		fID -= 128;
	
	fLocalAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	fPeerAuthenticationStatus = PPP_NOT_AUTHENTICATED;
	
	switch (State()) {
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
			SendTerminateAck(packet);
			break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			ZeroRestartCount();
			ThisLayerDown();
			SendTerminateAck(packet);
			break;
		
		default:
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			SendTerminateAck(packet);
	}
}


// receive terminate ack
void
KPPPStateMachine::RTAEvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RTAEvent() state=%d phase=%d\n", State(), Phase());
	
	if (fTerminateID != mtod(packet, ppp_lcp_packet*)->id) {
		// this packet is not a reply to our request
		
		// TODO:
		// log this event
		m_freem(packet);
		return;
	}
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RTA_EVENT);
			break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
			ThisLayerFinished();
			break;
		
		case PPP_STOPPING_STATE:
			NewState(PPP_STOPPED_STATE);
			ThisLayerFinished();
			break;
		
		case PPP_ACK_RCVD_STATE:
			NewState(PPP_REQ_SENT_STATE);
			break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_REQ_SENT_STATE);
			NewPhase(PPP_ESTABLISHMENT_PHASE);
				// indicates to handlers that we are reconfiguring
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
KPPPStateMachine::RUCEvent(struct mbuf *packet, uint16 protocolNumber,
	uint8 code)
{
	TRACE("KPPPSM: RUCEvent() state=%d phase=%d\n", State(), Phase());
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RUC_EVENT);
			m_freem(packet);
			break;
		
		default:
			SendCodeReject(packet, protocolNumber, code);
	}
}


// receive code/protocol reject (acceptable such as IPX reject)
void
KPPPStateMachine::RXJGoodEvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RXJGoodEvent() state=%d phase=%d\n", State(), Phase());
	
	// This method does not m_freem(packet) because the acceptable rejects are
	// also passed to the parent. RXJEvent() will m_freem(packet) when needed.
	
	switch (State()) {
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
KPPPStateMachine::RXJBadEvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RXJBadEvent() state=%d phase=%d\n", State(), Phase());
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXJ_BAD_EVENT);
			break;
		
		case PPP_CLOSING_STATE:
			NewState(PPP_CLOSED_STATE);
		
		case PPP_CLOSED_STATE:
			ThisLayerFinished();
			break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			NewState(PPP_STOPPED_STATE);
		
		case PPP_STOPPING_STATE:
			NewPhase(PPP_TERMINATION_PHASE);
		
		case PPP_STOPPED_STATE:
			ThisLayerFinished();
			break;
		
		case PPP_OPENED_STATE:
			NewState(PPP_STOPPING_STATE);
			NewPhase(PPP_TERMINATION_PHASE);
				// indicates to handlers that we are terminating
			InitializeRestartCount();
			ThisLayerDown();
			SendTerminateRequest();
			break;
	}
	
	m_freem(packet);
}


// receive echo request/reply, discard request
void
KPPPStateMachine::RXREvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RXREvent() state=%d phase=%d\n", State(), Phase());
	
	ppp_lcp_packet *echo = mtod(packet, ppp_lcp_packet*);
	
	if (echo->code == PPP_ECHO_REPLY && echo->id != fEchoID) {
		// TODO:
		// log that we got a reply, but no request was sent
	}
	
	switch (State()) {
		case PPP_INITIAL_STATE:
		case PPP_STARTING_STATE:
			IllegalEvent(PPP_RXR_EVENT);
			break;
		
		case PPP_OPENED_STATE:
			if (echo->code == PPP_ECHO_REQUEST)
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
KPPPStateMachine::TimerEvent()
{
#if DEBUG
	if (fNextTimeout != 0)
		TRACE("KPPPSM: TimerEvent()\n");
#endif
	
	if (fNextTimeout == 0 || fNextTimeout > system_time())
		return;
	
	fNextTimeout = 0;
	
	switch (State()) {
		case PPP_CLOSING_STATE:
		case PPP_STOPPING_STATE:
			if (fTerminateCounter <= 0)
				TOBadEvent();
			else
				TOGoodEvent();
			break;
		
		case PPP_REQ_SENT_STATE:
		case PPP_ACK_RCVD_STATE:
		case PPP_ACK_SENT_STATE:
			if (fRequestCounter <= 0)
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
KPPPStateMachine::RCREvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RCREvent() state=%d phase=%d\n", State(), Phase());
	
	KPPPConfigurePacket request(packet);
	KPPPConfigurePacket nak(PPP_CONFIGURE_NAK);
	KPPPConfigurePacket reject(PPP_CONFIGURE_REJECT);
	
	// we should not use the same id as the peer
	if (fID == mtod(packet, ppp_lcp_packet*)->id)
		fID -= 128;
	
	nak.SetID(request.ID());
	reject.SetID(request.ID());
	
	// each handler should add unacceptable values for each item
	status_t result;
		// the return value of ParseRequest()
	KPPPOptionHandler *optionHandler;
	for (int32 index = 0; index < request.CountItems(); index++) {
		optionHandler = LCP().OptionHandlerFor(request.ItemAt(index)->type);
		
		if (!optionHandler || !optionHandler->IsEnabled()) {
			ERROR("KPPPSM::RCREvent():unknown type:%d\n", request.ItemAt(index)->type);
			// unhandled items should be added to the reject
			reject.AddItem(request.ItemAt(index));
			continue;
		}
		
		TRACE("KPPPSM::RCREvent(): OH=%s\n",
			optionHandler->Name() ? optionHandler->Name() : "Unknown");
		result = optionHandler->ParseRequest(request, index, nak, reject);
		
		if (result == PPP_UNHANDLED) {
			// unhandled items should be added to the reject
			reject.AddItem(request.ItemAt(index));
			continue;
		} else if (result != B_OK) {
			// the request contains a value that has been sent more than
			// once or the value is corrupted
			ERROR("KPPPSM::RCREvent(): OptionHandler returned parse error!\n");
			m_freem(packet);
			CloseEvent();
			return;
		}
	}
	
	// Additional values may be appended.
	// If we sent too many naks we should not append additional values.
	if (fNakCounter > 0) {
		for (int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
			optionHandler = LCP().OptionHandlerAt(index);
			if (optionHandler && optionHandler->IsEnabled()) {
				result = optionHandler->ParseRequest(request, request.CountItems(),
					nak, reject);
				
				if (result != B_OK) {
					// the request contains a value that has been sent more than
					// once or the value is corrupted
					ERROR("KPPPSM::RCREvent():OptionHandler returned append error!\n");
					m_freem(packet);
					CloseEvent();
					return;
				}
			}
		}
	}
	
	if (reject.CountItems() > 0) {
		RCRBadEvent(NULL, reject.ToMbuf(Interface().MRU(), LCP().AdditionalOverhead()));
		m_freem(packet);
	} else if (nak.CountItems() > 0) {
		RCRBadEvent(nak.ToMbuf(Interface().MRU(), LCP().AdditionalOverhead()), NULL);
		m_freem(packet);
	} else
		RCRGoodEvent(packet);
}


// ReceiveCodeReject
// LCP received a code/protocol-reject packet and we look if it is acceptable.
// From here we call our Good/Bad counterparts.
void
KPPPStateMachine::RXJEvent(struct mbuf *packet)
{
	TRACE("KPPPSM: RXJEvent() state=%d phase=%d\n", State(), Phase());
	
	ppp_lcp_packet *reject = mtod(packet, ppp_lcp_packet*);
	
	if (reject->code == PPP_CODE_REJECT) {
		uint8 rejectedCode = reject->data[0];
		
		// test if the rejected code belongs to the minimum LCP requirements
		if (rejectedCode >= PPP_MIN_LCP_CODE && rejectedCode <= PPP_MAX_LCP_CODE) {
			if (Interface().IsMultilink() && !Interface().Parent()) {
				// Main interfaces do not have states between STARTING and OPENED.
				// An RXJBadEvent() would enter one of those states which is bad.
				m_freem(packet);
				CloseEvent();
			} else
				RXJBadEvent(packet);
			
			return;
		}
		
		// find the LCP extension and disable it
		KPPPLCPExtension *lcpExtension;
		for (int32 index = 0; index < LCP().CountLCPExtensions(); index++) {
			lcpExtension = LCP().LCPExtensionAt(index);
			
			if (lcpExtension->Code() == rejectedCode)
				lcpExtension->SetEnabled(false);
		}
		
		m_freem(packet);
	} else if (reject->code == PPP_PROTOCOL_REJECT) {
		// disable all handlers for rejected protocol type
		uint16 rejected = *((uint16*) reject->data);
			// rejected protocol number
		
		if (rejected == PPP_LCP_PROTOCOL) {
			// LCP must not be rejected!
			RXJBadEvent(packet);
			return;
		}
		
		// disable protocols with the rejected protocol number
		KPPPProtocol *protocol = Interface().FirstProtocol();
		for (; protocol; protocol = protocol->NextProtocol()) {
			if (protocol->ProtocolNumber() == rejected)
				protocol->SetEnabled(false);
					// disable protocol
		}
		
		RXJGoodEvent(packet);
			// this event handler does not m_freem(packet)!!!
		
		// notify parent, too
		if (Interface().Parent())
			Interface().Parent()->StateMachine().RXJEvent(packet);
		else
			m_freem(packet);
	}
}


// actions (all private)
void
KPPPStateMachine::IllegalEvent(ppp_event event)
{
	// TODO:
	// update error statistics
	ERROR("KPPPSM: IllegalEvent(event=%d) state=%d phase=%d\n",
		event, State(), Phase());
}


void
KPPPStateMachine::ThisLayerUp()
{
	TRACE("KPPPSM: ThisLayerUp() state=%d phase=%d\n", State(), Phase());
	
	// We begin with authentication phase and wait until each phase is done.
	// We stop when we reach established phase.
	
	// Do not forget to check if we are going down.
	if (Phase() != PPP_ESTABLISHMENT_PHASE)
		return;
	
	NewPhase(PPP_AUTHENTICATION_PHASE);
	
	BringProtocolsUp();
}


void
KPPPStateMachine::ThisLayerDown()
{
	TRACE("KPPPSM: ThisLayerDown() state=%d phase=%d\n", State(), Phase());
	
	// KPPPProtocol::Down() should block if needed.
	DownProtocols();
}


void
KPPPStateMachine::ThisLayerStarted()
{
	TRACE("KPPPSM: ThisLayerStarted() state=%d phase=%d\n", State(), Phase());
	
	if (Interface().Device() && !Interface().Device()->Up())
		Interface().Device()->UpFailedEvent();
}


void
KPPPStateMachine::ThisLayerFinished()
{
	TRACE("KPPPSM: ThisLayerFinished() state=%d phase=%d\n", State(), Phase());
	
	if (Interface().Device())
		Interface().Device()->Down();
}


void
KPPPStateMachine::InitializeRestartCount()
{
	fRequestCounter = fMaxRequest;
	fTerminateCounter = fMaxTerminate;
	fNakCounter = fMaxNak;
}


void
KPPPStateMachine::ZeroRestartCount()
{
	fRequestCounter = 0;
	fTerminateCounter = 0;
	fNakCounter = 0;
}


bool
KPPPStateMachine::SendConfigureRequest()
{
	TRACE("KPPPSM: SendConfigureRequest() state=%d phase=%d\n", State(), Phase());
	
	--fRequestCounter;
	fNextTimeout = system_time() + kPPPStateMachineTimeout;
	
	KPPPConfigurePacket request(PPP_CONFIGURE_REQUEST);
	request.SetID(NextID());
	fRequestID = request.ID();
	
	for (int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		// add all items
		if (LCP().OptionHandlerAt(index)->AddToRequest(request) != B_OK) {
			CloseEvent();
			return false;
		}
	}
	
	return LCP().Send(request.ToMbuf(Interface().MRU(),
		LCP().AdditionalOverhead())) == B_OK;
}


bool
KPPPStateMachine::SendConfigureAck(struct mbuf *packet)
{
	TRACE("KPPPSM: SendConfigureAck() state=%d phase=%d\n", State(), Phase());
	
	if (!packet)
		return false;
	
	mtod(packet, ppp_lcp_packet*)->code = PPP_CONFIGURE_ACK;
	KPPPConfigurePacket ack(packet);
	
	// notify all option handlers that we are sending an ack for each value
	for (int32 index = 0; index < LCP().CountOptionHandlers(); index++) {
		if (LCP().OptionHandlerAt(index)->SendingAck(ack) != B_OK) {
			m_freem(packet);
			CloseEvent();
			return false;
		}
	}
	
	return LCP().Send(packet) == B_OK;
}


bool
KPPPStateMachine::SendConfigureNak(struct mbuf *packet)
{
	TRACE("KPPPSM: SendConfigureNak() state=%d phase=%d\n", State(), Phase());
	
	if (!packet)
		return false;
	
	ppp_lcp_packet *nak = mtod(packet, ppp_lcp_packet*);
	if (nak->code == PPP_CONFIGURE_NAK) {
		if (fNakCounter == 0) {
			// We sent enough naks. Let's try a reject.
			nak->code = PPP_CONFIGURE_REJECT;
		} else
			--fNakCounter;
	}
	
	return LCP().Send(packet) == B_OK;
}


bool
KPPPStateMachine::SendTerminateRequest()
{
	TRACE("KPPPSM: SendTerminateRequest() state=%d phase=%d\n", State(), Phase());
	
	--fTerminateCounter;
	fNextTimeout = system_time() + kPPPStateMachineTimeout;
	
	struct mbuf *packet = m_gethdr(MT_DATA);
	if (!packet)
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
KPPPStateMachine::SendTerminateAck(struct mbuf *request)
{
	TRACE("KPPPSM: SendTerminateAck() state=%d phase=%d\n", State(), Phase());
	
	struct mbuf *reply = request;
	
	ppp_lcp_packet *ack;
	
	if (!reply) {
		reply = m_gethdr(MT_DATA);
		if (!reply)
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
KPPPStateMachine::SendCodeReject(struct mbuf *packet, uint16 protocolNumber, uint8 code)
{
	TRACE("KPPPSM: SendCodeReject(protocolNumber=%X;code=%d) state=%d phase=%d\n",
		protocolNumber, code, State(), Phase());
	
	if (!packet)
		return false;
	
	int32 length;
		// additional space needed for this reject
	if (code == PPP_PROTOCOL_REJECT)
		length = 6;
	else
		length = 4;
	
	M_PREPEND(packet, length);
		// add some space for the header
	
	// adjust packet if too big
	int32 adjust = Interface().MRU();
	if (packet->m_flags & M_PKTHDR) {
		adjust -= packet->m_pkthdr.len;
	} else
		adjust -= packet->m_len;
	
	if (adjust < 0)
		m_adj(packet, adjust);
	
	ppp_lcp_packet *reject = mtod(packet, ppp_lcp_packet*);
	reject->code = code;
	reject->id = NextID();
	if (packet->m_flags & M_PKTHDR)
		reject->length = htons(packet->m_pkthdr.len);
	else
		reject->length = htons(packet->m_len);
	
	protocolNumber = htons(protocolNumber);
	if (code == PPP_PROTOCOL_REJECT)
		memcpy(&reject->data, &protocolNumber, sizeof(protocolNumber));
	
	return LCP().Send(packet) == B_OK;
}


bool
KPPPStateMachine::SendEchoReply(struct mbuf *request)
{
	TRACE("KPPPSM: SendEchoReply() state=%d phase=%d\n", State(), Phase());
	
	if (!request)
		return false;
	
	ppp_lcp_packet *reply = mtod(request, ppp_lcp_packet*);
	reply->code = PPP_ECHO_REPLY;
		// the request becomes a reply
	
	if (request->m_flags & M_PKTHDR)
		request->m_pkthdr.len = 8;
	request->m_len = 8;
	
	memcpy(reply->data, &fMagicNumber, sizeof(fMagicNumber));
	
	return LCP().Send(request) == B_OK;
}


// methods for bringing protocols up
void
KPPPStateMachine::BringProtocolsUp()
{
	// use a simple check for phase changes (e.g., caused by CloseEvent())
	while (Phase() <= PPP_ESTABLISHED_PHASE && Phase() >= PPP_AUTHENTICATION_PHASE) {
		if (BringPhaseUp() > 0)
			break;
		
		if (Phase() < PPP_AUTHENTICATION_PHASE)
			return;
				// phase was changed by another event
		else if (Phase() == PPP_ESTABLISHED_PHASE) {
			if (Interface().Parent())
				Interface().Parent()->StateMachine().UpEvent(Interface());
			break;
		} else
			NewPhase((ppp_phase) (Phase() + 1));
	}
}


// this returns the number of handlers waiting to go up
uint32
KPPPStateMachine::BringPhaseUp()
{
	// Servers do not need to bring all protocols up.
	// The client specifies which protocols he wants to go up.
	
	// check for phase change
	if (Phase() < PPP_AUTHENTICATION_PHASE)
		return 0;
	
	uint32 count = 0;
	KPPPProtocol *protocol = Interface().FirstProtocol();
	for (; protocol; protocol = protocol->NextProtocol()) {
		if (protocol->IsEnabled() && protocol->ActivationPhase() == Phase()) {
			if (protocol->IsGoingUp() && Interface().Mode() == PPP_CLIENT_MODE)
				++count;
			else if (protocol->IsDown() && protocol->IsUpRequested()) {
				if (Interface().Mode() == PPP_CLIENT_MODE)
					++count;
				
				protocol->Up();
			}
		}
	}
	
	// We only wait until authentication is complete.
	if (Interface().Mode() == PPP_SERVER_MODE
			&& (LocalAuthenticationStatus() == PPP_AUTHENTICATING
			|| PeerAuthenticationStatus() == PPP_AUTHENTICATING))
		++count;
	
	return count;
}


void
KPPPStateMachine::DownProtocols()
{
	KPPPProtocol *protocol = Interface().FirstProtocol();
	
	for (; protocol; protocol = protocol->NextProtocol())
		if (protocol->IsEnabled())
			protocol->Down();
}


void
KPPPStateMachine::ResetLCPHandlers()
{
	for (int32 index = 0; index < LCP().CountOptionHandlers(); index++)
		LCP().OptionHandlerAt(index)->Reset();
	
	for (int32 index = 0; index < LCP().CountLCPExtensions(); index++)
		LCP().LCPExtensionAt(index)->Reset();
}
