//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

// Stdio.h must be included before PPPModule.h/PPPManager.h because
// dprintf is defined twice with different return values, once with
// void (KernelExport.h) and once with int (stdio.h).
#include <cstdio>
#include <cstring>
#include <new.h>

// now our headers...
#include <KPPPInterface.h>

// our other classes
#include <KPPPDevice.h>
#include <KPPPEncapsulator.h>
#include <KPPPModule.h>
#include <KPPPManager.h>
#include <KPPPUtils.h>

// general helper classes not only belonging to us
#include <LockerHelper.h>

// tools only for us :)
#include "settings_tools.h"


// TODO:
// - implement timers with support for settings next time instead of receiving timer
//    events periodically


// needed for redial:
typedef struct redial_info {
	PPPInterface *interface;
	thread_id *thread;
} redial_info;

status_t redial_func(void *data);

// other functions
status_t in_queue_thread(void *data);


PPPInterface::PPPInterface(uint32 ID, driver_settings *settings,
		PPPInterface *parent = NULL)
	: fID(ID), fSettings(dup_driver_settings(settings)),
	fStateMachine(*this), fLCP(*this), fReportManager(StateMachine().Locker()),
	fIfnet(NULL), fUpThread(-1), fRedialThread(-1), fDialRetry(0),
	fDialRetriesLimit(0), fIdleSince(0), fLinkMTU(1500), fDevice(NULL),
	fFirstEncapsulator(NULL), fLock(StateMachine().Locker())
{
	// set up queue
	fInQueue = start_ifq();
	fInQueueThread = spawn_thread(in_queue_thread, "PPPInterface: Input",
		B_NORMAL_PRIORITY, this);
	resume_thread(fInQueueThread);
	
	if(get_module(PPP_MANAGER_MODULE_NAME, (module_info**) &fManager) != B_OK)
		fManager = NULL;
	
	// are we a multilink subinterface?
	if(parent && parent->IsMultilink()) {
		fParent = parent;
		fParent->AddChild(this);
		fIsMultilink = true;
	} else {
		fParent = NULL;
		fIsMultilink = false;
	}
	
	if(!fSettings) {
		fMode = PPP_CLIENT_MODE;
		fDisconnectAfterIdleSince = 0;
		return;
	}
	
	const char *value;
	
	// get DisonnectAfterIdleSince settings
	value = get_settings_value(PPP_DISONNECT_AFTER_IDLE_SINCE_KEY, fSettings);
	if(!value)
		fDisconnectAfterIdleSince = 0;
	else
		fDisconnectAfterIdleSince = atoi(value) * 1000000;
	
	if(fDisconnectAfterIdleSince < 0)
		fDisconnectAfterIdleSince = 0;
	
	// get mode settings
	value = get_settings_value(PPP_MODE_KEY, fSettings);
	if(!strcasecmp(value, PPP_SERVER_MODE_VALUE))
		fMode = PPP_SERVER_MODE;
	else
		fMode = PPP_CLIENT_MODE;
		// we are a client by default
	
	SetDialOnDemand(
		get_boolean_value(
		get_settings_value(PPP_DIAL_ON_DEMAND_KEY, fSettings),
		false)
		);
		// dial on demand is disabled by default
	
	
	SetAutoRedial(
		get_boolean_value(
		get_settings_value(PPP_AUTO_REDIAL_KEY, fSettings),
		false)
		);
		// auto redial is disabled by default
	
	// load all protocols and the device
	if(LoadModules(fSettings, 0, fSettings->parameter_count))
		fInitStatus = B_OK;
	else
		fInitStatus = B_ERROR;
}


