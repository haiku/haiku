//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

// cstdio must be included before PPPModule.h/PPPManager.h because
// dprintf is defined twice with different return values, once with
// void (KernelExport.h) and once with int (stdio.h).
#include <cstdio>
#include <cstring>
#include <core_funcs.h>

// now our headers...
#include <KPPPInterface.h>

// our other classes
#include <PPPControl.h>
#include <KPPPDevice.h>
#include <KPPPLCPExtension.h>
#include <KPPPOptionHandler.h>
#include <KPPPModule.h>
#include <KPPPManager.h>
#include <KPPPUtils.h>

// general helper classes not only belonging to us
#include <LockerHelper.h>

// tools only for us :)
#include "settings_tools.h"

// internal modules
#include "_KPPPMRUHandler.h"
#include "_KPPPAuthenticationHandler.h"
#include "_KPPPPFCHandler.h"


#ifdef _KERNEL_MODE
	#define spawn_thread spawn_kernel_thread
	#define printf dprintf
#endif


// TODO:
// - implement timers with support for settings next time instead of receiving timer
//    events periodically


// needed for redial:
typedef struct redial_info {
	PPPInterface *interface;
	thread_id *thread;
	uint32 delay;
} redial_info;

status_t redial_thread(void *data);

// other functions
status_t interface_deleter_thread(void *data);
status_t call_open_event_thread(void *data);
status_t call_close_event_thread(void *data);


PPPInterface::PPPInterface(uint32 ID, const driver_settings *settings,
		PPPInterface *parent = NULL)
	: PPPLayer("PPPInterface", PPP_INTERFACE_LEVEL),
	fID(ID),
	fSettings(dup_driver_settings(settings)),
	fIfnet(NULL),
	fUpThread(-1),
	fOpenEventThread(-1),
	fCloseEventThread(-1),
	fRedialThread(-1),
	fDialRetry(0),
	fDialRetriesLimit(0),
	fManager(NULL),
	fIdleSince(0),
	fMRU(1500),
	fInterfaceMTU(1498),
	fHeaderLength(2),
	fParent(NULL),
	fIsMultilink(false),
	fAutoRedial(false),
	fDialOnDemand(false),
	fMode(PPP_CLIENT_MODE),
	fLocalPFCState(PPP_PFC_DISABLED),
	fPeerPFCState(PPP_PFC_DISABLED),
	fPFCOptions(0),
	fDevice(NULL),
	fFirstProtocol(NULL),
	fStateMachine(*this),
	fLCP(*this),
	fReportManager(StateMachine().fLock),
	fLock(StateMachine().fLock),
	fDeleteCounter(0)
{
	// add internal modules
	// LCP
	if(!AddProtocol(&LCP())) {
		fInitStatus = B_ERROR;
		return;
	}
	// MRU
	_PPPMRUHandler *mruHandler =
		new _PPPMRUHandler(*this);
	if(!LCP().AddOptionHandler(mruHandler) || mruHandler->InitCheck() != B_OK)
		delete mruHandler;
	// authentication
	_PPPAuthenticationHandler *authenticationHandler =
		new _PPPAuthenticationHandler(*this);
	if(!LCP().AddOptionHandler(authenticationHandler)
			|| authenticationHandler->InitCheck() != B_OK)
		delete authenticationHandler;
	// PFC
	_PPPPFCHandler *pfcHandler =
		new _PPPPFCHandler(fLocalPFCState, fPeerPFCState, *this);
	if(!LCP().AddOptionHandler(pfcHandler) || pfcHandler->InitCheck() != B_OK)
		delete pfcHandler;
	
	// set up dial delays
	fDialRetryDelay = 3000;
		// 3s delay between each new attempt to redial
	fRedialDelay = 1000;
		// 1s delay between lost connection and redial
	
	if(get_module(PPP_INTERFACE_MODULE_NAME, (module_info**) &fManager) != B_OK)
		printf("PPPInterface: Manager module not found!\n");
	
	// are we a multilink subinterface?
	if(parent && parent->IsMultilink()) {
		fParent = parent;
		fParent->AddChild(this);
		fIsMultilink = true;
	}
	
	RegisterInterface();
	
	if(!fSettings) {
		fInitStatus = B_ERROR;
		return;
	}
	
	const char *value;
	
	// get DisonnectAfterIdleSince settings
	value = get_settings_value(PPP_DISONNECT_AFTER_IDLE_SINCE_KEY, fSettings);
	if(!value)
		fDisconnectAfterIdleSince = 0;
	else
		fDisconnectAfterIdleSince = atoi(value) * 1000;
	
	if(fDisconnectAfterIdleSince < 0)
		fDisconnectAfterIdleSince = 0;
	
	// get mode settings
	value = get_settings_value(PPP_MODE_KEY, fSettings);
	if(value && !strcasecmp(value, PPP_SERVER_MODE_VALUE))
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
	if(!LoadModules(fSettings, 0, fSettings->parameter_count)) {
		printf("PPPInterface: Error loading modules!\n");
		fInitStatus = B_ERROR;
	}
}


