/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Fast logging facilities.

	Client interface.

	Every client opens a connection, passing a list of event
	ids together with their description. All logging calls
	write the encoded event into a buffer, using the event id
	to save space. Once the logging data is to be read, the
	buffer is decoded and written to the long buffers. The
	same happens when a client closes a connection, as the
	association between event ids and descriptions would be
	lost. 
	
	The buffer is split into a lower and an upper half, each
	of size fast_log_buffer_size. The currently active half
	is determined by bit 31 of fast_log_buffer_offset_alloc:
	zero means lower, one means upper half. 
	
	During write, fast_log_buffer_offset_alloc is first 
	incremented by the requested number of bytes atomically. 
	If there is enough space, the even data is copied into 
	the buffer and fast_log_buffer_offset_filled is 
	atomically incremented as well. This allows concurrent
	filling by different threads without using any locking.
	If the buffer is too short, logging data is discarded. 
	
	For read out, fast_log_buffer_offset_alloc 
	is atomically set to the beginning of the opposite buffer, 
	making sure that no further events are appended to the 
	previously active buffer. Then, we wait for 
	fast_log_buffer_offset having reached the value of
	fast_log_buffer_alloc when it was reset, i.e. until 
	all active writes are completed. Now, the buffer can
	be read out savely. 
	
	To detect buffer overflow, buffer halves are set to
	zero before they are activated. If a write fails because
	lack of space, nothing is writting to the buffer, though
	fast_log_buffer_offset and fast_log_buffer_alloc are
	incremented. This leads to an unwritten entry in the log,
	which content remains zero. The easier way is to detect
	whether fast_log_alloc is byond buffer size (which is 
	done as well), but because entries have different sizes, 
	we don't know which was the lastly written entry.
*/


#include "fast_log_int.h"
#include "dl_list.h"

#include <KernelExport.h>

#include <stdio.h>
#include <string.h>


// one client connection
typedef struct fast_log_connection {
	struct fast_log_connection *prev, *next;
	// prefix in log
	const char *prefix;
	// list of event types
	fast_log_event_type *types;
} fast_log_connection;

// one entry in logging buffer
typedef struct fast_log_entry {
	// client handle.
	// this must be first as it is checked against NULL
	// by reader to detect overflown entries; moving it
	// to a later offset would make reading of it dangerous
	// as it could be located byond end of buffer 
	// (perhaps this could be handled as well, but it
	//  would make things more complicated)
	fast_log_connection *handle;
	// event type
	int16 event;
	// number of entries in "params"
	int16 num_params;
	// timestamp
	bigtime_t time;
	// parameters
	uint32 params[1];
} fast_log_entry;

// list of clients
static fast_log_connection *fast_log_handles = NULL;
// to lock during access of fast_log_handles
sem_id fast_log_clients_sem;
// to lock during conversion of encoded buffer to long buffer
sem_id fast_log_convert_sem;
// logging buffer.
// it consists of 2*fast_log_buffer_size, see header
char *fast_log_buffer;
// offset in logging buffer for next write.
// bit 31 is set for upper buffer half and is rest for lower buffer half
uint32 fast_log_buffer_offset_alloc;
// offset of end of last write
uint32 fast_log_buffer_offset_filled;
// size of one half of log buffer
int fast_log_buffer_size = /*512*/ 32*1024;
	// ToDo: cut down because of current allocator...


device_manager_info *pnp;


static int32
atomic_exchange(vint32 *value, int32 newValue)
{
	int32 oldValue;
	
	__asm__ __volatile__ (
		"xchg %0,%1"
		: "=r" (oldValue), "=m" (*value)
		: "0" (newValue), "1" (*value)
	);
	
	return oldValue;
}


/**	convert event code to text
 *	text_buffer - buffer used if text must is composed manually
 *				  (must be at least 24 characters long)
 */

