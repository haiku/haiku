/*
 * Copyright 2006-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Lin Longzhou, linlongzhou@163.com
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */

#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <KernelExport.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <new>

#include <ppp_device.h>
#include <KPPPManager.h>


struct entry_private : ppp_interface_entry, DoublyLinkedListLinkImpl<entry_private> {
};

static DoublyLinkedList<entry_private> sEntryList;

net_buffer_module_info* gBufferModule = NULL;
net_stack_module_info* gStackModule = NULL;
// static struct net_interface* sinterface;
static mutex sListLock;
static uint32 ppp_interface_count;


//	#pragma mark -
static KPPPInterface* GetInterface(ppp_interface_id ID);

static ppp_interface_id
CreateInterface(const driver_settings* settings, ppp_interface_id parentID)
{
	MutexLocker _(sListLock); // auto_lock

	if (parentID > 0) {
		TRACE("Not support Multi_Link yet!\n");
		return 0l;
	}

	if (settings == NULL) {
		TRACE("No driver_settings yet!\n");
		return 0l;
	}

	ppp_interface_count++;
	if (GetInterface(ppp_interface_count) != NULL) {
		TRACE("PPP Interface already exists!\n");
		return 0l;
	}

	entry_private* pentry = new (std::nothrow) entry_private;
	memset(pentry, 0, sizeof(entry_private));
	pentry->accessing = ppp_interface_count;
	sEntryList.Add(pentry);

	KPPPInterface* device = new (std::nothrow) KPPPInterface(NULL,
		pentry, ppp_interface_count, NULL, NULL);

	if (device == NULL || pentry == NULL) {
		TRACE("device or pentry is NULL\n");
		ppp_interface_count--;
		return 0l;
	}

	TRACE("setting ppp_interface_count %" B_PRIu32 "\n", ppp_interface_count);

	return ppp_interface_count; // only support 1 ppp connection right now
}


static ppp_interface_id
CreateInterfaceWithName(const char* name, ppp_interface_id parentID)
{
	MutexLocker _(sListLock); // auto_lock

	if (parentID > 0) {
		dprintf("Not support Multi_Link yet!\n");
		return(0);
	}

	if (strncmp(name, "ppp", 3)) {
		dprintf("%s is not ppp device!\n", name);
		return(0);
	}

	ppp_interface_count++;
	if (GetInterface(ppp_interface_count) != NULL) {
		TRACE("PPP Interface already exists!\n");
		return 0l;
	}

	entry_private* pentry = new (std::nothrow) entry_private;
	memset(pentry, 0, sizeof(entry_private));
	pentry->accessing = ppp_interface_count;
	sEntryList.Add(pentry);

	KPPPInterface* device = new (std::nothrow) KPPPInterface(name,
		pentry, ppp_interface_count, NULL, NULL);


	if (device == NULL || pentry == NULL) {
		TRACE("can not create ppp interface!\n");
		ppp_interface_count--;
		return 0l;
	}

	TRACE("setting ppp_interface_count %" B_PRIu32 "\n", ppp_interface_count);

	return ppp_interface_count;
}


static bool
DeleteInterface(ppp_interface_id ID)
{
	MutexLocker _(sListLock); // auto_lock

	DoublyLinkedList<entry_private>::Iterator iterator
		= sEntryList.GetIterator();
	while (iterator.HasNext()) {
		entry_private* pentry = iterator.Next();
		if ((ppp_interface_id)pentry->accessing == ID) {
			pentry->deleting = true;
			return true;
		}
	}

	return false;
}


static bool
RemoveInterface(ppp_interface_id ID)
{
	MutexLocker _(sListLock); // auto_lock
	if (ID <= 0 || ID > ppp_interface_count)
		return false;

	DoublyLinkedList<entry_private>::Iterator iterator
		= sEntryList.GetIterator();
	while (iterator.HasNext()) {
		entry_private* pentry = iterator.Next();
		if ((ppp_interface_id)pentry->accessing == ID) {
			pentry->deleting = true;
			break;
		}
	}

	return false;
}


static net_device*
RegisterInterface(ppp_interface_id ID)
{
	// MutexLocker _(sListLock); // auto_lock

	entry_private* entry = NULL;

	DoublyLinkedList<entry_private>::Iterator iterator
		= sEntryList.GetIterator();
	while (iterator.HasNext()) {
		entry_private* pentry = iterator.Next();
		if ((ppp_interface_id)pentry->accessing == ID) {
			entry = pentry;
			break;
		}
	}

	if (entry == NULL)
		return NULL;

	ppp_device* device = new (std::nothrow) ppp_device;
	if (device == NULL)
		return NULL;

	memset(device, 0, sizeof(ppp_device));
	device->KPPP_Interface = entry->interface;

	return device;
}