PPPInterface::~PPPInterface()
{
#if DEBUG
	printf("PPPInterface: Destructor\n");
#endif
	
	++fDeleteCounter;
	
	// make sure we are not accessible by any thread before we continue
	UnregisterInterface();
	
	if(fManager)
		fManager->RemoveInterface(ID());
	
	// Call Down() until we get a lock on an interface that is down.
	// This lock is not released until we are actually deleted.
	while(true) {
		Down();
		fLock.Lock();
		if(State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)
			break;
		fLock.Unlock();
	}
	
	Report(PPP_DESTRUCTION_REPORT, 0, NULL, 0);
		// tell all listeners that we are being destroyed
	
	int32 tmp;
	send_data_with_timeout(fRedialThread, 0, NULL, 0, 200);
		// tell thread that we are being destroyed (200ms timeout)
	wait_for_thread(fRedialThread, &tmp);
	wait_for_thread(fOpenEventThread, &tmp);
	wait_for_thread(fCloseEventThread, &tmp);
	
	while(CountChildren())
		delete ChildAt(0);
	
	delete Device();
	
	while(FirstProtocol()) {
		if(FirstProtocol() == &LCP())
			fFirstProtocol = fFirstProtocol->NextProtocol();
		else
			delete FirstProtocol();
	}
	
	for(int32 index = 0; index < fModules.CountItems(); index++) {
		put_module(fModules.ItemAt(index));
		free(fModules.ItemAt(index));
	}
	
	free_driver_settings(fSettings);
	
	if(Parent())
		Parent()->RemoveChild(this);
	
	if(fManager)
		put_module(PPP_INTERFACE_MODULE_NAME);
}


void
PPPInterface::Delete()
{
	if(atomic_add(&fDeleteCounter, 1) > 0)
		return;
			// only one thread should delete us!
	
	if(fManager)
		fManager->DeleteInterface(ID());
			// This will mark us for deletion.
			// Any subsequent calls to delete_interface() will do nothing.
	else {
		// We were not created by the manager.
		// Spawn a thread that will delete us.
		thread_id interfaceDeleterThread = spawn_thread(interface_deleter_thread,
			"PPPInterface: interface_deleter_thread", B_NORMAL_PRIORITY, this);
		resume_thread(interfaceDeleterThread);
	}
}


status_t
PPPInterface::InitCheck() const
{
#if DEBUG
	printf("PPPInterface: InitCheck(): 0x%lX\n", fInitStatus);
#endif
	
	if(fInitStatus != B_OK)
		return fInitStatus;
	
	if(!fSettings || !fManager)
		return B_ERROR;
	
	// sub-interfaces should have a device
	if(IsMultilink()) {
		if(Parent() && !fDevice)
			return B_ERROR;
	} else if(!fDevice)
		return B_ERROR;
	
	return B_OK;
}


bool
PPPInterface::SetMRU(uint32 MRU)
{
#if DEBUG
	printf("PPPInterface: SetMRU(%ld)\n", MRU);
#endif
	
	if(Device() && MRU > Device()->MTU() - 2)
		return false;
	
	LockerHelper locker(fLock);
	
	fMRU = MRU;
	
	CalculateInterfaceMTU();
	
	return true;
}