PPPInterface::~PPPInterface()
{
	if(fLock.Lock() != B_OK)
		return;;
			// make sure no thread wants to call Unlock() on fLock after it is deleted
	
	Report(PPP_DESTRUCTION_REPORT, 0, NULL, 0);
		// tell all listeners that we are being destroyed
	
	int32 tmp;
	stop_ifq(InQueue());
	wait_for_thread(fInQueueThread, &tmp);
	
	Down();
	UnregisterInterface();
	
	wait_for_thread(fRedialThread, &tmp);
	
	while(CountChildren())
		ChildAt(0)->Delete();
	
	delete Device();
	
	while(CountProtocols())
		delete ProtocolAt(0);
	
	while(FirstEncapsulator())
		delete FirstEncapsulator();
	
	for(int32 index = 0; index < fModules.CountItems(); index++) {
		put_module(fModules.ItemAt(index));
		free(fModules.ItemAt(index));
	}
	
	put_module(PPP_MANAGER_MODULE_NAME);
	
	free_driver_settings(fSettings);
}


void
PPPInterface::Delete()
{
	fManager->delete_interface(ID());
}


status_t
PPPInterface::InitCheck() const
{
	if(!fSettings || !fManager || fInitStatus != B_OK)
		return B_ERROR;
	
	// sub-interfaces should have a device
	if(IsMultilink()) {
		if(Parent() && !fDevice)
			return B_ERROR;
	} else if(!fDevice)
		return B_ERROR;
	
	return B_OK;
}


void
PPPInterface::SetLinkMTU(uint32 linkMTU)
{
	LockerHelper locker(fLock);
	
	fLinkMTU = linkMTU;
	
	CalculateMRU();
}


status_t
PPPInterface::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		// TODO:
		// add:
		// - routing Control() to encapsulators/protocols/option_handlers
		//    (calling their Control() method)
		// - adding modules
		// - setting AutoRedial and DialOnDemand
		
		
		default:
			return B_ERROR;
	}
	
	return B_OK;
}


bool
PPPInterface::SetDevice(PPPDevice *device)
{
	if(!device)
		return false;
	
	if(IsMultilink() && !Parent())
		return false;
			// main interfaces do not have devices
	
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	if(fDevice && (IsUp() || fDevice->IsUp()))
		Down();
	
	fDevice = device;
	
	fLinkMTU = fDevice->MTU();
	
	CalculateMRU();
	CalculateBaudRate();
	
	return true;
}


bool
PPPInterface::AddProtocol(PPPProtocol *protocol)
{
	if(!protocol)
		return false;
	
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	fProtocols.AddItem(protocol);
	
	if(IsUp() || Phase() >= protocol->Phase())
		protocol->Up();
	
	return true;
}


bool
PPPInterface::RemoveProtocol(PPPProtocol *protocol)
{
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	if(!fProtocols.HasItem(protocol))
		return false;
	
	if(IsUp() || Phase() >= protocol->Phase())
		protocol->Down();
	
	return fProtocols.RemoveItem(protocol);
}


PPPProtocol*
PPPInterface::ProtocolAt(int32 index) const
{
	PPPProtocol *protocol = fProtocols.ItemAt(index);
	
	if(protocol == fProtocols.GetDefaultItem())
		return NULL;
	
	return protocol;
}


PPPProtocol*
PPPInterface::ProtocolFor(uint16 protocol, int32 start = 0) const
{
	if(start < 0)
		return NULL;
	
	for(int32 i = start; i < fProtocols.CountItems(); i++)
		if(fProtocols.ItemAt(i)->Protocol() == protocol)
			return fProtocols.ItemAt(i);
	
	return NULL;
}


bool
PPPInterface::AddEncapsulator(PPPEncapsulator *encapsulator)
{
	// Find instert position after the last encapsulator
	// with the same level.
	
	if(!encapsulator)
		return false;
	
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	PPPEncapsulator *current = fFirstEncapsulator, *previous = NULL;
	
	while(current) {
		if(current->Level() < encapsulator->Level())
			break;
		
		previous = current;
		current = current->Next();
	}
	
	if(!current) {
		if(!previous)
			fFirstEncapsulator = encapsulator;
		else
			previous->SetNext(encapsulator);
		
		encapsulator->SetNext(NULL);
	} else {
		encapsulator->SetNext(current);
		
		if(!previous)
			fFirstEncapsulator = encapsulator;
		else
			previous->SetNext(encapsulator);
	}
	
	CalculateMRU();
	
	if(IsUp())
		encapsulator->Up();
	
	return true;
}


