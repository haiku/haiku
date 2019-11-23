/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Mark-Jan Bastian. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*!	Ports for IPC */


#include <port.h>

#include <algorithm>
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


#if __GNUC__ >= 3
#	define GCC_2_NRV(x)
	// GCC >= 3.1 doesn't need it anymore
#else
#	define GCC_2_NRV(x) return x;
	// GCC 2 named return value syntax
	// see http://gcc.gnu.org/onlinedocs/gcc-2.95.2/gcc_5.html#SEC106
#endif


// Locking:
// * sPortsLock: Protects the sPorts and sPortsByName hash tables.
// * sTeamListLock[]: Protects Team::port_list. Lock index for given team is
//   (Team::id % kTeamListLockCount).
// * Port::lock: Protects all Port members save team_link, hash_link, lock and
//   state. id is immutable.
//
// Port::state ensures atomicity by providing a linearization point for adding
// and removing ports to the hash tables and the team port list.
// * sPortsLock and sTeamListLock[] are locked separately and not in a nested
//   fashion, so a port can be in the hash table but not in the team port list
//   or vice versa. => Without further provisions, insertion and removal are
//   not linearizable and thus not concurrency-safe.
// * To make insertion and removal linearizable, Port::state was added. It is
//   always only accessed atomically and updates are done using
//   atomic_test_and_set(). A port is only seen as existent when its state is
//   Port::kActive.
// * Deletion of ports is done in two steps: logical and physical deletion.
//   First, logical deletion happens and sets Port::state to Port::kDeleted.
//   This is an atomic operation and from then on, functions like
//   get_locked_port() consider this port as deleted and ignore it. Secondly,
//   physical deletion removes the port from hash tables and team port list.
//   In a similar way, port creation first inserts into hashes and team list
//   and only then sets port to Port::kActive.
//   This creates a linearization point at the atomic update of Port::state,
//   operations become linearizable and thus concurrency-safe. To help
//   understanding, the linearization points are annotated with comments.
// * Ports are reference-counted so it's not a problem when someone still
//   has a reference to a deleted port.


namespace {

struct port_message : DoublyLinkedListLinkImpl<port_message> {
	int32				code;
	size_t				size;
	uid_t				sender;
	gid_t				sender_group;
	team_id				sender_team;
	char				buffer[0];
};

typedef DoublyLinkedList<port_message> MessageList;

} // namespace


static void put_port_message(port_message* message);


namespace {

struct Port : public KernelReferenceable {
	enum State {
		kUnused = 0,
		kActive,
		kDeleted
	};

	struct list_link	team_link;
	Port*				hash_link;
	port_id				id;
	team_id				owner;
	Port*				name_hash_link;
	size_t				name_hash;
	int32				capacity;
	mutex				lock;
	int32				state;
	uint32				read_count;
	int32				write_count;
	ConditionVariable	read_condition;
	ConditionVariable	write_condition;
	int32				total_count;
		// messages read from port since creation
	select_info*		select_infos;
	MessageList			messages;

	Port(team_id owner, int32 queueLength, const char* name)
		:
		owner(owner),
		name_hash(0),
		capacity(queueLength),
		state(kUnused),
		read_count(0),
		write_count(queueLength),
		total_count(0),
		select_infos(NULL)
	{
		// id is initialized when the caller adds the port to the hash table

		mutex_init_etc(&lock, name, MUTEX_FLAG_CLONE_NAME);
		read_condition.Init(this, "port read");
		write_condition.Init(this, "port write");
	}

