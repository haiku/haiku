//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "PPPManager.h"
#include <PPPControl.h>
#include <KPPPUtils.h>

#include <kernel_cpp.h>
#include <LockerHelper.h>

#include <stdlib.h>
#include <sys/sockio.h>


#ifdef _KERNEL_MODE
#define spawn_thread spawn_kernel_thread
#endif


static const char ppp_if_name_base[] = "ppp";


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
		if(receive_data_with_timeout(&sender, &code, NULL, 0, 50000) == B_OK)
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
	fDeleterThread = spawn_thread(deleter_thread, "PPPManager: deleter_thread",
		B_NORMAL_PRIORITY, this);
	resume_thread(fDeleterThread);
	fPulseTimer = net_add_timer(pulse_timer, this, PPP_PULSE_RATE);
}


PPPManager::~PPPManager()
{
	int32 tmp;
	send_data(fDeleterThread, 0, NULL, 0);
		// notify deleter_thread that we are being destroyed
	wait_for_thread(fDeleterThread, &tmp);
	net_remove_timer(fPulseTimer);
	
	// now really delete the interfaces (the deleter_thread is not running)
	interface_entry *entry;
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
	LockerHelper locker(fLock);
	
	interface_entry *entry = EntryFor(ifp);
	if(!entry)
		return B_ERROR;
	
	DeleteInterface(entry->interface->ID());
	
	return B_OK;
}


int32
PPPManager::Output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
	struct rtentry *rt0)
{
	if(dst->sa_family == AF_UNSPEC)
		return B_ERROR;
	
	LockerHelper locker(fLock);
	interface_entry *entry = EntryFor(ifp);
	if(!entry)
		return B_ERROR;
	
	++entry->accessing;
	locker.UnlockNow();
	
	if(ifp->if_flags & (IFF_UP | IFF_RUNNING) != (IFF_UP | IFF_RUNNING)) {
		m_freem(buf);
		--entry->accessing;
		return ENETDOWN;
	}
	
	int32 result = B_ERROR;
	PPPProtocol *protocol = entry->interface->FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol()) {
		if(!protocol->IsEnabled() || protocol->AddressFamily() != dst->sa_family)
			continue;
		
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
	LockerHelper locker(fLock);
	interface_entry *entry = EntryFor(ifp);
	if(!entry || entry->deleting)
		return B_ERROR;
	
	++entry->accessing;
	locker.UnlockNow();
	
	switch(cmd) {
		case SIOCSIFFLAGS:
			if(((ifreq*)data)->ifr_flags & IFF_DOWN)
				DeleteInterface(entry->interface->ID());
			else if(((ifreq*)data)->ifr_flags & IFF_UP) {
				int32 result = entry->interface->Up() ? B_OK : B_ERROR;
				--entry->accessing;
				return result;
			}
		break;
		
		default:
			return entry->interface->StackControl(cmd, data);
	}
	
	--entry->accessing;
	return B_OK;
}


interface_id
PPPManager::CreateInterface(const driver_settings *settings,
	interface_id parentID = PPP_UNDEFINED_INTERFACE_ID)
{
	LockerHelper locker(fLock);
	
	interface_entry *parentEntry = EntryFor(parentID);
	if(parentID != PPP_UNDEFINED_INTERFACE_ID && !parentEntry)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	interface_id id = NextID();
	interface_entry *entry = new interface_entry;
	entry->accessing = 0;
	entry->deleting = false;
	entry->interface = new PPPInterface(id, settings,
		parentEntry ? parentEntry->interface : NULL);
	
	if(entry->interface->InitCheck() != B_OK) {
		delete entry->interface;
		delete entry;
		return PPP_UNDEFINED_INTERFACE_ID;
	}
	
	fEntries.AddItem(entry);
	locker.UnlockNow();
	
	if(Report(PPP_MANAGER_REPORT, PPP_REPORT_INTERFACE_CREATED,
			&id, sizeof(interface_id)) != B_OK) {
		DeleteInterface(id);
		return PPP_UNDEFINED_INTERFACE_ID;
	}
	
	if(!entry->interface->DoesDialOnDemand())
		entry->interface->Up();
	
	return id;
}


void
PPPManager::DeleteInterface(interface_id ID)
{
	// This only marks the interface for deletion.
	// Our deleter_thread does the real work.
	LockerHelper locker(fLock);
	
	interface_entry *entry = EntryFor(ID);
	if(entry)
		entry->deleting = true;
}


void
PPPManager::RemoveInterface(interface_id ID)
{
	LockerHelper locker(fLock);
	
	int32 index;
	interface_entry *entry = EntryFor(ID, &index);
	if(!entry || entry->deleting)
		return;
	
	UnregisterInterface(ID);
	
	delete entry;
	fEntries.RemoveItem(index);
}


