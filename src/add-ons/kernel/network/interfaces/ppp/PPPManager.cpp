//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <cstdio>

#include "PPPManager.h"
#include <PPPControl.h>
#include <KPPPModule.h>
#include <KPPPUtils.h>
#include <settings_tools.h>

#include <net_stack_driver.h>

#include <LockerHelper.h>

#include <cstdlib>
#include <sys/sockio.h>


static const char sKPPPIfNameBase[] =	"ppp";


static
status_t
interface_up_thread(void *data)
{
	ppp_interface_entry *entry = (ppp_interface_entry*) data;
	
	entry->interface->Up();
	--entry->accessing;
	
	return B_OK;
}


static
status_t
bring_interface_up(ppp_interface_entry *entry)
{
	thread_id upThread = spawn_kernel_thread(interface_up_thread,
		"PPPManager: up_thread", B_NORMAL_PRIORITY, entry);
	resume_thread(upThread);
	
	return B_OK;
}


#if DOWN_AS_THREAD
static
status_t
interface_down_thread(void *data)
{
	ppp_interface_entry *entry = (ppp_interface_entry*) data;
	
	entry->interface->Down();
	--entry->accessing;
	
	return B_OK;
}
#endif


static
status_t
bring_interface_down(ppp_interface_entry *entry)
{
#if DOWN_AS_THREAD
	thread_id downThread = spawn_kernel_thread(interface_down_thread,
		"PPPManager: down_thread", B_NORMAL_PRIORITY, entry);
	resume_thread(downThread); */
#else
	entry->interface->Down();
	--entry->accessing;
#endif
	
	return B_OK;
}


static
status_t
deleter_thread(void *data)
{
	PPPManager *manager = (PPPManager*) data;
	thread_id sender;
	int32 code;
	
	while(true) {
		manager->DeleterThreadEvent();
		
		// check if the manager is being destroyed
		if(receive_data_with_timeout(&sender, &code, NULL, 0, 500) == B_OK)
			return B_OK;
	}
	
	return B_OK;
}


static
void
pulse_timer(void *data)
{
	((PPPManager*)data)->Pulse();
}


PPPManager::PPPManager()
	: fReportManager(fReportLock),
	fNextID(1)
{
#if DEBUG
	dprintf("PPPManager: Constructor\n");
#endif
	
	fDeleterThread = spawn_kernel_thread(deleter_thread, "PPPManager: deleter_thread",
		B_NORMAL_PRIORITY, this);
	resume_thread(fDeleterThread);
	fPulseTimer = net_add_timer(pulse_timer, this, PPP_PULSE_RATE);
}


PPPManager::~PPPManager()
{
#if DEBUG
	dprintf("PPPManager: Destructor\n");
#endif
	
	int32 tmp;
	send_data(fDeleterThread, 0, NULL, 0);
		// notify deleter_thread that we are being destroyed
	wait_for_thread(fDeleterThread, &tmp);
	net_remove_timer(fPulseTimer);
	
	// now really delete the interfaces (the deleter_thread is not running)
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry)
			delete entry->interface;
		delete entry;
	}
}


int32
PPPManager::Stop(ifnet *ifp)
{
#if DEBUG
	dprintf("PPPManager: Stop(%s)\n", ifp ? ifp->if_name : "Unknown");
#endif
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ifp);
	if(!entry) {
		dprintf("PPPManager: Stop(): Could not find interface!\n");
		return B_ERROR;
	}
	
	DeleteInterface(entry->interface->ID());
	
	return B_OK;
}


int32
PPPManager::Output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
	struct rtentry *rt0)
{
#if DEBUG
	dprintf("PPPManager: Output(%s)\n", ifp ? ifp->if_name : "Unknown");
#endif
	
	if(dst->sa_family == AF_UNSPEC)
		return B_ERROR;
	
	LockerHelper locker(fLock);
	ppp_interface_entry *entry = EntryFor(ifp);
	if(!entry) {
		dprintf("PPPManager: Output(): Could not find interface!\n");
		return B_ERROR;
	}
	
	++entry->accessing;
	locker.UnlockNow();
	