static const char *
fast_log_event2text(fast_log_connection *handle, int16 event, char *text_buffer)
{
	fast_log_event_type *type;

	for (type = handle->types; type->event != 0; ++type) {
		if (type->event == event)
			return type->description;
	}

	// longest message is 11 characters + 11 integer characters + zero byte
	sprintf(text_buffer, "unknown (%d)", event);
	return text_buffer;
}


/** decode one entry and write it to long buffer */

static void
fast_log_decode_entry(fast_log_entry *entry)
{
	fast_log_connection *handle = entry->handle;
	char buffer[100];
	uint64 time;
	uint32 min, sec, mill, mic;
	char text_buffer[30];
	int i;

	time = entry->time; //entry->tsc / (sysinfo.cpu_clock_speed / 1000000);
	mic = time % 1000;
	time /= 1000;
	mill = time % 1000;
	time /= 1000;
	sec = time % 60;
	time /= 60;
	min = time;

	sprintf(buffer, "%03ld:%02ld:%03ld.%03ld ", min, sec, mill, mic);
	fast_log_write_long_log(buffer);

	fast_log_write_long_log(handle->prefix);
	fast_log_write_long_log(": ");
	fast_log_write_long_log(fast_log_event2text(handle, entry->event, text_buffer));
	fast_log_write_long_log(" ");

	for (i = 0; i < entry->num_params; ++i) {
		if (i > 0)
			fast_log_write_long_log(", ");

		sprintf(buffer, "%08x", (uint)entry->params[i]);
		fast_log_write_long_log(buffer);
	}

	fast_log_write_long_log("\n");
}


/** flush all compressed logging entries to long log */

void
fast_log_flush_buffer(void)
{
	uint32 offset;
	uint32 start, new_start, new_offset_alloc, size;
	int i;

	acquire_sem(fast_log_convert_sem);

	// flip between lower and upper half of buffer	
	if ((fast_log_buffer_offset_alloc & (1L << 31)) == 0) {
		start = 0;
		new_start = fast_log_buffer_size;
		new_offset_alloc = 1L << 31;
	} else {
		start = fast_log_buffer_size;
		new_start = 0;
		new_offset_alloc = 0;
	}

	// reset new buffer to zero to detect uninitialized, overflown entries
	memset(fast_log_buffer + new_start, 0, fast_log_buffer_size);

	// swap buffers
	size = atomic_exchange(&fast_log_buffer_offset_alloc, new_offset_alloc);

	// get rid of bit 31 flag
	size &= ~(1 << 31);

	// decrement offset of filled data by number of bytes allocated when
	// the buffer was swapped; if all writers are done, the offset is
	// >= 0 now, if some writers aren't finished, it is < 0
	atomic_add(&fast_log_buffer_offset_filled, -size);

	// wait until all writers are finished
	for (i = 1000000; i >= 0; --i) {
		if ((int32)fast_log_buffer_offset_filled >= 0)
			break;

		spin(1);
			// ToDo: spin?? If that should be a "yield", it should be snooze(1).
	}

	// perhaps we are in a spinlock or something nasty like that,
	// so the writer is blocked by us
	if (i < 0) {
		fast_log_write_long_log("<TIME-OUT> during waiting for log writers; some "
			"events got lost; perhaps a client disconnected with a spinlock hold\n");
	}

//	dprintf( "start=%d, size=%d\n", (int)start, (int)size );

	// uncompress log	
	for (offset = 0; offset < size; ) {
		fast_log_entry *entry;

		// check whether there was an overflow when the buffer
		// had zero bytes left
		if (offset > (uint32)fast_log_buffer_size) {
			fast_log_write_long_log("<TRUNCATED1>\n");
			break;
		}

		entry = (fast_log_entry *)&fast_log_buffer[start + offset];

		// check for overflow when the buffer had some bytes left;
		// in this case, the entry is uninitialized and thus contains
		// all zeros
		if (entry->handle == NULL) {
			fast_log_write_long_log( "<TRUNCATED2>\n" );
			// the entry may be partially byond end of buffer, so
			// we must not access any other element of the entry
			break;
		} else {
			// normal entry, so it's time to decode it
			fast_log_decode_entry(entry);
		}			

		offset += sizeof(fast_log_entry)
			+ (entry->num_params - 1) * sizeof(entry->params[0]);
	}

	release_sem(fast_log_convert_sem);
}


