/*
 * Copyright (c) 2004-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <ksystem_info.h>
#include <system_info.h>
#include <system_revision.h>
#include <arch/system_info.h>

#include <string.h>

#include <OS.h>
#include <KernelExport.h>

#include <AutoDeleter.h>

#include <block_cache.h>
#include <cpu.h>
#include <debug.h>
#include <kernel.h>
#include <lock.h>
#include <Notifications.h>
#include <messaging.h>
#include <port.h>
#include <real_time_clock.h>
#include <sem.h>
#include <smp.h>
#include <team.h>
#include <thread.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>


const static int64 kKernelVersion = 0x1;
const static char *kKernelName = "kernel_" HAIKU_ARCH;


static int
dump_info(int argc, char **argv)
{
	kprintf("kernel build: %s %s (gcc%d %s)\n", __DATE__, __TIME__, __GNUC__,
		__VERSION__);
	kprintf("revision: %s\n\n", get_haiku_revision());

	kprintf("cpu count: %" B_PRId32 ", active times:\n", smp_get_num_cpus());

	for (int32 i = 0; i < smp_get_num_cpus(); i++)
		kprintf("  [%" B_PRId32 "] %" B_PRId64 "\n", i + 1, gCPU[i].active_time);

	// ToDo: Add page_faults
	kprintf("pages:\t\t%" B_PRIuPHYSADDR " (%" B_PRIuPHYSADDR " max)\n",
		vm_page_num_pages() - vm_page_num_free_pages(), vm_page_num_pages());

	kprintf("sems:\t\t%" B_PRId32 " (%" B_PRId32 " max)\n", sem_used_sems(),
		sem_max_sems());
	kprintf("ports:\t\t%" B_PRId32 " (%" B_PRId32 " max)\n", port_used_ports(),
			port_max_ports());
	kprintf("threads:\t%" B_PRId32 " (%" B_PRId32 " max)\n",
		thread_used_threads(), thread_max_threads());
	kprintf("teams:\t\t%" B_PRId32 " (%" B_PRId32 " max)\n", team_used_teams(),
		team_max_teams());

	return 0;
}


//	#pragma mark - user notifications


class SystemNotificationService : private NotificationListener {
public:
	SystemNotificationService()
	{
		mutex_init(&fLock, "system notification service");
	}

	status_t Init()
	{
		status_t error = fTeamListeners.Init();
		if (error != B_OK)
			return error;

		error = NotificationManager::Manager().AddListener("teams",
			TEAM_ADDED | TEAM_REMOVED | TEAM_EXEC, *this);
		if (error != B_OK)
			return error;

		error = NotificationManager::Manager().AddListener("threads",
			THREAD_ADDED | THREAD_REMOVED | TEAM_EXEC, *this);
		if (error != B_OK)
			return error;

		return B_OK;
	}

	status_t StartListening(int32 object, uint32 flags, port_id port,
		int32 token)
	{
		// check the parameters
		if ((object < 0 && object != -1) || port < 0)
			return B_BAD_VALUE;

		if ((flags & B_WATCH_SYSTEM_ALL) == 0
			|| (flags & ~(uint32)B_WATCH_SYSTEM_ALL) != 0) {
			return B_BAD_VALUE;
		}

		MutexLocker locker(fLock);

		// maybe the listener already exists
		ListenerList* listenerList;
		Listener* listener = _FindListener(object, port, token, listenerList);
		if (listener != NULL) {
			// just add the new flags
			listener->flags |= flags;
			return B_OK;
		}

		// create a new listener
		listener = new(std::nothrow) Listener;
		if (listener == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<Listener> listenerDeleter(listener);

		listener->port = port;
		listener->token = token;
		listener->flags = flags;

		// if there's no list yet, create a new list
		if (listenerList == NULL) {
			listenerList = new(std::nothrow) ListenerList;
			if (listenerList == NULL)
				return B_NO_MEMORY;

			listenerList->object = object;

			fTeamListeners.Insert(listenerList);
		}

		listener->list = listenerList;
		listenerList->listeners.Add(listener);
		listenerDeleter.Detach();

		team_associate_data(listener);

		return B_OK;
	}

	status_t StopListening(int32 object, uint32 flags, port_id port,
		int32 token)
	{
		MutexLocker locker(fLock);

		// find the listener
		ListenerList* listenerList;
		Listener* listener = _FindListener(object, port, token, listenerList);
		if (listener == NULL)
			return B_ENTRY_NOT_FOUND;

		// clear the given flags
		listener->flags &= ~flags;

		if (listener->flags != 0)
			return B_OK;

		team_dissociate_data(listener);
		_RemoveListener(listener);

		return B_OK;
	}

private:
	struct ListenerList;

	struct Listener : AssociatedData {
		DoublyLinkedListLink<Listener>	listLink;
		ListenerList*					list;
		port_id							port;
		int32							token;
		uint32							flags;

		virtual void OwnerDeleted(AssociatedDataOwner* owner);
	};

	friend struct Listener;

	struct ListenerList {
		typedef DoublyLinkedList<Listener,
			DoublyLinkedListMemberGetLink<Listener, &Listener::listLink> > List;

		ListenerList*	hashNext;
		List			listeners;
		int32			object;
	};

	struct ListenerHashDefinition {
		typedef int32			KeyType;
		typedef	ListenerList	ValueType;

		size_t HashKey(int32 key) const
		{
			return key;
		}

		size_t Hash(const ListenerList* value) const
		{
			return HashKey(value->object);
		}

		bool Compare(int32 key, const ListenerList* value) const
		{
			return value->object == key;
		}

		ListenerList*& GetLink(ListenerList* value) const
		{
			return value->hashNext;
		}
	};

	typedef BOpenHashTable<ListenerHashDefinition> ListenerHash;

private:
	virtual void EventOccurred(NotificationService& service,
		const KMessage* event)
	{
		MutexLocker locker(fLock);

		int32 eventCode;
		int32 teamID;
		if (event->FindInt32("event", &eventCode) != B_OK
			|| event->FindInt32("team", &teamID) != B_OK) {
			return;
		}

		int32 object;
		uint32 opcode;
		uint32 flags;

		// translate the event
		if (event->What() == TEAM_MONITOR) {
			switch (eventCode) {
				case TEAM_ADDED:
					opcode = B_TEAM_CREATED;
					flags = B_WATCH_SYSTEM_TEAM_CREATION;
					break;
				case TEAM_REMOVED:
					opcode = B_TEAM_DELETED;
					flags = B_WATCH_SYSTEM_TEAM_DELETION;
					break;
				case TEAM_EXEC:
					opcode = B_TEAM_EXEC;
					flags = B_WATCH_SYSTEM_TEAM_CREATION
						| B_WATCH_SYSTEM_TEAM_DELETION;
					break;
				default:
					return;
			}

			object = teamID;
		} else if (event->What() == THREAD_MONITOR) {
			if (event->FindInt32("thread", &object) != B_OK)
				return;

			switch (eventCode) {
				case THREAD_ADDED:
					opcode = B_THREAD_CREATED;
					flags = B_WATCH_SYSTEM_THREAD_CREATION;
					break;
				case THREAD_REMOVED:
					opcode = B_THREAD_DELETED;
					flags = B_WATCH_SYSTEM_THREAD_DELETION;
					break;
				case THREAD_NAME_CHANGED:
					opcode = B_THREAD_NAME_CHANGED;
					flags = B_WATCH_SYSTEM_THREAD_PROPERTIES;
					break;
				default:
					return;
			}
		} else
			return;

		// find matching listeners
		messaging_target targets[kMaxMessagingTargetCount];
		int32 targetCount = 0;

		_AddTargets(fTeamListeners.Lookup(teamID), flags, targets,
			targetCount, object, opcode);
		_AddTargets(fTeamListeners.Lookup(-1), flags, targets, targetCount,
			object, opcode);

		// send the message
		if (targetCount > 0)
			_SendMessage(targets, targetCount, object, opcode);
	}

	void _AddTargets(ListenerList* listenerList, uint32 flags,
		messaging_target* targets, int32& targetCount, int32 object,
		uint32 opcode)
	{
		if (listenerList == NULL)
			return;

		for (ListenerList::List::Iterator it
				= listenerList->listeners.GetIterator();
			Listener* listener = it.Next();) {
			if ((listener->flags & flags) == 0)
				continue;

			// array is full -- need to flush it first
			if (targetCount == kMaxMessagingTargetCount) {
				_SendMessage(targets, targetCount, object, opcode);
				targetCount = 0;
			}

			// add the listener
			targets[targetCount].port = listener->port;
			targets[targetCount++].token = listener->token;
		}
	}

	void _SendMessage(messaging_target* targets, int32 targetCount,
		int32 object, uint32 opcode)
	{
		// prepare the message
		char buffer[128];
		KMessage message;
		message.SetTo(buffer, sizeof(buffer), B_SYSTEM_OBJECT_UPDATE);
		message.AddInt32("opcode", opcode);
		if (opcode < B_THREAD_CREATED)
			message.AddInt32("team", object);
		else
			message.AddInt32("thread", object);

		// send it
		send_message(message.Buffer(), message.ContentSize(), targets,
			targetCount);
	}

	Listener* _FindListener(int32 object, port_id port, int32 token,
		ListenerList*& _listenerList)
	{
		_listenerList = fTeamListeners.Lookup(object);
		if (_listenerList == NULL)
			return NULL;

		for (ListenerList::List::Iterator it
				= _listenerList->listeners.GetIterator();
			Listener* listener = it.Next();) {
			if (listener->port == port && listener->token == token)
				return listener;
		}

		return NULL;
	}

	void _RemoveObsoleteListener(Listener* listener)
	{
		MutexLocker locker(fLock);
		_RemoveListener(listener);
	}

	void _RemoveListener(Listener* listener)
	{
		// no flags anymore -- remove the listener
		ListenerList* listenerList = listener->list;
		listenerList->listeners.Remove(listener);
		listener->ReleaseReference();

		if (listenerList->listeners.IsEmpty()) {
			// no listeners in the list anymore -- remove the list from the hash
			// table
			fTeamListeners.Remove(listenerList);
			delete listenerList;
		}
	}

private:
	static const int32 kMaxMessagingTargetCount = 8;

	mutex			fLock;
	ListenerHash	fTeamListeners;
};

static SystemNotificationService sSystemNotificationService;


void
SystemNotificationService::Listener::OwnerDeleted(AssociatedDataOwner* owner)
{
	sSystemNotificationService._RemoveObsoleteListener(this);
}


//	#pragma mark -


status_t
_get_system_info(system_info *info, size_t size)
{
	if (size != sizeof(system_info))
		return B_BAD_VALUE;

	memset(info, 0, sizeof(system_info));

	info->boot_time = rtc_boot_time();
	info->cpu_count = smp_get_num_cpus();

	for (int32 i = 0; i < info->cpu_count; i++)
		info->cpu_infos[i].active_time = cpu_get_active_time(i);

	vm_page_get_stats(info);
	// TODO: Add page_faults

	info->used_threads = thread_used_threads();
	info->max_threads = thread_max_threads();
	info->used_teams = team_used_teams();
	info->max_teams = team_max_teams();
	info->used_ports = port_used_ports();
	info->max_ports = port_max_ports();
	info->used_sems = sem_used_sems();
	info->max_sems = sem_max_sems();

	info->kernel_version = kKernelVersion;
	strlcpy(info->kernel_name, kKernelName, B_FILE_NAME_LENGTH);
	strlcpy(info->kernel_build_date, __DATE__, B_OS_NAME_LENGTH);
	strlcpy(info->kernel_build_time, __TIME__, B_OS_NAME_LENGTH);
	info->abi = B_HAIKU_ABI;

	// all other stuff is architecture specific
	return arch_get_system_info(info, size);
}


status_t
system_info_init(struct kernel_args *args)
{
	add_debugger_command("info", &dump_info, "System info");

	return arch_system_info_init(args);
}


status_t
system_notifications_init()
{
	new (&sSystemNotificationService) SystemNotificationService;

	status_t error = sSystemNotificationService.Init();
	if (error != B_OK) {
		panic("system_info_init(): Failed to init system notification service");
		return error;
	}

	return B_OK;
}


//	#pragma mark -


status_t
_user_get_system_info(system_info *userInfo, size_t size)
{
	// The BeBook says get_system_info() always returns B_OK,
	// but that ain't true with invalid addresses
	if (userInfo == NULL || !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	system_info info;
	status_t status = _get_system_info(&info, size);
	if (status == B_OK) {
		if (user_memcpy(userInfo, &info, sizeof(system_info)) < B_OK)
			return B_BAD_ADDRESS;

		return B_OK;
	}

	return status;
}


status_t
_user_get_system_info_etc(int32 id, void* userInfo, size_t size)
{
	if (userInfo == NULL || !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	switch (id) {
		case B_MEMORY_INFO:
		{
			if (size < sizeof(system_memory_info))
				return B_BAD_VALUE;

			system_memory_info info;
			vm_get_info(&info);

			info.block_cache_memory = block_cache_used_memory();

			return user_memcpy(userInfo, &info, sizeof(system_memory_info));
		}

		default:
			return B_BAD_VALUE;
	}
}


status_t
_user_start_watching_system(int32 object, uint32 flags, port_id port,
	int32 token)
{
	return sSystemNotificationService.StartListening(object, flags, port,
		token);
}


status_t
_user_stop_watching_system(int32 object, uint32 flags, port_id port,
	int32 token)
{
	return sSystemNotificationService.StopListening(object, flags, port, token);
}
