/* ports for IPC */
/*
** Copyright 2001, Mark-Jan Bastian. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>
#include <port.h>
#include <sem.h>
#include <kernel.h>
#include <arch/int.h>
#include <debug.h>
#include <malloc.h>
#include <cbuf.h>
#include <Errors.h>
#include <int.h>
#include <kerrors.h>
#include <string.h>
#include <stdlib.h>


struct port_msg {
	int		msg_code;
	cbuf	*data_cbuf;
	size_t	data_len;
};

struct port_entry {
	port_id 			id;
	team_id 			owner;
	int32 				capacity;
	spinlock			lock;
	char				*name;
	sem_id				read_sem;
	sem_id				write_sem;
	int					head;
	int					tail;
	int					total_count;
	bool				closed;
	struct port_msg*	msg_queue;
};

// internal API
static int dump_port_list(int argc, char **argv);
static int dump_port_info(int argc, char **argv);
static void _dump_port_info(struct port_entry *port);


// MAX_PORTS must be power of 2
#define MAX_PORTS 4096
#define MAX_QUEUE_LENGTH 4096
#define PORT_MAX_MESSAGE_SIZE 65536

static struct port_entry *ports = NULL;
static region_id port_region = 0;
static bool ports_active = false;

static port_id next_port = 0;

static spinlock port_spinlock = 0;
#define GRAB_PORT_LIST_LOCK() acquire_spinlock(&port_spinlock)
#define RELEASE_PORT_LIST_LOCK() release_spinlock(&port_spinlock)
#define GRAB_PORT_LOCK(s) acquire_spinlock(&(s).lock)
#define RELEASE_PORT_LOCK(s) release_spinlock(&(s).lock)


status_t
port_init(kernel_args *ka)
{
	int i;
	int size = sizeof(struct port_entry) * MAX_PORTS;

	// create and initialize semaphore table
	port_region = create_area("port_table", (void **)&ports, B_ANY_KERNEL_ADDRESS,
		size, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (port_region < 0) {
		panic("unable to allocate kernel port table!\n");
	}

	memset(ports, 0, size);
	for (i = 0; i < MAX_PORTS; i++)
		ports[i].id = -1;

	// add debugger commands
	add_debugger_command("ports", &dump_port_list, "Dump a list of all active ports");
	add_debugger_command("port", &dump_port_info, "Dump info about a particular port");

	ports_active = true;

	return 0;
}


int
dump_port_list(int argc, char **argv)
{
	int i;

	for (i = 0; i < MAX_PORTS; i++) {
		if (ports[i].id >= 0)
			dprintf("%p\tid: 0x%lx\t\tname: '%s'\n", &ports[i], ports[i].id, ports[i].name);
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
	dprintf("cap:  %ld\n", port->capacity);
	dprintf("head: %d\n", port->head);
	dprintf("tail: %d\n", port->tail);
 	get_sem_count(port->read_sem, &cnt);
 	dprintf("read_sem:  %ld\n", cnt);
 	get_sem_count(port->read_sem, &cnt);
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
		unsigned long num = atoul(argv[1]);

		if (num > KERNEL_BASE && num <= (KERNEL_BASE + (KERNEL_SIZE - 1))) {
			// XXX semi-hack
			// one can use either address or a port_id, since KERNEL_BASE > MAX_PORTS assumed
			_dump_port_info((struct port_entry *)num);
			return 0;
		} else {
			unsigned slot = num % MAX_PORTS;
			if(ports[slot].id != (int)num) {
				dprintf("port 0x%lx doesn't exist!\n", num);
				return 0;
			}
			_dump_port_info(&ports[slot]);
			return 0;
		}
	}

	// walk through the ports list, trying to match name
	for (i = 0; i < MAX_PORTS; i++) {
		if (ports[i].name != NULL
			&& strcmp(argv[1], ports[i].name) == 0) {
			_dump_port_info(&ports[i]);
			return 0;
		}
	}
	return 0;
}


port_id		
create_port(int32 queue_length, const char *name)
{
	int 	i;
	int 	state;
	sem_id 	sem_r, sem_w;
	port_id retval;
	int		name_len;
	char 	*temp_name;
	struct port_msg *q;
	team_id	owner;
	
	if (ports_active == false)
		return B_BAD_PORT_ID;

	// check queue length
	if (queue_length < 1)
		return EINVAL;
	if (queue_length > MAX_QUEUE_LENGTH)
		return EINVAL;

	// check & dup name
	if (name == NULL)
		name = "unnamed port";

	name_len = strlen(name) + 1;
	name_len = min(name_len, SYS_MAX_OS_NAME_LEN);
	temp_name = (char *)malloc(name_len);
	if (temp_name == NULL)
		return ENOMEM;
	strlcpy(temp_name, name, name_len);

	// alloc queue
	q = (struct port_msg *)malloc(queue_length * sizeof(struct port_msg));
	if (q == NULL) {
		free(temp_name); // dealloc name, too
		return ENOMEM;
	}

	// init cbuf list of the queue
	for (i = 0; i < queue_length; i++)
		q[i].data_cbuf = 0;

	// create sem_r with owner set to -1
	sem_r = create_sem_etc(0, temp_name, -1);
	if (sem_r < 0) {
		// cleanup
		free(temp_name);
		free(q);
		return sem_r;
	}

	// create sem_w
	sem_w = create_sem_etc(queue_length, temp_name, -1);
	if (sem_w < 0) {
		// cleanup
		delete_sem(sem_r);
		free(temp_name);
		free(q);
		return sem_w;
	}
	owner = team_get_current_team_id();

	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	// find the first empty spot
	for (i = 0; i < MAX_PORTS; i++) {
		if(ports[i].id == -1) {
			// make the port_id be a multiple of the slot it's in
			if(i >= next_port % MAX_PORTS) {
				next_port += i - next_port % MAX_PORTS;
			} else {
				next_port += MAX_PORTS - (next_port % MAX_PORTS - i);
			}
			ports[i].id		= next_port++;
			ports[i].lock	= 0;
			GRAB_PORT_LOCK(ports[i]);
			RELEASE_PORT_LIST_LOCK();

			ports[i].capacity	= queue_length;
			ports[i].name 		= temp_name;

			// assign sem
			ports[i].read_sem	= sem_r;
			ports[i].write_sem	= sem_w;
			ports[i].msg_queue	= q;
			ports[i].head 		= 0;
			ports[i].tail 		= 0;
			ports[i].total_count= 0;
			ports[i].owner 		= owner;
			retval = ports[i].id;
			RELEASE_PORT_LOCK(ports[i]);
			goto out;
		}
	}
	// not enough ports...
	RELEASE_PORT_LIST_LOCK();
	retval = B_NO_MORE_PORTS;
	dprintf("create_port(): B_NO_MORE_PORTS\n");

	// cleanup
	delete_sem(sem_w);
	delete_sem(sem_r);
	free(temp_name);
	free(q);

out:
	restore_interrupts(state);

	return retval;
}


status_t
close_port(port_id id)
{
	int 	state;
	int		slot;

	if (ports_active == false)
		return B_BAD_PORT_ID;
	if (id < 0)
		return B_BAD_PORT_ID;
	slot = id % MAX_PORTS;

	// walk through the sem list, trying to match name
	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("close_port: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}

	// mark port to disable writing
	ports[slot].closed = true;

	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
}


status_t
delete_port(port_id id)
{
	int slot;
	int state;
	sem_id	r_sem, w_sem;
	int capacity;
	int i;

	char *old_name;
	struct port_msg *q;

	if (ports_active == false)
		return B_BAD_PORT_ID;
	if (id < 0)
		return B_BAD_PORT_ID;

	slot = id % MAX_PORTS;

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("delete_port: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}

	/* mark port as invalid */
	ports[slot].id	 = -1;
	old_name 		 = ports[slot].name;
	q				 = ports[slot].msg_queue;
	r_sem			 = ports[slot].read_sem;
	w_sem			 = ports[slot].write_sem;
	capacity		 = ports[slot].capacity;
	ports[slot].name = NULL;

	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);

	// delete the cbuf's that are left in the queue (if any)
	for (i = 0; i < capacity; i++) {
		if (q[i].data_cbuf != NULL)
	 		cbuf_free_chain(q[i].data_cbuf);
	}

	free(q);
	free(old_name);

	// release the threads that were blocking on this port by deleting the sem
	// read_port() will see the B_BAD_SEM_ID acq_sem() return value, and act accordingly
	delete_sem(r_sem);
	delete_sem(w_sem);

	return B_NO_ERROR;
}