	virtual ~Port()
	{
		while (port_message* message = messages.RemoveHead())
			put_port_message(message);

		mutex_destroy(&lock);
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


struct PortNameHashDefinition {
	typedef const char*	KeyType;
	typedef	Port		ValueType;

	size_t HashKey(const char* key) const
	{
		// Hash function: hash(key) =  key[0] * 31^(length - 1)
		//   + key[1] * 31^(length - 2) + ... + key[length - 1]

		const size_t length = strlen(key);

		size_t hash = 0;
		for (size_t index = 0; index < length; index++)
			hash = 31 * hash + key[index];

		return hash;
	}

	size_t Hash(Port* value) const
	{
		size_t& hash = value->name_hash;
		if (hash == 0)
			hash = HashKey(value->lock.name);
		return hash;
	}

	bool Compare(const char* key, Port* value) const
	{
		return (strcmp(key, value->lock.name) == 0);
	}

	Port*& GetLink(Port* value) const
	{
		return value->name_hash_link;
	}
};

typedef BOpenHashTable<PortNameHashDefinition> PortNameHashTable;


class PortNotificationService : public DefaultNotificationService {
public:
							PortNotificationService();

			void			Notify(uint32 opcode, port_id team);
};

} // namespace


// #pragma mark - tracing


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
	Read(const BReference<Port>& portRef, int32 code, ssize_t result)
		:
		fID(portRef->id),
		fReadCount(portRef->read_count),
		fWriteCount(portRef->write_count),
		fCode(code),
		fResult(result)
	{
		Initialized();
	}

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
	Info(const BReference<Port>& portRef, int32 code, ssize_t result)
		:
		fID(portRef->id),
		fReadCount(portRef->read_count),
		fWriteCount(portRef->write_count),
		fCode(code),
		fResult(result)
	{
		Initialized();
	}

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
static int32 sUsedPorts;

static PortHashTable sPorts;
static PortNameHashTable sPortsByName;
static ConditionVariable sNoSpaceCondition;
static int32 sTotalSpaceCommited;
static int32 sWaitingForSpace;
static port_id sNextPortID = 1;
static bool sPortsActive = false;
static rw_lock sPortsLock = RW_LOCK_INITIALIZER("ports list");

enum {
	kTeamListLockCount = 8
};

static mutex sTeamListLock[kTeamListLockCount] = {
	MUTEX_INITIALIZER("team ports list 1"),
	MUTEX_INITIALIZER("team ports list 2"),
	MUTEX_INITIALIZER("team ports list 3"),
	MUTEX_INITIALIZER("team ports list 4"),
	MUTEX_INITIALIZER("team ports list 5"),
	MUTEX_INITIALIZER("team ports list 6"),
	MUTEX_INITIALIZER("team ports list 7"),
	MUTEX_INITIALIZER("team ports list 8")
};

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
	char eventBuffer[128];
	KMessage event;
	event.SetTo(eventBuffer, sizeof(eventBuffer), PORT_MONITOR);
	event.AddInt32("event", opcode);
	event.AddInt32("port", port);

	DefaultNotificationService::Notify(event, opcode);
}


//	#pragma mark - debugger commands


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

		kprintf("%p %8" B_PRId32 " %4" B_PRId32 " %9" B_PRIu32 " %9" B_PRId32
			" %8" B_PRId32 " %6" B_PRId32 "  %s\n", port, port->id,
			port->capacity, port->read_count, port->write_count,
			port->total_count, port->owner, port->lock.name);
	}

	return 0;
}


