/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

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
// dprintf is defined twice with different return values, once with
// void (KernelExport.h) and once with int (stdio.h).
#include <cstdio>
#include <cstring>

#include <ByteOrder.h>
#include <net_buffer.h>
#include <net_stack.h>
#include <ppp_device.h>

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
// #include <lock.h>
// #include <util/AutoLock.h>

// tools only for us :)
#include "settings_tools.h"

// internal modules
#include "_KPPPMRUHandler.h"
#include "_KPPPAuthenticationHandler.h"
#include "_KPPPPFCHandler.h"


// TODO:
// - implement timers with support for setting next event instead of receiving timer
//    events periodically
// - add missing settings support (ConnectRetryDelay, etc.)


//!	Private structure needed for reconnecting.
typedef struct reconnect_info {
	KPPPInterface *interface;
	thread_id *thread;
	uint32 delay;
} reconnect_info;

extern net_buffer_module_info *gBufferModule;
extern net_stack_module_info *gStackModule;

status_t reconnect_thread(void *data);

// other functions
status_t interface_deleter_thread(void *data);


/*!	\brief Creates a new interface.

	\param name Name of the PPP interface description file.
	\param entry The PPP manager passes an internal structure to the constructor.
	\param ID The interface's ID.
	\param settings (Optional): If no name is given you must pass the settings here.
	\param parent (Optional): Interface's parent (only used for multilink interfaces).
*/
KPPPInterface::KPPPInterface(const char *name, ppp_interface_entry *entry,
	ppp_interface_id ID, const driver_settings *settings, KPPPInterface *parent)
	:
	KPPPLayer(name, PPP_INTERFACE_LEVEL, 2),
	fID(ID),
	fSettings(NULL),
	fIfnet(NULL),
	fReconnectThread(-1),
	fConnectAttempt(1),
	fConnectRetriesLimit(0),
	fManager(NULL),
	fConnectedSince(0),
	fIdleSince(0),
	fMRU(1500),
	fInterfaceMTU(1498),
	fHeaderLength(2),
	fParent(NULL),
	fIsMultilink(false),
	fAutoReconnect(false),
	fConnectOnDemand(true),
	fAskBeforeConnecting(false),
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
	entry->interface = this;

	if (name) {
		// load settings from description file
		char path[B_PATH_NAME_LENGTH];
		sprintf(path, "ptpnet/%s", name);

		void *handle = load_driver_settings(path);
		if (!handle) {
			ERROR("KPPPInterface: Unable to load %s PPP driver settings!\n", path);
			fInitStatus = B_ERROR;
			return;
		}

		fSettings = dup_driver_settings(get_driver_settings(handle));
		unload_driver_settings(handle);
	} else
		fSettings = dup_driver_settings(settings);
			// use the given settings

	if (!fSettings) {
		ERROR("KPPPInterface: No fSettings!\n");
		fInitStatus = B_ERROR;
		return;
	}

	// add internal modules
	// LCP
	if (!AddProtocol(&LCP())) {
		ERROR("KPPPInterface: Could not add LCP protocol!\n");
		fInitStatus = B_ERROR;
		return;
	}
	// MRU
	_KPPPMRUHandler *mruHandler =
		new _KPPPMRUHandler(*this);
	if (!LCP().AddOptionHandler(mruHandler) || mruHandler->InitCheck() != B_OK) {
		ERROR("KPPPInterface: Could not add MRU handler!\n");
		delete mruHandler;
	}
	// authentication
	_KPPPAuthenticationHandler *authenticationHandler =
		new _KPPPAuthenticationHandler(*this);
	if (!LCP().AddOptionHandler(authenticationHandler)
			|| authenticationHandler->InitCheck() != B_OK) {
		ERROR("KPPPInterface: Could not add authentication handler!\n");
		delete authenticationHandler;
	}
	// PFC
	_KPPPPFCHandler *pfcHandler =
		new _KPPPPFCHandler(fLocalPFCState, fPeerPFCState, *this);
	if (!LCP().AddOptionHandler(pfcHandler) || pfcHandler->InitCheck() != B_OK) {
		ERROR("KPPPInterface: Could not add PFC handler!\n");
		delete pfcHandler;
	}

	// set up connect delays
	fConnectRetryDelay = 3000;
		// 3s delay between each new attempt to reconnect
	fReconnectDelay = 1000;
		// 1s delay between lost connection and reconnect

	if (get_module(PPP_INTERFACE_MODULE_NAME, (module_info**) &fManager) != B_OK)
		ERROR("KPPPInterface: Manager module not found!\n");

	// are we a multilink subinterface?
	if (parent && parent->IsMultilink()) {
		fParent = parent;
		fParent->AddChild(this);
		fIsMultilink = true;
	}

	RegisterInterface();

	if (!fSettings) {
		fInitStatus = B_ERROR;
		return;
	}

	const char *value;

	// get login
	value = get_settings_value(PPP_USERNAME_KEY, fSettings);
	fUsername = value ? strdup(value) : strdup("");
	value = get_settings_value(PPP_PASSWORD_KEY, fSettings);
	fPassword = value ? strdup(value) : strdup("");

	// get DisonnectAfterIdleSince settings
	value = get_settings_value(PPP_DISONNECT_AFTER_IDLE_SINCE_KEY, fSettings);
	if (!value)
		fDisconnectAfterIdleSince = 0;
	else
		fDisconnectAfterIdleSince = atoi(value) * 1000;

	if (fDisconnectAfterIdleSince < 0)
		fDisconnectAfterIdleSince = 0;

	// get mode settings
	value = get_settings_value(PPP_MODE_KEY, fSettings);
	if (value && !strcasecmp(value, PPP_SERVER_MODE_VALUE))
		fMode = PPP_SERVER_MODE;
	else
		fMode = PPP_CLIENT_MODE;
		// we are a client by default

	SetAutoReconnect(
		get_boolean_value(
		get_settings_value(PPP_AUTO_RECONNECT_KEY, fSettings),
		false)
		);
		// auto reconnect is disabled by default

	fAskBeforeConnecting = get_boolean_value(
		get_settings_value(PPP_ASK_BEFORE_CONNECTING_KEY, fSettings), false);

	// load all protocols and the device
	if (!LoadModules(fSettings, 0, fSettings->parameter_count)) {
		ERROR("KPPPInterface: Error loading modules!\n");
		fInitStatus = B_ERROR;
	}
}