bool
PPPInterface::RemoveEncapsulator(PPPEncapsulator *encapsulator)
{
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	PPPEncapsulator *current = fFirstEncapsulator, *previous = NULL;
	
	while(current) {
		if(current == encapsulator) {
			if(IsUp())
				encapsulator->Down();
			
			if(previous)
				previous->SetNext(current->Next());
			else
				fFirstEncapsulator = current->Next();
			
			current->SetNext(NULL);
			
			CalculateMRU();
			
			return true;
		}
		
		previous = current;
		current = current->Next();
	}
	
	return false;
}


PPPEncapsulator*
PPPInterface::EncapsulatorFor(uint16 protocol,
	PPPEncapsulator *start = NULL) const
{
	PPPEncapsulator *current = start ? start : fFirstEncapsulator;
	
	for(; current; current = current->Next())
		if(current->Protocol() == protocol)
			return current;
	
	return current;
}


bool
PPPInterface::AddChild(PPPInterface *child)
{
	if(!child)
		return false;
	
	LockerHelper locker(fLock);
	
	if(fChildren.HasItem(child) || !fChildren.AddItem(child))
		return false;
	
	child->SetParent(this);
	
	return true;
}


bool
PPPInterface::RemoveChild(PPPInterface *child)
{
	LockerHelper locker(fLock);
	
	if(!fChildren.RemoveItem(child))
		return false;
	
	child->SetParent(NULL);
	
	// parents cannot exist without their children
	if(CountChildren() == 0 && fManager && Ifnet())
		fManager->delete_interface(ID());
	
	return true;
}


PPPInterface*
PPPInterface::ChildAt(int32 index) const
{
	PPPInterface *child = fChildren.ItemAt(index);
	
	if(child == fChildren.GetDefaultItem())
		return NULL;
	
	return child;
}


void
PPPInterface::SetAutoRedial(bool autoredial = true)
{
	if(Mode() == PPP_CLIENT_MODE)
		return;
	
	LockerHelper locker(fLock);
	
	fAutoRedial = autoredial;
}


void
PPPInterface::SetDialOnDemand(bool dialondemand = true)
{
	if(Mode() != PPP_CLIENT_MODE)
		return;
	
	LockerHelper locker(fLock);
	
	fDialOnDemand = dialondemand;
	
	// check if we need to register/unregister
	if(!Ifnet() && fDialOnDemand)
		RegisterInterface();
	else if(Ifnet() && !fDialOnDemand && Phase() == PPP_DOWN_PHASE) {
		UnregisterInterface();
		Delete();
	}
}


