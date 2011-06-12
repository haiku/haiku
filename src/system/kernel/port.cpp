/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Mark-Jan Bastian. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*!	Ports for IPC */


#include <port.h>

#include <ctype.h>
#include <iovec.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <AutoDeleter.h>

#include <arch/int.h>
#include <heap.h>
#include <kernel.h>
#include <Notifications.h>
#include <sem.h>
#include <syscall_restart.h>
#include <team.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/list.h>
#include <vm/vm.h>
#include <wait_for_objects.h>


//#define TRACE_PORTS
#ifdef TRACE_PORTS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


// Locking:
// * sPortsLock: Protects the sPorts hash table, Team::port_list, and
//   Port::owner.
// * Port::lock: Protects all Port members save team_link, hash_link, and lock.
//   id is immutable.
//
// The locking order is sPortsLock -> Port::lock. A port must be looked up
// in sPorts and locked with sPortsLock held. Afterwards sPortsLock can be
// dropped, unless any field guarded by sPortsLock is accessed.


struct port_message;


static void put_port_message(port_message* message);


struct port_message : DoublyLinkedListLinkImpl<port_message> {
	int32				code;
	size_t				size;
	uid_t				sender;
	gid_t				sender_group;
	team_id				sender_team;
	char				buffer[0];
};

typedef DoublyLinkedList<port_message> MessageList;


struct Port {
	struct list_link	team_link;
	Port*				hash_link;
	port_id				id;
	team_id				owner;
	int32		 		capacity;
	mutex				lock;
	uint32				read_count;
	int32				write_count;
	ConditionVariable	read_condition;
	ConditionVariable	write_condition;
	int32				total_count;
		// messages read from port since creation
	select_info*		select_infos;
	MessageList			messages;

	Port(team_id owner, int32 queueLength, char* name)
		:
		owner(owner),
		capacity(queueLength),
		read_count(0),
		write_count(queueLength),
		total_count(0),
		select_infos(NULL)
	{
		// id is initialized when the caller adds the port to the hash table

		mutex_init(&lock, name);
		read_condition.Init(this, "port read");
		write_condition.Init(this, "port write");
	}

	~Port()
	{
		while (port_message* message = messages.RemoveHead())
			put_port_message(message);

		free((char*)lock.name);
		lock.name = NULL;
	}
};


struct PortHashDefinition {
	typedef port_id		KeyType;
	typedef	Port		ValueType;

	size_t HashKey(port_id key) const
	{
		return key;
	}

	size_t Hash(Port* value) const
	{
		return HashKey(value->id);
	}

	bool Compare(port_id key, Port* value) const
	{
		return value->id == key;
	}

	Port*& GetLink(Port* value) const
	{
		return value->hash_link;
	}
};

typedef BOpenHashTable<PortHashDefinition> PortHashTable;


class PortNotificationService : public DefaultNotificationService {
public:
							PortNotificationService();

			void			Notify(uint32 opcode, port_id team);
};


#if PORT_TRACING
namespace PortTracing {

class Create : public AbstractTraceEntry {
public:
	Create(Port* port)
		:
		fID(port->id),
		fOwner(port->owner),
		fCapacity(port->capacity)
	{
		fName = alloc_tracing_buffer_strcpy(port->lock.name, B_OS_NAME_LENGTH,
			false);

		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("port %ld created, name \"%s\", owner %ld, capacity %ld",
			fID, fName, fOwner, fCapacity);
	}

private:
	port_id				fID;
	char*				fName;
	team_id				fOwner;
	int32		 		fCapacity;
};


class Delete : public AbstractTraceEntry {
public:
	Delete(Port* port)
		:
		fID(port->id)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("port %ld deleted", fID);
	}

private:
	port_id				fID;
};


class Read : public AbstractTraceEntry {
public:
	Read(port_id id, int32 readCount, int32 writeCount, int32 code,
		ssize_t result)
		:
		fID(id),
		fReadCount(readCount),
		fWriteCount(writeCount),
		fCode(code),
		fResult(result)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("port %ld read, read %ld, write %ld, code %lx: %ld",
			fID, fReadCount, fWriteCount, fCode, fResult);
	}

private:
	port_id				fID;
	int32				fReadCount;
	int32				fWriteCount;
	int32				fCode;
	ssize_t				fResult;
};


class Write : public AbstractTraceEntry {
public:
	Write(port_id id, int32 readCount, int32 writeCount, int32 code,
		size_t bufferSize, ssize_t result)
		:
		fID(id),
		fReadCount(readCount),
		fWriteCount(writeCount),
		fCode(code),
		fBufferSize(bufferSize),
		fResult(result)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("port %ld write, read %ld, write %ld, code %lx, size %ld: %ld",
			fID, fReadCount, fWriteCount, fCode, fBufferSize, fResult);
	}

private:
	port_id				fID;
	int32				fReadCount;
	int32				fWriteCount;
	int32				fCode;
	size_t				fBufferSize;
	ssize_t				fResult;
};


