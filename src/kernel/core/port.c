/* ports for IPC */

/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001, Mark-Jan Bastian. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>

#include <port.h>
#include <sem.h>
#include <team.h>
#include <util/list.h>
#include <arch/int.h>
#include <cbuf.h>

#include <string.h>
#include <stdlib.h>


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
} port_msg;

struct port_entry {
	port_id 	id;
	team_id 	owner;
	int32 		capacity;
	spinlock	lock;
	const char	*name;
	sem_id		read_sem;
	sem_id		write_sem;
	int32		total_count;
	struct list	msg_queue;
};

// internal API
static int dump_port_list(int argc, char **argv);
static int dump_port_info(int argc, char **argv);
static void _dump_port_info(struct port_entry *port);


// gMaxPorts must be power of 2
int32 gMaxPorts = 4096;

#define MAX_QUEUE_LENGTH 4096
#define PORT_MAX_MESSAGE_SIZE 65536

static struct port_entry *sPorts = NULL;
static area_id sPortArea = 0;
static bool sPortsActive = false;
static port_id sNextPort = 0;

static spinlock sPortSpinlock = 0;

#define GRAB_PORT_LIST_LOCK() acquire_spinlock(&sPortSpinlock)
#define RELEASE_PORT_LIST_LOCK() release_spinlock(&sPortSpinlock)
#define GRAB_PORT_LOCK(s) acquire_spinlock(&(s).lock)
#define RELEASE_PORT_LOCK(s) release_spinlock(&(s).lock)