//!	Destructor: Disconnects and marks interface for deletion.
KPPPInterface::~KPPPInterface()
{
	TRACE("KPPPInterface: Destructor\n");

	// tell protocols to uninit (remove routes, etc.)
	KPPPProtocol *protocol = FirstProtocol();
	for (; protocol; protocol = protocol->NextProtocol())
		protocol->Uninit();

	// make sure we are not accessible by any thread before we continue
	UnregisterInterface();

	if (fManager)
		fManager->RemoveInterface(ID());

	// Call Down() until we get a lock on an interface that is down.
	// This lock is not released until we are actually deleted.
	while (true) {
		Down();
		{
			MutexLocker (fLock);
			if (State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)
				break;
		}
	}

	Report(PPP_DESTRUCTION_REPORT, 0, &fID, sizeof(ppp_interface_id));
		// tell all listeners that we are being destroyed

	int32 tmp;
	send_data_with_timeout(fReconnectThread, 0, NULL, 0, 200);
		// tell thread that we are being destroyed (200ms timeout)
	wait_for_thread(fReconnectThread, &tmp);

	while (CountChildren())
		delete ChildAt(0);

	delete Device();

	while (FirstProtocol()) {
		if (FirstProtocol() == &LCP())
			fFirstProtocol = fFirstProtocol->NextProtocol();
		else
			delete FirstProtocol();
				// destructor removes protocol from list
	}

	for (int32 index = 0; index < fModules.CountItems(); index++) {
		put_module(fModules.ItemAt(index));
		delete[] fModules.ItemAt(index);
	}

	free_driver_settings(fSettings);

	if (Parent())
		Parent()->RemoveChild(this);

	if (fManager)
		put_module(PPP_INTERFACE_MODULE_NAME);
}


//!	Marks interface for deletion.
void
KPPPInterface::Delete()
{
	// MutexLocker locker(fLock);
		// alreay locked in KPPPStatemachine::DownEvent
		// uncomment this line will cause double lock

	if (fDeleteCounter > 0)
		return;
			// only one thread should delete us!

	fDeleteCounter = 1;

	fManager->DeleteInterface(ID());
		// This will mark us for deletion.
		// Any subsequent calls to delete_interface() will do nothing.
}


//!	Returns if interface was initialized correctly.
status_t
KPPPInterface::InitCheck() const
{
	if (fInitStatus != B_OK)
		return fInitStatus;

	if (!fSettings || !fManager)
		return B_ERROR;

	// sub-interfaces should have a device
	if (IsMultilink()) {
		if (Parent() && !fDevice)
			return B_ERROR;
	} else if (!fDevice)
		return B_ERROR;

	return B_OK;
}


//!	The username used for authentication.
const char*
KPPPInterface::Username() const
{
	// this data is not available before we authenticate
	if (Phase() < PPP_AUTHENTICATION_PHASE)
		return NULL;

	return fUsername;
}


//!	The password used for authentication.
const char*
KPPPInterface::Password() const
{
	// this data is not available before we authenticate
	if (Phase() < PPP_AUTHENTICATION_PHASE)
		return NULL;

	return fPassword;
}


//!	Sets interface MRU.
bool
KPPPInterface::SetMRU(uint32 MRU)
{
	TRACE("KPPPInterface: SetMRU(%ld)\n", MRU);

	if (Device() && MRU > Device()->MTU() - 2)
		return false;

	// MutexLocker locker(fLock);
		// uncomment this line will cause double lock
		// alreay locked in ::Up and ::KPPPInterface

	fMRU = MRU;

	CalculateInterfaceMTU();

	return true;
}