	if(!entry->interface->DoesDialOnDemand()
			&& ifp->if_flags & (IFF_UP | IFF_RUNNING) != (IFF_UP | IFF_RUNNING)) {
		m_freem(buf);
		--entry->accessing;
		return ENETDOWN;
	}
	
	int32 result = B_ERROR;
	KPPPProtocol *protocol = entry->interface->FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol()) {
		if(!protocol->IsEnabled() || protocol->AddressFamily() != dst->sa_family)
			continue;
		
		if(protocol->Flags() & PPP_INCLUDES_NCP)
			result = protocol->Send(buf, protocol->ProtocolNumber() & 0x7FFF);
		else
			result = protocol->Send(buf, protocol->ProtocolNumber());
		if(result == PPP_UNHANDLED)
			continue;
		
		--entry->accessing;
		return result;
	}
	
	m_freem(buf);
	--entry->accessing;
	return B_ERROR;
}


int32
PPPManager::Control(ifnet *ifp, ulong cmd, caddr_t data)
{
#if DEBUG
	dprintf("PPPManager: Control(%s)\n", ifp ? ifp->if_name : "Unknown");
#endif
	
	LockerHelper locker(fLock);
	ppp_interface_entry *entry = EntryFor(ifp);
	if(!entry || entry->deleting) {
		dprintf("PPPManager: Control(): Could not find interface!\n");
		return B_ERROR;
	}
	
	int32 status = B_OK;
	++entry->accessing;
	locker.UnlockNow();
	
	switch(cmd) {
		case SIOCSIFFLAGS:
			if(((ifreq*)data)->ifr_flags & IFF_DOWN) {
				if(entry->interface->DoesDialOnDemand()
						&& entry->interface->Phase() == PPP_DOWN_PHASE)
					DeleteInterface(entry->interface->ID());
				else
					return bring_interface_down(entry);
			} else if(((ifreq*)data)->ifr_flags & IFF_UP)
				return bring_interface_up(entry);
		break;
		
		default:
			status = entry->interface->StackControl(cmd, data);
	}
	
	--entry->accessing;
	return status;
}


ppp_interface_id
PPPManager::CreateInterface(const driver_settings *settings,
	const driver_settings *profile = NULL,
	ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID)
{
	return _CreateInterface(NULL, settings, profile, parentID);
}


ppp_interface_id
PPPManager::CreateInterfaceWithName(const char *name,
	const driver_settings *profile = NULL,
	ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID)
{
	if(!name)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	ppp_interface_id result = _CreateInterface(name, NULL, profile, parentID);
	
	return result;
}


bool
PPPManager::DeleteInterface(ppp_interface_id ID)
{
#if DEBUG
	dprintf("PPPManager: DeleteInterface(%ld)\n", ID);
#endif
	
	// This only marks the interface for deletion.
	// Our deleter_thread does the real work.
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ID);
	if(!entry)
		return false;
	
	if(entry->deleting)
		return true;
			// this check prevents a dead-lock
	
	entry->deleting = true;
	++entry->accessing;
	locker.UnlockNow();
	
	// bring interface down if needed
	if(entry->interface->State() != PPP_INITIAL_STATE
			|| entry->interface->Phase() != PPP_DOWN_PHASE)
		entry->interface->Down();
	
	--entry->accessing;
	
	return true;
}


bool
PPPManager::RemoveInterface(ppp_interface_id ID)
{
#if DEBUG
	dprintf("PPPManager: RemoveInterface(%ld)\n", ID);
#endif
	
	LockerHelper locker(fLock);
	
	int32 index;
	ppp_interface_entry *entry = EntryFor(ID, &index);
	if(!entry || entry->deleting)
		return false;
	
	UnregisterInterface(ID);
	
	delete entry;
	fEntries.RemoveItem(index);
	
	return true;
}