class Info : public AbstractTraceEntry {
public:
	Info(port_id id, int32 readCount, int32 writeCount, int32 code,
		ssize_t result)
		:
		fID(id),
		fReadCount(readCount),
		fWriteCount(writeCount),
		fCode(code),
		fResult(result)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("port %ld info, read %ld, write %ld, code %lx: %ld",
			fID, fReadCount, fWriteCount, fCode, fResult);
	}

private:
	port_id				fID;
	int32				fReadCount;
	int32				fWriteCount;
	int32				fCode;
	ssize_t				fResult;
};


class OwnerChange : public AbstractTraceEntry {
public:
	OwnerChange(Port* port, team_id newOwner, status_t status)
		:
		fID(port->id),
		fOldOwner(port->owner),
		fNewOwner(newOwner),
		fStatus(status)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("port %ld owner change from %ld to %ld: %s", fID, fOldOwner,
			fNewOwner, strerror(fStatus));
	}

private:
	port_id				fID;
	team_id				fOldOwner;
	team_id				fNewOwner;
	status_t	 		fStatus;
};

}	// namespace PortTracing

#	define T(x) new(std::nothrow) PortTracing::x;
#else
#	define T(x) ;
#endif


static const size_t kInitialPortBufferSize = 4 * 1024 * 1024;
static const size_t kTotalSpaceLimit = 64 * 1024 * 1024;
static const size_t kTeamSpaceLimit = 8 * 1024 * 1024;
static const size_t kBufferGrowRate = kInitialPortBufferSize;

#define MAX_QUEUE_LENGTH 4096
#define PORT_MAX_MESSAGE_SIZE (256 * 1024)

static int32 sMaxPorts = 4096;
static int32 sUsedPorts = 0;

static PortHashTable sPorts;
static heap_allocator* sPortAllocator;
static ConditionVariable sNoSpaceCondition;
static vint32 sTotalSpaceInUse;
static vint32 sAreaChangeCounter;
static vint32 sAllocatingArea;
static port_id sNextPortID = 1;
static bool sPortsActive = false;
static mutex sPortsLock = MUTEX_INITIALIZER("ports list");

static PortNotificationService sNotificationService;


//	#pragma mark - TeamNotificationService


PortNotificationService::PortNotificationService()
	:
	DefaultNotificationService("ports")
{
}


void
PortNotificationService::Notify(uint32 opcode, port_id port)
{
	char eventBuffer[64];
	KMessage event;
	event.SetTo(eventBuffer, sizeof(eventBuffer), PORT_MONITOR);
	event.AddInt32("event", opcode);
	event.AddInt32("port", port);

	DefaultNotificationService::Notify(event, opcode);
}


//	#pragma mark -


static int
dump_port_list(int argc, char** argv)
{
	const char* name = NULL;
	team_id owner = -1;

	if (argc > 2) {
		if (!strcmp(argv[1], "team") || !strcmp(argv[1], "owner"))
			owner = strtoul(argv[2], NULL, 0);
		else if (!strcmp(argv[1], "name"))
			name = argv[2];
	} else if (argc > 1)
		owner = strtoul(argv[1], NULL, 0);

	kprintf("port             id  cap  read-cnt  write-cnt   total   team  "
		"name\n");

	for (PortHashTable::Iterator it = sPorts.GetIterator();
		Port* port = it.Next();) {
		if ((owner != -1 && port->owner != owner)
			|| (name != NULL && strstr(port->lock.name, name) == NULL))
			continue;

		kprintf("%p %8ld %4ld %9ld %9ld %8ld %6ld  %s\n", port,
			port->id, port->capacity, port->read_count, port->write_count,
			port->total_count, port->owner, port->lock.name);
	}

	return 0;
}


static void
_dump_port_info(Port* port)
{
	kprintf("PORT: %p\n", port);
	kprintf(" id:              %ld\n", port->id);
	kprintf(" name:            \"%s\"\n", port->lock.name);
	kprintf(" owner:           %ld\n", port->owner);
	kprintf(" capacity:        %ld\n", port->capacity);
	kprintf(" read_count:      %ld\n", port->read_count);
	kprintf(" write_count:     %ld\n", port->write_count);
	kprintf(" total count:     %ld\n", port->total_count);

	if (!port->messages.IsEmpty()) {
		kprintf("messages:\n");

		MessageList::Iterator iterator = port->messages.GetIterator();
		while (port_message* message = iterator.Next()) {
			kprintf(" %p  %08lx  %ld\n", message, message->code, message->size);
		}
	}

	set_debug_variable("_port", (addr_t)port);
	set_debug_variable("_portID", port->id);
	set_debug_variable("_owner", port->owner);
}


static int
dump_port_info(int argc, char** argv)
{
	ConditionVariable* condition = NULL;
	const char* name = NULL;

	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc > 2) {
		if (!strcmp(argv[1], "address")) {
			_dump_port_info((Port*)parse_expression(argv[2]));
			return 0;
		} else if (!strcmp(argv[1], "condition"))
			condition = (ConditionVariable*)parse_expression(argv[2]);
		else if (!strcmp(argv[1], "name"))
			name = argv[2];
	} else if (parse_expression(argv[1]) > 0) {
		// if the argument looks like a number, treat it as such
		int32 num = parse_expression(argv[1]);
		Port* port = sPorts.Lookup(num);
		if (port == NULL) {
			kprintf("port %ld (%#lx) doesn't exist!\n", num, num);
			return 0;
		}
		_dump_port_info(port);
		return 0;
	} else
		name = argv[1];

	// walk through the ports list, trying to match name
	for (PortHashTable::Iterator it = sPorts.GetIterator();
		Port* port = it.Next();) {
		if ((name != NULL && port->lock.name != NULL
				&& !strcmp(name, port->lock.name))
			|| (condition != NULL && (&port->read_condition == condition
				|| &port->write_condition == condition))) {
			_dump_port_info(port);
			return 0;
		}
	}

	return 0;
}