bool
PPPInterface::Up()
{
	if(!InitCheck() || Phase() == PPP_TERMINATION_PHASE)
		return false;
	
	if(IsUp())
		return true;
	
	ppp_report_packet report;
	thread_id me = find_thread(NULL), sender;
	
	// one thread has to do the real task while all other threads are observers
	fLock.Lock();
	if(fUpThread == -1)
		fUpThread = me;
	
	ReportManager().EnableReports(PPP_CONNECTION_REPORT, me, PPP_WAIT_FOR_REPLY);
	fLock.Unlock();
	
	// fUpThread tells the state machine to go up
	if(me == fUpThread)
		StateMachine().OpenEvent();
	
	while(true) {
		if(IsUp()) {
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
			
			if(me == fUpThread) {
				fLock.Lock();
				fDialRetry = 0;
				fUpThread = -1;
				fLock.Unlock();
			}
			
			return true;
		}
		
		if(receive_data(&sender, &report, sizeof(report)) != PPP_REPORT_CODE)
			continue;
		
		if(IsUp()) {
			if(me == fUpThread) {
				fDialRetry = 0;
				fUpThread = -1;
			}
			
			PPP_REPLY(sender, B_OK);
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
			return true;
		}
		
		if(report.type == PPP_DESTRUCTION_REPORT) {
			if(me == fUpThread) {
				fDialRetry = 0;
				fUpThread = -1;
			}
			
			PPP_REPLY(sender, B_OK);
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
			return false;
		} else if(report.type != PPP_CONNECTION_REPORT) {
			PPP_REPLY(sender, B_OK);
			continue;
		}
		
		if(report.code == PPP_REPORT_GOING_UP) {
			PPP_REPLY(sender, B_OK);
			continue;
		} else if(report.code == PPP_REPORT_UP_SUCCESSFUL) {
			if(me == fUpThread) {
				fDialRetry = 0;
				fUpThread = -1;
			}
			
			PPP_REPLY(sender, B_OK);
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
			return true;
		} else if(report.code == PPP_REPORT_DOWN_SUCCESSFUL
				|| report.code == PPP_REPORT_UP_ABORTED
				|| report.code == PPP_REPORT_AUTHENTICATION_FAILED) {
			if(me == fUpThread) {
				fDialRetry = 0;
				fUpThread = -1;
				
				if(!DoesDialOnDemand() && report.code != PPP_REPORT_DOWN_SUCCESSFUL)
					fManager->delete_interface(ID());
			}
			
			PPP_REPLY(sender, B_OK);
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
			return false;
		}
		
		if(me != fUpThread) {
			// I am an observer
			if(report.code == PPP_REPORT_DEVICE_UP_FAILED) {
				if(fDialRetry >= fDialRetriesLimit || fUpThread == -1) {
					PPP_REPLY(sender, B_OK);
					ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
					return false;
				} else {
					PPP_REPLY(sender, B_OK);
					continue;
				}
			} else if(report.code == PPP_REPORT_CONNECTION_LOST) {
				if(DoesAutoRedial()) {
					PPP_REPLY(sender, B_OK);
					continue;
				} else {
					PPP_REPLY(sender, B_OK);
					ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
					return false;
				}
			}
		} else {
			// I am the thread for the real task
			if(report.code == PPP_REPORT_DEVICE_UP_FAILED) {
				if(fDialRetry >= fDialRetriesLimit) {
					fDialRetry = 0;
					fUpThread = -1;
					
					if(!DoesDialOnDemand()
							&& report.code != PPP_REPORT_DOWN_SUCCESSFUL)
						fManager->delete_interface(ID());
					
					PPP_REPLY(sender, B_OK);
					ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
					return false;
				} else {
					++fDialRetry;
					PPP_REPLY(sender, B_OK);
					StateMachine().OpenEvent();
					continue;
				}
			} else if(report.code == PPP_REPORT_CONNECTION_LOST) {
				// the state machine knows that we are going up and leaves
				// the redial task to us
				if(DoesAutoRedial() && fDialRetry < fDialRetriesLimit) {
					++fDialRetry;
					PPP_REPLY(sender, B_OK);
					StateMachine().OpenEvent();
					continue;
				} else {
					fDialRetry = 0;
					fUpThread = -1;
					PPP_REPLY(sender, B_OK);
					ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
					
					if(!DoesDialOnDemand()
							&& report.code != PPP_REPORT_DOWN_SUCCESSFUL)
						fManager->delete_interface(ID());
					
					return false;
				}
			}
		}
		
		// if the code is unknown we continue
		PPP_REPLY(sender, B_OK);
	}
	
	return false;
}


bool
PPPInterface::Down()
{
	if(!InitCheck())
		return false;
	
	// this locked section guarantees that there are no state changes before we
	// enable the connection reports
	LockerHelper locker(fLock);
	if(State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)
		return true;
	
	ReportManager().EnableReports(PPP_CONNECTION_REPORT, find_thread(NULL));
	locker.UnlockNow();
	
	thread_id sender;
	ppp_report_packet report;
	
	StateMachine().CloseEvent();
	
	while(true) {
		if(receive_data(&sender, &report, sizeof(report)) != PPP_REPORT_CODE)
			continue;
		
		if(report.code == PPP_REPORT_DOWN_SUCCESSFUL
				|| report.code == PPP_REPORT_UP_ABORTED
				|| (State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)) {
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, find_thread(NULL));
			break;
		}
	}
	
	if(!DoesDialOnDemand())
		fManager->delete_interface(ID());
	
	return true;
}


bool
PPPInterface::IsUp() const
{
	LockerHelper locker(fLock);
	
	return Phase() == PPP_ESTABLISHED_PHASE;
}