status_t
PPPInterface::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_INTERFACE_INFO: {
			if(length < sizeof(ppp_interface_info_t) || !data)
				return B_NO_MEMORY;
			
			ppp_interface_info *info = (ppp_interface_info*) data;
			memset(info, 0, sizeof(ppp_interface_info_t));
			info->settings = Settings();
			if(Ifnet())
				info->if_unit = Ifnet()->if_unit;
			else
				info->if_unit = -1;
			info->mode = Mode();
			info->state = State();
			info->phase = Phase();
			info->localAuthenticationStatus =
				StateMachine().LocalAuthenticationStatus();
			info->peerAuthenticationStatus =
				StateMachine().PeerAuthenticationStatus();
			info->localPFCState = LocalPFCState();
			info->peerPFCState = PeerPFCState();
			info->pfcOptions = PFCOptions();
			info->protocolsCount = CountProtocols();
			info->optionHandlersCount = LCP().CountOptionHandlers();
			info->LCPExtensionsCount = 0;
			info->childrenCount = CountChildren();
			info->MRU = MRU();
			info->interfaceMTU = InterfaceMTU();
			info->dialRetry = fDialRetry;
			info->dialRetriesLimit = fDialRetriesLimit;
			info->dialRetryDelay = DialRetryDelay();
			info->redialDelay = RedialDelay();
			info->idleSince = IdleSince();
			info->disconnectAfterIdleSince = DisconnectAfterIdleSince();
			info->doesDialOnDemand = DoesDialOnDemand();
			info->doesAutoRedial = DoesAutoRedial();
			info->hasDevice = Device();
			info->isMultilink = IsMultilink();
			info->hasParent = Parent();
		} break;
		
		case PPPC_SET_MRU:
			if(length < sizeof(uint32) || !data)
				return B_ERROR;
			
			SetMRU(*((uint32*)data));
		break;
		
		case PPPC_SET_DIAL_ON_DEMAND:
			if(length < sizeof(uint32) || !data)
				return B_NO_MEMORY;
			
			SetDialOnDemand(*((uint32*)data));
		break;
		
		case PPPC_SET_AUTO_REDIAL:
			if(length < sizeof(uint32) || !data)
				return B_NO_MEMORY;
			
			SetAutoRedial(*((uint32*)data));
		break;
		
		case PPPC_ENABLE_REPORTS: {
			if(length < sizeof(ppp_report_request) || !data)
				return B_ERROR;
			
			ppp_report_request *request = (ppp_report_request*) data;
			ReportManager().EnableReports(request->type, request->thread,
				request->flags);
		} break;
		
		case PPPC_DISABLE_REPORTS: {
			if(length < sizeof(ppp_report_request) || !data)
				return B_ERROR;
			
			ppp_report_request *request = (ppp_report_request*) data;
			ReportManager().DisableReports(request->type, request->thread);
		} break;
		
		case PPPC_CONTROL_DEVICE: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			if(control->index != 0 || !Device())
				return B_BAD_INDEX;
			
			return Device()->Control(control->op, control->data, control->length);
		} break;
		
		case PPPC_CONTROL_PROTOCOL: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			PPPProtocol *protocol = ProtocolAt(control->index);
			if(!protocol)
				return B_BAD_INDEX;
			
			return protocol->Control(control->op, control->data, control->length);
		} break;
		
		case PPPC_CONTROL_OPTION_HANDLER: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			PPPOptionHandler *optionHandler = LCP().OptionHandlerAt(control->index);
			if(!optionHandler)
				return B_BAD_INDEX;
			
			return optionHandler->Control(control->op, control->data,
				control->length);
		} break;
		
		case PPPC_CONTROL_LCP_EXTENSION: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			PPPLCPExtension *lcpExtension = LCP().LCPExtensionAt(control->index);
			if(!lcpExtension)
				return B_BAD_INDEX;
			
			return lcpExtension->Control(control->op, control->data,
				control->length);
		} break;
		
		case PPPC_CONTROL_CHILD: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			PPPInterface *child = ChildAt(control->index);
			if(!child)
				return B_BAD_INDEX;
			
			return child->Control(control->op, control->data, control->length);
		} break;
		
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


bool
PPPInterface::SetDevice(PPPDevice *device)
{
#if DEBUG
	printf("PPPInterface: SetDevice(%p)\n", device);
#endif
	
	if(device && &device->Interface() != this)
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
	SetNext(device);
	
	if(fDevice)
		fMRU = fDevice->MTU() - 2;
	
	CalculateInterfaceMTU();
	CalculateBaudRate();
	
	return true;
}


bool
PPPInterface::AddProtocol(PPPProtocol *protocol)
{
	// Find instert position after the last protocol
	// with the same level.
	
#if DEBUG
	printf("PPPInterface: AddProtocol()\n");
#endif
	
	if(!protocol || &protocol->Interface() != this
			|| protocol->Level() == PPP_INTERFACE_LEVEL)
		return false;
	
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	PPPProtocol *current = fFirstProtocol, *previous = NULL;
	
	while(current) {
		if(current->Level() < protocol->Level())
			break;
		
		previous = current;
		current = current->NextProtocol();
	}
	
	if(!current) {
		if(!previous)
			fFirstProtocol = protocol;
		else
			previous->SetNextProtocol(protocol);
		
		// set up the last protocol in the chain
		protocol->SetNextProtocol(NULL);
			// this also sets next to NULL
		protocol->SetNext(this);
			// we need to set us as the next layer for the last protocol
	} else {
		protocol->SetNextProtocol(current);
		
		if(!previous)
			fFirstProtocol = protocol;
		else
			previous->SetNextProtocol(protocol);
	}
	
	if(protocol->Level() < PPP_PROTOCOL_LEVEL)
		CalculateInterfaceMTU();
	
	if(IsUp() || Phase() >= protocol->ActivationPhase())
		protocol->Up();
	
	return true;
}


bool
PPPInterface::RemoveProtocol(PPPProtocol *protocol)
{
#if DEBUG
	printf("PPPInterface: RemoveProtocol()\n");
#endif
	
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	PPPProtocol *current = fFirstProtocol, *previous = NULL;
	
	while(current) {
		if(current == protocol) {
			if(!protocol->IsDown())
				protocol->Down();
			
			if(previous) {
				previous->SetNextProtocol(current->NextProtocol());
				
				// set us as next layer if needed
				if(!previous->Next())
					previous->SetNext(this);
			} else
				fFirstProtocol = current->NextProtocol();
			
			current->SetNextProtocol(NULL);
			
			CalculateInterfaceMTU();
			
			return true;
		}
		
		previous = current;
		current = current->NextProtocol();
	}
	
	return false;
}