static void
_dump_port_info(Port* port)
{
	kprintf("PORT: %p\n", port);
	kprintf(" id:              %" B_PRId32 "\n", port->id);
	kprintf(" name:            \"%s\"\n", port->lock.name);
	kprintf(" owner:           %" B_PRId32 "\n", port->owner);
	kprintf(" capacity:        %" B_PRId32 "\n", port->capacity);
	kprintf(" read_count:      %" B_PRIu32 "\n", port->read_count);
	kprintf(" write_count:     %" B_PRId32 "\n", port->write_count);
	kprintf(" total count:     %" B_PRId32 "\n", port->total_count);

	if (!port->messages.IsEmpty()) {
		kprintf("messages:\n");

		MessageList::Iterator iterator = port->messages.GetIterator();
		while (port_message* message = iterator.Next()) {
			kprintf(" %p  %08" B_PRIx32 "  %ld\n", message, message->code, message->size);
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
		if (port == NULL || port->state != Port::kActive) {
			kprintf("port %" B_PRId32 " (%#" B_PRIx32 ") doesn't exist!\n",
				num, num);
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


// #pragma mark - internal helper functions


/*!	Notifies the port's select events.
	The port must be locked.
*/
static void
notify_port_select_events(Port* port, uint16 events)
{
	if (port->select_infos)
		notify_select_events_list(port->select_infos, events);
}


static BReference<Port>
get_locked_port(port_id id) GCC_2_NRV(portRef)
{
#if __GNUC__ >= 3
	BReference<Port> portRef;
#endif
	{
		ReadLocker portsLocker(sPortsLock);
		portRef.SetTo(sPorts.Lookup(id));
	}

	if (portRef != NULL && portRef->state == Port::kActive) {
		if (mutex_lock(&portRef->lock) != B_OK)
			portRef.Unset();
	} else
		portRef.Unset();

	return portRef;
}


static BReference<Port>
get_port(port_id id) GCC_2_NRV(portRef)
{
#if __GNUC__ >= 3
	BReference<Port> portRef;
#endif
	ReadLocker portsLocker(sPortsLock);
	portRef.SetTo(sPorts.Lookup(id));

	return portRef;
}


/*!	You need to own the port's lock when calling this function */
static inline bool
is_port_closed(Port* port)
{
	return port->capacity == 0;
}


static void
put_port_message(port_message* message)
{
	const size_t size = sizeof(port_message) + message->size;
	free(message);

	atomic_add(&sTotalSpaceCommited, -size);
	if (sWaitingForSpace > 0)
		sNoSpaceCondition.NotifyAll();
}


/*! Port must be locked. */
static status_t
get_port_message(int32 code, size_t bufferSize, uint32 flags, bigtime_t timeout,
	port_message** _message, Port& port)
{
	const size_t size = sizeof(port_message) + bufferSize;

	while (true) {
		int32 previouslyCommited = atomic_add(&sTotalSpaceCommited, size);

		while (previouslyCommited + size > kTotalSpaceLimit) {
			// TODO: add per team limit

			// We are not allowed to allocate more memory, as our
			// space limit has been reached - just wait until we get
			// some free space again.

			atomic_add(&sTotalSpaceCommited, -size);

			// TODO: we don't want to wait - but does that also mean we
			// shouldn't wait for free memory?
			if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0)
				return B_WOULD_BLOCK;

			ConditionVariableEntry entry;
			sNoSpaceCondition.Add(&entry);

			port_id portID = port.id;
			mutex_unlock(&port.lock);

			atomic_add(&sWaitingForSpace, 1);

			// TODO: right here the condition could be notified and we'd
			//       miss it.

			status_t status = entry.Wait(flags, timeout);

			atomic_add(&sWaitingForSpace, -1);

			// re-lock the port
			BReference<Port> newPortRef = get_locked_port(portID);

			if (newPortRef.Get() != &port || is_port_closed(&port)) {
				// the port is no longer usable
				return B_BAD_PORT_ID;
			}

			if (status == B_TIMED_OUT)
				return B_TIMED_OUT;

			previouslyCommited = atomic_add(&sTotalSpaceCommited, size);
			continue;
		}

		// Quota is fulfilled, try to allocate the buffer
		port_message* message = (port_message*)malloc(size);
		if (message != NULL) {
			message->code = code;
			message->size = bufferSize;

			*_message = message;
			return B_OK;
		}

		// We weren't able to allocate and we'll start over,so we remove our
		// size from the commited-counter again.
		atomic_add(&sTotalSpaceCommited, -size);
		continue;
	}
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
	size_t size = std::min(bufferSize, message->size);

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
uninit_port(Port* port)
{
	MutexLocker locker(port->lock);

	notify_port_select_events(port, B_EVENT_INVALID);
	port->select_infos = NULL;

	// Release the threads that were blocking on this port.
	// read_port() will see the B_BAD_PORT_ID return value, and act accordingly
	port->read_condition.NotifyAll(B_BAD_PORT_ID);
	port->write_condition.NotifyAll(B_BAD_PORT_ID);
	sNotificationService.Notify(PORT_REMOVED, port->id);
}


/*! Caller must ensure there is still a reference to the port. (Either by
 *  holding a reference itself or by holding a lock on one of the data
 *  structures in which it is referenced.)
 */
static status_t
delete_port_logical(Port* port)
{
	for (;;) {
		// Try to logically delete
		const int32 oldState = atomic_test_and_set(&port->state,
			Port::kDeleted, Port::kActive);
			// Linearization point for port deletion

		switch (oldState) {
			case Port::kActive:
				// Logical deletion succesful
				return B_OK;

			case Port::kDeleted:
				// Someone else already deleted it in the meantime
				TRACE(("delete_port_logical: already deleted port_id %ld\n",
						port->id));
				return B_BAD_PORT_ID;

			case Port::kUnused:
				// Port is still being created, retry
				continue;

			default:
				// Port state got corrupted somehow
				panic("Invalid port state!\n");
		}
	}
}


//	#pragma mark - private kernel API


/*! This function deletes all the ports that are owned by the passed team.
*/
void
delete_owned_ports(Team* team)
{
	TRACE(("delete_owned_ports(owner = %ld)\n", team->id));

	list deletionList;
	list_init_etc(&deletionList, port_team_link_offset());

	const uint8 lockIndex = team->id % kTeamListLockCount;
	MutexLocker teamPortsListLocker(sTeamListLock[lockIndex]);

	// Try to logically delete all ports from the team's port list.
	// On success, move the port to deletionList.
	Port* port = (Port*)list_get_first_item(&team->port_list);
	while (port != NULL) {
		status_t status = delete_port_logical(port);
			// Contains linearization point

		Port* nextPort = (Port*)list_get_next_item(&team->port_list, port);

		if (status == B_OK) {
			list_remove_link(&port->team_link);
			list_add_item(&deletionList, port);
		}

		port = nextPort;
	}

	teamPortsListLocker.Unlock();

	// Remove all ports in deletionList from hashes
	{
		WriteLocker portsLocker(sPortsLock);

		for (Port* port = (Port*)list_get_first_item(&deletionList);
			 port != NULL;
			 port = (Port*)list_get_next_item(&deletionList, port)) {

			sPorts.Remove(port);
			sPortsByName.Remove(port);
			port->ReleaseReference();
				// joint reference for sPorts and sPortsByName
		}
	}

	// Uninitialize ports and release team port list references
	while (Port* port = (Port*)list_remove_head_item(&deletionList)) {
		atomic_add(&sUsedPorts, -1);
		uninit_port(port);
		port->ReleaseReference();
			// Reference for team port list
	}
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


size_t
port_team_link_offset()
{
	// Somewhat ugly workaround since we cannot use offsetof() for a class
	// with vtable (GCC4 throws a warning then).
	Port* port = (Port*)0;
	return (size_t)&port->team_link;
}


status_t
port_init(kernel_args *args)
{
	// initialize ports table and by-name hash
	new(&sPorts) PortHashTable;
	if (sPorts.Init() != B_OK) {
		panic("Failed to init port hash table!");
		return B_NO_MEMORY;
	}

	new(&sPortsByName) PortNameHashTable;
	if (sPortsByName.Init() != B_OK) {
		panic("Failed to init port by name hash table!");
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
	sNotificationService.Register();
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

	// create a port
	BReference<Port> port;
	{
		Port* newPort = new(std::nothrow) Port(team_get_current_team_id(),
			queueLength, name != NULL ? name : "unnamed port");
		if (newPort == NULL)
			return B_NO_MEMORY;
		port.SetTo(newPort, true);
	}

	// check the ports limit
	const int32 previouslyUsed = atomic_add(&sUsedPorts, 1);
	if (previouslyUsed + 1 >= sMaxPorts) {
		atomic_add(&sUsedPorts, -1);
		return B_NO_MORE_PORTS;
	}

	{
		WriteLocker locker(sPortsLock);

		// allocate a port ID
		do {
			port->id = sNextPortID++;

			// handle integer overflow
			if (sNextPortID < 0)
				sNextPortID = 1;
		} while (sPorts.Lookup(port->id) != NULL);

		// Insert port physically:
		// (1/2) Insert into hash tables
		port->AcquireReference();
			// joint reference for sPorts and sPortsByName

		sPorts.Insert(port);
		sPortsByName.Insert(port);
	}

	// (2/2) Insert into team list
	{
		const uint8 lockIndex = port->owner % kTeamListLockCount;
		MutexLocker teamPortsListLocker(sTeamListLock[lockIndex]);
		port->AcquireReference();
		list_add_item(&team->port_list, port);
	}

	// tracing, notifications, etc.
	T(Create(port));

	const port_id id = port->id;

	// Insert port logically by marking it active
	const int32 oldState = atomic_test_and_set(&port->state,
		Port::kActive, Port::kUnused);
		// Linearization point for port creation

	if (oldState != Port::kUnused) {
		// Nobody is allowed to tamper with the port before it's active.
		panic("Port state was modified during creation!\n");
	}

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
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL) {
		TRACE(("close_port: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker lock(&portRef->lock, true);

	// mark port to disable writing - deleting the semaphores will
	// wake up waiting read/writes
	portRef->capacity = 0;

	notify_port_select_events(portRef, B_EVENT_INVALID);
	portRef->select_infos = NULL;

	portRef->read_condition.NotifyAll(B_BAD_PORT_ID);
	portRef->write_condition.NotifyAll(B_BAD_PORT_ID);

	return B_OK;
}


status_t
delete_port(port_id id)
{
	TRACE(("delete_port(id = %ld)\n", id));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	BReference<Port> portRef = get_port(id);

	if (portRef == NULL) {
		TRACE(("delete_port: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	status_t status = delete_port_logical(portRef);
		// Contains linearization point
	if (status != B_OK)
		return status;

	// Now remove port physically:
	// (1/2) Remove from hash tables
	{
		WriteLocker portsLocker(sPortsLock);

		sPorts.Remove(portRef);
		sPortsByName.Remove(portRef);

		portRef->ReleaseReference();
			// joint reference for sPorts and sPortsByName
	}

	// (2/2) Remove from team port list
	{
		const uint8 lockIndex = portRef->owner % kTeamListLockCount;
		MutexLocker teamPortsListLocker(sTeamListLock[lockIndex]);

		list_remove_link(&portRef->team_link);
		portRef->ReleaseReference();
	}

	uninit_port(portRef);

	T(Delete(portRef));

	atomic_add(&sUsedPorts, -1);

	return B_OK;
}


status_t
select_port(int32 id, struct select_info* info, bool kernel)
{
	if (id < 0)
		return B_BAD_PORT_ID;

	// get the port
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(portRef->lock, true);

	// port must not yet be closed
	if (is_port_closed(portRef))
		return B_BAD_PORT_ID;

	if (!kernel && portRef->owner == team_get_kernel_team_id()) {
		// kernel port, but call from userland
		return B_NOT_ALLOWED;
	}

	info->selected_events &= B_EVENT_READ | B_EVENT_WRITE | B_EVENT_INVALID;

	if (info->selected_events != 0) {
		uint16 events = 0;

		info->next = portRef->select_infos;
		portRef->select_infos = info;

		// check for events
		if ((info->selected_events & B_EVENT_READ) != 0
			&& !portRef->messages.IsEmpty()) {
			events |= B_EVENT_READ;
		}

		if (portRef->write_count > 0)
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
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(portRef->lock, true);

	// find and remove the infos
	select_info** infoLocation = &portRef->select_infos;
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

	ReadLocker locker(sPortsLock);
	Port* port = sPortsByName.Lookup(name);
		// Since we have sPortsLock and don't return the port itself,
		// no BReference necessary

	if (port != NULL && port->state == Port::kActive)
		return port->id;

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
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL) {
		TRACE(("get_port_info: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(portRef->lock, true);

	// fill a port_info struct with info
	fill_port_info(portRef, info, size);
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
	const uint8 lockIndex = teamID % kTeamListLockCount;
	MutexLocker teamPortsListLocker(sTeamListLock[lockIndex]);

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
	BReference<Port> portRef = port;
	teamPortsListLocker.Unlock();
		// Only use portRef below this line...

	MutexLocker locker(portRef->lock);
	fill_port_info(portRef, info, size);

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
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(portRef->lock, true);

	if (is_port_closed(portRef) && portRef->messages.IsEmpty()) {
		T(Info(portRef, 0, B_BAD_PORT_ID));
		TRACE(("_get_port_message_info_etc(): closed port %ld\n", id));
		return B_BAD_PORT_ID;
	}

	while (portRef->read_count == 0) {
		// We need to wait for a message to appear
		if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0)
			return B_WOULD_BLOCK;

		ConditionVariableEntry entry;
		portRef->read_condition.Add(&entry);

		locker.Unlock();

		// block if no message, or, if B_TIMEOUT flag set, block with timeout
		status_t status = entry.Wait(flags, timeout);

		if (status != B_OK) {
			T(Info(portRef, 0, status));
			return status;
		}

		// re-lock
		BReference<Port> newPortRef = get_locked_port(id);
		if (newPortRef == NULL) {
			T(Info(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
		locker.SetTo(newPortRef->lock, true);

		if (newPortRef != portRef
			|| (is_port_closed(portRef) && portRef->messages.IsEmpty())) {
			// the port is no longer there
			T(Info(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
	}

	// determine tail & get the length of the message
	port_message* message = portRef->messages.Head();
	if (message == NULL) {
		panic("port %" B_PRId32 ": no messages found\n", portRef->id);
		return B_ERROR;
	}

	info->size = message->size;
	info->sender = message->sender;
	info->sender_group = message->sender_group;
	info->sender_team = message->sender_team;

	T(Info(portRef, message->code, B_OK));

	// notify next one, as we haven't read from the port
	portRef->read_condition.NotifyOne();

	return B_OK;
}


ssize_t
port_count(port_id id)
{
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	// get the port
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL) {
		TRACE(("port_count: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(portRef->lock, true);

	// return count of messages
	return portRef->read_count;
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
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL)
		return B_BAD_PORT_ID;
	MutexLocker locker(portRef->lock, true);

	if (is_port_closed(portRef) && portRef->messages.IsEmpty()) {
		T(Read(portRef, 0, B_BAD_PORT_ID));
		TRACE(("read_port_etc(): closed port %ld\n", id));
		return B_BAD_PORT_ID;
	}

	while (portRef->read_count == 0) {
		if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0)
			return B_WOULD_BLOCK;

		// We need to wait for a message to appear
		ConditionVariableEntry entry;
		portRef->read_condition.Add(&entry);

		locker.Unlock();

		// block if no message, or, if B_TIMEOUT flag set, block with timeout
		status_t status = entry.Wait(flags, timeout);

		// re-lock
		BReference<Port> newPortRef = get_locked_port(id);
		if (newPortRef == NULL) {
			T(Read(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
		locker.SetTo(newPortRef->lock, true);

		if (newPortRef != portRef
			|| (is_port_closed(portRef) && portRef->messages.IsEmpty())) {
			// the port is no longer there
			T(Read(id, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}

		if (status != B_OK) {
			T(Read(portRef, 0, status));
			return status;
		}
	}

	// determine tail & get the length of the message
	port_message* message = portRef->messages.Head();
	if (message == NULL) {
		panic("port %" B_PRId32 ": no messages found\n", portRef->id);
		return B_ERROR;
	}

	if (peekOnly) {
		size_t size = copy_port_message(message, _code, buffer, bufferSize,
			userCopy);

		T(Read(portRef, message->code, size));

		portRef->read_condition.NotifyOne();
			// we only peeked, but didn't grab the message
		return size;
	}

	portRef->messages.RemoveHead();
	portRef->total_count++;
	portRef->write_count++;
	portRef->read_count--;

	notify_port_select_events(portRef, B_EVENT_WRITE);
	portRef->write_condition.NotifyOne();
		// make one spot in queue available again for write

	T(Read(portRef, message->code, std::min(bufferSize, message->size)));

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

	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) != 0;

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

	status_t status;
	port_message* message = NULL;

	// get the port
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL) {
		TRACE(("write_port_etc: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(portRef->lock, true);

	if (is_port_closed(portRef)) {
		TRACE(("write_port_etc: port %ld closed\n", id));
		return B_BAD_PORT_ID;
	}

	if (portRef->write_count <= 0) {
		if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0)
			return B_WOULD_BLOCK;

		portRef->write_count--;

		// We need to block in order to wait for a free message slot
		ConditionVariableEntry entry;
		portRef->write_condition.Add(&entry);

		locker.Unlock();

		status = entry.Wait(flags, timeout);

		// re-lock
		BReference<Port> newPortRef = get_locked_port(id);
		if (newPortRef == NULL) {
			T(Write(id, 0, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}
		locker.SetTo(newPortRef->lock, true);

		if (newPortRef != portRef || is_port_closed(portRef)) {
			// the port is no longer there
			T(Write(id, 0, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}

		if (status != B_OK)
			goto error;
	} else
		portRef->write_count--;

	status = get_port_message(msgCode, bufferSize, flags, timeout,
		&message, *portRef);
	if (status != B_OK) {
		if (status == B_BAD_PORT_ID) {
			// the port had to be unlocked and is now no longer there
			T(Write(id, 0, 0, 0, 0, B_BAD_PORT_ID));
			return B_BAD_PORT_ID;
		}

		goto error;
	}

	// sender credentials
	message->sender = geteuid();
	message->sender_group = getegid();
	message->sender_team = team_get_current_team_id();

	if (bufferSize > 0) {
		size_t offset = 0;
		for (uint32 i = 0; i < vecCount; i++) {
			size_t bytes = msgVecs[i].iov_len;
			if (bytes > bufferSize)
				bytes = bufferSize;

			if (userCopy) {
				status_t status = user_memcpy(message->buffer + offset,
					msgVecs[i].iov_base, bytes);
				if (status != B_OK) {
					put_port_message(message);
					goto error;
				}
			} else
				memcpy(message->buffer + offset, msgVecs[i].iov_base, bytes);

			bufferSize -= bytes;
			if (bufferSize == 0)
				break;

			offset += bytes;
		}
	}

	portRef->messages.Add(message);
	portRef->read_count++;

	T(Write(id, portRef->read_count, portRef->write_count, message->code,
		message->size, B_OK));

	notify_port_select_events(portRef, B_EVENT_READ);
	portRef->read_condition.NotifyOne();
	return B_OK;

error:
	// Give up our slot in the queue again, and let someone else
	// try and fail
	T(Write(id, portRef->read_count, portRef->write_count, 0, 0, status));
	portRef->write_count++;
	notify_port_select_events(portRef, B_EVENT_WRITE);
	portRef->write_condition.NotifyOne();

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
	BReference<Port> portRef = get_locked_port(id);
	if (portRef == NULL) {
		TRACE(("set_port_owner: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	MutexLocker locker(portRef->lock, true);

	// transfer ownership to other team
	if (team->id != portRef->owner) {
		uint8 firstLockIndex  = portRef->owner % kTeamListLockCount;
		uint8 secondLockIndex = team->id % kTeamListLockCount;

		// Avoid deadlocks: always lock lower index first
		if (secondLockIndex < firstLockIndex) {
			uint8 temp = secondLockIndex;
			secondLockIndex = firstLockIndex;
			firstLockIndex = temp;
		}

		MutexLocker oldTeamPortsListLocker(sTeamListLock[firstLockIndex]);
		MutexLocker newTeamPortsListLocker;
		if (firstLockIndex != secondLockIndex) {
			newTeamPortsListLocker.SetTo(sTeamListLock[secondLockIndex],
					false);
		}

		// Now that we have locked the team port lists, check the state again
		if (portRef->state == Port::kActive) {
			list_remove_link(&portRef->team_link);
			list_add_item(&team->port_list, portRef.Get());
			portRef->owner = team->id;
		} else {
			// Port was already deleted. We haven't changed anything yet so
			// we can cancel the operation.
			return B_BAD_PORT_ID;
		}
	}

	T(OwnerChange(portRef, team->id, B_OK));
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
