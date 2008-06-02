/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
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
#include <cbuf.h>
#include <kernel.h>
#include <sem.h>
#include <syscall_restart.h>
#include <team.h>
#include <util/list.h>
#include <wait_for_objects.h>


//#define TRACE_PORTS
#ifdef TRACE_PORTS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


typedef struct port_msg {
	list_link	link;
	int32		code;
	cbuf		*buffer_chain;
	size_t		size;
	uid_t		sender;
	gid_t		sender_group;
	team_id		sender_team;
} port_msg;

struct port_entry {
	port_id 	id;
	team_id 	owner;
	int32 		capacity;
	spinlock	lock;
	const char	*name;
	sem_id		read_sem;
	sem_id		write_sem;
	int32		total_count;	// messages read from port since creation
	select_info	*select_infos;
	struct list	msg_queue;
};


#define MAX_QUEUE_LENGTH 4096
#define PORT_MAX_MESSAGE_SIZE 65536

// sMaxPorts must be power of 2
static int32 sMaxPorts = 4096;
static int32 sUsedPorts = 0;

static struct port_entry *sPorts = NULL;
static area_id sPortArea = 0;
static bool sPortsActive = false;
static port_id sNextPort = 1;
static int32 sFirstFreeSlot = 1;

static spinlock sPortSpinlock = B_SPINLOCK_INITIALIZER;

#define GRAB_PORT_LIST_LOCK() acquire_spinlock(&sPortSpinlock)
#define RELEASE_PORT_LIST_LOCK() release_spinlock(&sPortSpinlock)
#define GRAB_PORT_LOCK(s) acquire_spinlock(&(s).lock)
#define RELEASE_PORT_LOCK(s) release_spinlock(&(s).lock)


static int
dump_port_list(int argc, char **argv)
{
	const char *name = NULL;
	team_id owner = -1;
	int32 i;

	if (argc > 2) {
		if (!strcmp(argv[1], "team") || !strcmp(argv[1], "owner"))
			owner = strtoul(argv[2], NULL, 0);
		else if (!strcmp(argv[1], "name"))
			name = argv[2];
	} else if (argc > 1)
		owner = strtoul(argv[1], NULL, 0);

	kprintf("port           id  cap  r-sem  w-sem   team  name\n");

	for (i = 0; i < sMaxPorts; i++) {
		struct port_entry *port = &sPorts[i];
		if (port->id < 0
			|| (owner != -1 && port->owner != owner)
			|| (name != NULL && strstr(port->name, name) == NULL))
			continue;

		kprintf("%p %6ld %4ld %6ld %6ld %6ld  %s\n", port, port->id,
			port->capacity, port->read_sem, port->write_sem, port->owner,
			port->name);
	}
	return 0;
}


static void
_dump_port_info(struct port_entry *port)
{
	int32 count;

	kprintf("PORT: %p\n", port);
	kprintf(" id:              %ld\n", port->id);
	kprintf(" name:            \"%s\"\n", port->name);
	kprintf(" owner:           %ld\n", port->owner);
	kprintf(" capacity:        %ld\n", port->capacity);
	kprintf(" read_sem:        %ld\n", port->read_sem);
	kprintf(" write_sem:       %ld\n", port->write_sem);
 	get_sem_count(port->read_sem, &count);
 	kprintf(" read_sem count:  %ld\n", count);
 	get_sem_count(port->write_sem, &count);
	kprintf(" write_sem count: %ld\n", count);
	kprintf(" total count:     %ld\n", port->total_count);

	set_debug_variable("_port", (addr_t)port);
	set_debug_variable("_portID", port->id);
	set_debug_variable("_owner", port->owner);
	set_debug_variable("_readSem", port->read_sem);
	set_debug_variable("_writeSem", port->write_sem);
}