int32
PPPInterface::CountProtocols() const
{
	PPPProtocol *protocol = FirstProtocol();
	
	int32 count = 0;
	for(; protocol; protocol = protocol->NextProtocol())
		++count;
	
#if DEBUG
	printf("PPPInterface: CountProtocols(): %ld\n", count);
#endif
	
	return count;
}


PPPProtocol*
PPPInterface::ProtocolAt(int32 index) const
{
#if DEBUG
	printf("PPPInterface: ProtocolAt(%ld)\n", index);
#endif
	
	PPPProtocol *protocol = FirstProtocol();
	
	int32 currentIndex = 0;
	for(; protocol && currentIndex != index; protocol = protocol->NextProtocol())
		++currentIndex;
	
	return protocol;
}


PPPProtocol*
PPPInterface::ProtocolFor(uint16 protocolNumber, PPPProtocol *start = NULL) const
{
#if DEBUG
	printf("PPPInterface: ProtocolFor(0x%X)\n", protocolNumber);
#endif
	
	PPPProtocol *current = start ? start : FirstProtocol();
	
	for(; current; current = current->NextProtocol()) {
		if(current->ProtocolNumber() == protocolNumber
				|| (current->Flags() & PPP_INCLUDES_NCP
					&& current->ProtocolNumber() & 0x7FFF == protocolNumber & 0x7FFF))
			return current;
	}
	
	return current;
}


bool
PPPInterface::AddChild(PPPInterface *child)
{
#if DEBUG
	printf("PPPInterface: AddChild()\n");
#endif
	
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
#if DEBUG
	printf("PPPInterface: RemoveChild()\n");
#endif
	
	LockerHelper locker(fLock);
	
	if(!fChildren.RemoveItem(child))
		return false;
	
	child->SetParent(NULL);
	
	// parents cannot exist without their children
	if(CountChildren() == 0 && fManager && Ifnet())
		Delete();
	
	return true;
}


PPPInterface*
PPPInterface::ChildAt(int32 index) const
{
#if DEBUG
		printf("PPPInterface: ChildAt(%ld)\n", index);
#endif
	
	PPPInterface *child = fChildren.ItemAt(index);
	
	if(child == fChildren.GetDefaultItem())
		return NULL;
	
	return child;
}


void
PPPInterface::SetAutoRedial(bool autoRedial = true)
{
#if DEBUG
	printf("PPPInterface: SetAutoRedial(%s)\n", autoRedial ? "true" : "false");
#endif
	
	if(Mode() != PPP_CLIENT_MODE)
		return;
	
	LockerHelper locker(fLock);
	
	fAutoRedial = autoRedial;
}


void
PPPInterface::SetDialOnDemand(bool dialOnDemand = true)
{
	// All protocols must check if DialOnDemand was enabled/disabled after this
	// interface went down. This is the only situation where a change is relevant.
	
#if DEBUG
	printf("PPPInterface: SetDialOnDemand(%s)\n", dialOnDemand ? "true" : "false");
#endif
	
	// Only clients support DialOnDemand.
	if(Mode() != PPP_CLIENT_MODE) {
#if DEBUG
		printf("PPPInterface::SetDialOnDemand(): Wrong mode!\n");
#endif
		fDialOnDemand = false;
		return;
	} else if(DoesDialOnDemand() == dialOnDemand)
		return;
	
	LockerHelper locker(fLock);
	
	fDialOnDemand = dialOnDemand;
	
	// Do not allow changes when we are disconnected (only main interfaces).
	// This would make no sense because
	// - enabling: this cannot happen because hidden interfaces are deleted if they
	//    could not establish a connection (the user cannot access hidden interfaces)
	// - disabling: the interface disappears as seen from the user, so we delete it
	if(!Parent() && State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE) {
		if(!dialOnDemand)
			Delete();
				// as long as the protocols were not configured we can just delete us
		
		return;
	}
	
	// check if we need to set/unset flags
	if(dialOnDemand) {
		if(Ifnet())
			Ifnet()->if_flags |= IFF_UP;
	} else if(!dialOnDemand && Phase() < PPP_ESTABLISHED_PHASE) {
		if(Ifnet())
			Ifnet()->if_flags &= ~IFF_UP;
	}
}


bool
PPPInterface::SetPFCOptions(uint8 pfcOptions)
{
#if DEBUG
	printf("PPPInterface: SetPFCOptions(0x%X)\n", pfcOptions);
#endif
	
	if(PFCOptions() & PPP_FREEZE_PFC_OPTIONS)
		return false;
	
	fPFCOptions = pfcOptions;
	return true;
}