ifnet*
PPPManager::RegisterInterface(interface_id ID)
{
	LockerHelper locker(fLock);
	
	interface_entry *entry = EntryFor(ID);
	if(!entry || entry->deleting)
		return NULL;
	
	// maybe the interface is already registered
	if(entry->interface->Ifnet())
		return entry->interface->Ifnet();
	
	ifnet *ifp = (ifnet*) malloc(sizeof(ifnet));
	memset(ifp, 0, sizeof(ifnet));
	ifp->devid = -1;
	ifp->if_type = IFT_PPP;
	ifp->name = ppp_if_name_base;
	ifp->if_unit = FindUnit();
	ifp->if_flags = IFF_POINTOPOINT | IFF_UP;
	ifp->rx_thread = ifp->tx_thread = -1;
	ifp->start = NULL;
	ifp->stop = ppp_ifnet_stop;
	ifp->output = ppp_ifnet_output;
	ifp->ioctl = ppp_ifnet_ioctl;
	
	if_attach(ifp);
	return ifp;
}


bool
PPPManager::UnregisterInterface(interface_id ID)
{
	LockerHelper locker(fLock);
	
	interface_entry *entry = EntryFor(ID);
	if(!entry)
		return false;
	
	if(entry->interface->Ifnet()) {
		if_detach(entry->interface->Ifnet());
		delete entry->interface->Ifnet();
	}
	
	return true;
}


status_t
PPPManager::Control(uint32 op, void *data, size_t length)
{
	// this method is intended for use by userland applications
	
	switch(op) {
		case PPPC_CREATE_INTERFACE: {
			if(length < sizeof(pppc_create_interface_info) || !data)
				return B_ERROR;
			
			pppc_create_interface_info *info = (pppc_create_interface_info*) data;
			if(!info->settings)
				return B_ERROR;
			
			info->resultID = CreateInterface(info->settings);
				// parents cannot be set from userland
			return info->resultID != PPP_UNDEFINED_INTERFACE_ID ? B_OK : B_ERROR;
		} break;
		
		case PPPC_DELETE_INTERFACE:
			if(length != sizeof(interface_id) || !data)
				return B_ERROR;
			
			DeleteInterface(*(interface_id*)data);
		break;
		
		case PPPC_CONTROL_INTERFACE: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			LockerHelper locker(fLock);
			
			ppp_control_info *control = (ppp_control_info*) data;
			interface_entry *entry = EntryFor(control->index);
			if(!entry)
				return B_BAD_INDEX;
			
			return entry->interface->Control(control->op, control->data,
				control->length);
		} break;
		
		case PPPC_GET_INTERFACES: {
			if(length < sizeof(pppc_get_interfaces_info) || !data)
				return B_ERROR;
			
			pppc_get_interfaces_info *info = (pppc_get_interfaces_info*) data;
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
		
		case PPPC_ENABLE_MANAGER_REPORTS: {
			if(length < sizeof(ppp_report_request) || !data)
				return B_ERROR;
			
			ppp_report_request *request = (ppp_report_request*) data;
			ReportManager().EnableReports(request->type, request->thread,
				request->flags);
		} break;
		
		case PPPC_DISABLE_MANAGER_REPORTS: {
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
PPPManager::ControlInterface(interface_id ID, uint32 op, void *data, size_t length)
{
	LockerHelper locker(fLock);
	
	interface_entry *entry = EntryFor(ID);
	if(entry && !entry->deleting)
		return entry->interface->Control(op, data, length);
	
	return B_BAD_INDEX;
}


int32
PPPManager::GetInterfaces(interface_id *interfaces, int32 count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	LockerHelper locker(fLock);
	
	int32 item = 0;
		// the current item in 'interfaces' (also the number of ids copied)
	interface_entry *entry;
	
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
	LockerHelper locker(fLock);
	
	if(filter == PPP_ALL_INTERFACES)
		return fEntries.CountItems();
	
	int32 count = 0;
	interface_entry *entry;
	
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


interface_entry*
PPPManager::EntryFor(interface_id ID, int32 *saveIndex = NULL) const
{
	if(ID == PPP_UNDEFINED_INTERFACE_ID)
		return NULL;
	
	interface_entry *entry;
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


interface_entry*
PPPManager::EntryFor(ifnet *ifp, int32 *saveIndex = NULL) const
{
	if(!ifp)
		return NULL;
	
	interface_entry *entry;
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


static
int
greater(const void *a, const void *b)
{
	return (*(const int*)a - *(const int*)b);
}


int32
PPPManager::FindUnit() const
{
	// Find the smallest unused unit.
	int32 *units = new int32[fEntries.CountItems()];
	
	interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->Ifnet())
			units[index] = entry->interface->Ifnet()->if_unit;
		else
			units[index] = -1;
	}
	
	qsort(units, fEntries.CountItems(), sizeof(int32), greater);
	
	int32 unit = 0;
	for(int32 index = 0; index < fEntries.CountItems() - 1; index++) {
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
	interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(!entry) {
			fEntries.RemoveItem(index);
			--index;
			continue;
		}
		
		if(entry->deleting && entry->accessing == 0) {
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
	
	interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry)
			entry->interface->Pulse();
	}
}
