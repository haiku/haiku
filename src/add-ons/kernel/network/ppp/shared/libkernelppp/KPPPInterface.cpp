//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

/*!	\class KPPPInterface
	\brief The kernel representation of a PPP interface.
	
	This class is never created by the programmer directly. Instead, the PPP manager
	kernel module should be used. \n
	KPPPInterface handles all interface-specific commands from userspace and it
	passes packets to their receiver or sends them to the device. Additionally,
	it contains the KPPPLCP object with represents the LCP protocol and the
	KPPPStateMachine object which represents the state machine. \n
	All PPP modules are loaded from here. \n
	Protocols and encapsulators should be added to this class. LCP-specific extensions
	belong to the KPPPLCP object. \n
	Multilink support is distributed between KPPPInterface and KPPPStateMachine.
*/

// cstdio must be included before KPPPModule.h/KPPPManager.h because
// ddprintf is defined twice with different return values, once with
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


// TODO:
// - implement timers with support for settings next time instead of receiving timer
//    events periodically
// - add missing settings support (DialRetryDelay, etc.)


//!	Private structure needed for redialing.
typedef struct redial_info {
	KPPPInterface *interface;
	thread_id *thread;
	uint32 delay;
} redial_info;

status_t redial_thread(void *data);

// other functions
status_t interface_deleter_thread(void *data);
status_t call_open_event_thread(void *data);
status_t call_close_event_thread(void *data);


/*!	\brief Creates a new interface.
	
	\param name Name of the PPP interface description file.
	\param entry The PPP manager passes an internal structure to the constructor.
	\param ID The interface's ID.
	\param settings (Optional): If no name is given you must pass the settings here.
	\param profile (Optional): Overriding profile for this interface.
	\param parent (Optional): Interface's parent (only used for multilink interfaces).
	
	\sa KPPPProfile
*/
KPPPInterface::KPPPInterface(const char *name, ppp_interface_entry *entry,
		ppp_interface_id ID, const driver_settings *settings,
		const driver_settings *profile, KPPPInterface *parent = NULL)
	: KPPPLayer(name, PPP_INTERFACE_LEVEL, 2),
	fID(ID),
	fSettings(NULL),
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
	fProfile(*this),
	fReportManager(StateMachine().fLock),
	fLock(StateMachine().fLock),
	fDeleteCounter(0)
{
	entry->interface = this;
	
	if(name) {
		// load settings from description file
		char path[B_PATH_NAME_LENGTH];
		sprintf(path, "pppidf/%s", name);
			// XXX: TODO: change base path to "/etc/ppp" when settings API supports it
		
		void *handle = load_driver_settings(path);
		if(!handle) {
			fInitStatus = B_ERROR;
			return;
		}
		
		fSettings = dup_driver_settings(get_driver_settings(handle));
		unload_driver_settings(handle);
	} else
		fSettings = dup_driver_settings(settings);
			// use the given settings
	
	if(!fSettings) {
		fInitStatus = B_ERROR;
		return;
	}
	fProfile.LoadSettings(profile, fSettings);
	
	// add internal modules
	// LCP
	if(!AddProtocol(&LCP())) {
		fInitStatus = B_ERROR;
		return;
	}
	// MRU
	_KPPPMRUHandler *mruHandler =
		new _KPPPMRUHandler(*this);
	if(!LCP().AddOptionHandler(mruHandler) || mruHandler->InitCheck() != B_OK) {
		dprintf("KPPPInterface: Could not add MRU handler!\n");
		delete mruHandler;
	}
	// authentication
	_KPPPAuthenticationHandler *authenticationHandler =
		new _KPPPAuthenticationHandler(*this);
	if(!LCP().AddOptionHandler(authenticationHandler)
			|| authenticationHandler->InitCheck() != B_OK) {
		dprintf("KPPPInterface: Could not add authentication handler!\n");
		delete authenticationHandler;
	}
	// PFC
	_KPPPPFCHandler *pfcHandler =
		new _KPPPPFCHandler(fLocalPFCState, fPeerPFCState, *this);
	if(!LCP().AddOptionHandler(pfcHandler) || pfcHandler->InitCheck() != B_OK) {
		dprintf("KPPPInterface: Could not add PFC handler!\n");
		delete pfcHandler;
	}
	
	// set up dial delays
	fDialRetryDelay = 3000;
		// 3s delay between each new attempt to redial
	fRedialDelay = 1000;
		// 1s delay between lost connection and redial
	
	if(get_module(PPP_INTERFACE_MODULE_NAME, (module_info**) &fManager) != B_OK)
		dprintf("KPPPInterface: Manager module not found!\n");
	
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
	
	SetAutoRedial(
		get_boolean_value(
		get_settings_value(PPP_AUTO_REDIAL_KEY, fSettings),
		false)
		);
		// auto redial is disabled by default
	
	// load all protocols and the device
	if(!LoadModules(fSettings, 0, fSettings->parameter_count)) {
		dprintf("KPPPInterface: Error loading modules!\n");
		fInitStatus = B_ERROR;
	}
}