bool
PPPInterface::Up()
{
#if DEBUG
	printf("PPPInterface: Up()\n");
#endif
	
	if(InitCheck() != B_OK || Phase() == PPP_TERMINATION_PHASE)
		return false;
	
	if(IsUp())
		return true;
	
	ppp_report_packet report;
	thread_id me = find_thread(NULL), sender;
	
	// One thread has to do the real task while all other threads are observers.
	// Lock needs timeout because destructor could have locked the interface.
	while(fLock.LockWithTimeout(100000) != B_NO_ERROR)
		if(fDeleteCounter > 0)
			return false;
	if(fUpThread == -1)
		fUpThread = me;
	
	ReportManager().EnableReports(PPP_CONNECTION_REPORT, me, PPP_WAIT_FOR_REPLY);
	
	// fUpThread/fRedialThread tells the state machine to go up (using a new thread
	// because we might not receive report messages otherwise)
	if(me == fUpThread || me == fRedialThread) {
		if(fOpenEventThread != -1) {
			int32 tmp;
			wait_for_thread(fOpenEventThread, &tmp);
		}
		fOpenEventThread = spawn_thread(call_open_event_thread,
			"PPPInterface: call_open_event_thread", B_NORMAL_PRIORITY, this);
		resume_thread(fOpenEventThread);
	}
	fLock.Unlock();
	
	if(me == fRedialThread && me != fUpThread)
		return true;
			// the redial thread is doing a DialRetry in this case (fUpThread
			// is waiting for new reports)
	
	while(true) {
		if(IsUp()) {
			// lock needs timeout because destructor could have locked the interface
			while(!fLock.LockWithTimeout(100000) != B_NO_ERROR)
				if(fDeleteCounter > 0)
					return true;
			
			if(me == fUpThread) {
				fDialRetry = 0;
				fUpThread = -1;
			}
			
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
			fLock.Unlock();
			
			return true;
		}
		
		// A wrong code usually happens when the redial thread gets notified
		// of a Down() request. In that case a report will follow soon, so
		// this can be ignored.
		if(receive_data(&sender, &report, sizeof(report)) != PPP_REPORT_CODE)
			continue;
		
#if DEBUG
		printf("PPPInterface::Up(): Report: Type = %ld Code = %ld\n", report.type,
			report.code);
#endif
		
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
				send_data_with_timeout(fRedialThread, 0, NULL, 0, 200);
					// notify redial thread that we do not need it anymore
			}
			
			PPP_REPLY(sender, B_OK);
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
			return true;
		} else if(report.code == PPP_REPORT_DOWN_SUCCESSFUL
				|| report.code == PPP_REPORT_UP_ABORTED
				|| report.code == PPP_REPORT_LOCAL_AUTHENTICATION_FAILED
				|| report.code == PPP_REPORT_PEER_AUTHENTICATION_FAILED) {
			if(me == fUpThread) {
				fDialRetry = 0;
				fUpThread = -1;
				
				if(!DoesDialOnDemand() && report.code != PPP_REPORT_DOWN_SUCCESSFUL)
					Delete();
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
						Delete();
					
					PPP_REPLY(sender, B_OK);
					ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
					return false;
				} else {
					++fDialRetry;
					PPP_REPLY(sender, B_OK);
					Redial(DialRetryDelay());
					continue;
				}
			} else if(report.code == PPP_REPORT_CONNECTION_LOST) {
				// the state machine knows that we are going up and leaves
				// the redial task to us
				if(DoesAutoRedial() && fDialRetry < fDialRetriesLimit) {
					++fDialRetry;
					PPP_REPLY(sender, B_OK);
					Redial(DialRetryDelay());
					continue;
				} else {
					fDialRetry = 0;
					fUpThread = -1;
					PPP_REPLY(sender, B_OK);
					ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
					
					if(!DoesDialOnDemand()
							&& report.code != PPP_REPORT_DOWN_SUCCESSFUL)
						Delete();
					
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
#if DEBUG
	printf("PPPInterface: Down()\n");
#endif
	
	if(InitCheck() != B_OK)
		return false;
	
	send_data_with_timeout(fRedialThread, 0, NULL, 0, 200);
		// the redial thread should be notified that the user wants to disconnect
	
	// this locked section guarantees that there are no state changes before we
	// enable the connection reports
	LockerHelper locker(fLock);
	if(State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)
		return true;
	
	ReportManager().EnableReports(PPP_CONNECTION_REPORT, find_thread(NULL));
	
	thread_id sender;
	ppp_report_packet report;
	
	if(fCloseEventThread != -1) {
		int32 tmp;
		wait_for_thread(fCloseEventThread, &tmp);
	}
	fCloseEventThread = spawn_thread(call_close_event_thread,
		"PPPInterface: call_close_event_thread", B_NORMAL_PRIORITY, this);
	resume_thread(fCloseEventThread);
	locker.UnlockNow();
	
	while(true) {
		if(receive_data(&sender, &report, sizeof(report)) != PPP_REPORT_CODE)
			continue;
		
		if(report.type == PPP_DESTRUCTION_REPORT)
			return true;
		
		if(report.type != PPP_CONNECTION_REPORT)
			continue;
		
		if(report.code == PPP_REPORT_DOWN_SUCCESSFUL
				|| report.code == PPP_REPORT_UP_ABORTED
				|| (State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)) {
			ReportManager().DisableReports(PPP_CONNECTION_REPORT, find_thread(NULL));
			break;
		}
	}
	
	if(!DoesDialOnDemand())
		Delete();
	
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
#if DEBUG
	printf("PPPInterface: LoadModules()\n");
#endif
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	ppp_module_key_type type;
		// which type key was used for loading this module?
	
	const char *name = NULL;
	
	// multilink handling
	for(int32 index = start;
			index < settings->parameter_count && index < start + count; index++) {
		if(!strcasecmp(settings->parameters[index].name, PPP_MULTILINK_KEY)
				&& settings->parameters[index].value_count > 0) {
			if(!LoadModule(settings->parameters[index].values[0],
					&settings->parameters[index], PPP_MULTILINK_KEY_TYPE))
				return false;
			break;
		}
	}
	
	// are we a multilink main interface?
	if(IsMultilink() && !Parent()) {
		// main interfaces only load the multilink module
		// and create a child using their settings
		fManager->CreateInterface(settings, ID());
		return true;
	}
	
	for(int32 index = start;
			index < settings->parameter_count && index < start + count; index++) {
		type = PPP_UNDEFINED_KEY_TYPE;
		
		name = settings->parameters[index].name;
		
		if(!strcasecmp(name, PPP_LOAD_MODULE_KEY))
			type = PPP_LOAD_MODULE_KEY_TYPE;
		else if(!strcasecmp(name, PPP_DEVICE_KEY))
			type = PPP_DEVICE_KEY_TYPE;
		else if(!strcasecmp(name, PPP_PROTOCOL_KEY))
			type = PPP_PROTOCOL_KEY_TYPE;
		else if(!strcasecmp(name, PPP_AUTHENTICATOR_KEY))
			type = PPP_AUTHENTICATOR_KEY_TYPE;
		
		if(type >= 0)
			for(int32 value_id = 0; value_id < settings->parameters[index].value_count;
					value_id++)
				if(!LoadModule(settings->parameters[index].values[value_id],
						&settings->parameters[index], type))
					return false;
	}
	
	return true;
}


bool
PPPInterface::LoadModule(const char *name, driver_parameter *parameter,
	ppp_module_key_type type)
{
#if DEBUG
	printf("PPPInterface: LoadModule(%s)\n", name ? name : "XXX: NO NAME");
#endif
	
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
	
	return module->add_to(Parent() ? *Parent() : *this, this, parameter, type);
}


bool
PPPInterface::IsAllowedToSend() const
{
	return true;
}


status_t
PPPInterface::Send(struct mbuf *packet, uint16 protocolNumber)
{
#if DEBUG
	printf("PPPInterface: Send(0x%X)\n", protocolNumber);
#endif
	
	if(!packet)
		return B_ERROR;
	
	// we must pass the basic tests like:
	// do we have a device?
	// did we load all modules?
	if(InitCheck() != B_OK || !Device()) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// test whether are going down
	if(Phase() == PPP_TERMINATION_PHASE) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// go up if DialOnDemand enabled and we are down
	if(DoesDialOnDemand()
			&& (Phase() == PPP_DOWN_PHASE
				|| Phase() == PPP_ESTABLISHMENT_PHASE)
			&& !Up()) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// find the protocol handler for the current protocol number
	PPPProtocol *protocol = ProtocolFor(protocolNumber);
	while(protocol && !protocol->IsEnabled())
		protocol = protocol->NextProtocol() ?
			ProtocolFor(protocolNumber, protocol->NextProtocol()) : NULL;
	
	// make sure that protocol is allowed to send and everything is up
	if(!Device()->IsUp() || !protocol || !protocol->IsEnabled()
			|| !IsProtocolAllowed(*protocol)){
		m_freem(packet);
		return B_ERROR;
	}
	
	// encode in ppp frame and consider using PFC
	if(UseLocalPFC() && protocolNumber & 0xFF00 == 0) {
		M_PREPEND(packet, 1);
		
		if(packet == NULL)
			return B_ERROR;
		
		uint8 *header = mtod(packet, uint8*);
		*header = protocolNumber & 0xFF;
	} else {
		M_PREPEND(packet, 2);
		
		if(packet == NULL)
			return B_ERROR;
		
		// set protocol (the only header field)
		protocolNumber = htons(protocolNumber);
		uint16 *header = mtod(packet, uint16*);
		*header = protocolNumber;
	}
	
	fIdleSince = real_time_clock();
	
	// pass to device/children
	if(!IsMultilink() || Parent()) {
		// check if packet is too big for device
		if((packet->m_flags & M_PKTHDR && (uint32) packet->m_pkthdr.len > MRU())
				|| packet->m_len > MRU()) {
			m_freem(packet);
			return B_ERROR;
		}
		
		return SendToNext(packet, 0);
			// this is normally the device, but there can be something inbetween
	} else {
		// the multilink protocol should have sent it to some child interface
		m_freem(packet);
		return B_ERROR;
	}
}


status_t
PPPInterface::Receive(struct mbuf *packet, uint16 protocolNumber)
{
#if DEBUG
		printf("PPPInterface: Receive(0x%X)\n", protocolNumber);
#endif
	
	if(!packet)
		return B_ERROR;
	
	int32 result = PPP_REJECTED;
		// assume we have no handler
	
	// Set our interface as the receiver.
	// The real netstack protocols (IP, IPX, etc.) might get confused if our
	// interface is a main interface and at the same time is not registered
	// because then there is no receiver interface.
	// PPP NCPs should be aware of that!
	if(packet->m_flags & M_PKTHDR && Ifnet() != NULL)
		packet->m_pkthdr.rcvif = Ifnet();
	
	// Find handler and let it parse the packet.
	// The handler does need not be up because if we are a server
	// the handler might be upped by this packet.
	PPPProtocol *protocol = ProtocolFor(protocolNumber);
	for(; protocol;
			protocol = protocol->NextProtocol() ?
				ProtocolFor(protocolNumber, protocol->NextProtocol()) : NULL) {
		if(!protocol->IsEnabled() || !IsProtocolAllowed(*protocol))
			continue;
				// skip handler if disabled or not allowed
		
		result = protocol->Receive(packet, protocolNumber);
		if(result == PPP_UNHANDLED)
			continue;
		
		return result;
	}
	
	// maybe the parent interface can handle the packet
	if(Parent())
		return Parent()->Receive(packet, protocolNumber);
	
	if(result == PPP_UNHANDLED) {
		m_freem(packet);
		return PPP_DISCARDED;
	} else {
		StateMachine().RUCEvent(packet, protocolNumber);
		return PPP_REJECTED;
	}
}


status_t
PPPInterface::ReceiveFromDevice(struct mbuf *packet)
{
#if DEBUG
	printf("PPPInterface: ReceiveFromDevice()\n");
#endif
	
	if(!packet)
		return B_ERROR;
	
	if(InitCheck() != B_OK) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// decode ppp frame and recognize PFC
	uint16 protocolNumber = *mtod(packet, uint8*);
	if(protocolNumber == 0)
		m_adj(packet, 1);
	else {
		protocolNumber = ntohs(*mtod(packet, uint16*));
		m_adj(packet, 2);
	}
	
	return Receive(packet, protocolNumber);
}


void
PPPInterface::Pulse()
{
	if(fDeleteCounter > 0)
		return;
			// we have no pulse when we are dead ;)
	
	// check our idle time and disconnect if needed
	if(fDisconnectAfterIdleSince > 0 && fIdleSince != 0
			&& fIdleSince - real_time_clock() >= fDisconnectAfterIdleSince) {
		StateMachine().CloseEvent();
		return;
	}
	
	if(Device())
		Device()->Pulse();
	
	PPPProtocol *protocol = FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol())
		protocol->Pulse();
}