bool
PPPInterface::LoadModules(driver_settings *settings, int32 start, int32 count)
{
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	int32 type;
		// which type key was used for loading this module?
	
	const char *name = NULL;
	
	// multilink handling
	for(int32 i = start;
			i < settings->parameter_count && i < start + count; i++) {
		if(!strcasecmp(settings->parameters[i].name, PPP_MULTILINK_KEY)
				&& settings->parameters[i].value_count > 0) {
			if(!LoadModule(settings->parameters[i].values[0],
					settings->parameters[i].parameters, PPP_MULTILINK_TYPE))
				return false;
			break;
		}
		
	}
	
	// are we a multilink main interface?
	if(IsMultilink() && !Parent()) {
		// main interfaces only load the multilink module
		// and create a child using their settings
		fManager->create_interface(settings, ID());
		return true;
	}
	
	for(int32 i = start;
			i < settings->parameter_count && i < start + count; i++) {
		type = -1;
		
		name = settings->parameters[i].name;
		
		if(!strcasecmp(name, PPP_LOAD_MODULE_KEY))
			type = PPP_LOAD_MODULE_TYPE;
		else if(!strcasecmp(name, PPP_DEVICE_KEY))
			type = PPP_DEVICE_TYPE;
		else if(!strcasecmp(name, PPP_PROTOCOL_KEY))
			type = PPP_PROTOCOL_TYPE;
		else if(!strcasecmp(name, PPP_AUTHENTICATOR_KEY))
			type = PPP_AUTHENTICATOR_TYPE;
		else if(!strcasecmp(name, PPP_PEER_AUTHENTICATOR_KEY))
			type = PPP_PEER_AUTHENTICATOR_TYPE;
		
		if(type >= 0)
			for(int32 value_id = 0; value_id < settings->parameters[i].value_count;
					value_id++)
				if(!LoadModule(settings->parameters[i].values[value_id],
						settings->parameters[i].parameters, type))
					return false;
	}
	
	return true;
}


bool
PPPInterface::LoadModule(const char *name, driver_parameter *parameter,
	int32 type)
{
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	if(!name || strlen(name) > B_FILE_NAME_LENGTH)
		return false;
	
	char *module_name = (char*) malloc(B_PATH_NAME_LENGTH);
	
	sprintf(module_name, "%s/%s", PPP_MODULES_PATH, name);
	
	ppp_module_info *module;
	if(get_module(module_name, (module_info**) &module) != B_OK)
		return false;
	
	// add the module to the list of loaded modules
	// for putting them on our destruction
	fModules.AddItem(module_name);
	
	return module->add_to(Parent()?Parent():this, this, parameter, type);
}


status_t
PPPInterface::Send(struct mbuf *packet, uint16 protocol)
{
	if(!packet)
		return B_ERROR;
	
	// we must pass the basic tests!
	if(InitCheck() != B_OK) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// test whether are going down
	if(Phase() == PPP_TERMINATION_PHASE) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// go up if DialOnDemand enabled and we are down
	if(DoesDialOnDemand() && Phase() == PPP_DOWN_PHASE)
		Up();
	
	// If this protocol is always allowed we should send the packet.
	// Note that these protocols are not passed to encapsulators!
	PPPProtocol *sending_protocol = ProtocolFor(protocol);
	if(sending_protocol && sending_protocol->Flags() & PPP_ALWAYS_ALLOWED
			&& sending_protocol->IsEnabled() && fDevice->IsUp()) {
		fIdleSince = system_time();
		return SendToDevice(packet, protocol);
	}
	
	// never send normal protocols when we are down
	if(!IsUp()) {
		m_freem(packet);
		return B_ERROR;
	}
	
	fIdleSince = system_time();
	
	// send to next up encapsulator
	if(!fFirstEncapsulator)
		return SendToDevice(packet, protocol);
	
	if(!fFirstEncapsulator->IsEnabled())
		return fFirstEncapsulator->SendToNext(packet, protocol);
	
	return fFirstEncapsulator->Send(packet, protocol);
}