/*!	Notifies the port's select events.
	The port must be locked.
*/
static void
notify_port_select_events(Port* port, uint16 events)
{
	if (port->select_infos)
		notify_select_events_list(port->select_infos, events);
}


static void
put_port_message(port_message* message)
{
	size_t size = sizeof(port_message) + message->size;
	heap_free(sPortAllocator, message);

	atomic_add(&sTotalSpaceInUse, -size);
	sNoSpaceCondition.NotifyAll();
}


static status_t
get_port_message(int32 code, size_t bufferSize, uint32 flags, bigtime_t timeout,
	port_message** _message)
{
	size_t size = sizeof(port_message) + bufferSize;
	bool limitReached = false;

	while (true) {
		if (atomic_add(&sTotalSpaceInUse, size)
				> int32(kTotalSpaceLimit - size)) {
			// TODO: add per team limit
			// We are not allowed to create another heap area, as our
			// space limit has been reached - just wait until we get
			// some free space again.
			limitReached = true;

		wait:
			MutexLocker locker(sPortsLock);

			atomic_add(&sTotalSpaceInUse, -size);

			// TODO: we don't want to wait - but does that also mean we
			// shouldn't wait for the area creation?
			if (limitReached && (flags & B_RELATIVE_TIMEOUT) != 0
				&& timeout <= 0)
				return B_WOULD_BLOCK;

			ConditionVariableEntry entry;
			sNoSpaceCondition.Add(&entry);

			locker.Unlock();

			status_t status = entry.Wait(flags, timeout);
			if (status == B_TIMED_OUT)
				return B_TIMED_OUT;

			// just try again
			limitReached = false;
			continue;
		}

		int32 areaChangeCounter = atomic_get(&sAreaChangeCounter);

		// Quota is fulfilled, try to allocate the buffer

		port_message* message
			= (port_message*)heap_memalign(sPortAllocator, 0, size);
		if (message != NULL) {
			message->code = code;
			message->size = bufferSize;

			*_message = message;
			return B_OK;
		}

		if (atomic_or(&sAllocatingArea, 1) != 0) {
			// Just wait for someone else to create an area for us
			goto wait;
		}

		if (areaChangeCounter != atomic_get(&sAreaChangeCounter)) {
			atomic_add(&sTotalSpaceInUse, -size);
			continue;
		}

		// Create a new area for the heap to use

		addr_t base;
		area_id area = create_area("port grown buffer", (void**)&base,
			B_ANY_KERNEL_ADDRESS, kBufferGrowRate, B_NO_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (area < 0) {
			// it's time to let the userland feel our pain
			sNoSpaceCondition.NotifyAll();
			return B_NO_MEMORY;
		}

		heap_add_area(sPortAllocator, area, base, kBufferGrowRate);

		atomic_add(&sAreaChangeCounter, 1);
		sNoSpaceCondition.NotifyAll();
		atomic_and(&sAllocatingArea, 0);
	}
}


/*!	You need to own the port's lock when calling this function */
static inline bool
is_port_closed(Port* port)
{
	return port->capacity == 0;
}


/*!	Fills the port_info structure with information from the specified
	port.
	The port's lock must be held when called.
*/
static void
fill_port_info(Port* port, port_info* info, size_t size)
{
	info->port = port->id;
	info->team = port->owner;
	info->capacity = port->capacity;

	info->queue_count = port->read_count;
	info->total_count = port->total_count;

	strlcpy(info->name, port->lock.name, B_OS_NAME_LENGTH);
}


static ssize_t
copy_port_message(port_message* message, int32* _code, void* buffer,
	size_t bufferSize, bool userCopy)
{
	// check output buffer size
	size_t size = min_c(bufferSize, message->size);

	// copy message
	if (_code != NULL)
		*_code = message->code;

	if (size > 0) {
		if (userCopy) {
			status_t status = user_memcpy(buffer, message->buffer, size);
			if (status != B_OK)
				return status;
		} else
			memcpy(buffer, message->buffer, size);
	}

	return size;
}


static void
uninit_port_locked(Port* port)
{
	notify_port_select_events(port, B_EVENT_INVALID);
	port->select_infos = NULL;

	// Release the threads that were blocking on this port.
	// read_port() will see the B_BAD_PORT_ID return value, and act accordingly
	port->read_condition.NotifyAll(false, B_BAD_PORT_ID);
	port->write_condition.NotifyAll(false, B_BAD_PORT_ID);
	sNotificationService.Notify(PORT_REMOVED, port->id);
}


static Port*
get_locked_port(port_id id)
{
	MutexLocker portsLocker(sPortsLock);

	Port* port = sPorts.Lookup(id);
	if (port != NULL)
		mutex_lock(&port->lock);
	return port;
}


//	#pragma mark - private kernel API


/*! This function deletes all the ports that are owned by the passed team.
*/
void
delete_owned_ports(Team* team)
{
	TRACE(("delete_owned_ports(owner = %ld)\n", team->id));

	MutexLocker portsLocker(sPortsLock);

	// move the ports from the team's port list to a local list
	struct list queue;
	list_move_to_list(&team->port_list, &queue);

	// iterate through the list or ports, remove them from the hash table and
	// uninitialize them
	Port* port = (Port*)list_get_first_item(&queue);
	while (port != NULL) {
		MutexLocker locker(port->lock);
		sPorts.Remove(port);
		uninit_port_locked(port);
		sUsedPorts--;

		port = (Port*)list_get_next_item(&queue, port);
	}

	portsLocker.Unlock();

	// delete the ports
	while (Port* port = (Port*)list_remove_head_item(&queue))
		delete port;
}


int32
port_max_ports(void)
{
	return sMaxPorts;
}


int32
port_used_ports(void)
{
	return sUsedPorts;
}


status_t
port_init(kernel_args *args)
{
	// initialize ports table
	new(&sPorts) PortHashTable;
	if (sPorts.Init() != B_OK) {
		panic("Failed to init port hash table!");
		return B_NO_MEMORY;
	}

	addr_t base;
	if (create_area("port heap", (void**)&base, B_ANY_KERNEL_ADDRESS,
			kInitialPortBufferSize, B_NO_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA) < 0) {
			// TODO: Since port_init() is invoked before the boot partition is
			// mounted, the underlying VMAnonymousCache cannot commit swap space
			// upon creation and thus the pages aren't swappable after all. This
			// makes the area essentially B_LAZY_LOCK with additional overhead.
		panic("unable to allocate port area!\n");
		return B_ERROR;
	}

	static const heap_class kBufferHeapClass = {"default", 100,
		PORT_MAX_MESSAGE_SIZE + sizeof(port_message), 2 * 1024,
		sizeof(port_message), 8, 4, 64};
	sPortAllocator = heap_create_allocator("port buffer", base,
		kInitialPortBufferSize, &kBufferHeapClass, true);
	if (sPortAllocator == NULL) {
		panic("unable to create port heap");
		return B_NO_MEMORY;
	}

	sNoSpaceCondition.Init(&sPorts, "port space");

	// add debugger commands
	add_debugger_command_etc("ports", &dump_port_list,
		"Dump a list of all active ports (for team, with name, etc.)",
		"[ ([ \"team\" | \"owner\" ] <team>) | (\"name\" <name>) ]\n"
		"Prints a list of all active ports meeting the given\n"
		"requirement. If no argument is given, all ports are listed.\n"
		"  <team>             - The team owning the ports.\n"
		"  <name>             - Part of the name of the ports.\n", 0);
	add_debugger_command_etc("port", &dump_port_info,
		"Dump info about a particular port",
		"(<id> | [ \"address\" ] <address>) | ([ \"name\" ] <name>) "
			"| (\"condition\" <address>)\n"
		"Prints info about the specified port.\n"
		"  <address>   - Pointer to the port structure.\n"
		"  <name>      - Name of the port.\n"
		"  <condition> - address of the port's read or write condition.\n", 0);

	new(&sNotificationService) PortNotificationService();
	sPortsActive = true;
	return B_OK;
}


//	#pragma mark - public kernel API


port_id
create_port(int32 queueLength, const char* name)
{
	TRACE(("create_port(queueLength = %ld, name = \"%s\")\n", queueLength,
		name));

	if (!sPortsActive) {
		panic("ports used too early!\n");
		return B_BAD_PORT_ID;
	}
	if (queueLength < 1 || queueLength > MAX_QUEUE_LENGTH)
		return B_BAD_VALUE;

	Team* team = thread_get_current_thread()->team;
	if (team == NULL)
		return B_BAD_TEAM_ID;

	// check & dup name
	char* nameBuffer = strdup(name != NULL ? name : "unnamed port");
	if (nameBuffer == NULL)
		return B_NO_MEMORY;

	// create a port
	Port* port = new(std::nothrow) Port(team_get_current_team_id(), queueLength,
		nameBuffer);
	if (port == NULL) {
		free(nameBuffer);
		return B_NO_MEMORY;
	}
	ObjectDeleter<Port> portDeleter(port);

	MutexLocker locker(sPortsLock);

	// check the ports limit
	if (sUsedPorts >= sMaxPorts)
		return B_NO_MORE_PORTS;

	sUsedPorts++;

	// allocate a port ID
	do {
		port->id = sNextPortID++;

		// handle integer overflow
		if (sNextPortID < 0)
			sNextPortID = 1;
	} while (sPorts.Lookup(port->id) != NULL);

	// insert port in table and team list
	sPorts.Insert(port);
	list_add_item(&team->port_list, &port->team_link);
	portDeleter.Detach();

	// tracing, notifications, etc.
	T(Create(port));

	port_id id = port->id;

	locker.Unlock();

	TRACE(("create_port() done: port created %ld\n", id));

	sNotificationService.Notify(PORT_ADDED, id);
	return id;
}


status_t
close_port(port_id id)
{
	TRACE(("close_port(id = %ld)\n", id));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL) {
		TRACE(("close_port: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker lock(&port->lock, true);

	// mark port to disable writing - deleting the semaphores will
	// wake up waiting read/writes
	port->capacity = 0;

	notify_port_select_events(port, B_EVENT_INVALID);
	port->select_infos = NULL;

	port->read_condition.NotifyAll(false, B_BAD_PORT_ID);
	port->write_condition.NotifyAll(false, B_BAD_PORT_ID);

	return B_OK;
}


status_t
delete_port(port_id id)
{
	TRACE(("delete_port(id = %ld)\n", id));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	// get the port and remove it from the hash table and the team
	Port* port;
	MutexLocker locker;
	{
		MutexLocker portsLocker(sPortsLock);

		port = sPorts.Lookup(id);
		if (port == NULL) {
			TRACE(("delete_port: invalid port_id %ld\n", id));
			return B_BAD_PORT_ID;
		}

		sPorts.Remove(port);
		list_remove_link(&port->team_link);

		sUsedPorts--;

		locker.SetTo(port->lock, false);

		uninit_port_locked(port);
	}

	T(Delete(port));

	locker.Unlock();

	delete port;

	return B_OK;
}


status_t
select_port(int32 id, struct select_info* info, bool kernel)
{
	if (id < 0)
		return B_BAD_PORT_ID;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(port->lock, true);

	// port must not yet be closed
	if (is_port_closed(port))
		return B_BAD_PORT_ID;

	if (!kernel && port->owner == team_get_kernel_team_id()) {
		// kernel port, but call from userland
		return B_NOT_ALLOWED;
	}

	info->selected_events &= B_EVENT_READ | B_EVENT_WRITE | B_EVENT_INVALID;

	if (info->selected_events != 0) {
		uint16 events = 0;

		info->next = port->select_infos;
		port->select_infos = info;

		// check for events
		if ((info->selected_events & B_EVENT_READ) != 0
			&& !port->messages.IsEmpty()) {
			events |= B_EVENT_READ;
		}

		if (port->write_count > 0)
			events |= B_EVENT_WRITE;

		if (events != 0)
			notify_select_events(info, events);
	}

	return B_OK;
}


status_t
deselect_port(int32 id, struct select_info* info, bool kernel)
{
	if (id < 0)
		return B_BAD_PORT_ID;
	if (info->selected_events == 0)
		return B_OK;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(port->lock, true);

	// find and remove the infos
	select_info** infoLocation = &port->select_infos;
	while (*infoLocation != NULL && *infoLocation != info)
		infoLocation = &(*infoLocation)->next;

	if (*infoLocation == info)
		*infoLocation = info->next;

	return B_OK;
}


port_id
find_port(const char* name)
{
	TRACE(("find_port(name = \"%s\")\n", name));

	if (!sPortsActive) {
		panic("ports used too early!\n");
		return B_NAME_NOT_FOUND;
	}
	if (name == NULL)
		return B_BAD_VALUE;

	MutexLocker portsLocker(sPortsLock);

	for (PortHashTable::Iterator it = sPorts.GetIterator();
		Port* port = it.Next();) {
		if (!strcmp(name, port->lock.name))
			return port->id;
	}

	return B_NAME_NOT_FOUND;
}


status_t
_get_port_info(port_id id, port_info* info, size_t size)
{
	TRACE(("get_port_info(id = %ld)\n", id));

	if (info == NULL || size != sizeof(port_info))
		return B_BAD_VALUE;
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL) {
		TRACE(("get_port_info: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(port->lock, true);

	// fill a port_info struct with info
	fill_port_info(port, info, size);
	return B_OK;
}


status_t
_get_next_port_info(team_id teamID, int32* _cookie, struct port_info* info,
	size_t size)
{
	TRACE(("get_next_port_info(team = %ld)\n", teamID));

	if (info == NULL || size != sizeof(port_info) || _cookie == NULL
		|| teamID < 0) {
		return B_BAD_VALUE;
	}
	if (!sPortsActive)
		return B_BAD_PORT_ID;

	Team* team = Team::Get(teamID);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	// iterate through the team's port list
	MutexLocker portsLocker(sPortsLock);

	int32 stopIndex = *_cookie;
	int32 index = 0;

	Port* port = (Port*)list_get_first_item(&team->port_list);
	while (port != NULL) {
		if (!is_port_closed(port)) {
			if (index == stopIndex)
				break;
			index++;
		}

		port = (Port*)list_get_next_item(&team->port_list, port);
	}

	if (port == NULL)
		return B_BAD_PORT_ID;

	// fill in the port info
	MutexLocker locker(port->lock);
	portsLocker.Unlock();
	fill_port_info(port, info, size);

	*_cookie = stopIndex + 1;
	return B_OK;
}


ssize_t
port_buffer_size(port_id id)
{
	return port_buffer_size_etc(id, 0, 0);
}


ssize_t
port_buffer_size_etc(port_id id, uint32 flags, bigtime_t timeout)
{
	port_message_info info;
	status_t error = get_port_message_info_etc(id, &info, flags, timeout);
	return error != B_OK ? error : info.size;
}


status_t
_get_port_message_info_etc(port_id id, port_message_info* info,
	size_t infoSize, uint32 flags, bigtime_t timeout)
{
	if (info == NULL || infoSize != sizeof(port_message_info))
		return B_BAD_VALUE;
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	flags &= B_CAN_INTERRUPT | B_KILL_CAN_INTERRUPT | B_RELATIVE_TIMEOUT
		| B_ABSOLUTE_TIMEOUT;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(port->lock, true);

	if (is_port_closed(port) && port->messages.IsEmpty()) {
		T(Info(port, 0, B_BAD_PORT_ID));
		TRACE(("_get_port_message_info_etc(): closed port %ld\n", id));
		return B_BAD_PORT_ID;
	}

	while (port->read_count == 0) {
		// We need to wait for a message to appear
		if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0)
			return B_WOULD_BLOCK;

		ConditionVariableEntry entry;
		port->read_condition.Add(&entry);

		locker.Unlock();

		// block if no message, or, if B_TIMEOUT flag set, block with timeout
		status_t status = entry.Wait(flags, timeout);

		if (status != B_OK) {
			T(Info(port, 0, status));
			return status;
		}

		// re-lock
		Port* newPort = get_locked_port(id);
		if (newPort == NULL) {
			T(Info(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
		locker.SetTo(newPort->lock, true);

		if (newPort != port
			|| (is_port_closed(port) && port->messages.IsEmpty())) {
			// the port is no longer there
			T(Info(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
	}

	// determine tail & get the length of the message
	port_message* message = port->messages.Head();
	if (message == NULL) {
		panic("port %ld: no messages found\n", port->id);
		return B_ERROR;
	}

	info->size = message->size;
	info->sender = message->sender;
	info->sender_group = message->sender_group;
	info->sender_team = message->sender_team;

	T(Info(id, id->read_count, id->write_count, message->code, B_OK));

	// notify next one, as we haven't read from the port
	port->read_condition.NotifyOne();

	return B_OK;
}


ssize_t
port_count(port_id id)
{
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL) {
		TRACE(("port_count: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(port->lock, true);

	// return count of messages
	return port->read_count;
}


ssize_t
read_port(port_id port, int32* msgCode, void* buffer, size_t bufferSize)
{
	return read_port_etc(port, msgCode, buffer, bufferSize, 0, 0);
}


ssize_t
read_port_etc(port_id id, int32* _code, void* buffer, size_t bufferSize,
	uint32 flags, bigtime_t timeout)
{
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;
	if ((buffer == NULL && bufferSize > 0) || timeout < 0)
		return B_BAD_VALUE;

	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) != 0;
	bool peekOnly = !userCopy && (flags & B_PEEK_PORT_MESSAGE) != 0;
		// TODO: we could allow peeking for user apps now

	flags &= B_CAN_INTERRUPT | B_KILL_CAN_INTERRUPT | B_RELATIVE_TIMEOUT
		| B_ABSOLUTE_TIMEOUT;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(port->lock, true);

	if (is_port_closed(port) && port->messages.IsEmpty()) {
		T(Read(port, 0, B_BAD_PORT_ID));
		TRACE(("read_port_etc(): closed port %ld\n", id));
		return B_BAD_PORT_ID;
	}

	while (port->read_count == 0) {
		if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0)
			return B_WOULD_BLOCK;

		// We need to wait for a message to appear
		ConditionVariableEntry entry;
		port->read_condition.Add(&entry);

		locker.Unlock();

		// block if no message, or, if B_TIMEOUT flag set, block with timeout
		status_t status = entry.Wait(flags, timeout);

		// re-lock
		Port* newPort = get_locked_port(id);
		if (newPort == NULL) {
			T(Read(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
		locker.SetTo(newPort->lock, true);

		if (newPort != port
			|| (is_port_closed(port) && port->messages.IsEmpty())) {
			// the port is no longer there
			T(Read(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}

		if (status != B_OK) {
			T(Read(port, 0, status));
			return status;
		}
	}

	// determine tail & get the length of the message
	port_message* message = port->messages.Head();
	if (message == NULL) {
		panic("port %ld: no messages found\n", port->id);
		return B_ERROR;
	}

	if (peekOnly) {
		size_t size = copy_port_message(message, _code, buffer, bufferSize,
			userCopy);

		T(Read(port, message->code, size));

		port->read_condition.NotifyOne();
			// we only peeked, but didn't grab the message
		return size;
	}

	port->messages.RemoveHead();
	port->total_count++;
	port->write_count++;
	port->read_count--;

	notify_port_select_events(port, B_EVENT_WRITE);
	port->write_condition.NotifyOne();
		// make one spot in queue available again for write

	T(Read(id, port->read_count, port->write_count, message->code,
		min_c(bufferSize, message->size)));

	locker.Unlock();

	size_t size = copy_port_message(message, _code, buffer, bufferSize,
		userCopy);

	put_port_message(message);
	return size;
}


status_t
write_port(port_id id, int32 msgCode, const void* buffer, size_t bufferSize)
{
	iovec vec = { (void*)buffer, bufferSize };

	return writev_port_etc(id, msgCode, &vec, 1, bufferSize, 0, 0);
}


status_t
write_port_etc(port_id id, int32 msgCode, const void* buffer,
	size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	iovec vec = { (void*)buffer, bufferSize };

	return writev_port_etc(id, msgCode, &vec, 1, bufferSize, flags, timeout);
}


status_t
writev_port_etc(port_id id, int32 msgCode, const iovec* msgVecs,
	size_t vecCount, size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;
	if (bufferSize > PORT_MAX_MESSAGE_SIZE)
		return B_BAD_VALUE;

	// mask irrelevant flags (for acquire_sem() usage)
	flags &= B_CAN_INTERRUPT | B_KILL_CAN_INTERRUPT | B_RELATIVE_TIMEOUT
		| B_ABSOLUTE_TIMEOUT;
	if ((flags & B_RELATIVE_TIMEOUT) != 0
		&& timeout != B_INFINITE_TIMEOUT && timeout > 0) {
		// Make the timeout absolute, since we have more than one step where
		// we might have to wait
		flags = (flags & ~B_RELATIVE_TIMEOUT) | B_ABSOLUTE_TIMEOUT;
		timeout += system_time();
	}

	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) > 0;

	status_t status;
	port_message* message = NULL;

	// get the port
	Port* port = get_locked_port(id);
	if (port == NULL) {
		TRACE(("write_port_etc: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(port->lock, true);

	if (is_port_closed(port)) {
		TRACE(("write_port_etc: port %ld closed\n", id));
		return B_BAD_PORT_ID;
	}

	if (port->write_count <= 0) {
		if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0)
			return B_WOULD_BLOCK;

		port->write_count--;

		// We need to block in order to wait for a free message slot
		ConditionVariableEntry entry;
		port->write_condition.Add(&entry);

		locker.Unlock();

		status = entry.Wait(flags, timeout);

		// re-lock
		Port* newPort = get_locked_port(id);
		if (newPort == NULL) {
			T(Write(id, 0, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
		locker.SetTo(newPort->lock, true);

		if (newPort != port || is_port_closed(port)) {
			// the port is no longer there
			T(Write(id, 0, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}

		if (status != B_OK)
			goto error;
	} else
		port->write_count--;

	status = get_port_message(msgCode, bufferSize, flags, timeout,
		&message);
	if (status != B_OK)
		goto error;

	// sender credentials
	message->sender = geteuid();
	message->sender_group = getegid();
	message->sender_team = team_get_current_team_id();

	if (bufferSize > 0) {
		uint32 i;
		if (userCopy) {
			// copy from user memory
			for (i = 0; i < vecCount; i++) {
				size_t bytes = msgVecs[i].iov_len;
				if (bytes > bufferSize)
					bytes = bufferSize;

				status_t status = user_memcpy(message->buffer,
					msgVecs[i].iov_base, bytes);
				if (status != B_OK) {
					put_port_message(message);
					goto error;
				}

				bufferSize -= bytes;
				if (bufferSize == 0)
					break;
			}
		} else {
			// copy from kernel memory
			for (i = 0; i < vecCount; i++) {
				size_t bytes = msgVecs[i].iov_len;
				if (bytes > bufferSize)
					bytes = bufferSize;

				memcpy(message->buffer, msgVecs[i].iov_base, bytes);

				bufferSize -= bytes;
				if (bufferSize == 0)
					break;
			}
		}
	}

	port->messages.Add(message);
	port->read_count++;

	T(Write(id, port->read_count, port->write_count, message->code,
		message->size, B_OK));

	notify_port_select_events(port, B_EVENT_READ);
	port->read_condition.NotifyOne();
	return B_OK;

error:
	// Give up our slot in the queue again, and let someone else
	// try and fail
	T(Write(id, port->read_count, port->write_count, 0, 0, status));
	port->write_count++;
	notify_port_select_events(port, B_EVENT_WRITE);
	port->write_condition.NotifyOne();

	return status;
}


status_t
set_port_owner(port_id id, team_id newTeamID)
{
	TRACE(("set_port_owner(id = %ld, team = %ld)\n", id, newTeamID));

	if (id < 0)
		return B_BAD_PORT_ID;

	// get the new team
	Team* team = Team::Get(newTeamID);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	// get the port
	MutexLocker portsLocker(sPortsLock);
	Port* port = sPorts.Lookup(id);
	if (port == NULL) {
		TRACE(("set_port_owner: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(port->lock);

	// transfer ownership to other team
	if (team->id != port->owner) {
		list_remove_link(&port->team_link);
		list_add_item(&team->port_list, &port->team_link);
		port->owner = team->id;
	}

	T(OwnerChange(port, team->id, B_OK));
	return B_OK;
}


//	#pragma mark - syscalls


port_id
_user_create_port(int32 queueLength, const char *userName)
{
	char name[B_OS_NAME_LENGTH];

	if (userName == NULL)
		return create_port(queueLength, NULL);

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return create_port(queueLength, name);
}


status_t
_user_close_port(port_id id)
{
	return close_port(id);
}


status_t
_user_delete_port(port_id id)
{
	return delete_port(id);
}


port_id
_user_find_port(const char *userName)
{
	char name[B_OS_NAME_LENGTH];

	if (userName == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return find_port(name);
}


status_t
_user_get_port_info(port_id id, struct port_info *userInfo)
{
	struct port_info info;
	status_t status;

	if (userInfo == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = get_port_info(id, &info);

	// copy back to user space
	if (status == B_OK
		&& user_memcpy(userInfo, &info, sizeof(struct port_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_port_info(team_id team, int32 *userCookie,
	struct port_info *userInfo)
{
	struct port_info info;
	status_t status;
	int32 cookie;

	if (userCookie == NULL || userInfo == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userCookie) || !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	status = get_next_port_info(team, &cookie, &info);

	// copy back to user space
	if (user_memcpy(userCookie, &cookie, sizeof(int32)) < B_OK
		|| (status == B_OK && user_memcpy(userInfo, &info,
				sizeof(struct port_info)) < B_OK))
		return B_BAD_ADDRESS;

	return status;
}


ssize_t
_user_port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout)
{
	syscall_restart_handle_timeout_pre(flags, timeout);

	status_t status = port_buffer_size_etc(port, flags | B_CAN_INTERRUPT,
		timeout);

	return syscall_restart_handle_timeout_post(status, timeout);
}


ssize_t
_user_port_count(port_id port)
{
	return port_count(port);
}


status_t
_user_set_port_owner(port_id port, team_id team)
{
	return set_port_owner(port, team);
}


ssize_t
_user_read_port_etc(port_id port, int32 *userCode, void *userBuffer,
	size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	int32 messageCode;
	ssize_t	bytesRead;

	syscall_restart_handle_timeout_pre(flags, timeout);

	if (userBuffer == NULL && bufferSize != 0)
		return B_BAD_VALUE;
	if ((userCode != NULL && !IS_USER_ADDRESS(userCode))
		|| (userBuffer != NULL && !IS_USER_ADDRESS(userBuffer)))
		return B_BAD_ADDRESS;

	bytesRead = read_port_etc(port, &messageCode, userBuffer, bufferSize,
		flags | PORT_FLAG_USE_USER_MEMCPY | B_CAN_INTERRUPT, timeout);

	if (bytesRead >= 0 && userCode != NULL
		&& user_memcpy(userCode, &messageCode, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	return syscall_restart_handle_timeout_post(bytesRead, timeout);
}


status_t
_user_write_port_etc(port_id port, int32 messageCode, const void *userBuffer,
	size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	iovec vec = { (void *)userBuffer, bufferSize };

	syscall_restart_handle_timeout_pre(flags, timeout);

	if (userBuffer == NULL && bufferSize != 0)
		return B_BAD_VALUE;
	if (userBuffer != NULL && !IS_USER_ADDRESS(userBuffer))
		return B_BAD_ADDRESS;

	status_t status = writev_port_etc(port, messageCode, &vec, 1, bufferSize,
		flags | PORT_FLAG_USE_USER_MEMCPY | B_CAN_INTERRUPT, timeout);

	return syscall_restart_handle_timeout_post(status, timeout);
}


status_t
_user_writev_port_etc(port_id port, int32 messageCode, const iovec *userVecs,
	size_t vecCount, size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	syscall_restart_handle_timeout_pre(flags, timeout);

	if (userVecs == NULL && bufferSize != 0)
		return B_BAD_VALUE;
	if (userVecs != NULL && !IS_USER_ADDRESS(userVecs))
		return B_BAD_ADDRESS;

	iovec *vecs = NULL;
	if (userVecs && vecCount != 0) {
		vecs = (iovec*)malloc(sizeof(iovec) * vecCount);
		if (vecs == NULL)
			return B_NO_MEMORY;

		if (user_memcpy(vecs, userVecs, sizeof(iovec) * vecCount) < B_OK) {
			free(vecs);
			return B_BAD_ADDRESS;
		}
	}

	status_t status = writev_port_etc(port, messageCode, vecs, vecCount,
		bufferSize, flags | PORT_FLAG_USE_USER_MEMCPY | B_CAN_INTERRUPT,
		timeout);

	free(vecs);
	return syscall_restart_handle_timeout_post(status, timeout);
}


status_t
_user_get_port_message_info_etc(port_id port, port_message_info *userInfo,
	size_t infoSize, uint32 flags, bigtime_t timeout)
{
	if (userInfo == NULL || infoSize != sizeof(port_message_info))
		return B_BAD_VALUE;

	syscall_restart_handle_timeout_pre(flags, timeout);

	port_message_info info;
	status_t error = _get_port_message_info_etc(port, &info, sizeof(info),
		flags | B_CAN_INTERRUPT, timeout);

	// copy info to userland
	if (error == B_OK && (!IS_USER_ADDRESS(userInfo)
			|| user_memcpy(userInfo, &info, sizeof(info)) != B_OK)) {
		error = B_BAD_ADDRESS;
	}

	return syscall_restart_handle_timeout_post(error, timeout);
}