bool
PPPInterface::RegisterInterface()
{
#if DEBUG
	printf("PPPInterface: RegisterInterface()\n");
#endif
	
	if(fIfnet)
		return true;
			// we are already registered
	
	LockerHelper locker(fLock);
	
	// only MainInterfaces get an ifnet
	if(IsMultilink() && Parent() && Parent()->RegisterInterface())
		return true;
	
	if(!fManager)
		return false;
	
	fIfnet = fManager->RegisterInterface(ID());
	
	if(!fIfnet)
		return false;
	
	if(DoesDialOnDemand())
		fIfnet->if_flags |= IFF_UP;
	
	CalculateInterfaceMTU();
	CalculateBaudRate();
	
	return true;
}


bool
PPPInterface::UnregisterInterface()
{
#if DEBUG
	printf("PPPInterface: UnregisterInterface()\n");
#endif
	
	if(!fIfnet)
		return true;
			// we are already unregistered
	
	LockerHelper locker(fLock);
	
	// only MainInterfaces get an ifnet
	if(IsMultilink() && Parent())
		return true;
	
	if(!fManager)
		return false;
	
	fManager->UnregisterInterface(ID());
		// this will delete fIfnet, so do not access it anymore!
	fIfnet = NULL;
	
	return true;
}


// called by PPPManager: manager routes stack ioctls to interface
status_t
PPPInterface::StackControl(uint32 op, void *data)
{
#if DEBUG
	printf("PPPInterface: StackControl(0x%lX)\n", op);
#endif
	
	switch(op) {
		default:
			return StackControlEachHandler(op, data);
	}
	
	return B_OK;
}