ifnet*
PPPManager::RegisterInterface(ppp_interface_id ID)
{
#if DEBUG
	dprintf("PPPManager: RegisterInterface(%ld)\n", ID);
#endif
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ID);
	if(!entry || entry->deleting)
		return NULL;
	
	// maybe the interface is already registered
	if(entry->interface->Ifnet())
		return entry->interface->Ifnet();
	
	ifnet *ifp = (ifnet*) malloc(sizeof(ifnet));
	memset(ifp, 0, sizeof(ifnet));
	ifp->devid = -1;
	ifp->if_type = IFT_PPP;
	ifp->name = sKPPPIfNameBase;
	ifp->if_unit = FindUnit();
	ifp->if_flags = IFF_POINTOPOINT;
	ifp->rx_thread = ifp->tx_thread = -1;
	ifp->start = NULL;
	ifp->stop = ppp_ifnet_stop;
	ifp->output = ppp_ifnet_output;
	ifp->ioctl = ppp_ifnet_ioctl;
	
#if DEBUG
	dprintf("PPPManager::RegisterInterface(): Created new ifnet: %s%d\n",
		ifp->name, ifp->if_unit);
#endif
	
	if_attach(ifp);
	return ifp;
}


bool
PPPManager::UnregisterInterface(ppp_interface_id ID)
{
#if DEBUG
	dprintf("PPPManager: UnregisterInterface(%ld)\n", ID);
#endif
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ID);
	if(!entry)
		return false;
	
	if(entry->interface->Ifnet()) {
		if_detach(entry->interface->Ifnet());
		free(entry->interface->Ifnet());
	}
	
	return true;
}