status_t
port_init(kernel_args *ka)
{
	int i;
	int size = sizeof(struct port_entry) * gMaxPorts;

	// create and initialize ports table
	sPortArea = create_area("port_table", (void **)&sPorts, B_ANY_KERNEL_ADDRESS,
		size, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (sPortArea < 0) {
		panic("unable to allocate kernel port table!\n");
	}

	// ToDo: investigate preallocating a list of port_msgs to
	//	speed up actual message sending/receiving, a slab allocator
	//	might do it as well, though :-)

	memset(sPorts, 0, size);
	for (i = 0; i < gMaxPorts; i++)
		sPorts[i].id = -1;

	// add debugger commands
	add_debugger_command("ports", &dump_port_list, "Dump a list of all active ports");
	add_debugger_command("port", &dump_port_info, "Dump info about a particular port");

	sPortsActive = true;

	return 0;
}


#ifdef DEBUG
// ToDo: the test code does not belong here!
/*
 * testcode
 */

static int32 port_test_thread_func(void *arg);

port_id test_p1, test_p2, test_p3, test_p4;

void
port_test()
{
	char testdata[5];
	thread_id t;
	int res;
	int32 dummy;
	int32 dummy2;

	strcpy(testdata, "abcd");

	dprintf("porttest: create_port()\n");
	test_p1 = create_port(1,    "test port #1");
	test_p2 = create_port(10,   "test port #2");
	test_p3 = create_port(1024, "test port #3");
	test_p4 = create_port(1024, "test port #4");

	dprintf("porttest: find_port()\n");
	dprintf("'test port #1' has id %ld (should be %ld)\n", find_port("test port #1"), test_p1);

	dprintf("porttest: write_port() on 1, 2 and 3\n");
	write_port(test_p1, 1, &testdata, sizeof(testdata));
	write_port(test_p2, 666, &testdata, sizeof(testdata));
	write_port(test_p3, 999, &testdata, sizeof(testdata));
	dprintf("porttest: port_count(test_p1) = %ld\n", port_count(test_p1));

	dprintf("porttest: write_port() on 1 with timeout of 1 sec (blocks 1 sec)\n");
	write_port_etc(test_p1, 1, &testdata, sizeof(testdata), B_TIMEOUT, 1000000);
	dprintf("porttest: write_port() on 2 with timeout of 1 sec (wont block)\n");
	res = write_port_etc(test_p2, 777, &testdata, sizeof(testdata), B_TIMEOUT, 1000000);
	dprintf("porttest: res=%d, %s\n", res, res == 0 ? "ok" : "BAD");

	dprintf("porttest: read_port() on empty port 4 with timeout of 1 sec (blocks 1 sec)\n");
	res = read_port_etc(test_p4, &dummy, &dummy2, sizeof(dummy2), B_TIMEOUT, 1000000);
	dprintf("porttest: res=%d, %s\n", res, res == B_TIMED_OUT ? "ok" : "BAD");

	dprintf("porttest: spawning thread for port 1\n");
	t = spawn_kernel_thread(port_test_thread_func, "port_test", B_NORMAL_PRIORITY, NULL);
	resume_thread(t);

	dprintf("porttest: write\n");
	write_port(test_p1, 1, &testdata, sizeof(testdata));

	// now we can write more (no blocking)
	dprintf("porttest: write #2\n");
	write_port(test_p1, 2, &testdata, sizeof(testdata));
	dprintf("porttest: write #3\n");
	write_port(test_p1, 3, &testdata, sizeof(testdata));

	dprintf("porttest: waiting on spawned thread\n");
	wait_for_thread(t, NULL);

	dprintf("porttest: close p1\n");
	close_port(test_p2);
	dprintf("porttest: attempt write p1 after close\n");
	res = write_port(test_p2, 4, &testdata, sizeof(testdata));
	dprintf("porttest: write_port ret %d\n", res);

	dprintf("porttest: testing delete p2\n");
	delete_port(test_p2);

	dprintf("porttest: end test main thread\n");
	
}


static int32
port_test_thread_func(void *arg)
{
	int32 msg_code;
	int n;
	char buf[6];
	buf[5] = '\0';

	dprintf("porttest: port_test_thread_func()\n");
	
	n = read_port(test_p1, &msg_code, &buf, 3);
	dprintf("read_port #1 code %ld len %d buf %s\n", msg_code, n, buf);
	n = read_port(test_p1, &msg_code, &buf, 4);
	dprintf("read_port #1 code %ld len %d buf %s\n", msg_code, n, buf);
	buf[4] = 'X';
	n = read_port(test_p1, &msg_code, &buf, 5);
	dprintf("read_port #1 code %ld len %d buf %s\n", msg_code, n, buf);

	dprintf("porttest: testing delete p1 from other thread\n");
	delete_port(test_p1);
	dprintf("porttest: end port_test_thread_func()\n");
	
	return 0;
}
#endif	/* DEBUG */


int
dump_port_list(int argc, char **argv)
{
	int i;

	for (i = 0; i < gMaxPorts; i++) {
		if (sPorts[i].id >= 0)
			dprintf("%p\tid: 0x%lx\t\tname: '%s'\n", &sPorts[i], sPorts[i].id, sPorts[i].name);
	}
	return 0;
}


static void
_dump_port_info(struct port_entry *port)
{
	int32 cnt;
	dprintf("PORT:   %p\n", port);
	dprintf("name:  '%s'\n", port->name);
	dprintf("owner: 0x%lx\n", port->owner);
	dprintf("capacity: %ld\n", port->capacity);
 	get_sem_count(port->read_sem, &cnt);
 	dprintf("read_sem:  %ld\n", cnt);
 	get_sem_count(port->write_sem, &cnt);
	dprintf("write_sem: %ld\n", cnt);
}


static int
dump_port_info(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		dprintf("port: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a hex number, treat it as such
	if (strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		unsigned long num = strtoul(argv[1], NULL, 16);

		if (num > KERNEL_BASE && num <= (KERNEL_BASE + (KERNEL_SIZE - 1))) {
			// XXX semi-hack
			// one can use either address or a port_id, since KERNEL_BASE > gMaxPorts assumed
			_dump_port_info((struct port_entry *)num);
			return 0;
		} else {
			unsigned slot = num % gMaxPorts;
			if(sPorts[slot].id != (int)num) {
				dprintf("port 0x%lx doesn't exist!\n", num);
				return 0;
			}
			_dump_port_info(&sPorts[slot]);
			return 0;
		}
	}

	// walk through the ports list, trying to match name
	for (i = 0; i < gMaxPorts; i++) {
		if (sPorts[i].name != NULL
			&& strcmp(argv[1], sPorts[i].name) == 0) {
			_dump_port_info(&sPorts[i]);
			return 0;
		}
	}
	return 0;
}


/** this function cycles through the ports table, deleting all
 *	the ports that are owned by the passed team_id
 */

int
delete_owned_ports(team_id owner)
{
	// ToDo: investigate maintaining a list of ports in the team
	//	to make this simpler and more efficient.
	cpu_status state;
	int i;
	int count = 0;

	if (!sPortsActive)
		return B_BAD_PORT_ID;

	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	for (i = 0; i < gMaxPorts; i++) {
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


//	#pragma mark -
// public kernel API


port_id		
create_port(int32 queueLength, const char *name)
{
	cpu_status state;
	char nameBuffer[B_OS_NAME_LENGTH];
	sem_id readSem, writeSem;
	port_id returnValue;
	team_id	owner;
	int i;

	if (!sPortsActive)
		return B_BAD_PORT_ID;

	// check queue length
	if (queueLength < 1
		|| queueLength > MAX_QUEUE_LENGTH)
		return B_BAD_VALUE;

	// check & dup name
	if (name == NULL)
		name = "unnamed port";

	// ToDo: we could save the memory and use the semaphore name only instead
	strlcpy(nameBuffer, name, B_OS_NAME_LENGTH);
	name = strdup(nameBuffer);
	if (name == NULL)
		return B_NO_MEMORY;

	// create read sem with owner set to -1
	// ToDo: should be B_SYSTEM_TEAM
	readSem = create_sem_etc(0, name, -1);
	if (readSem < B_OK) {
		// cleanup
		free((char *)name);
		return readSem;
	}

	// create write sem
	writeSem = create_sem_etc(queueLength, name, -1);
	if (writeSem < 0) {
		// cleanup
		delete_sem(readSem);
		free((char *)name);
		return writeSem;
	}

	owner = team_get_current_team_id();

	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	// find the first empty spot
	for (i = 0; i < gMaxPorts; i++) {
		if (sPorts[i].id == -1) {
			// make the port_id be a multiple of the slot it's in
			if (i >= sNextPort % gMaxPorts)
				sNextPort += i - sNextPort % gMaxPorts;
			else
				sNextPort += gMaxPorts - (sNextPort % gMaxPorts - i);

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
			returnValue = sPorts[i].id;

			RELEASE_PORT_LOCK(sPorts[i]);
			goto out;
		}
	}

	// not enough ports...
	RELEASE_PORT_LIST_LOCK();
	returnValue = B_NO_MORE_PORTS;
	dprintf("create_port(): B_NO_MORE_PORTS\n");

	// cleanup
	delete_sem(writeSem);
	delete_sem(readSem);
	free((char *)name);

out:
	restore_interrupts(state);

	return returnValue;
}


status_t
close_port(port_id id)
{
	cpu_status state;
	int slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % gMaxPorts;

	// walk through the sem list, trying to match name
	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		dprintf("close_port: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}

	// mark port to disable writing
	sPorts[slot].capacity = 0;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

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
	int slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % gMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		dprintf("delete_port: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}

	/* mark port as invalid */
	sPorts[slot].id	= -1;
	name = sPorts[slot].name;
	readSem = sPorts[slot].read_sem;
	writeSem = sPorts[slot].write_sem;
	sPorts[slot].name = NULL;
	list_move_to_list(&sPorts[slot].msg_queue, &list);

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

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


port_id
find_port(const char *name)
{
	port_id portFound = B_NAME_NOT_FOUND;
	cpu_status state;
	int i;

	if (!sPortsActive)
		return B_NAME_NOT_FOUND;
	if (name == NULL)
		return B_BAD_VALUE;

	// Since we have to check every single port, and we don't
	// care if it goes away at any point, we're only grabbing
	// the port lock in question, not the port list lock

	// loop over list
	for (i = 0; i < gMaxPorts && portFound < B_OK; i++) {
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


/** Fills the port_info structure with information from the specified
 *	port.
 *	The port lock must be held when called.
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


status_t
_get_port_info(port_id id, port_info *info, size_t size)
{
	cpu_status state;
	int slot;

	if (info == NULL || size != sizeof(port_info))
		return B_BAD_VALUE;
	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % gMaxPorts;

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

	if (info == NULL || size != sizeof(port_info) || _cookie == NULL || team < B_OK)
		return B_BAD_VALUE;
	if (!sPortsActive)
		return B_BAD_PORT_ID;

	slot = *_cookie;
	if (slot >= gMaxPorts)
		return B_BAD_PORT_ID;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();

	info->port = -1; // used as found flag

	// spinlock
	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	while (slot < gMaxPorts) {
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
	cpu_status state;
	sem_id cachedSem;
	status_t status;
	port_msg *msg;
	ssize_t size;
	int slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % gMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("get_buffer_size_etc: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}
	
	cachedSem = sPorts[slot].read_sem;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// block if no message, or, if B_TIMEOUT flag set, block with timeout

	status = acquire_sem_etc(cachedSem, 1, flags, timeout);

	if (status == B_BAD_SEM_ID) {
		// somebody deleted the port
		return B_BAD_PORT_ID;
	}
	if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
		return status;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		// the port is no longer there
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		return B_BAD_PORT_ID;
	}

	// determine tail & get the length of the message
	msg = list_get_first_item(&sPorts[slot].msg_queue);
	if (msg == NULL)
		panic("port %ld: no messages found", sPorts[slot].id);

	size = msg->size;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// restore read_sem, as we haven't read from the port
	release_sem(cachedSem);

	// return length of item at end of queue
	return size;
}


ssize_t
port_count(port_id id)
{
	cpu_status state;
	int32 count;
	int slot;

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % gMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("port_count: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	get_sem_count(sPorts[slot].read_sem, &count);
	// do not return negative numbers
	if (count < 0)
		count = 0;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// return count of messages (sem_count)
	return count;
}


status_t
read_port(port_id port, int32 *msgCode, void *msgBuffer, size_t bufferSize)
{
	return read_port_etc(port, msgCode, msgBuffer, bufferSize, 0, 0);
}


status_t
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

	if (_msgCode == NULL
		|| (msgBuffer == NULL && bufferSize > 0)
		|| timeout < 0)
		return B_BAD_VALUE;

	flags = flags & (B_CAN_INTERRUPT | B_TIMEOUT);
	slot = id % gMaxPorts;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		dprintf("read_port_etc: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}
	// store sem_id in local variable
	cachedSem = sPorts[slot].read_sem;

	// unlock port && enable ints/
	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	status = acquire_sem_etc(cachedSem, 1, flags, timeout);
		// get 1 entry from the queue, block if needed

	if (status == B_BAD_SEM_ID || status == B_INTERRUPTED) {
		/* somebody deleted the port or the sem went away */
		return B_BAD_PORT_ID;
	}

	if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
		return status;

	if (status != B_NO_ERROR) {
		dprintf("write_port_etc: unknown error %ld\n", status);
		return status;
	}

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	// first, let's check if the port is still alive
	if (sPorts[slot].id == -1) {
		// the port has been deleted in the meantime
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		return B_BAD_PORT_ID;
	}

	msg = list_get_first_item(&sPorts[slot].msg_queue);
	if (msg == NULL)
		panic("port %ld: no messages found", sPorts[slot].id);

	list_remove_link(msg);

	sPorts[slot].total_count++;

	cachedSem = sPorts[slot].write_sem;

	RELEASE_PORT_LOCK(sPorts[slot]);
	restore_interrupts(state);

	// check output buffer size
	size = min(bufferSize, msg->size);

	// copy message
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
	return write_port_etc(id, msgCode, msgBuffer, bufferSize, 0, 0);
}


status_t
write_port_etc(port_id id, int32 msgCode, const void *msgBuffer,
	size_t bufferSize, uint32 flags, bigtime_t timeout)
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
	flags = flags & (B_CAN_INTERRUPT | B_TIMEOUT);
	slot = id % gMaxPorts;

	if (bufferSize > PORT_MAX_MESSAGE_SIZE)
		return EINVAL;

	state = disable_interrupts();
	GRAB_PORT_LOCK(sPorts[slot]);

	if (sPorts[slot].id != id) {
		RELEASE_PORT_LOCK(sPorts[slot]);
		restore_interrupts(state);
		TRACE(("write_port_etc: invalid port_id %ld\n", id));
		return B_BAD_PORT_ID;
	}

	if (sPorts[slot].capacity == 0) {
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

	if (status == B_BAD_SEM_ID || status == B_INTERRUPTED) {
		/* somebody deleted the port or the sem while we were waiting */
		return B_BAD_PORT_ID;
	}

	if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
		return status;

	if (status != B_NO_ERROR) {
		dprintf("write_port_etc: unknown error %ld\n", status);
		return status;
	}

	msg = get_port_msg(msgCode, bufferSize);
	if (msg == NULL)
		return B_NO_MEMORY;

	if (bufferSize > 0) {
		if (userCopy) {
			// copy from user memory
			if ((status = cbuf_user_memcpy_to_chain(msg->buffer_chain, 0, msgBuffer, bufferSize)) < B_OK)
				return status;
		} else {
			// copy from kernel memory
			if ((status = cbuf_memcpy_to_chain(msg->buffer_chain, 0, msgBuffer, bufferSize)) < 0)
				return status;
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

	if (!sPortsActive || id < 0)
		return B_BAD_PORT_ID;

	slot = id % gMaxPorts;

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


//	#pragma mark -
/* 
 *	user level ports
 */


port_id
user_create_port(int32 queueLength, const char *userName)
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
user_close_port(port_id id)
{
	return close_port(id);
}


status_t
user_delete_port(port_id id)
{
	return delete_port(id);
}


port_id
user_find_port(const char *userName)
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
user_get_port_info(port_id id, struct port_info *userInfo)
{
	struct port_info info;
	status_t status;

	if (userInfo == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = get_port_info(id, &info);

	// copy back to user space
	if (status == B_OK && user_memcpy(userInfo, &info, sizeof(struct port_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
user_get_next_port_info(team_id team, int32 *userCookie, struct port_info *userInfo)
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
		|| (status == B_OK && user_memcpy(userInfo, &info, sizeof(struct port_info)) < B_OK))
		return B_BAD_ADDRESS;

	return status;
}


ssize_t
user_port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout)
{
	return port_buffer_size_etc(port, flags | B_CAN_INTERRUPT, timeout);
}


ssize_t
user_port_count(port_id port)
{
	return port_count(port);
}


status_t
user_read_port_etc(port_id port, int32 *userCode, void *userBuffer,
	size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	int32 messageCode;
	ssize_t	status;

	if (userCode == NULL || userBuffer == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userCode) || !IS_USER_ADDRESS(userBuffer))
		return B_BAD_ADDRESS;

	status = read_port_etc(port, &messageCode, userBuffer, bufferSize,
				flags | PORT_FLAG_USE_USER_MEMCPY | B_CAN_INTERRUPT, timeout);

	if (status == B_OK && user_memcpy(userCode, &messageCode, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
user_set_port_owner(port_id port, team_id team)
{
	return set_port_owner(port, team);
}


status_t
user_write_port_etc(port_id port, int32 messageCode, void *userBuffer,
	size_t bufferSize, uint32 flags, bigtime_t timeout)
{
	if (userBuffer == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userBuffer))
		return B_BAD_ADDRESS;

	return write_port_etc(port, messageCode, userBuffer, bufferSize, 
				flags | PORT_FLAG_USE_USER_MEMCPY | B_CAN_INTERRUPT, timeout);
}