static int
dump_port_info(int argc, char **argv)
{
	const char *name = NULL;
	sem_id sem = -1;
	int i;

	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc > 2) {
		if (!strcmp(argv[1], "address")) {
			_dump_port_info((struct port_entry *)strtoul(argv[2], NULL, 0));
			return 0;
		} else if (!strcmp(argv[1], "sem"))
			sem = strtoul(argv[2], NULL, 0);
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
		if ((name != NULL && sPorts[i].name != NULL
				&& !strcmp(name, sPorts[i].name))
			|| (sem != -1 && (sPorts[i].read_sem == sem
				|| sPorts[i].write_sem == sem))) {
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
put_port_msg(port_msg *msg)
{
	cbuf_free_chain(msg->buffer_chain);
	free(msg);
}


static port_msg *
get_port_msg(int32 code, size_t bufferSize)
{
	// ToDo: investigate preallocation of port_msgs (or use a slab allocator)
	cbuf *bufferChain = NULL;

	port_msg *msg = (port_msg *)malloc(sizeof(port_msg));
	if (msg == NULL)
		return NULL;

	if (bufferSize > 0) {
		bufferChain = cbuf_get_chain(bufferSize);
		if (bufferChain == NULL) {
			free(msg);
			return NULL;
		}
	}

	msg->code = code;
	msg->buffer_chain = bufferChain;
	msg->size = bufferSize;
	return msg;
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
fill_port_info(struct port_entry *port, port_info *info, size_t size)
{
	int32 count;

	info->port = port->id;
	info->team = port->owner;
	info->capacity = port->capacity;

	get_sem_count(port->read_sem, &count);
	if (count < 0)
		count = 0;

	info->queue_count = count;
	info->total_count = port->total_count;

	strlcpy(info->name, port->name, B_OS_NAME_LENGTH);
}


//	#pragma mark - private kernel API


/*! This function cycles through the ports table, deleting all
	the ports that are owned by the passed team_id
*/
int
delete_owned_ports(team_id owner)
{
	// ToDo: investigate maintaining a list of ports in the team
	//	to make this simpler and more efficient.
	cpu_status state;
	int i;
	int count = 0;

	TRACE(("delete_owned_ports(owner = %ld)\n", owner));

	if (!sPortsActive)
		return B_BAD_PORT_ID;

	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	for (i = 0; i < sMaxPorts; i++) {
		if (sPorts[i].id != -1 && sPorts[i].owner == owner) {
			port_id id = sPorts[i].id;

			RELEASE_PORT_LIST_LOCK();
			restore_interrupts(state);

			delete_port(id);
			count++;

			state = disable_interrupts();
			GRAB_PORT_LIST_LOCK();
		}
	}

	RELEASE_PORT_LIST_LOCK();
	restore_interrupts(state);

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
	int32 i;

	// create and initialize ports table
	sPortArea = create_area("port_table", (void **)&sPorts, B_ANY_KERNEL_ADDRESS,
		size, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (sPortArea < 0) {
		panic("unable to allocate kernel port table!\n");
		return sPortArea;
	}

	// ToDo: investigate preallocating a list of port_msgs to
	//	speed up actual message sending/receiving, a slab allocator
	//	might do it as well, though :-)

	memset(sPorts, 0, size);
	for (i = 0; i < sMaxPorts; i++)
		sPorts[i].id = -1;

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

	sPortsActive = true;
	return B_OK;
}


//	#pragma mark - public kernel API


port_id		
create_port(int32 queueLength, const char *name)
{
	cpu_status state;
	char nameBuffer[B_OS_NAME_LENGTH];
	sem_id readSem, writeSem;
	status_t status;
	team_id	owner;
	int32 slot;

	TRACE(("create_port(queueLength = %ld, name = \"%s\")\n", queueLength, name));

	if (!sPortsActive)
		return B_BAD_PORT_ID;

	// check queue length
	if (queueLength < 1
		|| queueLength > MAX_QUEUE_LENGTH)
		return B_BAD_VALUE;

	// check early on if there are any free port slots to use
	if (atomic_add(&sUsedPorts, 1) >= sMaxPorts) {
		status = B_NO_MORE_PORTS;
		goto err1;
	}

	// check & dup name
	if (name == NULL)
		name = "unnamed port";

	// ToDo: we could save the memory and use the semaphore name only instead
	strlcpy(nameBuffer, name, B_OS_NAME_LENGTH);
	name = strdup(nameBuffer);
	if (name == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	// create read sem with owner set to -1
	// ToDo: should be B_SYSTEM_TEAM
	readSem = create_sem_etc(0, name, -1);
	if (readSem < B_OK) {
		status = readSem;
		goto err2;
	}

	// create write sem
	writeSem = create_sem_etc(queueLength, name, -1);
	if (writeSem < B_OK) {
		status = writeSem;
		goto err3;
	}

	owner = team_get_current_team_id();

	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	// find the first empty spot
	for (slot = 0; slot < sMaxPorts; slot++) {
		int32 i = (slot + sFirstFreeSlot) % sMaxPorts;

		if (sPorts[i].id == -1) {
			port_id id;

			// make the port_id be a multiple of the slot it's in
			if (i >= sNextPort % sMaxPorts)
				sNextPort += i - sNextPort % sMaxPorts;
			else
				sNextPort += sMaxPorts - (sNextPort % sMaxPorts - i);
			sFirstFreeSlot = slot + 1;

			GRAB_PORT_LOCK(sPorts[i]);
			sPorts[i].id = sNextPort++;
			RELEASE_PORT_LIST_LOCK();

			sPorts[i].capacity = queueLength;
			sPorts[i].owner = owner;
			sPorts[i].name = name;

			sPorts[i].read_sem	= readSem;
			sPorts[i].write_sem	= writeSem;

			list_init(&sPorts[i].msg_queue);
			sPorts[i].total_count = 0;
			sPorts[i].select_infos = NULL;
			id = sPorts[i].id;

			RELEASE_PORT_LOCK(sPorts[i]);
			restore_interrupts(state);

			TRACE(("create_port() done: port created %ld\n", id));

			return id;
		}
	}

	// not enough ports...

	// ToDo: due to sUsedPorts, this cannot happen anymore - as
	//		long as sMaxPorts stays constant over the kernel run
	//		time (which it should be). IOW we could simply panic()
	//		here.

	RELEASE_PORT_LIST_LOCK();
	restore_interrupts(state);

	status = B_NO_MORE_PORTS;

	delete_sem(writeSem);
err3:
	delete_sem(readSem);
err2:
	free((char *)name);
err1:
	atomic_add(&sUsedPorts, -1);

	return status;
}


status_t
close_port(port_id id)
{
	sem_id readSem, writeSem;
	cpu_status state;
	int32 slot;

	TRACE(("close_port(id = %ld)\n", id));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % sMaxPorts;

	// walk through the sem list, trying to match name
	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("close_port: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	// mark port to disable writing - deleting the semaphores will
	// wake up waiting read/writes
	sPorts[slot].capacity = 0;
	readSem = sPorts[slot].read_sem;
	writeSem = sPorts[slot].write_sem;

	notify_port_select_events(slot, B_EVENT_INVALID);
	sPorts[slot].select_infos = NULL;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	delete_sem(readSem);
	delete_sem(writeSem);

	return B_NO_ERROR;
}


status_t
delete_port(port_id id)
{
	cpu_status state;
	sem_id readSem, writeSem;
	const char *name;
	struct list list;
	port_msg *msg;
	int32 slot;

	TRACE(("delete_port(id = %ld)\n", id));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);

		TRACE(("delete_port: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	/* mark port as invalid */
	sPorts[slot].id	= -1;
	name = sPorts[slot].name;
	readSem = sPorts[slot].read_sem;
	writeSem = sPorts[slot].write_sem;
	sPorts[slot].name = NULL;
	list_move_to_list(&sPorts[slot].msg_queue, &list);

	notify_port_select_events(slot, B_EVENT_INVALID);
	sPorts[slot].select_infos = NULL;

	RELEASE_PORT_LOCK(sPorts[slot]);

	// update the first free slot hint in the array	
	GRAB_PORT_LIST_LOCK();
	if (slot < sFirstFreeSlot)
		sFirstFreeSlot = slot;
	RELEASE_PORT_LIST_LOCK();

	restore_interrupts(state);

	atomic_add(&sUsedPorts, -1);

	// free the queue
	while ((msg = (port_msg *)list_remove_head_item(&list)) != NULL) {
		put_port_msg(msg);
	}

	free((char *)name);

	// release the threads that were blocking on this port by deleting the sem
	// read_port() will see the B_BAD_SEM_ID acq_sem() return value, and act accordingly
	delete_sem(readSem);
	delete_sem(writeSem);

	return B_OK;
}


status_t
select_port(int32 id, struct select_info *info, bool kernel)
{
	cpu_status state;
	int32 slot;
	status_t error = B_OK;

	if (id < 0)
		return B_BAD_PORT_ID;

	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id || is_port_closed(slot)) {
		// bad port ID
		error = B_BAD_SEM_ID;
	} else if (!kernel && sPorts[slot].owner == team_get_kernel_team_id()) {
		// kernel port, but call from userland
		error = B_NOT_ALLOWED;
	} else {
		info->selected_events &= B_EVENT_READ | B_EVENT_WRITE | B_EVENT_INVALID;

		if (info->selected_events != 0) {
			uint16 events = 0;
			int32 writeCount = 0;

			info->next = sPorts[slot].select_infos;
			sPorts[slot].select_infos = info;

			// check for events
			if ((info->selected_events & B_EVENT_READ) != 0
				&& !list_is_empty(&sPorts[slot].msg_queue)) {
				events |= B_EVENT_READ;
			}

			if (get_sem_count(sPorts[slot].write_sem, &writeCount) == B_OK
				&& writeCount > 0) {
				events |= B_EVENT_WRITE;
			}

			if (events != 0)
				notify_select_events(info, events);
		}
	}

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	return error;
}


status_t
deselect_port(int32 id, struct select_info *info, bool kernel)
{
	cpu_status state;
	int32 slot;

	if (id < 0)
		return B_BAD_PORT_ID;

	if (info->selected_events == 0)
		return B_OK;

	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id == id) {
		select_info** infoLocation = &sPorts[slot].select_infos;
		while (*infoLocation != NULL && *infoLocation != info)
			infoLocation = &(*infoLocation)->next;

		if (*infoLocation == info)
			*infoLocation = info->next;
	}

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	return B_OK;
}


port_id
find_port(const char *name)
{
	port_id portFound = B_NAME_NOT_FOUND;
	cpu_status state;
	int32 i;

	TRACE(("find_port(name = \"%s\")\n", name));

	if (!sPortsActive)
		return B_NAME_NOT_FOUND;
	if (name == NULL)
		return B_BAD_VALUE;

	// Since we have to check every single port, and we don't
	// care if it goes away at any point, we're only grabbing
	// the port lock in question, not the port list lock

	// loop over list
	for (i = 0; i < sMaxPorts && portFound < B_OK; i++) {
		// lock every individual port before comparing
		state = disable_interrupts();
		GRAB_PORT_LOCK(sPorts[i]);

		if (sPorts[i].id >= 0 && !strcmp(name, sPorts[i].name))
			portFound = sPorts[i].id;

		RELEASE_PORT_LOCK(sPorts[i]);
		restore_interrupts(state);
	}

	return portFound;
}


status_t
_get_port_info(port_id id, port_info *info, size_t size)
{
	cpu_status state;
	int slot;

	TRACE(("get_port_info(id = %ld)\n", id));

	if (info == NULL || size != sizeof(port_info))
		return B_BAD_VALUE;
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id || sPorts[slot].capacity == 0) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("get_port_info: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	// fill a port_info struct with info
	fill_port_info(&sPorts[slot], info, size);

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	return B_OK;
}


status_t
_get_next_port_info(team_id team, int32 *_cookie, struct port_info *info, size_t size)
{
	cpu_status state;
	int slot;

	TRACE(("get_next_port_info(team = %ld)\n", team));

	if (info == NULL || size != sizeof(port_info) || _cookie == NULL || team < B_OK)
		return B_BAD_VALUE;
	if (!sPortsActive)
		return B_BAD_PORT_ID;

	slot = *_cookie;
	if (slot >= sMaxPorts)
		return B_BAD_PORT_ID;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();

	info->port = -1; // used as found flag

	// spinlock
	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	while (slot < sMaxPorts) {
		GRAB_PORT_LOCK(sPorts[slot]);
		if (sPorts[slot].id != -1 && sPorts[slot].capacity != 0 && sPorts[slot].owner == team) {
			// found one!
			fill_port_info(&sPorts[slot], info, size);

			RELEASE_PORT_LOCK(sPorts[slot]);
			slot++;
			break;
		}
		RELEASE_PORT_LOCK(sPorts[slot]);
		slot++;
	}
	RELEASE_PORT_LIST_LOCK();
	restore_interrupts(state);

	if (info->port == -1)
		return B_BAD_PORT_ID;

	*_cookie = slot;
	return B_NO_ERROR;
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
_get_port_message_info_etc(port_id id, port_message_info *info,
	size_t infoSize, uint32 flags, bigtime_t timeout)
{
	if (info == NULL || infoSize != sizeof(port_message_info))
		return B_BAD_VALUE;

	cpu_status state;
	sem_id cachedSem;
	status_t status;
	port_msg *msg;
	int32 slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id
		|| (is_port_closed(slot) && list_is_empty(&sPorts[slot].msg_queue))) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("port_buffer_size_etc(): %s port %ld\n",
			sPorts[slot].id == id ? "closed" : "invalid", id));
		return B_BAD_PORT_ID;
	}

	cachedSem = sPorts[slot].read_sem;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// block if no message, or, if B_TIMEOUT flag set, block with timeout

	status = acquire_sem_etc(cachedSem, 1, flags, timeout);
	if (status != B_OK && status != B_BAD_SEM_ID)
		return status;

	// in case of B_BAD_SEM_ID, the port might have been closed but not yet
	// deleted, ie. there could still be messages waiting for us

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		// the port is no longer there
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		return B_BAD_PORT_ID;
	}

	// determine tail & get the length of the message
	status_t error = B_OK;
	msg = (port_msg*)list_get_first_item(&sPorts[slot].msg_queue);
	if (msg == NULL) {
		if (status == B_OK)
			panic("port %ld: no messages found\n", sPorts[slot].id);

		error = B_BAD_PORT_ID;
	} else {
		info->size = msg->size;
		info->sender = msg->sender;
		info->sender_group = msg->sender_group;
		info->sender_team = msg->sender_team;
	}

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// restore read_sem, as we haven't read from the port
	release_sem(cachedSem);

	// return length of item at end of queue
	return error;
}


ssize_t
port_count(port_id id)
{
	cpu_status state;
	int32 count = 0;
	int32 slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("port_count: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	if (get_sem_count(sPorts[slot].read_sem, &count) == B_OK) {
		// do not return negative numbers
		if (count < 0)
			count = 0;
	} else {
		// the port might have been closed - we need to actually count the messages
		void *message = NULL;
		while ((message = list_get_next_item(&sPorts[slot].msg_queue, message)) != NULL) {
			count++;
		}
	}

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// return count of messages
	return count;
}


ssize_t
read_port(port_id port, int32 *msgCode, void *msgBuffer, size_t bufferSize)
{
	return read_port_etc(port, msgCode, msgBuffer, bufferSize, 0, 0);
}


ssize_t
read_port_etc(port_id id, int32 *_msgCode, void *msgBuffer, size_t bufferSize,
	uint32 flags, bigtime_t timeout)
{
	cpu_status state;
	sem_id cachedSem;
	status_t status;
	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) > 0;
	port_msg *msg;
	size_t size;
	int slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	if ((msgBuffer == NULL && bufferSize > 0)
		|| timeout < 0)
		return B_BAD_VALUE;

	flags = flags & (B_CAN_INTERRUPT | B_KILL_CAN_INTERRUPT
				| B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT);
	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id
		|| (is_port_closed(slot) && list_is_empty(&sPorts[slot].msg_queue))) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("read_port_etc(): %s port %ld\n",
			sPorts[slot].id == id ? "closed" : "invalid", id));
		return B_BAD_PORT_ID;
	}
	// store sem_id in local variable
	cachedSem = sPorts[slot].read_sem;

	// unlock port && enable ints/
	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	status = acquire_sem_etc(cachedSem, 1, flags, timeout);
		// get 1 entry from the queue, block if needed

	if (status != B_OK && status != B_BAD_SEM_ID)
		return status;

	// in case of B_BAD_SEM_ID, the port might have been closed but not yet
	// deleted, ie. there could still be messages waiting for us

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	// first, let's check if the port is still alive
	if (sPorts[slot].id == -1) {
		// the port has been deleted in the meantime
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		return B_BAD_PORT_ID;
	}

	msg = (port_msg*)list_get_first_item(&sPorts[slot].msg_queue);
	if (msg == NULL) {
		if (status == B_OK)
			panic("port %ld: no messages found", sPorts[slot].id);

		// the port has obviously been closed, but no messages are left anymore
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		return B_BAD_PORT_ID;
	}

	list_remove_link(msg);

	sPorts[slot].total_count++;

	notify_port_select_events(slot, B_EVENT_WRITE);

	cachedSem = sPorts[slot].write_sem;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// check output buffer size
	size = min_c(bufferSize, msg->size);

	// copy message
	if (_msgCode != NULL)
		*_msgCode = msg->code;
	if (size > 0) {
		if (userCopy) {
			if ((status = cbuf_user_memcpy_from_chain(msgBuffer, msg->buffer_chain, 0, size) < B_OK)) {
				// leave the port intact, for other threads that might not crash
				put_port_msg(msg);
				release_sem(cachedSem);
				return status;
			}
		} else
			cbuf_memcpy_from_chain(msgBuffer, msg->buffer_chain, 0, size);
	}
	put_port_msg(msg);

	// make one spot in queue available again for write
	release_sem(cachedSem);
		// ToDo: we might think about setting B_NO_RESCHEDULE here
		//	from time to time (always?)

	return size;
}