// used by ControlEachHandler()
template<class T>
class CallStackControl {
	public:
		inline CallStackControl(uint32 op, void *data, status_t& result)
			: fOp(op), fData(data), fResult(result) {}
		inline void operator() (T *item)
			{
				if(!item || !item->IsEnabled())
					return;
				status_t tmp = item->StackControl(fOp, fData);
				if(tmp == B_OK && fResult == B_BAD_VALUE)
					fResult = B_OK;
				else if(tmp != B_BAD_VALUE)
					fResult = tmp;
			}
	private:
		uint32 fOp;
		void *fData;
		status_t& fResult;
};

// This calls Control() with the given parameters for each handler.
// Return values:
//  B_OK: all handlers returned B_OK
//  B_BAD_VALUE: no handler was found
//  any other value: the error value that was returned by the last handler that failed
status_t
PPPInterface::StackControlEachHandler(uint32 op, void *data)
{
#if DEBUG
	printf("PPPInterface: StackControlEachHandler(0x%lX)\n", op);
#endif
	
	status_t result = B_BAD_VALUE, tmp;
	
	PPPProtocol *protocol = FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol()) {
		tmp = protocol->StackControl(op, data);
		if(tmp == B_OK && result == B_BAD_VALUE)
			result = B_OK;
		else if(tmp != B_BAD_VALUE)
			result = tmp;
	}
	
	ForEachItem(LCP().fLCPExtensions,
		CallStackControl<PPPLCPExtension>(op, data, result));
	ForEachItem(LCP().fOptionHandlers,
		CallStackControl<PPPOptionHandler>(op, data, result));
	
	return result;
}


