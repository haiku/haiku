//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "PPPManager.h"

#include <kernel_cpp.h>
#include <LockerHelper.h>


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
	
}


ifnet*
PPPManager::RegisterInterface(interface_id ID)
{
	return NULL;
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