//!	Returns number of bytes spent for protocol overhead. Includes device overhead.
uint32
KPPPInterface::PacketOverhead() const
{
	uint32 overhead = fHeaderLength + 2;

	if (Device())
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
		- \c B_NOT_ALLOWED: Operation not allowed (at this point in time).
		- \c B_BAD_INDEX: Wrong index (e.g.: when accessing interface submodules).
		- \c B_BAD_VALUE: Unknown op.
		- Return value of submodule (when controlling one).
*/
status_t
KPPPInterface::Control(uint32 op, void *data, size_t length)
{
	TRACE("%s:%s\n", __FILE__, __func__);

        control_net_module_args* args = (control_net_module_args*)data;
        if (op != NET_STACK_CONTROL_NET_MODULE) {
		dprintf("unknow op!!\n");
		return B_BAD_VALUE;
	}

	switch (args->op) {
		case PPPC_COUNT_INTERFACES:
		{
			// should be implepented
			dprintf("PPPC_COUNT_INTERFACES should be implepentd\n");

			return B_OK;
		}

		case PPPC_GET_INTERFACES:
		{
			dprintf("PPPC_GET_INTERFACES\n");
			ppp_get_interfaces_info* info = (ppp_get_interfaces_info*)args->data;
			dprintf("info->interfaces: %p\n", info->interfaces);
			*(info->interfaces) = 1;
			info->resultCount = 1;

			return B_OK;
		}

		case PPPC_CONTROL_INTERFACE:
		{
			dprintf("PPPC_CONTROL_INTERFACE\n");
			ppp_control_info* control = (ppp_control_info*)args->data;

			switch (control->op) {
			case PPPC_GET_INTERFACE_INFO:
			{
				dprintf("PPPC_GET_INTERFACE_INFO\n");
				if (control->length < sizeof(ppp_interface_info_t) || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				ppp_interface_info *info = (ppp_interface_info*) control->data;
				dprintf("info addr:%p\n", info);
				memset(info, 0, sizeof(ppp_interface_info_t));
				if (Name())
					strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);

				if (Ifnet())
					info->if_unit = Ifnet()->index;
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
				info->connectAttempt = fConnectAttempt;
				info->connectRetriesLimit = fConnectRetriesLimit;
				info->connectRetryDelay = ConnectRetryDelay();
				info->reconnectDelay = ReconnectDelay();
				info->connectedSince = ConnectedSince();
				info->idleSince = IdleSince();
				info->disconnectAfterIdleSince = DisconnectAfterIdleSince();
				info->doesConnectOnDemand = DoesConnectOnDemand();
				info->doesAutoReconnect = DoesAutoReconnect();
				info->hasDevice = Device();
				info->isMultilink = IsMultilink();
				info->hasParent = Parent();
				break;
			}

			case PPPC_SET_USERNAME:
			{
				dprintf("PPPC_SET_USERNAME\n");
				if (control->length > PPP_HANDLER_NAME_LENGTH_LIMIT || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				MutexLocker locker(fLock);
				// login information can only be changed before we authenticate
				if (Phase() >= PPP_AUTHENTICATION_PHASE)
					return B_NOT_ALLOWED;

				free(fUsername);
				fUsername = control->data ? strdup((const char*) control->data) : strdup("");
				dprintf("set ppp user name to %s\n", fUsername);

				break;
			}

			case PPPC_SET_PASSWORD:
			{
				dprintf("PPPC_SET_PASSWORD\n");
				if (control->length > PPP_HANDLER_NAME_LENGTH_LIMIT || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				MutexLocker locker(fLock);
				// login information can only be changed before we authenticate
				if (Phase() >= PPP_AUTHENTICATION_PHASE)
					return B_NOT_ALLOWED;

				free(fPassword);
				fPassword = control->data ? strdup((const char*) control->data) : strdup("");
				dprintf("set ppp password to %s!\n", fPassword);
				break;
			}

			case PPPC_SET_ASK_BEFORE_CONNECTING:
			{
				dprintf("PPPC_SET_ASK_BEFORE_CONNECTING\n");
				if (control->length < sizeof(uint32) || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				SetAskBeforeConnecting(*((uint32*)control->data));
				dprintf("goto PPPC_SET_ASK_BEFORE_CONNECTING here!\n");
				break;
			}

			case PPPC_GET_STATISTICS:
			{
				dprintf("PPPC_GET_STATISTICS\n");
				if (control->length < sizeof(ppp_statistics) || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				dprintf("should PPPC_GET_STATISTICS here!\n");

				memcpy(control->data, &fStatistics, sizeof(ppp_statistics));
				break;
			}

			case PPPC_HAS_INTERFACE_SETTINGS:
			{
				dprintf("PPPC_HAS_INTERFACE_SETTINGS\n");
				if (control->length < sizeof(driver_settings) || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				dprintf("should PPPC_HAS_INTERFACE_SETTINGS here!\n");

				if (equal_interface_settings(Settings(), (driver_settings*)control->data))
					return B_OK;
				else
					return B_ERROR;
				break;

			}

			case PPPC_ENABLE_REPORTS:
			{
				dprintf("PPPC_ENABLE_REPORTS\n");
				if (control->length < sizeof(ppp_report_request) || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				dprintf("should PPPC_ENABLE_REPORTS here!\n");

				MutexLocker locker(fLock);
				ppp_report_request *request = (ppp_report_request*) control->data;
				// first, we send an initial state report
				if (request->type == PPP_CONNECTION_REPORT) {
					ppp_report_packet report;
					report.type = PPP_CONNECTION_REPORT;
					report.code = StateMachine().fLastConnectionReportCode;
					report.length = sizeof(fID);
					KPPPReportManager::SendReport(request->thread, &report);
					if (request->flags & PPP_REMOVE_AFTER_REPORT)
						return B_OK;
				}
				ReportManager().EnableReports(request->type, request->thread,
					request->flags);
				break;
			}

			case PPPC_DISABLE_REPORTS:
			{
				dprintf("PPPC_DISABLE_REPORTS\n");
				if (control->length < sizeof(ppp_report_request) || !control->data) {
					dprintf("size wrong!\n");
					return B_ERROR;
				}

				dprintf("should PPPC_DISABLE_REPORTS here!\n");

				ppp_report_request *request = (ppp_report_request*) control->data;
				ReportManager().DisableReports(request->type, request->thread);
				break;
			}

			case PPPC_CONTROL_DEVICE:
			{
				dprintf("PPPC_CONTROL_DEVICE\n");
				if (control->length < sizeof(ppp_control_info) || !control->data)
					return B_ERROR;

				ppp_control_info *controlInfo = (ppp_control_info*) control->data;
				if (controlInfo->index != 0 || !Device())
				{
					dprintf("index is 0 or no Device\n");
					return B_BAD_INDEX;
				}

				return Device()->Control(controlInfo->op, controlInfo->data, controlInfo->length);
			}

			case PPPC_CONTROL_PROTOCOL:
			{
				dprintf("PPPC_CONTROL_PROTOCOL\n");
				if (control->length < sizeof(ppp_control_info) || !control->data)
					return B_ERROR;

				ppp_control_info *controlInfo = (ppp_control_info*) control->data;
				KPPPProtocol *protocol = ProtocolAt(controlInfo->index);
				if (!protocol)
					return B_BAD_INDEX;

				return protocol->Control(controlInfo->op, controlInfo->data, controlInfo->length);
			}

			case PPPC_CONTROL_OPTION_HANDLER:
			{
				dprintf("PPPC_CONTROL_OPTION_HANDLER\n");
				if (control->length < sizeof(ppp_control_info) || !control->data)
					return B_ERROR;

				ppp_control_info *controlInfo = (ppp_control_info*) control->data;
				KPPPOptionHandler *optionHandler = LCP().OptionHandlerAt(controlInfo->index);
				if (!optionHandler) {
					dprintf("optionHandler no avail\n");
					return B_BAD_INDEX;
				}

				return optionHandler->Control(controlInfo->op, controlInfo->data,
					controlInfo->length);
			}

			case PPPC_CONTROL_LCP_EXTENSION:
			{
				dprintf("PPPC_CONTROL_LCP_EXTENSION\n");
				if (control->length < sizeof(ppp_control_info) || !control->data)
					return B_ERROR;

				ppp_control_info *controlInfo = (ppp_control_info*) control->data;
				KPPPLCPExtension *lcpExtension = LCP().LCPExtensionAt(controlInfo->index);
				if (!lcpExtension)
					return B_BAD_INDEX;

				return lcpExtension->Control(controlInfo->op, controlInfo->data,
					controlInfo->length);
			}

			case PPPC_CONTROL_CHILD:
			{
				dprintf("PPPC_CONTROL_CHILD\n");
				if (control->length < sizeof(ppp_control_info) || !control->data)
					return B_ERROR;

				ppp_control_info *controlInfo = (ppp_control_info*) control->data;
				KPPPInterface *child = ChildAt(controlInfo->index);
				if (!child)
					return B_BAD_INDEX;

				return child->Control(controlInfo->op, controlInfo->data, controlInfo->length);
			}

			default :
				return B_ERROR;
		}

			return B_OK;
		}

		case PPPC_GET_INTERFACE_INFO:
		{
			if (length < sizeof(ppp_interface_info_t) || !data)
				return B_ERROR;

			ppp_interface_info *info = (ppp_interface_info*) data;
			memset(info, 0, sizeof(ppp_interface_info_t));
			if (Name())
				strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			if (Ifnet())
				info->if_unit = Ifnet()->index;
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
			info->connectAttempt = fConnectAttempt;
			info->connectRetriesLimit = fConnectRetriesLimit;
			info->connectRetryDelay = ConnectRetryDelay();
			info->reconnectDelay = ReconnectDelay();
			info->connectedSince = ConnectedSince();
			info->idleSince = IdleSince();
			info->disconnectAfterIdleSince = DisconnectAfterIdleSince();
			info->doesConnectOnDemand = DoesConnectOnDemand();
			info->doesAutoReconnect = DoesAutoReconnect();
			info->hasDevice = Device();
			info->isMultilink = IsMultilink();
			info->hasParent = Parent();
			break;
		}

		case PPPC_SET_USERNAME:
		{
			if (!data)
				return B_ERROR;

			MutexLocker locker(fLock);
			// login information can only be changed before we authenticate
			if (Phase() >= PPP_AUTHENTICATION_PHASE)
				return B_NOT_ALLOWED;

			free(fUsername);
			fUsername = data ? strdup((const char*) data) : strdup("");
			break;
		}

		case PPPC_SET_PASSWORD:
		{
			if (!data)
				return B_ERROR;

			MutexLocker locker(fLock);
			// login information can only be changed before we authenticate
			if (Phase() >= PPP_AUTHENTICATION_PHASE)
				return B_NOT_ALLOWED;

			free(fPassword);
			fPassword = data ? strdup((const char*) data) : strdup("");
			break;
		}

		case PPPC_SET_ASK_BEFORE_CONNECTING:
			if (length < sizeof(uint32) || !data)
				return B_ERROR;

			SetAskBeforeConnecting(*((uint32*)data));
			break;

		case PPPC_SET_MRU:
			if (length < sizeof(uint32) || !data)
				return B_ERROR;

			SetMRU(*((uint32*)data));
			break;

		case PPPC_SET_CONNECT_ON_DEMAND:
			if (length < sizeof(uint32) || !data)
				return B_ERROR;

			SetConnectOnDemand(*((uint32*)data));
			break;

		case PPPC_SET_AUTO_RECONNECT:
			if (length < sizeof(uint32) || !data)
				return B_ERROR;

			SetAutoReconnect(*((uint32*)data));
			break;

		case PPPC_HAS_INTERFACE_SETTINGS:
			if (length < sizeof(driver_settings) || !data)
				return B_ERROR;

			if (equal_interface_settings(Settings(), (driver_settings*) data))
				return B_OK;
			else
				return B_ERROR;
			break;

		case PPPC_ENABLE_REPORTS:
		{
			if (length < sizeof(ppp_report_request) || !data)
				return B_ERROR;

			MutexLocker locker(fLock);
			ppp_report_request *request = (ppp_report_request*) data;
			// first, we send an initial state report
			if (request->type == PPP_CONNECTION_REPORT) {
				ppp_report_packet report;
				report.type = PPP_CONNECTION_REPORT;
				report.code = StateMachine().fLastConnectionReportCode;
				report.length = sizeof(fID);
				KPPPReportManager::SendReport(request->thread, &report);
				if (request->flags & PPP_REMOVE_AFTER_REPORT)
					return B_OK;
			}
			ReportManager().EnableReports(request->type, request->thread,
				request->flags);
			break;
		}

		case PPPC_DISABLE_REPORTS:
		{
			if (length < sizeof(ppp_report_request) || !data)
				return B_ERROR;

			ppp_report_request *request = (ppp_report_request*) data;
			ReportManager().DisableReports(request->type, request->thread);
			break;
		}

		case PPPC_GET_STATISTICS:
			if (length < sizeof(ppp_statistics) || !data)
				return B_ERROR;

			memcpy(data, &fStatistics, sizeof(ppp_statistics));
			break;

		case PPPC_CONTROL_DEVICE:
		{
			if (length < sizeof(ppp_control_info) || !data)
				return B_ERROR;

			ppp_control_info *control = (ppp_control_info*) data;
			if (control->index != 0 || !Device())
				return B_BAD_INDEX;

			return Device()->Control(control->op, control->data, control->length);
		}

		case PPPC_CONTROL_PROTOCOL:
		{
			if (length < sizeof(ppp_control_info) || !data)
				return B_ERROR;

			ppp_control_info *control = (ppp_control_info*) data;
			KPPPProtocol *protocol = ProtocolAt(control->index);
			if (!protocol)
				return B_BAD_INDEX;

			return protocol->Control(control->op, control->data, control->length);
		}

		case PPPC_CONTROL_OPTION_HANDLER:
		{
			if (length < sizeof(ppp_control_info) || !data)
				return B_ERROR;

			ppp_control_info *control = (ppp_control_info*) data;
			KPPPOptionHandler *optionHandler = LCP().OptionHandlerAt(control->index);
			if (!optionHandler)
				return B_BAD_INDEX;

			return optionHandler->Control(control->op, control->data,
				control->length);
		}

		case PPPC_CONTROL_LCP_EXTENSION:
		{
			if (length < sizeof(ppp_control_info) || !data)
				return B_ERROR;

			ppp_control_info *control = (ppp_control_info*) data;
			KPPPLCPExtension *lcpExtension = LCP().LCPExtensionAt(control->index);
			if (!lcpExtension)
				return B_BAD_INDEX;

			return lcpExtension->Control(control->op, control->data,
				control->length);
		}

		case PPPC_CONTROL_CHILD:
		{
			if (length < sizeof(ppp_control_info) || !data)
				return B_ERROR;

			ppp_control_info *control = (ppp_control_info*) data;
			KPPPInterface *child = ChildAt(control->index);
			if (!child)
				return B_BAD_INDEX;

			return child->Control(control->op, control->data, control->length);
		}

		default:
			dprintf("bad ppp_interface_control!\n");
			return B_BAD_VALUE;
	}

	return B_OK;
}


/*!	\brief Sets a new device for this interface.

	A device add-on should call this method to register itself. The best place to do
	this is in your module's \c add_to() function.

	\param device The device object.

	\return \c true if successful or \c false otherwise.

	\sa KPPPDevice
	\sa kppp_module_info
*/
bool
KPPPInterface::SetDevice(KPPPDevice *device)
{
	TRACE("KPPPInterface: SetDevice(%p)\n", device);

	if (device && &device->Interface() != this)
		return false;

	if (IsMultilink() && !Parent())
		return false;
			// main interfaces do not have devices

	MutexLocker locker(fLock);

	if (Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	if (fDevice && (IsUp() || fDevice->IsUp()))
		Down();

	fDevice = device;
	SetNext(device);

	if (fDevice)
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

	\return \c true if successful or \c false otherwise.

	\sa KPPPProtocol
	\sa kppp_module_info
*/
bool
KPPPInterface::AddProtocol(KPPPProtocol *protocol)
{
	// Find insert position after the last protocol
	// with the same level.

	TRACE("KPPPInterface: AddProtocol(%X)\n",
		protocol ? protocol->ProtocolNumber() : 0);

	if (!protocol || &protocol->Interface() != this
			|| protocol->Level() == PPP_INTERFACE_LEVEL)
		return false;

	MutexLocker locker(fLock);

	if (Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	KPPPProtocol *current = fFirstProtocol, *previous = NULL;

	while (current) {
		if (current->Level() < protocol->Level())
			break;

		previous = current;
		current = current->NextProtocol();
	}

	if (!current) {
		if (!previous)
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

		if (!previous)
			fFirstProtocol = protocol;
		else
			previous->SetNextProtocol(protocol);
	}

	if (protocol->Level() < PPP_PROTOCOL_LEVEL)
		CalculateInterfaceMTU();

	if (IsUp() || Phase() >= protocol->ActivationPhase())
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

	\return \c true if successful or \c false otherwise.
*/
bool
KPPPInterface::RemoveProtocol(KPPPProtocol *protocol)
{
	TRACE("KPPPInterface: RemoveProtocol(%X)\n",
		protocol ? protocol->ProtocolNumber() : 0);

	MutexLocker locker(fLock);

	if (Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	KPPPProtocol *current = fFirstProtocol, *previous = NULL;

	while (current) {
		if (current == protocol) {
			if (!protocol->IsDown())
				protocol->Down();

			if (previous) {
				previous->SetNextProtocol(current->NextProtocol());

				// set us as next layer if needed
				if (!previous->Next())
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
	MutexLocker locker(fLock);

	KPPPProtocol *protocol = FirstProtocol();

	int32 count = 0;
	for (; protocol; protocol = protocol->NextProtocol())
		++count;

	return count;
}


//!	Returns the protocol at the given \a index or \c NULL if it could not be found.
KPPPProtocol*
KPPPInterface::ProtocolAt(int32 index) const
{
	MutexLocker locker(fLock);

	KPPPProtocol *protocol = FirstProtocol();

	int32 currentIndex = 0;
	for (; protocol && currentIndex != index; protocol = protocol->NextProtocol())
		++currentIndex;

	return protocol;
}


/*!	\brief Returns the protocol object responsible for a given protocol number.

	\param protocolNumber The protocol number that the object should handle.
	\param start (Optional): Start with this protocol. Can be used for iteration.

	\return Either the object that was found or \c NULL.
*/
KPPPProtocol*
KPPPInterface::ProtocolFor(uint16 protocolNumber, KPPPProtocol *start) const
{
	TRACE("KPPPInterface: ProtocolFor(%X)\n", protocolNumber);

	// MutexLocker locker(fLock);
		// already locked in ::Receive, uncomment this line will cause double lock

	KPPPProtocol *current = start ? start : FirstProtocol();

	for (; current; current = current->NextProtocol()) {
		if (current->ProtocolNumber() == protocolNumber
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
	TRACE("KPPPInterface: AddChild(%lX)\n", child ? child->ID() : 0);

	if (!child)
		return false;

	MutexLocker locker(fLock);

	if (fChildren.HasItem(child) || !fChildren.AddItem(child))
		return false;

	child->SetParent(this);

	return true;
}


//!	Removes a new child from this interface (used for multilink interfaces).
bool
KPPPInterface::RemoveChild(KPPPInterface *child)
{
	TRACE("KPPPInterface: RemoveChild(%lX)\n", child ? child->ID() : 0);

	MutexLocker locker(fLock);

	if (!fChildren.RemoveItem(child))
		return false;

	child->SetParent(NULL);

	// parents cannot exist without their children
	if (CountChildren() == 0 && fManager && Ifnet())
		Delete();

	return true;
}


//!	Returns the child interface at the given \a index (used for multilink interfaces).
KPPPInterface*
KPPPInterface::ChildAt(int32 index) const
{
	TRACE("KPPPInterface: ChildAt(%ld)\n", index);

	MutexLocker locker(fLock);

	KPPPInterface *child = fChildren.ItemAt(index);

	if (child == fChildren.GetDefaultItem())
		return NULL;

	return child;
}


//!	Enables or disables the auto-reconnect feture.
void
KPPPInterface::SetAutoReconnect(bool autoReconnect)
{
	TRACE("KPPPInterface: SetAutoReconnect(%s)\n", autoReconnect ? "true" : "false");

	if (Mode() != PPP_CLIENT_MODE)
		return;

	fAutoReconnect = autoReconnect;
}


//!	Enables or disables the connect-on-demand feature.
void
KPPPInterface::SetConnectOnDemand(bool connectOnDemand)
{
	// All protocols must check if ConnectOnDemand was enabled/disabled after this
	// interface went down. This is the only situation where a change is relevant.

	TRACE("KPPPInterface: SetConnectOnDemand(%s)\n", connectOnDemand ? "true" : "false");

	MutexLocker locker(fLock);

	// Only clients support ConnectOnDemand.
	if (Mode() != PPP_CLIENT_MODE) {
		TRACE("KPPPInterface::SetConnectOnDemand(): Wrong mode!\n");
		fConnectOnDemand = false;
		return;
	} else if (DoesConnectOnDemand() == connectOnDemand)
		return;

	fConnectOnDemand = connectOnDemand;

	// Do not allow changes when we are disconnected (only main interfaces).
	// This would make no sense because
	// - enabling: this cannot happen because hidden interfaces are deleted if they
	//    could not establish a connection (the user cannot access hidden interfaces)
	// - disabling: the interface disappears as seen from the user, so we delete it
	if (!Parent() && State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE) {
		if (!connectOnDemand)
			Delete();
				// as long as the protocols were not configured we can just delete us

		return;
	}

	// check if we need to set/unset flags
	if (connectOnDemand) {
		if (Ifnet())
			Ifnet()->flags |= IFF_UP;
	} else if (!connectOnDemand && Phase() < PPP_ESTABLISHED_PHASE) {
		if (Ifnet())
			Ifnet()->flags &= ~IFF_UP;
	}
}


//!	Sets whether the user is asked before establishing the connection.
void
KPPPInterface::SetAskBeforeConnecting(bool ask)
{
	MutexLocker locker(fLock);

	bool old = fAskBeforeConnecting;
	fAskBeforeConnecting = ask;

	if (old && fAskBeforeConnecting == false && State() == PPP_STARTING_STATE
			&& Phase() == PPP_DOWN_PHASE) {
		// locker.Unlock();
		StateMachine().ContinueOpenEvent();
	}
}


//!	Sets Protocol-Field-Compression options.
bool
KPPPInterface::SetPFCOptions(uint8 pfcOptions)
{
	TRACE("KPPPInterface: SetPFCOptions(0x%X)\n", pfcOptions);

	MutexLocker locker(fLock);

	if (PFCOptions() & PPP_FREEZE_PFC_OPTIONS)
		return false;

	fPFCOptions = pfcOptions;
	return true;
}


/*!	\brief Brings this interface up.

	\c Down() overrides all \c Up() requests. \n
	This method runs an asynchronous process (it returns immediately).

	\return \c false on error.
*/
bool
KPPPInterface::Up()
{
	TRACE("KPPPInterface: Up()\n");

	if (InitCheck() != B_OK || Phase() == PPP_TERMINATION_PHASE)
		return false;

	if (IsUp())
		return true;

	MutexLocker locker(fLock);
	StateMachine().OpenEvent();

	return true;
}


/*!	\brief Brings this interface down.

	\c Down() overrides all \c Up() requests. \n
	This method runs an asynchronous process (it returns immediately).

	\return \c false on error.
*/
bool
KPPPInterface::Down()
{
	TRACE("KPPPInterface: Down()\n");

	if (InitCheck() != B_OK)
		return false;
	else if (State() == PPP_INITIAL_STATE && Phase() == PPP_DOWN_PHASE)
		return true;

	send_data_with_timeout(fReconnectThread, 0, NULL, 0, 200);
		// tell the reconnect thread to abort its attempt (if it's still waiting)

	MutexLocker locker(fLock);
	StateMachine().CloseEvent();

	return true;
}


//!	Waits for connection establishment. Returns true if successful.
bool
KPPPInterface::WaitForConnection()
{
	TRACE("KPPPInterface: WaitForConnection()\n");

	if (InitCheck() != B_OK)
		return false;

	// just delay ~3 seconds to wait for ppp go up
	for (uint32 i = 0; i < 10000; i++)
		for (uint32 j = 0; j < 3000000; j++)

	return true; // for temporary

	ReportManager().EnableReports(PPP_CONNECTION_REPORT, find_thread(NULL));

	ppp_report_packet report;
	thread_id sender;
	bool successful = false;
	while (true) {
		if (receive_data(&sender, &report, sizeof(report)) != PPP_REPORT_CODE)
			continue;

		if (report.type == PPP_DESTRUCTION_REPORT)
			break;
		else if (report.type != PPP_CONNECTION_REPORT)
			continue;

		if (report.code == PPP_REPORT_UP_SUCCESSFUL) {
			successful = true;
			break;
		} else if (report.code == PPP_REPORT_DOWN_SUCCESSFUL)
			break;
	}

	ReportManager().DisableReports(PPP_CONNECTION_REPORT, find_thread(NULL));
	dprintf("KPPPInterface: WaitForConnection():%s\n", successful ? "True" : "False");
	return successful;
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
	TRACE("KPPPInterface: LoadModules()\n");

	if (Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	ppp_module_key_type type;
		// which type key was used for loading this module?

	const char *name = NULL;

	// multilink handling
	for (int32 index = start;
			index < settings->parameter_count && index < (start + count); index++) {
		if (!strcasecmp(settings->parameters[index].name, PPP_MULTILINK_KEY)
				&& settings->parameters[index].value_count > 0) {
			if (!LoadModule(settings->parameters[index].values[0],
					&settings->parameters[index], PPP_MULTILINK_KEY_TYPE))
				return false;
			break;
		}
	}

	// are we a multilink main interface?
	if (IsMultilink() && !Parent()) {
		// main interfaces only load the multilink module
		// and create a child using their settings
		fManager->CreateInterface(settings, ID());
		return true;
	}

	for (int32 index = start;
			index < settings->parameter_count && index < start + count; index++) {
		type = PPP_UNDEFINED_KEY_TYPE;

		name = settings->parameters[index].name;

		if (!strcasecmp(name, PPP_LOAD_MODULE_KEY))
			type = PPP_LOAD_MODULE_KEY_TYPE;
		else if (!strcasecmp(name, PPP_DEVICE_KEY))
			type = PPP_DEVICE_KEY_TYPE;
		else if (!strcasecmp(name, PPP_PROTOCOL_KEY))
			type = PPP_PROTOCOL_KEY_TYPE;
		else if (!strcasecmp(name, PPP_AUTHENTICATOR_KEY))
			type = PPP_AUTHENTICATOR_KEY_TYPE;

		if (type >= 0)
			for (int32 value_id = 0; value_id < settings->parameters[index].value_count;
					value_id++)
				if (!LoadModule(settings->parameters[index].values[value_id],
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
	TRACE("KPPPInterface: LoadModule(%s)\n", name ? name : "XXX: NO NAME");

	if (Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change

	if (!name || strlen(name) > B_FILE_NAME_LENGTH)
		return false;

	char *moduleName = new char[B_PATH_NAME_LENGTH];

	sprintf(moduleName, "%s/%s", PPP_MODULES_PATH, name);

	ppp_module_info *module;
	if (get_module(moduleName, (module_info**) &module) != B_OK) {
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

	This brings the interface up if connect-on-demand is enabled and we are not
	connected. \n
	PFC encoding is handled here. \n
	NOTE: In order to prevent interface destruction while sending you must either
	hold a refcount for this interface or make sure it is locked.

	\param packet The packet.
	\param protocolNumber The packet's protocol number.

	\return
		- \c B_OK: Sending was successful.
		- \c B_ERROR: Some error occured.
*/
status_t
KPPPInterface::Send(net_buffer *packet, uint16 protocolNumber)
{
	TRACE("KPPPInterface: Send(0x%X)\n", protocolNumber);

	if (!packet)
		return B_ERROR;

	// we must pass the basic tests like:
	// do we have a device?
	// did we load all modules?
	if (InitCheck() != B_OK) {
		ERROR("InitCheck() fail\n");
		gBufferModule->free(packet);
		return B_ERROR;
	}


	// go up if ConnectOnDemand is enabled and we are disconnected
	// TODO: our new netstack will simplify ConnectOnDemand handling, so
	// we do not have to handle it here
	if ((protocolNumber != PPP_LCP_PROTOCOL && DoesConnectOnDemand()
			&& (Phase() == PPP_DOWN_PHASE
				|| Phase() == PPP_ESTABLISHMENT_PHASE)
			&& !Up()) && !WaitForConnection()) {
		dprintf("DoesConnectOnDemand fail!\n");
		// gBufferModule->free(packet);
		return B_ERROR;
	}

	// find the protocol handler for the current protocol number
	KPPPProtocol *protocol = ProtocolFor(protocolNumber);
	while (protocol && !protocol->IsEnabled())
		protocol = protocol->NextProtocol() ?
			ProtocolFor(protocolNumber, protocol->NextProtocol()) : NULL;

#if DEBUG
	if (!protocol)
		TRACE("KPPPInterface::Send(): no protocol found!\n");
	else if (!Device()->IsUp())
		TRACE("KPPPInterface::Send(): device is not up!\n");
	else if (!protocol->IsEnabled())
		TRACE("KPPPInterface::Send(): protocol not enabled!\n");
	else if (!IsProtocolAllowed(*protocol))
		TRACE("KPPPInterface::Send(): protocol not allowed to send!\n");
	else
		TRACE("KPPPInterface::Send(): protocol allowed\n");
#endif

	// make sure that protocol is allowed to send and everything is up
	if (Device()->IsUp() && protocolNumber == 0x0021) {
			// IP_PROTOCOL 0x0021
		TRACE("send IP packet!\n");
	} else if (!Device()->IsUp() || !protocol || !protocol->IsEnabled()
			|| !IsProtocolAllowed(*protocol)) {
		ERROR("KPPPInterface::Send(): Device is down, throw packet away!!\n");
		// gBufferModule->free(packet);
		return B_ERROR;
	}

	// encode in ppp frame and consider using PFC
	if (UseLocalPFC() && (protocolNumber & 0xFF00) == 0) {
		TRACE("%s::%s should not go here\n", __FILE__, __func__);
		NetBufferPrepend<uint8> bufferHeader(packet);
		if (bufferHeader.Status() != B_OK)
			return bufferHeader.Status();

		uint8 &header = bufferHeader.Data();

		// memcpy(header.destination, ether_pppoe_ppp_header, 1);
		header = (protocolNumber & 0x00FF);
		// bufferHeader.Sync();

	} else {
		NetBufferPrepend<uint16> bufferHeader(packet);
		if (bufferHeader.Status() != B_OK)
			return bufferHeader.Status();

		uint16 &header = bufferHeader.Data();

		// set protocol (the only header field)
		header = htons(protocolNumber);
		// bufferHeader.Sync();
	}

	// pass to device if we're either not a multilink interface or a child interface
	if (!IsMultilink() || Parent()) {
		// check if packet is too big for device
		uint32 length = packet->size;

		if (length > MRU()) {
			dprintf("packet too large!\n");
			gBufferModule->free(packet);
			return B_ERROR;
		}

		atomic_add64(&fStatistics.bytesSent, length);
		atomic_add64(&fStatistics.packetsSent, 1);
		TRACE("%s::%s SendToNext\n", __FILE__, __func__);
		return SendToNext(packet, 0);
			// this is normally the device, but there can be something inbetween
	} else {
		// the multilink protocol should have sent it to some child interface
		TRACE("It is weird to go here!\n");
		gBufferModule->free(packet);
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
KPPPInterface::Receive(net_buffer *packet, uint16 protocolNumber)
{
	TRACE("KPPPInterface: Receive(0x%X)\n", protocolNumber);

	if (!packet)
		return B_ERROR;

	MutexLocker locker(fLock);

	int32 result = PPP_REJECTED;
		// assume we have no handler

	if (protocolNumber == 0x0021 && Device() && Device()->IsUp()) {
			// IP_PROTOCOL 0x0021
		TRACE("%s::%s: receiving IP packet\n", __FILE__, __func__);
		ppp_device* dev=(ppp_device*)Ifnet();

		if (dev)
			return gStackModule->fifo_enqueue_buffer(&(dev->ppp_fifo), packet);
		else {
			dprintf("%s::%s: no ppp_device\n", __FILE__, __func__);
			return B_ERROR;
		}
	}
	// // Set our interface as the receiver.
	// // The real netstack protocols (IP, IPX, etc.) might get confused if our
	// // interface is a main interface and at the same time not registered
	// // because then there is no receiver interface.
	// // PPP NCPs should be aware of that!
	// if (packet->m_flags & M_PKTHDR && Ifnet() != NULL)
	// 	packet->m_pkthdr.rcvif = Ifnet();

	// Find handler and let it parse the packet.
	// The handler does need not be up because if we are a server
	// the handler might be upped by this packet.
	// If authenticating we only allow authentication phase protocols.
	KPPPProtocol *protocol = ProtocolFor(protocolNumber);
	for (; protocol;
			protocol = protocol->NextProtocol() ?
				ProtocolFor(protocolNumber, protocol->NextProtocol()) : NULL) {
		TRACE("KPPPInterface::Receive(): trying protocol\n");

		if (!protocol->IsEnabled() || !IsProtocolAllowed(*protocol))
			continue;
				// skip handler if disabled or not allowed

		result = protocol->Receive(packet, protocolNumber);
		if (result == PPP_UNHANDLED)
			continue;

		return result;
	}

	TRACE("KPPPInterface::Receive(): trying parent\n");

	// maybe the parent interface can handle the packet
	if (Parent())
		return Parent()->Receive(packet, protocolNumber);

	if (result == PPP_UNHANDLED) {
		gBufferModule->free(packet);
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
KPPPInterface::ReceiveFromDevice(net_buffer *packet)
{
	TRACE("KPPPInterface: ReceiveFromDevice()\n");

	if (!packet)
		return B_ERROR;

	if (InitCheck() != B_OK) {
		gBufferModule->free(packet);
		return B_ERROR;
	}

	uint32 length = packet->size;
	// decode ppp frame and recognize PFC
	NetBufferHeaderReader<uint16> bufferHeader(packet);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	uint16 &header = bufferHeader.Data();
	uint16 protocolNumber = ntohs(header); // copy the protocol number
	TRACE("%s::%s: ppp protocol number:%x\n", __FILE__, __func__, protocolNumber);

	bufferHeader.Remove();
	bufferHeader.Sync();

	// PFC only use 1 byte for protocol, the next byte is in first byte of message
	if (protocolNumber & (1UL<<8)) {
		NetBufferPrepend<uint8> bufferprepend(packet);
		if (bufferprepend.Status() < B_OK) {
			gBufferModule->free(packet);
			return bufferprepend.Status();
		}

		uint8 &prepend = bufferprepend.Data();
		prepend = (uint8)(protocolNumber & 0x00FF);
		// bufferprepend.Sync();
		TRACE("%s::%s:PFC protocol:%x\n", __FILE__, __func__, protocolNumber>>8);
	} else {
		TRACE("%s::%s:Non-PFC protocol:%x\n", __FILE__, __func__, protocolNumber);
	}

	atomic_add64(&fStatistics.bytesReceived, length);
	atomic_add64(&fStatistics.packetsReceived, 1);
	return Receive(packet, protocolNumber);
}


//!	Manages Pulse() calls for all add-ons and hanldes idle-disconnection.
void
KPPPInterface::Pulse()
{
	MutexLocker locker(fLock);

	if (Device())
		Device()->Pulse();

	KPPPProtocol *protocol = FirstProtocol();
	for (; protocol; protocol = protocol->NextProtocol())
		protocol->Pulse();

	uint32 currentTime = real_time_clock();
	if (fUpdateIdleSince) {
		fIdleSince = currentTime;
		fUpdateIdleSince = false;
	}

	// check our idle time and disconnect if needed
	if (fDisconnectAfterIdleSince > 0 && fIdleSince != 0
			&& fIdleSince - currentTime >= fDisconnectAfterIdleSince)
		StateMachine().CloseEvent();
}


//!	Registers an ifnet structure for this interface.
bool
KPPPInterface::RegisterInterface()
{
	TRACE("KPPPInterface: RegisterInterface()\n");

	if (fIfnet)
		return true;
			// we are already registered

	MutexLocker locker(fLock);

	// only MainInterfaces get an ifnet
	if (IsMultilink() && Parent() && Parent()->RegisterInterface())
		return true;

	if (!fManager)
		return false;

	fIfnet = fManager->RegisterInterface(ID());

	if (!fIfnet) {
		dprintf("%s:%s: damn it no fIfnet device\n", __FILE__, __func__);
		return false;
	}

	if (DoesConnectOnDemand())
		fIfnet->flags |= IFF_UP;

	CalculateInterfaceMTU();
	CalculateBaudRate();

	return true;
}


//!	Unregisters this interface's ifnet structure.
bool
KPPPInterface::UnregisterInterface()
{
	TRACE("KPPPInterface: UnregisterInterface()\n");

	if (!fIfnet)
		return true;
			// we are already unregistered

	MutexLocker locker(fLock);

	// only MainInterfaces get an ifnet
	if (IsMultilink() && Parent())
		return true;

	if (!fManager)
		return false;

	fManager->UnregisterInterface(ID());
		// this will delete fIfnet, so do not access it anymore!
	fIfnet = NULL;

	return true;
}


//!	Called by KPPPManager: manager routes stack ioctls to the corresponding interface.
status_t
KPPPInterface::StackControl(uint32 op, void *data)
{
	TRACE("KPPPInterface: StackControl(0x%lX)\n", op);

	switch (op) {
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
				if (!item || !item->IsEnabled())
					return;
				status_t tmp = item->StackControl(fOp, fData);
				if (tmp == B_OK && fResult == B_BAD_VALUE)
					fResult = B_OK;
				else if (tmp != B_BAD_VALUE)
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
	TRACE("KPPPInterface: StackControlEachHandler(0x%lX)\n", op);

	status_t result = B_BAD_VALUE, tmp;

	MutexLocker locker(fLock);

	KPPPProtocol *protocol = FirstProtocol();
	for (; protocol; protocol = protocol->NextProtocol()) {
		tmp = protocol->StackControl(op, data);
		if (tmp == B_OK && result == B_BAD_VALUE)
			result = B_OK;
		else if (tmp != B_BAD_VALUE)
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
	TRACE("KPPPInterface: CalculateInterfaceMTU()\n");

	// MutexLocker locker(fLock);
		// uncomment this line will cause double lock
		// alreay locked in ::KPPPInterface

	fInterfaceMTU = fMRU;
	fHeaderLength = 2;

	// sum all headers (the protocol field is not counted)
	KPPPProtocol *protocol = FirstProtocol();
	for (; protocol; protocol = protocol->NextProtocol()) {
		if (protocol->Level() < PPP_PROTOCOL_LEVEL)
			fHeaderLength += protocol->Overhead();
	}

	fInterfaceMTU -= fHeaderLength;

	if (Ifnet()) {
		Ifnet()->mtu = fInterfaceMTU;
		Ifnet()->header_length = fHeaderLength;
		return;
	}

	if (Parent())
		Parent()->CalculateInterfaceMTU();
}


//!	Recalculates the baud rate.
void
KPPPInterface::CalculateBaudRate()
{
	TRACE("KPPPInterface: CalculateBaudRate()\n");

	// MutexLocker locker(fLock); // uncomment this will cause double lock

	if (!Ifnet())
		return;

	if (Device())
		fIfnet->link_speed = max_c(Device()->InputTransferRate(),
		Device()->OutputTransferRate());
	else {
		fIfnet->link_speed = 0;
		for (int32 index = 0; index < CountChildren(); index++)
			if (ChildAt(index)->Ifnet())
				fIfnet->link_speed += ChildAt(index)->Ifnet()->link_speed;
				return;
	}
}


//!	Reconnects. Waits a given delay (in miliseconds) before reconnecting.
void
KPPPInterface::Reconnect(uint32 delay)
{
	TRACE("KPPPInterface: Reconnect(%ld)\n", delay);

	MutexLocker locker(fLock);

	if (fReconnectThread != -1)
		return;

	++fConnectAttempt;

	// start a new thread that calls our Up() method
	reconnect_info info;
	info.interface = this;
	info.thread = &fReconnectThread;
	info.delay = delay;

	fReconnectThread = spawn_kernel_thread(reconnect_thread,
		"KPPPInterface: reconnect_thread", B_NORMAL_PRIORITY, NULL);

	resume_thread(fReconnectThread);

	send_data(fReconnectThread, 0, &info, sizeof(reconnect_info));
}


status_t
reconnect_thread(void *data)
{
	reconnect_info info;
	thread_id sender;
	int32 code;

	receive_data(&sender, &info, sizeof(reconnect_info));

	// we try to receive data instead of snooze, so we can quit on destruction
	if (receive_data_with_timeout(&sender, &code, NULL, 0, info.delay) == B_OK) {
		*info.thread = -1;
		return B_OK;
	}

	info.interface->Up();
	*info.thread = -1;

	return B_OK;
}
