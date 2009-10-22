/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
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

#include <arch/int.h>
#include <heap.h>
#include <kernel.h>
#include <Notifications.h>
#include <sem.h>
#include <syscall_restart.h>
#include <team.h>
#include <util/AutoLock.h>
#include <util/list.h>
#include <wait_for_objects.h>


//#define TRACE_PORTS
#ifdef TRACE_PORTS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct port_message : DoublyLinkedListLinkImpl<port_message> {
	int32				code;
	size_t				size;
	uid_t				sender;
	gid_t				sender_group;
	team_id				sender_team;
	char				buffer[0];
};

typedef DoublyLinkedList<port_message> MessageList;

struct port_entry {
	port_id				id;
	team_id				owner;
	int32		 		capacity;
	mutex				lock;
	int32				read_count;
	int32				write_count;
	ConditionVariable	read_condition;
	ConditionVariable	write_condition;
	int32				total_count;
		// messages read from port since creation
	select_info*		select_infos;
	MessageList			messages;
};

class PortNotificationService : public DefaultNotificationService {
public:
							PortNotificationService();

			void			Notify(uint32 opcode, port_id team);
};

static const size_t kInitialPortBufferSize = 4 * 1024 * 1024;
#define MAX_QUEUE_LENGTH 4096
#define PORT_MAX_MESSAGE_SIZE (256 * 1024)

// sMaxPorts must be power of 2
static int32 sMaxPorts = 4096;
static int32 sUsedPorts = 0;

static struct port_entry* sPorts;
static area_id sPortArea;
static heap_allocator* sPortAllocator;
static bool sPortsActive = false;
static port_id sNextPort = 1;
static int32 sFirstFreeSlot = 1;
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
	int32 i;

	if (argc > 2) {
		if (!strcmp(argv[1], "team") || !strcmp(argv[1], "owner"))
			owner = strtoul(argv[2], NULL, 0);
		else if (!strcmp(argv[1], "name"))
			name = argv[2];
	} else if (argc > 1)
		owner = strtoul(argv[1], NULL, 0);

	kprintf("port             id  cap  read-cnt  write-cnt   total   team  "
		"name\n");

	for (i = 0; i < sMaxPorts; i++) {
		struct port_entry* port = &sPorts[i];
		if (port->id < 0
			|| (owner != -1 && port->owner != owner)
			|| (name != NULL && strstr(port->lock.name, name) == NULL))
			continue;

		kprintf("%p %8ld %4ld %9ld %9ld %8ld %6ld  %s\n", port,
			port->id, port->capacity, port->read_count, port->write_count,
			port->total_count, port->owner, port->lock.name);
	}

	return 0;
}


static void
_dump_port_info(struct port_entry* port)
{
	kprintf("PORT: %p\n", port);
	kprintf(" id:              %ld\n", port->id);
	kprintf(" name:            \"%s\"\n", port->lock.name);
	kprintf(" owner:           %ld\n", port->owner);
	kprintf(" capacity:        %ld\n", port->capacity);
	kprintf(" read_count:      %ld\n", port->read_count);
	kprintf(" write_count:     %ld\n", port->write_count);
	kprintf(" total count:     %ld\n", port->total_count);

	set_debug_variable("_port", (addr_t)port);
	set_debug_variable("_portID", port->id);
	set_debug_variable("_owner", port->owner);
}


