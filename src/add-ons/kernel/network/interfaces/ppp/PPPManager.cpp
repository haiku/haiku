//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "PPPManager.h"

#include <core_funcs.h>
#include <kernel_cpp.h>
#include <LockerHelper.h>

#include <stdlib.h>


static int ppp_ifnet_stop(ifnet *ifp)
{
	return B_ERROR;
}


int ppp_ifnet_output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
	struct rtentry *rt0)
{
	return B_ERROR;
}


int ppp_ifnet_ioctl(struct ifnet *ifp, ulong cmd, caddr_t data)
{
	return B_ERROR;
}


static const char ppp_if_name_base[] = "ppp";


PPPManager::PPPManager()
	: fReportManager(fLock),
	fNextID(1)
{
}


PPPManager::~PPPManager()
{
}


interface_id
PPPManager::CreateInterface(const driver_settings *settings, interface_id parentID)
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
	
	int32 index = 0;
	interface_entry *entry = EntryFor(ID, &index);
	if(!entry)
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
	if(!entry)
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
	return false;
}


status_t
PPPManager::Control(uint32 op, void *data, size_t length)
{
	return B_ERROR;
}


status_t
PPPManager::ControlInterface(interface_id ID, uint32 op, void *data, size_t length)
{
	return B_ERROR;
}


int32
PPPManager::GetInterfaces(interface_id *interfaces, int32 count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	return 0;
}


int32
PPPManager::CountInterfaces(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	return 0;
}


interface_entry*
PPPManager::EntryFor(interface_id ID, int32 *start = NULL) const
{
	if(ID == PPP_UNDEFINED_INTERFACE_ID)
		return NULL;
	
	interface_entry *entry;
	
	for(int32 index = start ? *start : 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->ID() == ID) {
			if(start)
				*start = index;
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