static KPPPInterface*
GetInterface(ppp_interface_id ID)
{
	// MutexLocker _(sListLock); // auto_lock

	DoublyLinkedList<entry_private>::Iterator iterator
		= sEntryList.GetIterator();
	while (iterator.HasNext()) {
		entry_private* pentry = iterator.Next();
		TRACE("testing interface id:%" B_PRId32 "\n", pentry->accessing);
		if ((ppp_interface_id)(pentry->accessing) == ID) {
			TRACE("get interface id:%" B_PRId32 "\n", ID);
			return pentry->interface;
		}
	}

	TRACE("can not get interface id:%" B_PRId32 "\n", ID);

	return NULL;
}


static bool
UnregisterInterface(ppp_interface_id ID)
{
	MutexLocker _(sListLock); // auto_lock

	if (ID <= 0 || ID > ppp_interface_count)
		return false;
	return true;
}


static status_t
ControlInterface(ppp_interface_id ID, uint32 op, void* data, size_t length)
{
	MutexLocker _(sListLock); // auto_lock

	if (ID <= 0 || ID > ppp_interface_count)
		return B_ERROR;

	DoublyLinkedList<entry_private>::Iterator iterator
		= sEntryList.GetIterator();
	while (iterator.HasNext()) {
		entry_private* pentry = iterator.Next();
		if ((ppp_interface_id)pentry->accessing == ID)
			return B_OK;
	}

	return B_ERROR;
}


static int32
GetInterfaces(ppp_interface_id* interfaces, int32 count, ppp_interface_filter filter)
{
	MutexLocker _(sListLock);

	if (count <= 0 || count > (int32)ppp_interface_count)
		return(0);

	uint32 ppp_count = 0;

	DoublyLinkedList<entry_private>::Iterator iterator
		= sEntryList.GetIterator();
	while (iterator.HasNext()) {
		entry_private* pentry = iterator.Next();
		interfaces[ppp_count] = pentry->accessing;
		ppp_count++;
	}

	return ppp_count;
}


static int32
CountInterfaces(ppp_interface_filter filter)
{
	MutexLocker _(sListLock); // auto_lock

	uint32 ppp_count = 0;

	DoublyLinkedList<entry_private>::Iterator iterator
		= sEntryList.GetIterator();
	while (iterator.HasNext()) {
		iterator.Next();
		ppp_count++;
	}

	return ppp_count;
}


static void
EnableReports(ppp_report_type type, thread_id thread, int32 flags)
{
	MutexLocker _(sListLock); // auto_lock

	return; // need more portrait
}


static void
DisableReports(ppp_report_type type, thread_id thread)
{
	MutexLocker _(sListLock); // auto_lock

	return; // need more portrait
}


static bool
DoesReport(ppp_report_type type, thread_id thread)
{
	MutexLocker _(sListLock); // auto_lock

	return false; // need more portrait
}


static status_t
KPPPManager_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			mutex_init(&sListLock, "KPPPManager");
			new (&sEntryList) DoublyLinkedList<entry_private>;
				// static C++ objects are not initialized in the module startup
			ppp_interface_count = 0;

			if (get_module(NET_STACK_MODULE_NAME, (module_info**)&gStackModule)
				!= B_OK)
				return B_ERROR;

			if (get_module(NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule)
				!= B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return B_ERROR;
			}

			return B_OK;

		case B_MODULE_UNINIT:
			mutex_destroy(&sListLock);

			put_module(NET_BUFFER_MODULE_NAME);
			put_module(NET_STACK_MODULE_NAME);
			return B_OK;

		default:
			return B_ERROR;
	}
}


static ppp_interface_module_info sKPPPManagerModule = {
	{
		PPP_INTERFACE_MODULE_NAME,
		0,
		KPPPManager_std_ops
	},
	CreateInterface,
	CreateInterfaceWithName,
	DeleteInterface,
	RemoveInterface,
	RegisterInterface,
	GetInterface,
	UnregisterInterface,
	ControlInterface,
	GetInterfaces,
	CountInterfaces,
	EnableReports,
	DisableReports,
	DoesReport
};


module_info* modules[] = {
	(module_info*)&sKPPPManagerModule,
	NULL
};