port_id
find_port(const char *port_name)
{
	int i;
	int state;
	int ret_val = B_BAD_PORT_ID;

	if (ports_active == false)
		return B_BAD_PORT_ID;
	if (port_name == NULL)
		return B_BAD_PORT_ID;

	// lock list of ports
	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	// loop over list
	for (i = 0; i < MAX_PORTS; i++) {
		// lock every individual port before comparing
		GRAB_PORT_LOCK(ports[i]);
		if(strcmp(port_name, ports[i].name) == 0) {
			ret_val = ports[i].id;
			RELEASE_PORT_LOCK(ports[i]);
			break;
		}
		RELEASE_PORT_LOCK(ports[i]);
	}
	
	RELEASE_PORT_LIST_LOCK();
	restore_interrupts(state);

	return ret_val;
}


status_t
_get_port_info(port_id id, port_info *info, size_t size)
{
	int slot;
	int state;

	if (ports_active == false)
		return B_BAD_PORT_ID;
	if (info == NULL)
		return EINVAL;
	if (id < 0)
		return B_BAD_PORT_ID;

	slot = id % MAX_PORTS;

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("get_port_info: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}

	// fill a port_info struct with info
	info->port			= ports[slot].id;
//	info->owner 		= ports[slot].owner;
	strncpy(info->name, ports[slot].name, min(strlen(ports[slot].name),SYS_MAX_OS_NAME_LEN-1));
	info->capacity		= ports[slot].capacity;
	get_sem_count(ports[slot].read_sem, &info->queue_count);
	info->total_count	= ports[slot].total_count;

	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);
	
	// from our port_entry
	return B_NO_ERROR;
}