status_t
PPPManager::Control(uint32 op, void *data, size_t length)
{
#if DEBUG
	dprintf("PPPManager: Control(%ld)\n", op);
#endif
	
	// this method is intended for use by userland applications
	
	switch(op) {
		case PPPC_CONTROL_MODULE: {
			if(length < sizeof(control_net_module_args) || !data)
				return B_ERROR;
			
			control_net_module_args *args = (control_net_module_args*) data;
			if(!args->name)
				return B_ERROR;
			
			char name[B_PATH_NAME_LENGTH];
			sprintf(name, "%s/%s", PPP_MODULES_PATH, args->name);
			ppp_module_info *module;
			if(get_module(name, (module_info**) &module) != B_OK
					|| !module->control)
				return B_NAME_NOT_FOUND;
			
			return module->control(args->op, args->data, args->length);
		} break;
		
		case PPPC_CREATE_INTERFACE: {
			if(length < sizeof(ppp_interface_description_info) || !data)
				return B_ERROR;
			
			ppp_interface_description_info *info
				= (ppp_interface_description_info*) data;
			if(!info->u.settings)
				return B_ERROR;
			
			info->interface = CreateInterface(info->u.settings, info->profile);
				// parents cannot be set from userland
			return info->interface != PPP_UNDEFINED_INTERFACE_ID ? B_OK : B_ERROR;
		} break;
		
		case PPPC_CREATE_INTERFACE_WITH_NAME: {
			if(length < sizeof(ppp_interface_description_info) || !data)
				return B_ERROR;
			
			ppp_interface_description_info *info
				= (ppp_interface_description_info*) data;
			if(!info->u.name)
				return B_ERROR;
			
			info->interface = CreateInterfaceWithName(info->u.name, info->profile);
				// parents cannot be set from userland
			return info->interface != PPP_UNDEFINED_INTERFACE_ID ? B_OK : B_ERROR;
		} break;
		
		case PPPC_DELETE_INTERFACE:
			if(length != sizeof(ppp_interface_id) || !data)
				return B_ERROR;
			
			if(!DeleteInterface(*(ppp_interface_id*)data))
				return B_ERROR;
		break;
		
		case PPPC_BRING_INTERFACE_UP: {
			if(length != sizeof(ppp_interface_id) || !data)
				return B_ERROR;
			
			LockerHelper locker(fLock);
			
			ppp_interface_entry *entry = EntryFor(*(ppp_interface_id*)data);
			if(!entry || entry->deleting)
				return B_BAD_INDEX;
			
			++entry->accessing;
			locker.UnlockNow();
			
			return bring_interface_up(entry);
		} break;
		
		case PPPC_BRING_INTERFACE_DOWN: {
			if(length != sizeof(ppp_interface_id) || !data)
				return B_ERROR;
			
			LockerHelper locker(fLock);
			
			ppp_interface_entry *entry = EntryFor(*(ppp_interface_id*)data);
			if(!entry || entry->deleting)
				return B_BAD_INDEX;
			
			++entry->accessing;
			locker.UnlockNow();
			
			return bring_interface_down(entry);
		} break;
		
		case PPPC_CONTROL_INTERFACE: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			
			return ControlInterface(control->index, control->op, control->data,
				control->length);
		} break;
		
		case PPPC_GET_INTERFACES: {
			if(length < sizeof(ppp_get_interfaces_info) || !data)
				return B_ERROR;
			
			ppp_get_interfaces_info *info = (ppp_get_interfaces_info*) data;
			if(!info->interfaces)
				return B_ERROR;
			
			info->resultCount = GetInterfaces(info->interfaces, info->count,
				info->filter);
			return info->resultCount >= 0 ? B_OK : B_ERROR;
		} break;
		
		case PPPC_COUNT_INTERFACES:
			if(length != sizeof(ppp_interface_filter) || !data)
				return B_ERROR;
			
			return CountInterfaces(*(ppp_interface_filter*)data);
		break;
		
		case PPPC_FIND_INTERFACE_WITH_SETTINGS: {
			if(length < sizeof(ppp_interface_description_info) || !data)
				return B_ERROR;
			
			ppp_interface_description_info *info
				= (ppp_interface_description_info*) data;
			if(!info->u.settings)
				return B_ERROR;
			
			LockerHelper locker(fLock);
			
			ppp_interface_entry *entry = EntryFor(info->u.settings);
			if(entry)
				info->interface = entry->interface->ID();
			else {
				info->interface = PPP_UNDEFINED_INTERFACE_ID;
				return B_ERROR;
			}
		} break;
		
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
		
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


status_t
PPPManager::ControlInterface(ppp_interface_id ID, uint32 op, void *data, size_t length)
{
#if DEBUG
	dprintf("PPPManager: ControlInterface(%ld)\n", ID);
#endif
	
	LockerHelper locker(fLock);
	
	status_t result = B_BAD_INDEX;
	ppp_interface_entry *entry = EntryFor(ID);
	if(entry && !entry->deleting) {
		++entry->accessing;
		locker.UnlockNow();
		result = entry->interface->Control(op, data, length);
		--entry->accessing;
	}
	
	return result;
}


int32
PPPManager::GetInterfaces(ppp_interface_id *interfaces, int32 count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
#if DEBUG
	dprintf("PPPManager: GetInterfaces()\n");
#endif
	
	LockerHelper locker(fLock);
	
	int32 item = 0;
		// the current item in 'interfaces' (also the number of ids copied)
	ppp_interface_entry *entry;
	
	for(int32 index = 0; item < count && index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(!entry || entry->deleting)
			continue;
		
		switch(filter) {
			case PPP_ALL_INTERFACES:
				interfaces[item++] = entry->interface->ID();
			break;
			
			case PPP_REGISTERED_INTERFACES:
				if(entry->interface->Ifnet())
					interfaces[item++] = entry->interface->ID();
			break;
			
			case PPP_UNREGISTERED_INTERFACES:
				if(!entry->interface->Ifnet())
					interfaces[item++] = entry->interface->ID();
			break;
			
			default:
				return B_ERROR;
					// the filter is unknown
		}
	}
	
	return item;
}


int32
PPPManager::CountInterfaces(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
#if DEBUG
	dprintf("PPPManager: CountInterfaces()\n");
#endif
	
	LockerHelper locker(fLock);
	
	if(filter == PPP_ALL_INTERFACES)
		return fEntries.CountItems();
	
	int32 count = 0;
	ppp_interface_entry *entry;
	
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(!entry || entry->deleting)
			continue;
		
		switch(filter) {
			case PPP_REGISTERED_INTERFACES:
				if(entry->interface->Ifnet())
					++count;
			break;
			
			case PPP_UNREGISTERED_INTERFACES:
				if(!entry->interface->Ifnet())
					++count;
			break;
			
			default:
				return B_ERROR;
					// the filter is unknown
		}
	}
	
	return count;
}