status_t
PPPInterface::Receive(struct mbuf *packet, uint16 protocol)
{
	if(!packet)
		return B_ERROR;
	
	int32 result = PPP_REJECTED;
		// assume we have no handler
	
	// Find handler and let it parse the packet.
	// The handler does need not be up because if we are a server
	// the handler might be upped by this packet.
	PPPEncapsulator *encapsulator_handler = EncapsulatorFor(protocol);
	for(; encapsulator_handler;
		encapsulator_handler = EncapsulatorFor(protocol, encapsulator_handler->Next())) {
		if(!encapsulator_handler->IsEnabled()) {
			if(!encapsulator_handler->Next())
				break;
			
			continue;
				// disabled handlers should not be used
		}
		
		result = encapsulator_handler->Receive(packet, protocol);
		if(result == PPP_UNHANDLED) {
			if(!encapsulator_handler->Next())
				break;
			
			continue;
		}
		
		return result;
	}
	
	// no encapsulator handler could be found; try a protocol handler
	PPPProtocol *protocol_handler;
	for(int32 index = 0; index < CountProtocols(); index++) {
		protocol_handler = ProtocolAt(index);
		if(protocol != protocol_handler->Protocol())
			continue;
		
		if(!protocol_handler->IsEnabled()) {
			// disabled handlers should not be used
			result = PPP_REJECTED;
			continue;
		}
		
		result = protocol_handler->Receive(packet, protocol);
		if(result == PPP_UNHANDLED)
			continue;
		
		return result;
	}
	
	// maybe the parent interface can handle it
	if(Parent())
		return Parent()->Receive(packet, protocol);
	
	if(result == PPP_UNHANDLED) {
		m_freem(packet);
		return PPP_DISCARDED;
	} else {
		StateMachine().RUCEvent(packet, protocol);
		return PPP_REJECTED;
	}
}


status_t
PPPInterface::SendToDevice(struct mbuf *packet, uint16 protocol)
{
	if(!packet)
		return B_ERROR;
	
	// we must pass the basic tests like:
	// do we have a device (as main interface)?
	// did we load all modules?
	if(InitCheck() != B_OK) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// test whether are going down
	if(Phase() == PPP_TERMINATION_PHASE) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// go up if DialOnDemand enabled and we are down
	if(DoesDialOnDemand() && (Phase() == PPP_DOWN_PHASE
		|| Phase() == PPP_ESTABLISHMENT_PHASE))
		Up();
			// Up() waits until it is done
	
	// check if protocol is allowed and everything is up
	PPPProtocol *sending_protocol = ProtocolFor(protocol);
	if(!fDevice->IsUp()
		|| (!IsUp() && protocol != PPP_LCP_PROTOCOL
			&& (!sending_protocol
				|| sending_protocol->Flags() & PPP_ALWAYS_ALLOWED == 0
				|| !sending_protocol->IsEnabled()
				)
			)
		) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// encode in ppp frame
	M_PREPEND(packet, 2);
	
	if(packet == NULL)
		return B_ERROR;
	
	// set protocol (the only header field)
	protocol = htons(protocol);
	uint16 *header = mtod(packet, uint16*);
	*header = protocol;
	
	fIdleSince = system_time();
	
	// pass to device/children
	if(!IsMultilink() || Parent()) {
		// check if packet is too big for device
		if((packet->m_flags & M_PKTHDR && (uint32) packet->m_pkthdr.len > LinkMTU())
				|| packet->m_len > LinkMTU()) {
			m_freem(packet);
			return B_ERROR;
		}
		
		return Device()->Send(packet);
	} else {
		// the multilink encapsulator should have sent it to some child interface
		m_freem(packet);
		return B_ERROR;
	}
}


status_t
PPPInterface::ReceiveFromDevice(struct mbuf *packet)
{
	if(!packet)
		return B_ERROR;
	
	if(!InitCheck()) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// decode ppp frame
	uint16 *protocol = mtod(packet, uint16*);
	*protocol = ntohs(*protocol);
	
	m_adj(packet, sizeof(uint16));
	
	// Set our interface as the receiver.
	// This might be NULL, so protocols that belong to the network stack (IP, etc.)
	// will probably be confused (as there is no interface from which the packet came).
	// Protocols that live only in the PPP interface should have no problems with this.
	if(packet->m_flags & M_PKTHDR)
		packet->m_pkthdr.rcvif = Ifnet();
	
	return Receive(packet, *protocol);
}