//!	Destructor: Disconnects and marks interface for deletion.
KPPPInterface::~KPPPInterface()
{
#if DEBUG
	dprintf("KPPPInterface: Destructor\n");
#endif
	
	++fDeleteCounter;
	
	// tell protocols to uninit (remove routes, etc.)
	KPPPProtocol *protocol = FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol())
		protocol->Uninit();
	
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
	
	Report(PPP_DESTRUCTION_REPORT, 0, &fID, sizeof(ppp_interface_id));
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
				// destructor removes protocol from list
	}
	
	for(int32 index = 0; index < fModules.CountItems(); index++) {
		put_module(fModules.ItemAt(index));
		delete[] fModules.ItemAt(index);
	}
	
	free_driver_settings(fSettings);
	
	if(Parent())
		Parent()->RemoveChild(this);
	
	if(fManager)
		put_module(PPP_INTERFACE_MODULE_NAME);
}


//!	Marks interface for deletion.
void
KPPPInterface::Delete()
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
		thread_id interfaceDeleterThread
			= spawn_kernel_thread(interface_deleter_thread,
				"KPPPInterface: interface_deleter_thread", B_NORMAL_PRIORITY, this);
		resume_thread(interfaceDeleterThread);
	}
}


//!	Returns if interface was initialized correctly.
status_t
KPPPInterface::InitCheck() const
{
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


//!	Sets interface MRU.
bool
KPPPInterface::SetMRU(uint32 MRU)
{
#if DEBUG
	dprintf("KPPPInterface: SetMRU(%ld)\n", MRU);
#endif
	
	if(Device() && MRU > Device()->MTU() - 2)
		return false;
	
	LockerHelper locker(fLock);
	
	fMRU = MRU;
	
	CalculateInterfaceMTU();
	
	return true;
}


//!	Returns number of bytes spent for protocol overhead. Includes device overhead.
uint32
KPPPInterface::PacketOverhead() const
{
	uint32 overhead = fHeaderLength + 2;
	
	if(Device())
		overhead += Device()->Overhead();
	
	return overhead;
}


/*!	\brief Allows accessing additional functions.
	
	This is normally called by userland apps to get information about the interface.
	
	\param op The op value (see ppp_control_ops enum).
	\param data (Optional): Additional data may be needed for this op.
	\param length Length of data.
	
	\return
		- \c B_OK: \c Control() was successful.
		- \c B_ERROR: Either \a length is too small or data is NULL.
		- \c B_BAD_INDEX: Wrong index (e.g.: when accessing interface submodules).
		- \c B_BAD_VALUE: Unknown op.
		- Return value of submodule (when controlling one).
*/
status_t
KPPPInterface::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_INTERFACE_INFO: {
			if(length < sizeof(ppp_interface_info_t) || !data)
				return B_ERROR;
			
			ppp_interface_info *info = (ppp_interface_info*) data;
			memset(info, 0, sizeof(ppp_interface_info_t));
			if(Name())
				strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
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
				return B_ERROR;
			
			SetDialOnDemand(*((uint32*)data));
		break;
		
		case PPPC_SET_AUTO_REDIAL:
			if(length < sizeof(uint32) || !data)
				return B_ERROR;
			
			SetAutoRedial(*((uint32*)data));
		break;
		
		case PPPC_HAS_INTERFACE_SETTINGS:
			if(length < sizeof(driver_settings) || !data)
				return B_ERROR;
			
			if(equal_interface_settings(Settings(), (driver_settings*) data))
				return B_OK;
			else
				return B_ERROR;
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
		
		case PPPC_SET_PROFILE: {
			if(!data)
				return B_ERROR;
			
			driver_settings *profile = (driver_settings*) data;
			fProfile.LoadSettings(profile, fSettings);
			
			UpdateProfile();
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
			KPPPProtocol *protocol = ProtocolAt(control->index);
			if(!protocol)
				return B_BAD_INDEX;
			
			return protocol->Control(control->op, control->data, control->length);
		} break;
		
		case PPPC_CONTROL_OPTION_HANDLER: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			KPPPOptionHandler *optionHandler = LCP().OptionHandlerAt(control->index);
			if(!optionHandler)
				return B_BAD_INDEX;
			
			return optionHandler->Control(control->op, control->data,
				control->length);
		} break;
		
		case PPPC_CONTROL_LCP_EXTENSION: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			KPPPLCPExtension *lcpExtension = LCP().LCPExtensionAt(control->index);
			if(!lcpExtension)
				return B_BAD_INDEX;
			
			return lcpExtension->Control(control->op, control->data,
				control->length);
		} break;
		
		case PPPC_CONTROL_CHILD: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			KPPPInterface *child = ChildAt(control->index);
			if(!child)
				return B_BAD_INDEX;
			
			return child->Control(control->op, control->data, control->length);
		} break;
		
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