/** start logging of one client */

static fast_log_handle
fast_log_start_log(const char *prefix, fast_log_event_type types[])
{
	fast_log_connection *handle;

	handle = malloc(sizeof(*handle));
	if (handle == NULL)
		return NULL;

	handle->prefix = prefix;
	handle->types = types;

	acquire_sem(fast_log_clients_sem);
	ADD_DL_LIST_HEAD(handle, fast_log_handles, );
	release_sem(fast_log_clients_sem);

	return handle;
}


/** close connection of one client */

static void
fast_log_stop_log(fast_log_handle handle)
{
	if (handle == NULL)
		return;

	fast_log_flush_buffer();

	acquire_sem(fast_log_clients_sem);
	REMOVE_DL_LIST(handle, fast_log_handles, );
	release_sem(fast_log_clients_sem);

	free(handle);
}

#define ENTRY_SIZE(num_params) (sizeof(fast_log_entry) + sizeof(uint32) * ((num_params)-1))

/** allocate space in log */

static fast_log_entry *
fast_log_alloc_entry(fast_log_handle handle, int num_params)
{
	uint32 offset, real_offset, chunk_offset;
	fast_log_entry *entry;

	if (handle == NULL)
		return NULL;

	// alloc space in buffer
	offset = atomic_add( &fast_log_buffer_offset_alloc, ENTRY_SIZE( num_params ));

	real_offset = offset & ~(1L << 31);
	chunk_offset = ((offset & (1L << 31)) == 0 ? 0 : fast_log_buffer_size);

	// take care of bit 31 trick to get the address of the new entry
	entry = (fast_log_entry *)&fast_log_buffer[chunk_offset + real_offset];

	if (real_offset + ENTRY_SIZE( num_params ) < (uint32)fast_log_buffer_size) {
		// there is enough space in buffer, so we can fill it
		entry->time = system_time();
		entry->handle = handle;
		entry->num_params = num_params;
		return entry;	
	} else {
		// buffer overflow, return NULL to notify caller
		atomic_add( &fast_log_buffer_offset_filled, ENTRY_SIZE( num_params ));
		return NULL;
	} 
}


/** log event without parameters */

static void
fast_log_log_0(fast_log_handle handle, int event)
{
	fast_log_entry *entry;

	entry = fast_log_alloc_entry(handle, 0);

	if (entry != NULL) {
		entry->event = event;
		atomic_add(&fast_log_buffer_offset_filled, ENTRY_SIZE(0));
	}
}


/** log event with one parameter */

static void
fast_log_log_1(fast_log_handle handle, int event, uint32 param1)
{
	fast_log_entry *entry;

	entry = fast_log_alloc_entry(handle, 1);

	if (entry != NULL) {
		entry->event = event;
		entry->params[0] = param1;
		atomic_add(&fast_log_buffer_offset_filled, ENTRY_SIZE(1));
	}
}


/** log event with two parameters */

static void
fast_log_log_2(fast_log_handle handle, int event, uint32 param1, uint32 param2)
{
	fast_log_entry *entry;

	entry = fast_log_alloc_entry(handle, 2);

	if (entry != NULL) {
		entry->event = event;
		entry->params[0] = param1;
		entry->params[1] = param2;
		atomic_add(&fast_log_buffer_offset_filled, ENTRY_SIZE(2));
	}
}


/** log event with three parameters */