ppp_interface_entry*
PPPManager::EntryFor(ppp_interface_id ID, int32 *saveIndex = NULL) const
{
#if DEBUG
	dprintf("PPPManager: EntryFor(%ld)\n", ID);
#endif
	
	if(ID == PPP_UNDEFINED_INTERFACE_ID)
		return NULL;
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->ID() == ID) {
			if(saveIndex)
				*saveIndex = index;
			return entry;
		}
	}
	
	return NULL;
}


ppp_interface_entry*
PPPManager::EntryFor(ifnet *ifp, int32 *saveIndex = NULL) const
{
	if(!ifp)
		return NULL;
	
#if DEBUG
	dprintf("PPPManager: EntryFor(%s%d)\n", ifp->name, ifp->if_unit);
#endif
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->Ifnet() == ifp) {
			if(saveIndex)
				*saveIndex = index;
			return entry;
		}
	}
	
	return NULL;
}


ppp_interface_entry*
PPPManager::EntryFor(const driver_settings *settings) const
{
	if(!settings)
		return NULL;
	
#if DEBUG
	dprintf("PPPManager: EntryFor(settings)\n");
#endif
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && equal_interface_settings(entry->interface->Settings(), settings))
			return entry;
	}
	
	return NULL;
}


static
int
greater(const void *a, const void *b)
{
	return (*(const int*)a - *(const int*)b);
}


// used by the public CreateInterface() methods
ppp_interface_id
PPPManager::_CreateInterface(const char *name, const driver_settings *settings,
	const driver_settings *profile, ppp_interface_id parentID)
{
#if DEBUG
	dprintf("PPPManager: CreateInterface(%s)\n", name ? name : "Unnamed");
#endif
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *parentEntry = EntryFor(parentID);
	if(parentID != PPP_UNDEFINED_INTERFACE_ID && !parentEntry)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	// the day when NextID() returns 0 will come, be prepared! ;)
	ppp_interface_id id = NextID();
	if(id == PPP_UNDEFINED_INTERFACE_ID)
		id = NextID();
	
	ppp_interface_entry *entry = new ppp_interface_entry;
	entry->accessing = 1;
	entry->deleting = false;
	fEntries.AddItem(entry);
		// nothing bad can happen because we are in a locked section
	
	new KPPPInterface(name, entry, id, settings, profile,
		parentEntry ? parentEntry->interface : NULL);
			// KPPPInterface will add itself to the entry (no need to do it here)
	if(entry->interface->InitCheck() != B_OK) {
		delete entry->interface;
		delete entry;
		return PPP_UNDEFINED_INTERFACE_ID;
	}
	
	locker.UnlockNow();
		// it is safe to access the manager from userland now
	
	if(!Report(PPP_MANAGER_REPORT, PPP_REPORT_INTERFACE_CREATED,
			&id, sizeof(ppp_interface_id))) {
		DeleteInterface(id);
		--entry->accessing;
		return PPP_UNDEFINED_INTERFACE_ID;
	}
	
	// notify handlers that interface has been created and they can initialize it now
	entry->interface->StateMachine().DownProtocols();
	entry->interface->StateMachine().ResetLCPHandlers();
	
	--entry->accessing;
	
	return id;
}


int32
PPPManager::FindUnit() const
{
	// Find the smallest unused unit.
	int32 *units = new int32[fEntries.CountItems()];
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->Ifnet())
			units[index] = entry->interface->Ifnet()->if_unit;
		else
			units[index] = -1;
	}
	
	qsort(units, fEntries.CountItems(), sizeof(int32), greater);
	
	int32 unit = 0;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		if(units[index] > unit)
			return unit;
		else if(units[index] == unit)
			++unit;
	}
	
	return unit;
}


void
PPPManager::DeleterThreadEvent()
{
	LockerHelper locker(fLock);
	
	// delete and remove marked interfaces
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(!entry) {
			fEntries.RemoveItem(index);
			--index;
			continue;
		}
		
		if(entry->deleting && entry->accessing <= 0) {
			delete entry->interface;
			delete entry;
			fEntries.RemoveItem(index);
			--index;
		}
	}
}


void
PPPManager::Pulse()
{
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry)
			entry->interface->Pulse();
	}
}