/*!	\brief Sets a new device for this interface.
	
	A device add-on should call this method to register itself. The best place to do
	this is in your module's \c add_to() function.
	
	\param device The device object.
	
	\return \c true if successful or \false otherwise.
	
	\sa KPPPDevice
	\sa kppp_module_info
*/
bool
KPPPInterface::SetDevice(KPPPDevice *device)
{
#if DEBUG
	dprintf("KPPPInterface: SetDevice(%p)\n", device);
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


/*!	\brief Adds a new protocol to this interface.
	
	NOTE: You can only add protocols in \c PPP_DOWN_PHASE. \n
	A protocol add-on should call this method to register itself. The best place to do
	this is in your module's \c add_to() function.
	
	\param protocol The protocol object.
	
	\return \c true if successful or \false otherwise.
	
	\sa KPPPProtocol
	\sa kppp_module_info
*/
bool
KPPPInterface::AddProtocol(KPPPProtocol *protocol)
{
	// Find instert position after the last protocol
	// with the same level.
	
#if DEBUG
	dprintf("KPPPInterface: AddProtocol(%X)\n",
		protocol ? protocol->ProtocolNumber() : 0);
#endif
	
	if(!protocol || &protocol->Interface() != this
			|| protocol->Level() == PPP_INTERFACE_LEVEL)
		return false;
	
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	KPPPProtocol *current = fFirstProtocol, *previous = NULL;
	
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


/*!	\brief Removes a protocol from this interface.
	
	NOTE: You can only remove protocols in \c PPP_DOWN_PHASE. \n
	A protocol add-on should call this method to remove itself explicitly from the
	interface. \n
	Normally, this method is called in KPPPProtocol's destructor. Do not call it
	yourself unless you know what you do!
	
	\param protocol The protocol object.
	
	\return \c true if successful or \false otherwise.
*/
bool
KPPPInterface::RemoveProtocol(KPPPProtocol *protocol)
{
#if DEBUG
	dprintf("KPPPInterface: RemoveProtocol(%X)\n",
		protocol ? protocol->ProtocolNumber() : 0);
#endif
	
	LockerHelper locker(fLock);
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	KPPPProtocol *current = fFirstProtocol, *previous = NULL;
	
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


//!	Returns the number of protocol modules belonging to this interface.
int32
KPPPInterface::CountProtocols() const
{
	KPPPProtocol *protocol = FirstProtocol();
	
	int32 count = 0;
	for(; protocol; protocol = protocol->NextProtocol())
		++count;
	
	return count;
}


//!	Returns the protocol at the given \a index or \c NULL if it could not be found.
KPPPProtocol*
KPPPInterface::ProtocolAt(int32 index) const
{
	KPPPProtocol *protocol = FirstProtocol();
	
	int32 currentIndex = 0;
	for(; protocol && currentIndex != index; protocol = protocol->NextProtocol())
		++currentIndex;
	
	return protocol;
}


/*!	\brief Returns the protocol object responsible for a given protocol number.
	
	\param protocolNumber The protocol number that the object should handle.
	\param start (Optional): Start with this protocol. Can be used for iteration.
	
	\return Either the object that was found or \c NULL.
*/
KPPPProtocol*
KPPPInterface::ProtocolFor(uint16 protocolNumber, KPPPProtocol *start = NULL) const
{
#if DEBUG
	dprintf("KPPPInterface: ProtocolFor(%X)\n", protocolNumber);
#endif
	
	KPPPProtocol *current = start ? start : FirstProtocol();
	
	for(; current; current = current->NextProtocol()) {
		if(current->ProtocolNumber() == protocolNumber
				|| (current->Flags() & PPP_INCLUDES_NCP
					&& (current->ProtocolNumber() & 0x7FFF)
						== (protocolNumber & 0x7FFF)))
			return current;
	}
	
	return NULL;
}


//!	Adds a new child interface (used for multilink interfaces).
bool
KPPPInterface::AddChild(KPPPInterface *child)
{
#if DEBUG
	dprintf("KPPPInterface: AddChild(%lX)\n",
		child ? child->ID() : 0);
#endif
	
	if(!child)
		return false;
	
	LockerHelper locker(fLock);
	
	if(fChildren.HasItem(child) || !fChildren.AddItem(child))
		return false;
	
	child->SetParent(this);
	
	return true;
}


//!	Removes a new child from this interface (used for multilink interfaces).
bool
KPPPInterface::RemoveChild(KPPPInterface *child)
{
#if DEBUG
	dprintf("KPPPInterface: RemoveChild(%lX)\n",
		child ? child->ID() : 0);
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


//!	Returns the child interface at the given \a index (used for multilink interfaces).
KPPPInterface*
KPPPInterface::ChildAt(int32 index) const
{
#if DEBUG
		dprintf("KPPPInterface: ChildAt(%ld)\n", index);
#endif
	
	KPPPInterface *child = fChildren.ItemAt(index);
	
	if(child == fChildren.GetDefaultItem())
		return NULL;
	
	return child;
}


//!	Enables or disables the auto-redial feture.
void
KPPPInterface::SetAutoRedial(bool autoRedial = true)
{
#if DEBUG
	dprintf("KPPPInterface: SetAutoRedial(%s)\n", autoRedial ? "true" : "false");
#endif
	
	if(Mode() != PPP_CLIENT_MODE)
		return;
	
	LockerHelper locker(fLock);
	
	fAutoRedial = autoRedial;
}


//!	Enables or disables the dial-on-demand feature.
void
KPPPInterface::SetDialOnDemand(bool dialOnDemand = true)
{
	// All protocols must check if DialOnDemand was enabled/disabled after this
	// interface went down. This is the only situation where a change is relevant.
	
#if DEBUG
	dprintf("KPPPInterface: SetDialOnDemand(%s)\n", dialOnDemand ? "true" : "false");
#endif
	
	// Only clients support DialOnDemand.
	if(Mode() != PPP_CLIENT_MODE) {
#if DEBUG
		dprintf("KPPPInterface::SetDialOnDemand(): Wrong mode!\n");
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


//!	Sets Protocol-Field-Compression options.
bool
KPPPInterface::SetPFCOptions(uint8 pfcOptions)
{
#if DEBUG
	dprintf("KPPPInterface: SetPFCOptions(0x%X)\n", pfcOptions);
#endif
	
	if(PFCOptions() & PPP_FREEZE_PFC_OPTIONS)
		return false;
	
	fPFCOptions = pfcOptions;
	return true;
}


/*!	\brief Brings this interface up.
	
	\c Down() overrides all \c Up() requests. \n
	This blocks until the connection process is finished.
	
	\return \c true if successful or \c false otherwise.
*/
bool
KPPPInterface::Up()
{
#if DEBUG
	dprintf("KPPPInterface: Up()\n");
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
		fOpenEventThread = spawn_kernel_thread(call_open_event_thread,
			"KPPPInterface: call_open_event_thread", B_NORMAL_PRIORITY, this);
		resume_thread(fOpenEventThread);
	}
	fLock.Unlock();
	
	if(me == fRedialThread && me != fUpThread)
		return true;
			// the redial thread is doing a DialRetry in this case (fUpThread
			// is waiting for new reports)
	
	while(true) {
		// A wrong code usually happens when the redial thread gets notified
		// of a Down() request. In that case a report will follow soon, so
		// this can be ignored.
		if(receive_data(&sender, &report, sizeof(report)) != PPP_REPORT_CODE)
			continue;
		
//#if DEBUG
//		dprintf("KPPPInterface::Up(): Report: Type = %ld Code = %ld\n", report.type,
//			report.code);
//#endif
		
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
#if DEBUG
					dprintf("KPPPInterface::Up(): DEVICE_UP_FAILED: >=maxretries!\n");
#endif
					fDialRetry = 0;
					fUpThread = -1;
					
					if(!DoesDialOnDemand())
						Delete();
					
					PPP_REPLY(sender, B_OK);
					ReportManager().DisableReports(PPP_CONNECTION_REPORT, me);
					return false;
				} else {
#if DEBUG
					dprintf("KPPPInterface::Up(): DEVICE_UP_FAILED: <maxretries\n");
#endif
					++fDialRetry;
					PPP_REPLY(sender, B_OK);
#if DEBUG
					dprintf("KPPPInterface::Up(): DEVICE_UP_FAILED: replied\n");
#endif
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
					
					if(!DoesDialOnDemand())
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


/*!	\brief Brings this interface down.
	
	\c Down() overrides all \c Up() requests. \n
	This blocks until diconnecting finished.
	
	\return \c true if successful or \c false otherwise.
*/
bool
KPPPInterface::Down()
{
#if DEBUG
	dprintf("KPPPInterface: Down()\n");
#endif
	
	if(InitCheck() != B_OK)
		return false;
	else if(State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)
		return true;
	
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
	
	fCloseEventThread = spawn_kernel_thread(call_close_event_thread,
		"KPPPInterface: call_close_event_thread", B_NORMAL_PRIORITY, this);
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


//!	Returns if the interface is connected.
bool
KPPPInterface::IsUp() const
{
	LockerHelper locker(fLock);
	
	return Phase() == PPP_ESTABLISHED_PHASE;
}


/*!	\brief Loads modules specified in the settings structure.
	
	\param settings PPP interface description file format settings.
	\param start Index of driver_parameter to start with.
	\param count Number of driver_parameters to look at.
	
	\return \c true if successful or \c false otherwise.
*/
bool
KPPPInterface::LoadModules(driver_settings *settings, int32 start, int32 count)
{
#if DEBUG
	dprintf("KPPPInterface: LoadModules()\n");
#endif
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	ppp_module_key_type type;
		// which type key was used for loading this module?
	
	const char *name = NULL;
	
	// multilink handling
	for(int32 index = start;
			index < settings->parameter_count && index < (start + count); index++) {
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
		fManager->CreateInterface(settings, Profile().Settings(), ID());
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


/*!	\brief Loads a specific module.
	
	\param name Name of the module.
	\param parameter Module settings.
	\param type Type of module.
	
	\return \c true if successful or \c false otherwise.
*/
bool
KPPPInterface::LoadModule(const char *name, driver_parameter *parameter,
	ppp_module_key_type type)
{
#if DEBUG
	dprintf("KPPPInterface: LoadModule(%s)\n", name ? name : "XXX: NO NAME");
#endif
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	if(!name || strlen(name) > B_FILE_NAME_LENGTH)
		return false;
	
	char *moduleName = new char[B_PATH_NAME_LENGTH];
	
	sprintf(moduleName, "%s/%s", PPP_MODULES_PATH, name);
	
	ppp_module_info *module;
	if(get_module(moduleName, (module_info**) &module) != B_OK) {
		delete[] moduleName;
		return false;
	}
	
	// add the module to the list of loaded modules
	// for putting them on our destruction
	fModules.AddItem(moduleName);
	
	return module->add_to(Parent() ? *Parent() : *this, this, parameter, type);
}


//!	Always returns true.
bool
KPPPInterface::IsAllowedToSend() const
{
	return true;
}


/*!	\brief Sends a packet to the device.
	
	This brings the interface up if dial-on-demand is enabled and we are not
	connected. \n
	PFC encoding is handled here.
	
	\param packet The packet.
	\param protocolNumber The packet's protocol number.
	
	\return
		- \c B_OK: Sending was successful.
		- \c B_ERROR: Some error occured.
*/
status_t
KPPPInterface::Send(struct mbuf *packet, uint16 protocolNumber)
{
#if DEBUG
	dprintf("KPPPInterface: Send(0x%X)\n", protocolNumber);
#endif
	
	if(!packet)
		return B_ERROR;
	
	// we must pass the basic tests like:
	// do we have a device?
	// did we load all modules?
	if(InitCheck() != B_OK) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// go up if DialOnDemand enabled and we are down
	if(protocolNumber != PPP_LCP_PROTOCOL && DoesDialOnDemand()
			&& (Phase() == PPP_DOWN_PHASE
				|| Phase() == PPP_ESTABLISHMENT_PHASE)
			&& !Up()) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// find the protocol handler for the current protocol number
	KPPPProtocol *protocol = ProtocolFor(protocolNumber);
	while(protocol && !protocol->IsEnabled())
		protocol = protocol->NextProtocol() ?
			ProtocolFor(protocolNumber, protocol->NextProtocol()) : NULL;
	
#if DEBUG
	if(!protocol)
		dprintf("KPPPInterface::Send(): no protocol found!\n");
	else if(!Device()->IsUp())
		dprintf("KPPPInterface::Send(): device is not up!\n");
	else if(!protocol->IsEnabled())
		dprintf("KPPPInterface::Send(): protocol not enabled!\n");
	else if(!IsProtocolAllowed(*protocol))
		dprintf("KPPPInterface::Send(): protocol not allowed to send!\n");
	else
		dprintf("KPPPInterface::Send(): protocol allowed\n");
#endif
	
	// make sure that protocol is allowed to send and everything is up
	if(!Device()->IsUp() || !protocol || !protocol->IsEnabled()
			|| !IsProtocolAllowed(*protocol)) {
		dprintf("KPPPInterface::Send(): cannot send!\n");
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


/*!	\brief Receives a packet.
	
	Encapsulation protocols may use this method to pass encapsulated packets to the
	PPP interface. Packets will be handled as if they were raw packets that came
	directly from the device via \c ReceiveFromDevice(). \n
	If no handler could be found in this interface the parent's \c Receive() method
	is called.
	
	\param packet The packet.
	\param protocolNumber The packet's protocol number.
	
	\return
		- \c B_OK: Receiving was successful.
		- \c B_ERROR: Some error occured.
		- \c PPP_REJECTED: No protocol handler could be found for this packet.
		- \c PPP_DISCARDED: The protocol handler(s) did not handle this packet.
*/
status_t
KPPPInterface::Receive(struct mbuf *packet, uint16 protocolNumber)
{
#if DEBUG
	dprintf("KPPPInterface: Receive(0x%X)\n", protocolNumber);
#endif
	
	if(!packet)
		return B_ERROR;
	
	int32 result = PPP_REJECTED;
		// assume we have no handler
	
	// Set our interface as the receiver.
	// The real netstack protocols (IP, IPX, etc.) might get confused if our
	// interface is a main interface and at the same time not registered
	// because then there is no receiver interface.
	// PPP NCPs should be aware of that!
	if(packet->m_flags & M_PKTHDR && Ifnet() != NULL)
		packet->m_pkthdr.rcvif = Ifnet();
	
	// Find handler and let it parse the packet.
	// The handler does need not be up because if we are a server
	// the handler might be upped by this packet.
	// If authenticating we only allow authentication phase protocols.
	KPPPProtocol *protocol = ProtocolFor(protocolNumber);
	for(; protocol;
			protocol = protocol->NextProtocol() ?
				ProtocolFor(protocolNumber, protocol->NextProtocol()) : NULL) {
#if DEBUG
		dprintf("KPPPInterface::Receive(): trying protocol\n");
#endif
		
		if(!protocol->IsEnabled() || !IsProtocolAllowed(*protocol))
			continue;
				// skip handler if disabled or not allowed
		
		result = protocol->Receive(packet, protocolNumber);
		if(result == PPP_UNHANDLED)
			continue;
		
		return result;
	}
	
#if DEBUG
	dprintf("KPPPInterface::Receive(): trying parent\n");
#endif
	
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


/*!	\brief Receives a base PPP packet from the device.
	
	KPPPDevice should call this method when it receives a packet. \n
	PFC decoding is handled here.
	
	\param packet The packet.
	
	\return
		- \c B_OK: Receiving was successful.
		- \c B_ERROR: Some error occured.
		- \c PPP_REJECTED: No protocol handler could be found for this packet.
		- \c PPP_DISCARDED: The protocol handler(s) did not handle this packet.
*/
status_t
KPPPInterface::ReceiveFromDevice(struct mbuf *packet)
{
#if DEBUG
	dprintf("KPPPInterface: ReceiveFromDevice()\n");
#endif
	
	if(!packet)
		return B_ERROR;
	
	if(InitCheck() != B_OK) {
		m_freem(packet);
		return B_ERROR;
	}
	
	// decode ppp frame and recognize PFC
	uint16 protocolNumber = *mtod(packet, uint8*);
	if(protocolNumber & 1) {
		m_adj(packet, 1);
	} else {
		protocolNumber = ntohs(*mtod(packet, uint16*));
		m_adj(packet, 2);
	}
	
	return Receive(packet, protocolNumber);
}


//!	Manages Pulse() calls for all add-ons and hanldes idle-disconnection.
void
KPPPInterface::Pulse()
{
	if(Device())
		Device()->Pulse();
	
	KPPPProtocol *protocol = FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol())
		protocol->Pulse();
	
	uint32 currentTime = real_time_clock();
	if(fUpdateIdleSince) {
		fIdleSince = currentTime;
		fUpdateIdleSince = false;
	}
	
	// check our idle time and disconnect if needed
	if(fDisconnectAfterIdleSince > 0 && fIdleSince != 0
			&& fIdleSince - currentTime >= fDisconnectAfterIdleSince)
		StateMachine().CloseEvent();
}


//!	Registers an ifnet structure for this interface.
bool
KPPPInterface::RegisterInterface()
{
#if DEBUG
	dprintf("KPPPInterface: RegisterInterface()\n");
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


//!	Unregisters this interface's ifnet structure.
bool
KPPPInterface::UnregisterInterface()
{
#if DEBUG
	dprintf("KPPPInterface: UnregisterInterface()\n");
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


//!	Called when profile changes.
void
KPPPInterface::UpdateProfile()
{
	KPPPLayer *layer = FirstProtocol();
	for(; layer; layer = layer->Next())
		layer->ProfileChanged();
}


//!	Called by KPPPManager: manager routes stack ioctls to the corresponding interface.
status_t
KPPPInterface::StackControl(uint32 op, void *data)
{
#if DEBUG
	dprintf("KPPPInterface: StackControl(0x%lX)\n", op);
#endif
	
	switch(op) {
		default:
			return StackControlEachHandler(op, data);
	}
	
	return B_OK;
}


//!	Utility class used by ControlEachHandler().
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

/*!	\brief This calls Control() with the given parameters for each add-on.
	
	\return
		- \c B_OK: All handlers returned B_OK.
		- \c B_BAD_VALUE: No handler was found.
		- Any other value: Error code which was returned by the last failing handler.
*/
status_t
KPPPInterface::StackControlEachHandler(uint32 op, void *data)
{
#if DEBUG
	dprintf("KPPPInterface: StackControlEachHandler(0x%lX)\n", op);
#endif
	
	status_t result = B_BAD_VALUE, tmp;
	
	KPPPProtocol *protocol = FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol()) {
		tmp = protocol->StackControl(op, data);
		if(tmp == B_OK && result == B_BAD_VALUE)
			result = B_OK;
		else if(tmp != B_BAD_VALUE)
			result = tmp;
	}
	
	ForEachItem(LCP().fLCPExtensions,
		CallStackControl<KPPPLCPExtension>(op, data, result));
	ForEachItem(LCP().fOptionHandlers,
		CallStackControl<KPPPOptionHandler>(op, data, result));
	
	return result;
}


//!	Recalculates the MTU from the MRU (includes encapsulation protocol overheads).
void
KPPPInterface::CalculateInterfaceMTU()
{
#if DEBUG
	dprintf("KPPPInterface: CalculateInterfaceMTU()\n");
#endif
	
	fInterfaceMTU = fMRU;
	fHeaderLength = 2;
	
	// sum all headers (the protocol field is not counted)
	KPPPProtocol *protocol = FirstProtocol();
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


//!	Recalculates the baud rate.
void
KPPPInterface::CalculateBaudRate()
{
#if DEBUG
	dprintf("KPPPInterface: CalculateBaudRate()\n");
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


//!	Redials. Waits a given delay (in miliseconds) before redialing.
void
KPPPInterface::Redial(uint32 delay)
{
#if DEBUG
	dprintf("KPPPInterface: Redial(%ld)\n", delay);
#endif
	
	if(fRedialThread != -1)
		return;
	
	// start a new thread that calls our Up() method
	redial_info info;
	info.interface = this;
	info.thread = &fRedialThread;
	info.delay = delay;
	
	fRedialThread = spawn_kernel_thread(redial_thread, "KPPPInterface: redial_thread",
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
		ppp_interface_id id = info.interface->ID();
		info.interface->Report(PPP_CONNECTION_REPORT, PPP_REPORT_UP_ABORTED,
			&id, sizeof(ppp_interface_id));
		return B_OK;
	}
	
	info.interface->Up();
	*info.thread = -1;
	
	return B_OK;
}


// ----------------------------------
// Function: interface_deleter_thread
// ----------------------------------
//!	Private class.
class KPPPInterfaceAccess {
	public:
		KPPPInterfaceAccess() {}
		
		void Delete(KPPPInterface *interface)
			{ delete interface; }
		void CallOpenEvent(KPPPInterface *interface)
		{
			while(interface->fLock.LockWithTimeout(100000) != B_NO_ERROR)
				if(interface->fDeleteCounter > 0)
					return;
			interface->CallOpenEvent();
			interface->fOpenEventThread = -1;
			interface->fLock.Unlock();
		}
		void CallCloseEvent(KPPPInterface *interface)
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
	KPPPInterfaceAccess access;
	access.Delete((KPPPInterface*) data);
	
	return B_OK;
}


status_t
call_open_event_thread(void *data)
{
	KPPPInterfaceAccess access;
	access.CallOpenEvent((KPPPInterface*) data);
	
	return B_OK;
}


status_t
call_close_event_thread(void *data)
{
	KPPPInterfaceAccess access;
	access.CallCloseEvent((KPPPInterface*) data);
	
	return B_OK;
}