void
PPPInterface::CalculateInterfaceMTU()
{
#if DEBUG
	printf("PPPInterface: CalculateInterfaceMTU()\n");
#endif
	
	fInterfaceMTU = fMRU;
	fHeaderLength = 2;
	
	// sum all headers (the protocol field is not counted)
	PPPProtocol *protocol = FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol()) {
		if(protocol->Level() < PPP_PROTOCOL_LEVEL)
			fHeaderLength += protocol->Overhead();
	}
	
	fInterfaceMTU -= fHeaderLength;
	
	if(Ifnet()) {
		Ifnet()->if_mtu = fInterfaceMTU;
		Ifnet()->if_hdrlen = fHeaderLength;
	}
	
	if(Parent())
		Parent()->CalculateInterfaceMTU();
}


void
PPPInterface::CalculateBaudRate()
{
#if DEBUG
	printf("PPPInterface: CalculateBaudRate()\n");
#endif
	
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
PPPInterface::Redial(uint32 delay)
{
#if DEBUG
	printf("PPPInterface: Redial(%ld)\n", delay);
#endif
	
	if(fRedialThread != -1)
		return;
	
	// start a new thread that calls our Up() method
	redial_info info;
	info.interface = this;
	info.thread = &fRedialThread;
	info.delay = delay;
	
	fRedialThread = spawn_thread(redial_thread, "PPPInterface: redial_thread",
		B_NORMAL_PRIORITY, NULL);
	
	resume_thread(fRedialThread);
	
	send_data(fRedialThread, 0, &info, sizeof(redial_info));
}


status_t
redial_thread(void *data)
{
	redial_info info;
	thread_id sender;
	int32 code;
	
	receive_data(&sender, &info, sizeof(redial_info));
	
	// we try to receive data instead of snooze, so we can quit on destruction
	if(receive_data_with_timeout(&sender, &code, NULL, 0, info.delay) == B_OK) {
		*info.thread = -1;
		info.interface->Report(PPP_CONNECTION_REPORT, PPP_REPORT_UP_ABORTED, NULL, 0);
		return B_OK;
	}
	
	info.interface->Up();
	*info.thread = -1;
	
	return B_OK;
}


// ----------------------------------
// Function: interface_deleter_thread
// ----------------------------------
// The destructor is private, so this thread function cannot delete our interface.
// To solve this problem we create a 'fake' class PPPManager (friend of PPPInterface)
// which is only defined here (the real class is defined in the ppp interface module).
class PPPManager {
	public:
		PPPManager() {}
		
		void Delete(PPPInterface *interface)
			{ delete interface; }
		void CallOpenEvent(PPPInterface *interface)
		{
			while(interface->fLock.LockWithTimeout(100000) != B_NO_ERROR)
				if(interface->fDeleteCounter > 0)
					return;
			interface->CallOpenEvent();
			interface->fOpenEventThread = -1;
			interface->fLock.Unlock();
		}
		void CallCloseEvent(PPPInterface *interface)
		{
			while(interface->fLock.LockWithTimeout(100000) != B_NO_ERROR)
				if(interface->fDeleteCounter > 0)
					return;
			interface->CallCloseEvent();
			interface->fCloseEventThread = -1;
			interface->fLock.Unlock();
		}
};


status_t
interface_deleter_thread(void *data)
{
	PPPManager manager;
	manager.Delete((PPPInterface*) data);
	
	return B_OK;
}


status_t
call_open_event_thread(void *data)
{
	PPPManager manager;
	manager.CallOpenEvent((PPPInterface*) data);
	
	return B_OK;
}


status_t
call_close_event_thread(void *data)
{
	PPPManager manager;
	manager.CallCloseEvent((PPPInterface*) data);
	
	return B_OK;
}