static int
dump_port_info(int argc, char** argv)
{
	const char* name = NULL;
	ConditionVariable* condition = NULL;
	int i;

	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc > 2) {
		if (!strcmp(argv[1], "address")) {
			_dump_port_info((struct port_entry*)strtoul(argv[2], NULL, 0));
			return 0;
		} else if (!strcmp(argv[1], "condition"))
			condition = (ConditionVariable*)strtoul(argv[2], NULL, 0);
		else if (!strcmp(argv[1], "name"))
			name = argv[2];
	} else if (isdigit(argv[1][0])) {
		// if the argument looks like a number, treat it as such
		uint32 num = strtoul(argv[1], NULL, 0);
		uint32 slot = num % sMaxPorts;
		if (sPorts[slot].id != (int)num) {
			kprintf("port %ld (%#lx) doesn't exist!\n", num, num);
			return 0;
		}
		_dump_port_info(&sPorts[slot]);
		return 0;
	} else
		name = argv[1];

	// walk through the ports list, trying to match name
	for (i = 0; i < sMaxPorts; i++) {
		if ((name != NULL && sPorts[i].lock.name != NULL
				&& !strcmp(name, sPorts[i].lock.name))
			|| (condition != NULL && (&sPorts[i].read_condition == condition
				|| &sPorts[i].write_condition == condition))) {
			_dump_port_info(&sPorts[i]);
			return 0;
		}
	}

	return 0;
}


static void
notify_port_select_events(int slot, uint16 events)
{
	if (sPorts[slot].select_infos)
		notify_select_events_list(sPorts[slot].select_infos, events);
}


static void
put_port_message(port_message* message)
{
	heap_free(sPortAllocator, message);
}


static port_message*
get_port_message(int32 code, size_t bufferSize)
{
	port_message* message = (port_message*)heap_memalign(sPortAllocator,
		0, sizeof(port_message) + bufferSize);
	if (message == NULL) {
		// TODO: add another heap area until we ran into some limit
		return NULL;
	}

	message->code = code;
	message->size = bufferSize;

	return message;
}


/*!	You need to own the port's lock when calling this function */
static bool
is_port_closed(int32 slot)
{
	return sPorts[slot].capacity == 0;
}