void
PPPInterface::Pulse()
{
	// check our idle time and disconnect if needed
	if(fDisconnectAfterIdleSince > 0 && fIdleSince != 0
			&& fIdleSince - system_time() >= fDisconnectAfterIdleSince) {
		StateMachine().CloseEvent();
		return;
	}
	
	if(Device())
		Device()->Pulse();
	
	for(int32 index = 0; index < CountProtocols(); index++)
		ProtocolAt(index)->Pulse();
	
	PPPEncapsulator *encapsulator = fFirstEncapsulator;
	for(; encapsulator; encapsulator = encapsulator->Next())
		encapsulator->Pulse();
}


bool
PPPInterface::RegisterInterface()
{
	if(fIfnet)
		return true;
			// we are already registered
	
	if(!InitCheck())
		return false;
			// we cannot register if something is wrong
	
	// only MainInterfaces get an ifnet
	if(IsMultilink() && Parent() && Parent()->RegisterInterface())
		return true;
	
	if(!fManager)
		return false;
	
	fIfnet = fManager->register_interface(ID());
	
	if(!fIfnet)
		return false;
	
	CalculateBaudRate();
	
	return true;
}


bool
PPPInterface::UnregisterInterface()
{
	if(!fIfnet)
		return true;
			// we are already unregistered
	
	// only MainInterfaces get an ifnet
	if(IsMultilink() && Parent())
		return true;
	
	if(!fManager)
		return false;
	
	fManager->unregister_interface(ID());
	fIfnet = NULL;
	
	return true;
}


void
PPPInterface::CalculateMRU()
{
	fMRU = fLinkMTU;
	
	// sum all headers
	fHeaderLength = sizeof(uint16);
	
	PPPEncapsulator *encapsulator = fFirstEncapsulator;
	for(; encapsulator; encapsulator = encapsulator->Next())
		fHeaderLength += encapsulator->Overhead();
	
	fMRU -= fHeaderLength;
	
	if(Ifnet()) {
		Ifnet()->if_mtu = fMRU;
		Ifnet()->if_hdrlen = fHeaderLength;
	}
	
	if(Parent())
		Parent()->CalculateMRU();
}


void
PPPInterface::CalculateBaudRate()
{
	if(!Ifnet())
		return;
	
	if(Device())
		fIfnet->if_baudrate = max_c(Device()->InputTransferRate(),
			Device()->OutputTransferRate());
	else {
		fIfnet->if_baudrate = 0;
		for(int32 index = 0; index < CountChildren(); index++)
			if(ChildAt(index)->Ifnet())
				fIfnet->if_baudrate += ChildAt(index)->Ifnet()->if_baudrate;
	}
}


void
PPPInterface::Redial()
{
	if(fRedialThread != -1)
		return;
	
	// start a new thread that calls our Up() method
	redial_info *info = new redial_info;
	info->interface = this;
	info->thread = &fRedialThread;
	
	fRedialThread = spawn_thread(redial_func, "PPPInterface: redial_thread",
		B_NORMAL_PRIORITY, info);
	
	resume_thread(fRedialThread);
}


status_t
redial_func(void *data)
{
	redial_info *info = (redial_info*) data;
	
	info->interface->Up();
	*info->thread = -1;
	
	delete info;
	
	return B_OK;
}


status_t
in_queue_thread(void *data)
{
	PPPInterface *interface = (PPPInterface*) data;
	struct ifq *queue = interface->InQueue();
	struct mbuf *packet;
	status_t error;
	
	while(true) {
		error = acquire_sem_etc(queue->pop, 1, B_CAN_INTERRUPT | B_DO_NOT_RESCHEDULE, 0);
		
		if(error == B_INTERRUPTED)
			continue;
		else if(error != B_NO_ERROR)
			break;
		
		IFQ_DEQUEUE(queue, packet);
		
		if(packet)
			interface->ReceiveFromDevice(packet);
	}
	
	return B_ERROR;
}