status_t
_get_next_port_info(team_id team, int32 *cookie, struct port_info *info, size_t size)
{
	int state;
	int slot;
	
	if (ports_active == false)
		return B_BAD_PORT_ID;
	if (cookie == NULL)
		return B_BAD_VALUE;

	if (*cookie == 0) {
		// return first found
		slot = 0;
	} else {
		// start at index cookie, but check cookie against MAX_PORTS
		slot = *cookie;
		if (slot >= MAX_PORTS)
			return B_BAD_PORT_ID;
	}

	// spinlock
	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();
	
	info->port = -1; // used as found flag
	while (slot < MAX_PORTS) {
		GRAB_PORT_LOCK(ports[slot]);
		if (ports[slot].id != -1)
			if (ports[slot].owner == team) {
				// found one!
				// copy the info
				info->port			= ports[slot].id;
//				info->owner 		= ports[slot].owner;
				strncpy(info->name, ports[slot].name, min(strlen(ports[slot].name),SYS_MAX_OS_NAME_LEN-1));
				info->capacity		= ports[slot].capacity;
				get_sem_count(ports[slot].read_sem, &info->queue_count);
				info->total_count	= ports[slot].total_count;
				RELEASE_PORT_LOCK(ports[slot]);
				slot++;
				break;
			}
		RELEASE_PORT_LOCK(ports[slot]);
		slot++;
	}
	RELEASE_PORT_LIST_LOCK();
	restore_interrupts(state);

	if (info->port == -1)
		return B_BAD_PORT_ID;
	*cookie = slot;
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
	int slot;
	int res;
	int t;
	int len;
	int state;
	
	if (ports_active == false)
		return B_BAD_PORT_ID;
	if (id < 0)
		return B_BAD_PORT_ID;

	slot = id % MAX_PORTS;

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("get_port_info: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}
	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);

	// block if no message, 
	// if TIMEOUT flag set, block with timeout

	// XXX - is it a race condition to acquire a sem just after we
	// unlocked the port ?
	// XXX: call an acquire_sem which does the release lock, restore int & block the right way
	res = acquire_sem_etc(ports[slot].read_sem, 1, flags & (B_TIMEOUT | B_CAN_INTERRUPT), timeout);

	GRAB_PORT_LOCK(ports[slot]);
	if (res == B_BAD_SEM_ID) {
		// somebody deleted the port
		RELEASE_PORT_LOCK(ports[slot]);
		return B_BAD_PORT_ID;
	}
	if (res == B_TIMED_OUT) {
		RELEASE_PORT_LOCK(ports[slot]);
		return B_TIMED_OUT;
	}

	// once message arrived, read data's length
	
	// determine tail
	// read data's head length
	t = ports[slot].head;
	if (t < 0)
		panic("port %ld: tail < 0", ports[slot].id);
	if (t > ports[slot].capacity)
		panic("port %ld: tail > cap %ld", ports[slot].id, ports[slot].capacity);
	len = ports[slot].msg_queue[t].data_len;
	
	// restore readsem
	release_sem(ports[slot].read_sem);

	RELEASE_PORT_LOCK(ports[slot]);
	
	// return length of item at end of queue
	return len;
}