/*!	Fills the port_info structure with information from the specified
	port.
	The port lock must be held when called.
*/
static void
fill_port_info(struct port_entry* port, port_info* info, size_t size)
{
	info->port = port->id;
	info->team = port->owner;
	info->capacity = port->capacity;

	int32 count = port->read_count;
	if (count < 0)
		count = 0;

	info->queue_count = count;
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


//	#pragma mark - private kernel API


/*! This function cycles through the ports table, deleting all
	the ports that are owned by the passed team_id
*/
int
delete_owned_ports(team_id owner)
{
	// TODO: investigate maintaining a list of ports in the team
	//	to make this simpler and more efficient.

	TRACE(("delete_owned_ports(owner = %ld)\n", owner));

	MutexLocker locker(sPortsLock);

	int32 count = 0;

	for (int32 i = 0; i < sMaxPorts; i++) {
		if (sPorts[i].id != -1 && sPorts[i].owner == owner) {
			port_id id = sPorts[i].id;

			locker.Unlock();

			delete_port(id);
			count++;

			locker.Lock();
		}
	}

	return count;
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
	size_t size = sizeof(struct port_entry) * sMaxPorts;

	// create and initialize ports table
	sPortArea = create_area("port_table",
		(void**)&sPorts, B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (sPortArea < 0) {
		panic("unable to allocate kernel port table!\n");
		return sPortArea;
	}

	memset(sPorts, 0, size);
	for (int32 i = 0; i < sMaxPorts; i++) {
		mutex_init(&sPorts[i].lock, NULL);
		sPorts[i].id = -1;
		sPorts[i].read_condition.Init(&sPorts[i], "port read");
		sPorts[i].write_condition.Init(&sPorts[i], "port write");
	}

	addr_t base;
	if (create_area("port heap", (void**)&base, B_ANY_KERNEL_ADDRESS,
			kInitialPortBufferSize, B_NO_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA) < 0) {
		panic("unable to allocate port area!\n");
		return B_ERROR;
	}

	static const heap_class kBufferHeapClass = {"default", 100,
		PORT_MAX_MESSAGE_SIZE + sizeof(port_message), 2 * 1024,
		sizeof(port_message), 8, 4, 64};
	sPortAllocator = heap_create_allocator("port buffer", base,
		kInitialPortBufferSize, &kBufferHeapClass, true);

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
		"([ \"address\" ] <address>) | ([ \"name\" ] <name>) "
			"| (\"sem\" <sem>)\n"
		"Prints info about the specified port.\n"
		"  <address>  - Pointer to the port structure.\n"
		"  <name>     - Name of the port.\n"
		"  <sem>      - ID of the port's read or write semaphore.\n", 0);

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

	MutexLocker locker(sPortsLock);

	// check early on if there are any free port slots to use
	if (sUsedPorts >= sMaxPorts)
		return B_NO_MORE_PORTS;

	// check & dup name
	char* nameBuffer = strdup(name != NULL ? name : "unnamed port");
	if (nameBuffer == NULL)
		return B_NO_MEMORY;

	sUsedPorts++;

	// find the first empty spot
	for (int32 slot = 0; slot < sMaxPorts; slot++) {
		int32 i = (slot + sFirstFreeSlot) % sMaxPorts;

		if (sPorts[i].id == -1) {
			// make the port_id be a multiple of the slot it's in
			if (i >= sNextPort % sMaxPorts)
				sNextPort += i - sNextPort % sMaxPorts;
			else
				sNextPort += sMaxPorts - (sNextPort % sMaxPorts - i);
			sFirstFreeSlot = slot + 1;

			MutexLocker portLocker(sPorts[i].lock);
			sPorts[i].id = sNextPort++;
			locker.Unlock();

			sPorts[i].capacity = queueLength;
			sPorts[i].owner = team_get_current_team_id();
			sPorts[i].lock.name = nameBuffer;
			sPorts[i].read_count = 0;
			sPorts[i].write_count = queueLength;
			sPorts[i].total_count = 0;
			sPorts[i].select_infos = NULL;

			port_id id = sPorts[i].id;
			portLocker.Unlock();

			TRACE(("create_port() done: port created %ld\n", id));

			sNotificationService.Notify(PORT_ADDED, id);
			return id;
		}
	}

	// Still not enough ports... - due to sUsedPorts, this cannot really
	// happen anymore.
	panic("out of ports, but sUsedPorts is broken");
	return B_NO_MORE_PORTS;
}


status_t
close_port(port_id id)
{
	TRACE(("close_port(id = %ld)\n", id));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	int32 slot = id % sMaxPorts;

	// walk through the sem list, trying to match name
	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id) {
		TRACE(("close_port: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	// mark port to disable writing - deleting the semaphores will
	// wake up waiting read/writes
	sPorts[slot].capacity = 0;

	notify_port_select_events(slot, B_EVENT_INVALID);
	sPorts[slot].select_infos = NULL;

	sPorts[slot].read_condition.NotifyAll(false, B_BAD_PORT_ID);
	sPorts[slot].write_condition.NotifyAll(false, B_BAD_PORT_ID);

	return B_OK;
}


status_t
delete_port(port_id id)
{
	TRACE(("delete_port(id = %ld)\n", id));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id) {
		TRACE(("delete_port: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	// mark port as invalid
	sPorts[slot].id	= -1;
	free((char*)sPorts[slot].lock.name);
	sPorts[slot].lock.name = NULL;

	while (port_message* message = sPorts[slot].messages.RemoveHead()) {
		put_port_message(message);
	}

	notify_port_select_events(slot, B_EVENT_INVALID);
	sPorts[slot].select_infos = NULL;

	// Release the threads that were blocking on this port.
	// read_port() will see the B_BAD_PORT_ID return value, and act accordingly
	sPorts[slot].read_condition.NotifyAll(B_BAD_PORT_ID);
	sPorts[slot].write_condition.NotifyAll(B_BAD_PORT_ID);
	sNotificationService.Notify(PORT_REMOVED, id);

	locker.Unlock();

	MutexLocker _(sPortsLock);

	// update the first free slot hint in the array
	if (slot < sFirstFreeSlot)
		sFirstFreeSlot = slot;

	sUsedPorts--;
	return B_OK;
}


status_t
select_port(int32 id, struct select_info* info, bool kernel)
{
	if (id < 0)
		return B_BAD_PORT_ID;

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id || is_port_closed(slot))
		return B_BAD_PORT_ID;
	if (!kernel && sPorts[slot].owner == team_get_kernel_team_id()) {
		// kernel port, but call from userland
		return B_NOT_ALLOWED;
	}

	info->selected_events &= B_EVENT_READ | B_EVENT_WRITE | B_EVENT_INVALID;

	if (info->selected_events != 0) {
		uint16 events = 0;

		info->next = sPorts[slot].select_infos;
		sPorts[slot].select_infos = info;

		// check for events
		if ((info->selected_events & B_EVENT_READ) != 0
			&& !sPorts[slot].messages.IsEmpty()) {
			events |= B_EVENT_READ;
		}

		if (sPorts[slot].write_count > 0)
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

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id == id) {
		select_info** infoLocation = &sPorts[slot].select_infos;
		while (*infoLocation != NULL && *infoLocation != info)
			infoLocation = &(*infoLocation)->next;

		if (*infoLocation == info)
			*infoLocation = info->next;
	}

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

	// Since we have to check every single port, and we don't
	// care if it goes away at any point, we're only grabbing
	// the port lock in question, not the port list lock

	// loop over list
	for (int32 i = 0; i < sMaxPorts; i++) {
		// lock every individual port before comparing
		MutexLocker _(sPorts[i].lock);

		if (sPorts[i].id >= 0 && !strcmp(name, sPorts[i].lock.name))
			return sPorts[i].id;
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

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id || sPorts[slot].capacity == 0) {
		TRACE(("get_port_info: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	// fill a port_info struct with info
	fill_port_info(&sPorts[slot], info, size);
	return B_OK;
}


status_t
_get_next_port_info(team_id team, int32* _cookie, struct port_info* info,
	size_t size)
{
	TRACE(("get_next_port_info(team = %ld)\n", team));

	if (info == NULL || size != sizeof(port_info) || _cookie == NULL
		|| team < B_OK)
		return B_BAD_VALUE;
	if (!sPortsActive)
		return B_BAD_PORT_ID;

	int32 slot = *_cookie;
	if (slot >= sMaxPorts)
		return B_BAD_PORT_ID;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();

	info->port = -1; // used as found flag

	while (slot < sMaxPorts) {
		MutexLocker locker(sPorts[slot].lock);

		if (sPorts[slot].id != -1 && !is_port_closed(slot)
			&& sPorts[slot].owner == team) {
			// found one!
			fill_port_info(&sPorts[slot], info, size);
			slot++;
			break;
		}

		slot++;
	}

	if (info->port == -1)
		return B_BAD_PORT_ID;

	*_cookie = slot;
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

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id
		|| (is_port_closed(slot) && sPorts[slot].messages.IsEmpty())) {
		TRACE(("port_buffer_size_etc(): %s port %ld\n",
			sPorts[slot].id == id ? "closed" : "invalid", id));
		return B_BAD_PORT_ID;
	}

	if (sPorts[slot].read_count <= 0) {
		// We need to wait for a message to appear
		ConditionVariableEntry entry;
		sPorts[slot].read_condition.Add(&entry);

		locker.Unlock();

		// block if no message, or, if B_TIMEOUT flag set, block with timeout
		status_t status = entry.Wait(flags, timeout);
		if (status != B_OK)
			return status;
		if (entry.WaitStatus() != B_OK)
			return entry.WaitStatus();

		locker.Lock();
	}

	if (sPorts[slot].id != id) {
		// the port is no longer there
		return B_BAD_PORT_ID;
	}

	// determine tail & get the length of the message
	port_message* message = sPorts[slot].messages.Head();
	if (message == NULL) {
		panic("port %ld: no messages found\n", sPorts[slot].id);
		return B_ERROR;
	}

	info->size = message->size;
	info->sender = message->sender;
	info->sender_group = message->sender_group;
	info->sender_team = message->sender_team;

	// notify next one, as we haven't read from the port
	sPorts[slot].read_condition.NotifyOne();

	return B_OK;
}


ssize_t
port_count(port_id id)
{
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id) {
		TRACE(("port_count: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	int32 count = sPorts[slot].read_count;
	// do not return negative numbers
	if (count < 0)
		count = 0;

	// return count of messages
	return count;
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

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id
		|| (is_port_closed(slot) && sPorts[slot].messages.IsEmpty())) {
		TRACE(("read_port_etc(): %s port %ld\n",
			sPorts[slot].id == id ? "closed" : "invalid", id));
		return B_BAD_PORT_ID;
	}

	if (sPorts[slot].read_count-- <= 0) {
		// We need to wait for a message to appear
		ConditionVariableEntry entry;
		sPorts[slot].read_condition.Add(&entry);

		locker.Unlock();

		// block if no message, or, if B_TIMEOUT flag set, block with timeout
		status_t status = entry.Wait(flags, timeout);

		locker.Lock();

		if (sPorts[slot].id != id) {
			// the port is no longer there
			return B_BAD_PORT_ID;
		}

		if (status != B_OK || entry.WaitStatus() != B_OK) {
			sPorts[slot].read_count++;
			return status != B_OK ? status : entry.WaitStatus();
		}
	}

	// determine tail & get the length of the message
	port_message* message = sPorts[slot].messages.Head();
	if (message == NULL) {
		panic("port %ld: no messages found\n", sPorts[slot].id);
		return B_ERROR;
	}

	if (peekOnly) {
		size_t size = copy_port_message(message, _code, buffer, bufferSize,
			userCopy);

		sPorts[slot].read_count++;
		sPorts[slot].read_condition.NotifyOne();
			// we only peeked, but didn't grab the message
		return size;
	}

	sPorts[slot].messages.RemoveHead();
	sPorts[slot].total_count++;
	sPorts[slot].write_count++;

	notify_port_select_events(slot, B_EVENT_WRITE);
	sPorts[slot].write_condition.NotifyOne();
		// make one spot in queue available again for write

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
	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) > 0;

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id) {
		TRACE(("write_port_etc: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	if (is_port_closed(slot)) {
		TRACE(("write_port_etc: port %ld closed\n", id));
		return B_BAD_PORT_ID;
	}

	if (sPorts[slot].write_count-- <= 0) {
		// We need to block in order to wait for a free message slot
		ConditionVariableEntry entry;
		sPorts[slot].write_condition.Add(&entry);

		locker.Unlock();

		status_t status = entry.Wait(flags, timeout);

		locker.Lock();

		if (sPorts[slot].id != id) {
			// the port is no longer there
			return B_BAD_PORT_ID;
		}

		if (status != B_OK || entry.WaitStatus() != B_OK) {
			sPorts[slot].write_count++;
			return status != B_OK ? status : entry.WaitStatus();
		}
	}

	port_message* message = get_port_message(msgCode, bufferSize);
	if (message == NULL) {
		// Give up our slot in the queue again, and let someone else
		// try and fail
		sPorts[slot].write_condition.NotifyOne();
		return B_NO_MEMORY;
	}

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
					return status;
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

	sPorts[slot].messages.Add(message);
	sPorts[slot].read_count++;

	notify_port_select_events(slot, B_EVENT_READ);
	sPorts[slot].read_condition.NotifyOne();

	return B_OK;
}


status_t
set_port_owner(port_id id, team_id team)
{
	TRACE(("set_port_owner(id = %ld, team = %ld)\n", id, team));

	if (id < 0)
		return B_BAD_PORT_ID;

	int32 slot = id % sMaxPorts;

	MutexLocker locker(sPorts[slot].lock);

	if (sPorts[slot].id != id) {
		TRACE(("set_port_owner: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	if (!team_is_valid(team))
		return B_BAD_TEAM_ID;

	// transfer ownership to other team
	sPorts[slot].owner = team;

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