static void
fast_log_log_3(fast_log_handle handle, int event,
	uint32 param1, uint32 param2, uint32 param3)
{
	fast_log_entry *entry;

	entry = fast_log_alloc_entry(handle, 3);

	if (entry != NULL) {
		entry->event = event;
		entry->params[0] = param1;
		entry->params[1] = param2;
		entry->params[2] = param3;
		atomic_add(&fast_log_buffer_offset_filled, ENTRY_SIZE(3));
	}
}


/** log event with four parameters */

static void
fast_log_log_4(fast_log_handle handle, int event, uint32 param1,
	uint32 param2, uint32 param3, uint32 param4)
{
	fast_log_entry *entry;

	entry = fast_log_alloc_entry(handle, 4);

	if (entry != NULL) {
		entry->event = event;
		entry->params[0] = param1;
		entry->params[1] = param2;
		entry->params[2] = param3;
		entry->params[3] = param4;
		atomic_add(&fast_log_buffer_offset_filled, ENTRY_SIZE(4));
	}
}


/** log event with n parameters */

static void
fast_log_log_n(fast_log_handle handle, int event, int num_params, ...)
{
	fast_log_entry *entry;
	va_list vl;

	entry = fast_log_alloc_entry(handle, num_params);

	if (entry != NULL) {
		int i;
		va_start(vl, num_params);

		entry->event = event;

		for (i = 0; i < num_params; ++i) {
			entry->params[i] = va_arg(vl, int);
		}

		va_end(vl);

		atomic_add(&fast_log_buffer_offset_filled, ENTRY_SIZE(num_params));
	}
}


/** clean up client structures */

static void
fast_log_uninit_clients()
{
	free(fast_log_buffer);
	delete_sem(fast_log_clients_sem);
	delete_sem(fast_log_convert_sem);
}


/**	init client structures.
 *	you must call fast_log_uninit_clients on fail
 */

static status_t
fast_log_init_clients()
{
	fast_log_buffer = NULL;
	fast_log_clients_sem = -1;
	fast_log_convert_sem = -1;

	fast_log_clients_sem = create_sem(1, "fast_log_clients_sem");
	if (fast_log_clients_sem < B_OK)
		return fast_log_clients_sem;

	fast_log_convert_sem = create_sem(1, "fast_log_convert_sem");
	if (fast_log_convert_sem < B_OK)
		return fast_log_convert_sem;

	fast_log_buffer = malloc(2 * fast_log_buffer_size);
	if (fast_log_buffer == NULL)
		return B_NO_MEMORY;

	memset(fast_log_buffer, 0, 2 * fast_log_buffer_size);

	fast_log_buffer_offset_filled = fast_log_buffer_offset_alloc = 0;

	return B_OK;
}


/** uninitialize module */

static status_t
fast_log_uninit(void)
{
	fast_log_uninit_dev();
	fast_log_uninit_long_buffer();
	fast_log_uninit_clients();

	return B_OK;
}


/** initialize module */

static status_t
fast_log_init(void)
{
	status_t res;

	if ((res = fast_log_init_long_buffer()) < B_OK)
		goto err3;

	if ((res = fast_log_init_clients()) < B_OK)
		goto err2;

	if ((res = fast_log_init_dev()) < B_OK)
		// yes: don't call uninit for device.c
		goto err2;

	return B_OK;

err2:
	fast_log_uninit_clients();
err3:
	fast_log_uninit_long_buffer();
	return res;
}
		

static status_t
fast_log_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return fast_log_init();
		case B_MODULE_UNINIT:
			return fast_log_uninit();

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{ DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};

// client interface
fast_log_info fast_log_module = {
	{
		FAST_LOG_MODULE_NAME,
		0,			
		fast_log_std_ops
	},

	fast_log_start_log,
	fast_log_stop_log,

	fast_log_log_0,
	fast_log_log_1,
	fast_log_log_2,
	fast_log_log_3,
	fast_log_log_4,
	fast_log_log_n
};

// embedded modules
module_info *modules[] = {
	(module_info *)&fast_log_module,
	(module_info *)&fast_log_devfs_module,
	NULL
};