ssize_t
port_count(port_id id)
{
	int slot;
	int state;
	int32 count;

	if (ports_active == false)
		return B_BAD_PORT_ID;
	if (id < 0)
		return B_BAD_PORT_ID;

	slot = id % MAX_PORTS;

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("port_count: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}
	
	get_sem_count(ports[slot].read_sem, &count);
	// do not return negative numbers 
	if (count < 0)
		count = 0;

	RELEASE_PORT_LOCK(ports[slot]);
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
read_port_etc(port_id id, int32 *msgCode, void *msgBuffer, size_t bufferSize,
	uint32 flags, bigtime_t timeout)
{
	int slot;
	int state;
	sem_id cached_semid;
	size_t size;
	status_t status;
	int t;
	cbuf *msgStore;
	int32 code;
	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) > 0;

	if (ports_active == false
		|| id < 0)
		return B_BAD_PORT_ID;

	if (msgCode == NULL
		|| (msgBuffer == NULL && bufferSize > 0)
		|| timeout < 0)
		return B_BAD_VALUE;

	flags = flags & (B_CAN_INTERRUPT | B_TIMEOUT);
	slot = id % MAX_PORTS;

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("read_port_etc: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}
	// store sem_id in local variable
	cached_semid = ports[slot].read_sem;

	// unlock port && enable ints/
	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);

	// XXX -> possible race condition if port gets deleted (->sem deleted too), therefore
	// sem_id is cached in local variable up here
	
	status = acquire_sem_etc(cached_semid, 1, flags, timeout);
		// get 1 entry from the queue, block if needed

	// XXX: possible race condition if port read by two threads...
	//      both threads will read in 2 different slots allocated above, simultaneously
	// 		slot is a thread-local variable

	if (status == B_BAD_SEM_ID || status == EINTR) {
		/* somebody deleted the port or the sem went away */
		return B_BAD_PORT_ID;
	}

	if (status == B_TIMED_OUT) {
		// timed out, or, if timeout=0, 'would block'
		return B_BAD_PORT_ID;
	}

	if (status != B_NO_ERROR) {
		dprintf("write_port_etc: unknown error %ld\n", status);
		return status;
	}

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	t = ports[slot].tail;
	if (t < 0)
		panic("port %ld: tail < 0", ports[slot].id);
	if (t > ports[slot].capacity)
		panic("port %ld: tail > cap %ld", ports[slot].id, ports[slot].capacity);

	ports[slot].tail = (ports[slot].tail + 1) % ports[slot].capacity;

	msgStore = ports[slot].msg_queue[t].data_cbuf;
	code = ports[slot].msg_queue[t].msg_code;

	// mark queue entry unused
	ports[slot].msg_queue[t].data_cbuf = NULL;

	// check output buffer size
	size = min(bufferSize, ports[slot].msg_queue[t].data_len);

	cached_semid = ports[slot].write_sem;

	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);

	// copy message
	*msgCode = code;
	if (size > 0) {
		if (userCopy) {
			if ((status = cbuf_user_memcpy_from_chain(msgBuffer, msgStore, 0, size) < B_OK)) {
				// leave the port intact, for other threads that might not crash
				cbuf_free_chain(msgStore);
				release_sem(cached_semid); 
				return status;
			}
		} else
			cbuf_memcpy_from_chain(msgBuffer, msgStore, 0, size);
	}
	// free the cbuf
	cbuf_free_chain(msgStore);

	// make one spot in queue available again for write
	release_sem(cached_semid);

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
	int slot;
	int state;
	status_t status;
	sem_id cached_semid;
	int h;
	cbuf *msgStore;
	bool userCopy = (flags & PORT_FLAG_USE_USER_MEMCPY) > 0;

	if (ports_active == false
		|| id < 0)
		return B_BAD_PORT_ID;

	// mask irrelevant flags (for acquire_sem() usage)
	flags = flags & (B_CAN_INTERRUPT | B_TIMEOUT);
	slot = id % MAX_PORTS;

	if (bufferSize > PORT_MAX_MESSAGE_SIZE)
		return EINVAL;

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("write_port_etc: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}

	if (ports[slot].closed) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("write_port_etc: port %ld closed\n", id);
		return B_BAD_PORT_ID;
	}
	
	// store sem_id in local variable 
	cached_semid = ports[slot].write_sem;

	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);
	
	// XXX -> possible race condition if port gets deleted (->sem deleted too), 
	// and queue is full therefore sem_id is cached in local variable up here
	
	status = acquire_sem_etc(cached_semid, 1, flags, timeout);
		// get 1 entry from the queue, block if needed

	// XXX: possible race condition if port written by two threads...
	//      both threads will write in 2 different slots allocated above, simultaneously
	// 		slot is a thread-local variable

	if (status == B_BAD_PORT_ID || status == B_INTERRUPTED) {
		/* somebody deleted the port or the sem while we were waiting */
		return B_BAD_PORT_ID;
	}

	if (status == B_TIMED_OUT) {
		// timed out, or, if timeout = 0, 'would block'
		return B_TIMED_OUT;
	}

	if (status != B_NO_ERROR) {
		dprintf("write_port_etc: unknown error %ld\n", status);
		return status;
	}

	if (bufferSize > 0) {
		msgStore = cbuf_get_chain(bufferSize);
		if (msgStore == NULL)
			return B_NO_MEMORY;

		if (userCopy) {
			// copy from user memory
			if ((status = cbuf_user_memcpy_to_chain(msgStore, 0, msgBuffer, bufferSize)) < B_OK)
				return status;
		} else {
			// copy from kernel memory
			if ((status = cbuf_memcpy_to_chain(msgStore, 0, msgBuffer, bufferSize)) < 0)
				return status;
		}
	} else
		msgStore = NULL;

	// attach copied message to queue
	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	h = ports[slot].head;
	if (h < 0)
		panic("port %ld: head < 0", ports[slot].id);
	if (h >= ports[slot].capacity)
		panic("port %ld: head > cap %ld", ports[slot].id, ports[slot].capacity);

	ports[slot].msg_queue[h].msg_code	= msgCode;
	ports[slot].msg_queue[h].data_cbuf	= msgStore;
	ports[slot].msg_queue[h].data_len	= bufferSize;
	ports[slot].head = (ports[slot].head + 1) % ports[slot].capacity;
	ports[slot].total_count++;

	// store sem_id in local variable 
	cached_semid = ports[slot].read_sem;

	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);

	// release sem, allowing read (might reschedule)
	release_sem(cached_semid);

	return B_NO_ERROR;
}