status_t
write_port(port_id id, int32 msgCode, const void *msgBuffer, size_t bufferSize)
{
	iovec vec = { (void *)msgBuffer, bufferSize };

	return writev_port_etc(id, msgCode, &vec, 1, bufferSize, 0, 0);
}


status_t
write_port_etc(port_id id, int32 msgCode, const void *msgBuffer,
	size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	iovec vec = { (void *)msgBuffer, bufferSize };

	return writev_port_etc(id, msgCode, &vec, 1, bufferSize, flags, timeout);
}


status_t
writev_port_etc(port_id id, int32 msgCode, const iovec *msgVecs,
	size_t vecCount, size_t bufferSize, uint32 flags,
	bigtime_t timeout)
{
	cpu_status state;
	sem_id cachedSem;
	status_t status;
	port_msg *msg;
	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) > 0;
	int slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	// mask irrelevant flags (for acquire_sem() usage)
	flags = flags & (B_CAN_INTERRUPT | B_KILL_CAN_INTERRUPT
				| B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT);
	slot = id % sMaxPorts;

	if (bufferSize > PORT_MAX_MESSAGE_SIZE)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("write_port_etc: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	if (is_port_closed(slot)) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("write_port_etc: port %ld closed\n", id));
		return B_BAD_PORT_ID;
	}

	// store sem_id in local variable 
	cachedSem = sPorts[slot].write_sem;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	status = acquire_sem_etc(cachedSem, 1, flags, timeout);
		// get 1 entry from the queue, block if needed

	if (status == B_BAD_SEM_ID) {
		// somebody deleted or closed the port
		return B_BAD_PORT_ID;
	}
	if (status != B_OK)
		return status;

	msg = get_port_msg(msgCode, bufferSize);
	if (msg == NULL)
		return B_NO_MEMORY;

	// sender credentials
	msg->sender = geteuid();
	msg->sender_group = getegid();
	msg->sender_team = team_get_current_team_id();

	if (bufferSize > 0) {
		uint32 i;
		if (userCopy) {
			// copy from user memory
			for (i = 0; i < vecCount; i++) {
				size_t bytes = msgVecs[i].iov_len;
				if (bytes > bufferSize)
					bytes = bufferSize;

				if ((status = cbuf_user_memcpy_to_chain(msg->buffer_chain,
						0, msgVecs[i].iov_base, bytes)) < B_OK) {
					put_port_msg(msg);
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

				if ((status = cbuf_memcpy_to_chain(msg->buffer_chain,
						0, msgVecs[i].iov_base, bytes)) < 0) {
					put_port_msg(msg);
					return status;
				}

				bufferSize -= bytes;
				if (bufferSize == 0)
					break;
			}
		}
	}

	// attach message to queue
	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	// first, let's check if the port is still alive
	if (sPorts[slot].id == -1) {
		// the port has been deleted in the meantime
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);

		put_port_msg(msg);
		return B_BAD_PORT_ID;
	}

	list_add_item(&sPorts[slot].msg_queue, msg);

	notify_port_select_events(slot, B_EVENT_READ);

	// store sem_id in local variable 
	cachedSem = sPorts[slot].read_sem;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// release sem, allowing read (might reschedule)
	release_sem(cachedSem);

	return B_NO_ERROR;
}


status_t
set_port_owner(port_id id, team_id team)
{
	cpu_status state;
	int slot;
// ToDo: Shouldn't we at least check, whether the team exists?

	TRACE(("set_port_owner(id = %ld, team = %ld)\n", id, team));

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % sMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("set_port_owner: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	// transfer ownership to other team
	sPorts[slot].owner = team;

	// unlock port
	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
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