status_t
set_port_owner(port_id id, team_id team)
{
	int slot;
	int state;

	if (ports_active == false
		|| id < 0)
		return B_BAD_PORT_ID;

	slot = id % MAX_PORTS;

	state = disable_interrupts();
	GRAB_PORT_LOCK(ports[slot]);

	if (ports[slot].id != id) {
		RELEASE_PORT_LOCK(ports[slot]);
		restore_interrupts(state);
		dprintf("set_port_owner: invalid port_id %ld\n", id);
		return B_BAD_PORT_ID;
	}

	// transfer ownership to other team
	ports[slot].owner = team;

	// unlock port
	RELEASE_PORT_LOCK(ports[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
}


/** this function cycles through the ports table, deleting all
 *	the ports that are owned by the passed team_id
 */

int
delete_owned_ports(team_id owner)
{
	int state;
	int i;
	int count = 0;

	if (ports_active == false)
		return B_BAD_PORT_ID;

	state = disable_interrupts();
	GRAB_PORT_LIST_LOCK();

	for (i = 0; i < MAX_PORTS; i++) {
		if(ports[i].id != -1 && ports[i].owner == owner) {
			port_id id = ports[i].id;

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


#ifdef DEBUG
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


//	#pragma mark -
/* 
 *	user level ports
 */


port_id
user_create_port(int32 queue_length, const char *uname)
{
	if(uname != NULL) {
		char name[SYS_MAX_OS_NAME_LEN];
		status_t rc;

		if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
			return ERR_VM_BAD_USER_MEMORY;

		rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
		if(rc < 0)
			return rc;
		name[SYS_MAX_OS_NAME_LEN-1] = 0;

		return create_port(queue_length, name);
	} else {
		return create_port(queue_length, NULL);
	}
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
user_find_port(const char *port_name)
{
	if (port_name != NULL) {
		char name[SYS_MAX_OS_NAME_LEN];
		status_t rc;

		if((addr)port_name >= KERNEL_BASE && (addr)port_name <= KERNEL_TOP)
			return ERR_VM_BAD_USER_MEMORY;

		rc = user_strncpy(name, port_name, SYS_MAX_OS_NAME_LEN-1);
		if(rc < 0)
			return rc;
		name[SYS_MAX_OS_NAME_LEN-1] = 0;

		return find_port(name);
	} else {
		return EINVAL;
	}
}


status_t
user_get_port_info(port_id id, struct port_info *uinfo)
{
	status_t 			res;
	struct port_info	info;
	status_t 			rc;

	if (uinfo == NULL)
		return EINVAL;
	if ((addr)uinfo >= KERNEL_BASE && (addr)uinfo <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	res = get_port_info(id, &info);
	// copy to userspace
	rc = user_memcpy(uinfo, &info, sizeof(struct port_info));
	if (rc < 0)
		return rc;
	return res;
}


status_t
user_get_next_port_info(team_id uteam, int32 *ucookie, struct port_info *uinfo)
{
	status_t 			res;
	struct port_info	info;
	int32				cookie;
	status_t 			rc;

	if (ucookie == NULL)
		return EINVAL;
	if (uinfo == NULL)
		return EINVAL;
	if ((addr)ucookie >= KERNEL_BASE && (addr)ucookie <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if ((addr)uinfo >= KERNEL_BASE && (addr)uinfo <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	// copy from userspace
	rc = user_memcpy(&cookie, ucookie, sizeof(int32));
	if (rc < 0)
		return rc;
	
	res = get_next_port_info(uteam, &cookie, &info);
	// copy to userspace
	rc = user_memcpy(ucookie, &cookie, sizeof(int32));
	if (rc < 0)
		return rc;
	rc = user_memcpy(uinfo,   &info, sizeof(struct port_info));
	if (rc < 0)
		return rc;
	return res;
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
user_read_port_etc(port_id uport, int32 *umsg_code, void *umsg_buffer,
	size_t ubuffer_size, uint32 uflags, bigtime_t utimeout)
{
	ssize_t		res;
	int32		msg_code;
	status_t	rc;
	
	if (umsg_code == NULL)
		return EINVAL;
	if (umsg_buffer == NULL)
		return EINVAL;

	if((addr)umsg_code >= KERNEL_BASE && (addr)umsg_code <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if((addr)umsg_buffer >= KERNEL_BASE && (addr)umsg_buffer <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	res = read_port_etc(uport, &msg_code, umsg_buffer, ubuffer_size, 
						uflags | PORT_FLAG_USE_USER_MEMCPY | B_CAN_INTERRUPT, utimeout);

	rc = user_memcpy(umsg_code, &msg_code, sizeof(int32));
	if(rc < 0)
		return rc;

	return res;
}


status_t
user_set_port_owner(port_id port, team_id team)
{
	return set_port_owner(port, team);
}


status_t
user_write_port_etc(port_id uport, int32 umsg_code, void *umsg_buffer,
	size_t ubuffer_size, uint32 uflags, bigtime_t utimeout)
{
	if (umsg_buffer == NULL)
		return EINVAL;
	if((addr)umsg_buffer >= KERNEL_BASE && (addr)umsg_buffer <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	return write_port_etc(uport, umsg_code, umsg_buffer, ubuffer_size, 
		uflags | PORT_FLAG_USE_USER_MEMCPY | B_CAN_INTERRUPT, utimeout);
}

