#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iovec.h>
#include <stdarg.h>

#include <KernelExport.h>
#include <OS.h>

#include "memory_pool.h"
#include "net_stack.h"
#include "dump.h"
#include "buffer.h"
#include "attribute.h"

typedef struct net_buffer_chunk
{
	struct net_buffer_chunk	*next;
	struct net_buffer_chunk *next_data;
	uint32					ref_count;		// this data is referenced by 'ref_count' net_buffer_chunk(s)
	const void 				*data;			// address of data
	uint32					len;			// in bytes (could be 0, with *guest* add_buffer_free_element() node
	buffer_chunk_free_func	free_func;		// if NULL, don't free it!
	void *					free_cookie;	// if != NULL, free_func(cookie, data) is called... otherwise, free_func(data)
} net_buffer_chunk;

struct net_buffer {
	struct net_buffer 		*next;		// for chained buffers, like fifo'ed ones...
	struct net_buffer_chunk *all_chunks;	// unordored but ref counted net_buffer_chunk(s) linked list, to free on delete_data()
	struct net_buffer_chunk *data_chunks;	// ordered net_buffer_chunk(s) linked list
	size_t					len;		// total bytes in this net_buffer
	uint32					flags;
		#define BUFFER_TO_FREE (1)
		#define BUFFER_IS_URGENT (2)
	net_attribute 			*attributes;  // buffer attributes
};

struct net_buffer_queue
{
  benaphore  		lock;
  sem_id        	sync;
  volatile int32  	waiting;
  volatile int32  	interrupt;
  
  size_t       		max_bytes;
  size_t        	current_bytes;
  
  net_buffer 		*head;
  net_buffer		*tail;
};


#define DPRINTF printf

#define BUFFERS_PER_POOL		(64)
#define BUFFER_CHUNKS_PER_POOL	(256)

static memory_pool *	g_buffers_pool 			= NULL;
static memory_pool *	g_buffer_chunks_pool	= NULL;

static thread_id		g_buffers_purgatory_thread = -1;
static volatile sem_id	g_buffers_purgatory_sync = -1;		// use to notify buffers purgatory thread that some net_buffers need to be deleted (interrupt safely...)

#define BUFFER_QUEUES_PER_POOL	16

static memory_pool *	g_buffer_queues_pool = NULL;

extern memory_pool_module_info *g_memory_pool;

// Privates prototypes
// -------------------

static net_buffer_chunk * 	new_buffer_chunk(const void *data, uint32 len);
static net_buffer_chunk * 	find_buffer_chunk(net_buffer *buffer, uint32 offset, uint32 *offset_in_chunk, net_buffer_chunk **previous_chunk);

static status_t	prepend_buffer(net_buffer *buffer, const void *data, uint32 bytes, buffer_chunk_free_func freethis);
static status_t append_buffer(net_buffer *buffer, const void *data, uint32 bytes, buffer_chunk_free_func freethis);

static int32				buffers_purgatory_thread(void * data);

static status_t		 		buffer_killer(memory_pool * pool, void * node, void * cookie);
static status_t	 			buffer_queue_killer(memory_pool * pool, void * node, void * cookie);


// LET'S GO FOR IMPLEMENTATION
// ------------------------------------


//	#pragma mark [Start/Stop Service functions]


// --------------------------------------------------
status_t start_buffers_service()
{
	g_buffers_purgatory_sync = create_sem(0, "net buffers purgatory");
#ifdef _KERNEL_MODE
	set_sem_owner(g_buffers_purgatory_sync, B_SYSTEM_TEAM);
#endif

	// fire buffers_purgatory_thread
	g_buffers_purgatory_thread = spawn_kernel_thread(buffers_purgatory_thread, "buffers serial killer", B_LOW_PRIORITY, 0);
	if (g_buffers_purgatory_thread < B_OK)
		return g_buffers_purgatory_thread;

	puts("net buffers service started.");
		
	return resume_thread(g_buffers_purgatory_thread);
}


// --------------------------------------------------
status_t stop_buffers_service()
{
	status_t	status;

	// Free all buffers queues inner stuff (sem, locker, etc)
	g_memory_pool->for_each_pool_node(g_buffer_queues_pool, buffer_queue_killer, NULL);
	g_memory_pool->delete_pool(g_buffer_queues_pool);
	g_buffer_queues_pool	= NULL;

	// this would stop buffers_purgatory_thread
	delete_sem(g_buffers_purgatory_sync);
	g_buffers_purgatory_sync = -1;
	wait_for_thread(g_buffers_purgatory_thread, &status);
	
	// As the purgatory thread stop, some net_buffer(s) still could be flagged
	// PACKET_TO_FREE, but not deleted... yet.
	g_memory_pool->for_each_pool_node(g_buffers_pool, buffer_killer, NULL);

	// free buffers-related pools
	g_memory_pool->delete_pool(g_buffers_pool);
	g_memory_pool->delete_pool(g_buffer_chunks_pool);
	
	g_buffers_pool 		= NULL;
	g_buffer_chunks_pool	= NULL;
	
	puts("net buffers service stopped.");
	
	return B_OK;
}


// #pragma mark [Public functions]

	
// --------------------------------------------------
net_buffer * new_buffer(void)
{
	net_buffer *buffer;
	
	if (! g_buffers_pool)
		g_buffers_pool = g_memory_pool->new_pool(sizeof(*buffer), BUFFERS_PER_POOL);
			
	if (! g_buffers_pool)
		return NULL;
	
	buffer = (net_buffer *) g_memory_pool->new_pool_node(g_buffers_pool);
	if (! buffer)
		return NULL;
	
	buffer->next 		= NULL;
	buffer->data_chunks = NULL;
	buffer->all_chunks	= NULL;
	buffer->attributes 	= NULL;
	buffer->len			= 0;
	buffer->flags		= 0;
	
	return buffer;
}



// --------------------------------------------------
status_t delete_buffer(net_buffer *buffer, bool interrupt_safe)
{
	if (! buffer)
		return B_BAD_VALUE;

	if (! g_buffers_pool)
		// Uh? From where come this net_buffer!?!
		return B_ERROR;
		
	printf("delete_buffer(%p)\n", buffer);
		
	if (interrupt_safe) {
	  // We're called from a interrupt handler. Don't use any blocking calls (delete_pool_node() is!)
	  // We flag this net_buffer as to be free by the buffers purgatory thread
	  // Notify him that he have a new buffer purgatory candidate
      // (notice the B_DO_NOT_RESCHEDULE usage, the only release_sem_etc() interrupt-safe mode)
	  buffer->flags |= BUFFER_TO_FREE;
      return release_sem_etc(g_buffers_purgatory_sync, 1, B_DO_NOT_RESCHEDULE); 
	};

	if (buffer->all_chunks) {
		// Okay, we can (try to) free each buffer chunk(s) right now!

		net_buffer_chunk *chunk;
		net_buffer_chunk *next_chunk;
	
		chunk = buffer->all_chunks;
		while (chunk) {
			// okay, one net_buffer_chunk less referencing this data
			atomic_add(&chunk->ref_count, -1);
			next_chunk = chunk->next;
		
			if (chunk->ref_count == 0) {
				// it was the last net_buffer_chunk to reference this data, 
				// so free it now!

				// do we have to call the free_func() on this data?
				if (chunk->free_func) {
					// yes, please!!!
					// call the right free_func kind on this data
					if (chunk->free_cookie)
						chunk->free_func(chunk->free_cookie, (void *) chunk->data);
					else
						chunk->free_func((void *) chunk->data);
				};
				// delete this net_buffer_chunk
				g_memory_pool->delete_pool_node(g_buffer_chunks_pool, chunk);
			};
			
			chunk = next_chunk;
		};
	};
	
	if (buffer->attributes) {
		// Free buffer attributes

		net_attribute *attr;
		net_attribute *next_attr;

		attr = buffer->attributes;
		while (attr) {
			next_attr = attr->next;
			delete_attribute(attr, NULL);
			attr = next_attr;
		};
	};
	
	// Last, delete the net_buffer itself
	return g_memory_pool->delete_pool_node(g_buffers_pool, buffer);
}


// --------------------------------------------------
net_buffer * duplicate_buffer(net_buffer *buffer)
{
	return NULL; // B_UNSUPPORTED;
}


// --------------------------------------------------
net_buffer * clone_buffer(net_buffer *buffer)
{
	net_buffer *clone;
	net_buffer_chunk *chunk;
	
	if (! g_buffers_pool)
		g_buffers_pool = g_memory_pool->new_pool(sizeof(*buffer), BUFFERS_PER_POOL);
			
	if (! g_buffers_pool)
		return NULL;
	
	clone = (net_buffer *) g_memory_pool->new_pool_node(g_buffers_pool);
	if (! clone)
		return NULL;
	
	clone->next = NULL;
	clone->data_chunks 	= buffer->data_chunks;
	clone->all_chunks 	= buffer->all_chunks;
	clone->len 			= buffer->len;
	clone->flags		= buffer->flags;

	// Okay, increase all chunks ref count, as we have another buffer referencing them
	chunk = buffer->all_chunks;
	while (chunk) {
		// okay, one net_buffer_chunk less referencing this data
		atomic_add(&chunk->ref_count, 1);
		chunk = chunk->next;
	};

	return clone;
}


// --------------------------------------------------
net_buffer * split_buffer(net_buffer *buffer, uint32 offset)
{
	return NULL; // B_UNSUPPORTED;
}

// --------------------------------------------------
status_t concatenate_buffers(net_buffer *begin_buffer, net_buffer *end_buffer)
{
	return B_ERROR;
}


// --------------------------------------------------
status_t attach_buffer_free_element(net_buffer *buffer, void *arg1, void *arg2, buffer_chunk_free_func freethis)
{
	net_buffer_chunk *chunk;

	if (! buffer)
		return B_BAD_VALUE;
	
	chunk = new_buffer_chunk(arg1, 0); // unknown data length
	if (! chunk)
		return B_ERROR;
	
	chunk->next_data		= NULL;	// not a data_chunks member, just a *guest star* all_chunks member
	chunk->free_func		= freethis;
	chunk->free_cookie	= arg2;	

	// add to all_chunks (chunks order don't matter in this list :-)
	chunk->next 		= buffer->all_chunks;
	buffer->all_chunks 	= chunk;
	
	return B_OK;
}


// --------------------------------------------------
status_t add_to_buffer(net_buffer *buffer, uint32 offset, const void *data, uint32 bytes, buffer_chunk_free_func freethis)
{
	net_buffer_chunk 	*previous_chunk;
	net_buffer_chunk 	*next_chunk;
	net_buffer_chunk 	*split_chunk;
	net_buffer_chunk 	*new_chunk;
	uint32 offset_in_chunk;
	
	if (offset == 0)
		return prepend_buffer(buffer, data, bytes, freethis);

	if (offset >= buffer->len)
		return append_buffer(buffer, data, bytes, freethis);

	if (! buffer)
		return B_BAD_VALUE;
	
	if (bytes < 1)
		return B_BAD_VALUE;


	next_chunk = find_buffer_chunk(buffer, offset, &offset_in_chunk, &previous_chunk);
	if (! next_chunk)
		return B_BAD_VALUE;
	
	split_chunk	= NULL;
	new_chunk	= NULL;
	
	if (offset_in_chunk) {
		// we must split next_chunk data in two parts :-(
		uint8 *split;

		split = (uint8 *) next_chunk->data;
		split += offset_in_chunk;
		split_chunk = new_buffer_chunk(split, next_chunk->len - offset_in_chunk);
		if (! split_chunk)
			goto error1;

		next_chunk->len = offset_in_chunk; // cut the data len of original chunk

		// as split_chunk data comes from 'next_chunk', we don't 
		// ask to free this chunk data, as it would be by previous_chunk
		split_chunk->free_func = NULL;	
	
		// add the split_chunk to buffer's all_chunks list
		split_chunk->next 	= buffer->all_chunks;
		buffer->all_chunks	= split_chunk;
		
		// insert the split_chunk between the two part of the *splitted* next_chunk
		split_chunk->next_data = next_chunk->next_data;
		next_chunk->next_data = split_chunk;
		
		previous_chunk = next_chunk;
		next_chunk     = split_chunk;
	};

	// create a new chunk to host inserted data
	new_chunk = new_buffer_chunk(data, bytes);
	if (! new_chunk)
		goto error2;
	
	new_chunk->free_func	= freethis; // can be NULL = don't free this chunk
	new_chunk->free_cookie	= NULL;	
	
	// add the new_chunk to buffer's all_chunks list
	new_chunk->next 	= buffer->all_chunks;
	buffer->all_chunks 	= new_chunk;

	// Insert this new_chunk between previous and next chunk
	previous_chunk->next_data 	= new_chunk;
	new_chunk->next_data		= next_chunk;

	buffer->len += bytes;
	
	return B_OK;

error2:
	g_memory_pool->delete_pool_node(g_buffer_chunks_pool, new_chunk);
error1:
	g_memory_pool->delete_pool_node(g_buffer_chunks_pool, split_chunk);
	return B_ERROR;
}


// --------------------------------------------------
status_t remove_from_buffer(net_buffer *buffer, uint32 offset, uint32 bytes)
{
	net_buffer_chunk 	*first_chunk;
	net_buffer_chunk 	*last_chunk;
	net_buffer_chunk	*previous_chunk;
	uint32 offset_in_first_chunk;
	uint32 offset_in_last_chunk;
	
	if (! buffer)
		return B_BAD_VALUE;
	
	if (offset >= buffer->len)
		return B_BAD_VALUE;

	if (bytes < 1)
		return B_BAD_VALUE;

	first_chunk = find_buffer_chunk(buffer, offset, &offset_in_first_chunk, &previous_chunk);
	if (! first_chunk)
		return B_BAD_VALUE;
	
	last_chunk = find_buffer_chunk(buffer, offset + bytes, &offset_in_last_chunk, NULL);
	if (! last_chunk)
		return B_BAD_VALUE;
	
	if (offset_in_last_chunk) {
		// we must split last_chunk data in two parts,
		// removing beginnin chunk from data lists and adding the ending one :-(

		net_buffer_chunk *split_chunk;
		uint8 *split;
		
		// remove the beginning of last_chunk data
		
		split = (uint8 *) last_chunk->data;
		split += offset_in_last_chunk;
		split_chunk = new_buffer_chunk(split, last_chunk->len - offset_in_last_chunk);
		// if (! split_chunk)
		//	goto error1;
		// as split_chunk data comes from 'last_chunk', we don't 
		// ask to free this chunk data, as it would be by previous_chunk
		split_chunk->free_func = NULL;
		split_chunk->next_data = last_chunk->next_data;

		// add the split_chunk to buffer's all_chunks list
		split_chunk->next 	= buffer->all_chunks;
		buffer->all_chunks	= split_chunk;
		
		last_chunk = split_chunk;
	};

	if (offset_in_first_chunk) {
		// remove the end of first_chunk data: just forgot about last bytes of data :-)
		first_chunk->len = offset_in_first_chunk;
	} else
		first_chunk = previous_chunk;

	// fix the data chunks list to remove chunk(s), if any
	if (first_chunk)
		first_chunk->next_data = last_chunk;
	else
		buffer->data_chunks = last_chunk; 
	
	buffer->len -= bytes;	
	return B_OK;
}


// --------------------------------------------------
uint32 read_buffer(net_buffer *buffer, uint32 offset, void *data, uint32 bytes)
{
	net_buffer_chunk 	*chunk;
	uint32				offset_in_chunk;
	uint32				len;
	uint32				chunk_len;
	uint8 				*from;
	uint8 				*to;
	
	to = (uint8 *) data;
	len = 0;

	chunk = find_buffer_chunk(buffer, offset, &offset_in_chunk, NULL);
	while (chunk) {
		from  = (uint8 *) chunk->data;
		from += offset_in_chunk;
		chunk_len = min((chunk->len - offset_in_chunk), (bytes - len));
		
		memcpy(to, from, chunk_len);

		len += chunk_len;
		if (len >= bytes)
			break;

		to += chunk_len;
		offset_in_chunk = 0; // only the first chunk
		
		chunk = chunk->next_data;	// next chunk
	};
		
	return len;	
}


// --------------------------------------------------
uint32 write_buffer(net_buffer *buffer, uint32 offset, const void *data, uint32 bytes)
{
	net_buffer_chunk 	*chunk;
	uint32			offset_in_chunk;
	uint32			len;
	uint32			chunk_len;
	uint8 			*from;
	uint8 			*to;
	
	from = (uint8 *) data;
	len = 0;

	chunk = find_buffer_chunk(buffer, offset, &offset_in_chunk, NULL);
	if (! chunk)
		return B_ERROR;
		
	while (chunk) {
		to  = (uint8 *) chunk->data;
		to += offset_in_chunk;
		chunk_len = min((chunk->len - offset_in_chunk), (bytes - len));
		
		memcpy(to, from, chunk_len);

		len += chunk_len;
		if (len >= bytes)
			break;

		from += chunk_len;
		offset_in_chunk = 0; // only the first chunk
		
		chunk = chunk->next_data;	// next chunk
	};
		
	return len;	
}


// --------------------------------------------------
status_t add_buffer_attribute(net_buffer *buffer, const void *id, int type, ...)
{
	net_attribute *attr;
	
	if (! buffer)
		return B_BAD_VALUE;
	
	attr = new_attribute(&buffer->attributes, id);
	if (! attr)
		return B_NO_MEMORY;

	if (type & FROM_BUFFER) {
		va_list args;
		net_buffer_chunk *chunk;
		int offset;
		uint32	offset_in_chunk;
		int size;
		uint8 *ptr;
			
		type = (type & NET_ATTRIBUTE_FLAGS_MASK);
		
		va_start(args, type);
		offset 	= va_arg(args, int);
		size 	= va_arg(args, int);
		va_end(args);
		
		chunk = find_buffer_chunk(buffer, offset, &offset_in_chunk, NULL);
		if (chunk == NULL) {
			delete_attribute(attr, &buffer->attributes);
			return B_BAD_VALUE;
		};

		attr->size = size;
		
		if (size < chunk->len - offset_in_chunk) {
			ptr  = (uint8 *) chunk->data;
			ptr += offset_in_chunk;

			attr->type = type | NET_ATTRIBUTE_POINTER;
			attr->u.ptr = ptr;	
		} else {
			// data overlap this chunk, use iovec attribute type
			int i;
			uint32 len;
			uint32 chunk_len;
		
			i = 0;
			len = 0;
			while (chunk) {
				ptr  = (uint8 *) chunk->data;
				ptr += offset_in_chunk;
				chunk_len = min((chunk->len - offset_in_chunk), (size - len));
	
				attr->u.vec[i].iov_base = ptr;
				attr->u.vec[i].iov_len = chunk_len;
	
				len += chunk_len;
				i++;
				if (len >= size || i > 16)
					break;
	
				offset_in_chunk = 0; // only the first chunk
			
				chunk = chunk->next_data;	// next chunk
			};
	
			attr->type = type | NET_ATTRIBUTE_IOVEC;
		}

	} else {
		// Not implemented
		return B_ERROR;	
	}

	return B_OK;
}


// --------------------------------------------------
status_t remove_buffer_attribute(net_buffer *buffer, const void *id)
{
	net_attribute *attr;

	if (! buffer)
		return B_BAD_VALUE;
	
	attr = find_attribute(buffer->attributes, id, NULL, NULL, NULL);
	if (! attr)
		return B_NAME_NOT_FOUND;

	return delete_attribute(attr, &buffer->attributes);
}


status_t find_buffer_attribute(net_buffer *buffer, const void *id, int *type, void **value, size_t *size)
{
	net_attribute *attr;

	if (! buffer)
		return B_BAD_VALUE;
	
	attr = find_attribute(buffer->attributes, id, type, value, size);
	if (! attr)
		return B_NAME_NOT_FOUND;

	return B_OK;
}


//	#pragma mark [buffer(s) queues functions]

// --------------------------------------------------
net_buffer_queue * new_buffer_queue(size_t max_bytes)
{
	net_buffer_queue *queue;
	
	if (! g_buffer_queues_pool)
		g_buffer_queues_pool = g_memory_pool->new_pool(sizeof(net_buffer_queue), BUFFER_QUEUES_PER_POOL);
			
	if (! g_buffer_queues_pool)
		return NULL;
	
	queue = (net_buffer_queue *) g_memory_pool->new_pool_node(g_buffer_queues_pool);
	if (! queue)
		return NULL;
	
	create_benaphore(&queue->lock, "net_buffer_queue lock");
	queue->sync = create_sem(0, "net_buffer_queue sem");
	
	queue->max_bytes 		= max_bytes;
	queue->current_bytes 	= 0;
	
	queue->head = queue->tail = NULL;
	
	return queue;
}


// --------------------------------------------------
status_t	delete_buffer_queue(net_buffer_queue *queue)
{
	status_t status;
	
	if (! queue)
		return B_BAD_VALUE;
		
	if (! g_buffer_queues_pool)
		// Uh? From where come this queue then!?!
		return B_ERROR;

	// free the net_buffer's (still) in this queue
	status = empty_buffer_queue(queue);
	if (status != B_OK)
		return status;
	
	delete_sem(queue->sync);
	delete_benaphore(&queue->lock);

	return g_memory_pool->delete_pool_node(g_buffer_queues_pool, queue);
}


// --------------------------------------------------
status_t	empty_buffer_queue(net_buffer_queue *queue)
{
	net_buffer *buffer;
	net_buffer *next_buffer;
		
	if (! queue)
		return B_BAD_VALUE;
	
	lock_benaphore(&queue->lock);
	
	buffer = queue->head;
	while (buffer) {
		next_buffer = buffer->next;
		delete_buffer(buffer, true);
		buffer = next_buffer;
	};
	
	queue->head = NULL;
	queue->tail = NULL;

	delete_sem(queue->sync);
	queue->sync = create_sem(0, "net_buffer_queue sem");
	
	unlock_benaphore(&queue->lock);
	return B_OK;
}


// --------------------------------------------------
status_t enqueue_buffer(net_buffer_queue *queue, net_buffer *buffer)
{
	if (! queue)
		return B_BAD_VALUE;
	
	if (! buffer)
		return B_BAD_VALUE;
	
	buffer->next = NULL;
	
/*
	if (queue->current_bytes + buffer->len > queue->max_bytes)
		// what to do? dump some enqueued buffer(s) to free space? drop the new net_buffer?
		// TODO: dequeue enought net_buffer(s) to free space to queue this one...
		NULL;
*/
	lock_benaphore(&queue->lock);
		
	if (! queue->head)
		queue->head = buffer;
	if (queue->tail)
		queue->tail->next = buffer;
	queue->tail =  buffer;
	
	queue->current_bytes += buffer->len;

	unlock_benaphore(&queue->lock);
	
	return release_sem_etc(queue->sync, 1, B_DO_NOT_RESCHEDULE);
}


// --------------------------------------------------
size_t  dequeue_buffer(net_buffer_queue *queue, net_buffer **buffer, bigtime_t timeout, bool peek)
{
	status_t	status;
	net_buffer	*head_buffer;
	
	if (! queue)
		return B_BAD_VALUE;

	status = acquire_sem_etc(queue->sync, 1, B_RELATIVE_TIMEOUT, timeout);
	if (status != B_OK)
		return status;
	
	lock_benaphore(&queue->lock);
	
	head_buffer = queue->head;
	
	if (! peek) {
		// detach the head net_buffer from this fifo
		queue->head = head_buffer->next;
		if (queue->tail == head_buffer)
			queue->tail = queue->head;

		queue->current_bytes -= head_buffer->len;

		head_buffer->next = NULL;	// we never know :-)
	};
	
	unlock_benaphore(&queue->lock);
	
	*buffer = head_buffer;
	
	return head_buffer->len;
}	


//	#pragma mark [Helper functions]

// --------------------------------------------------
static net_buffer_chunk * new_buffer_chunk(const void *data, uint32 len)
{
	net_buffer_chunk *chunk;
	
	if (! g_buffer_chunks_pool)
		g_buffer_chunks_pool = g_memory_pool->new_pool(sizeof(net_buffer_chunk), BUFFER_CHUNKS_PER_POOL);
			
	if (! g_buffer_chunks_pool)
		return NULL;
	
	chunk = (net_buffer_chunk *) g_memory_pool->new_pool_node(g_buffer_chunks_pool);
	
	chunk->data = data;
	chunk->len  = len;

	chunk->ref_count = 1;

	chunk->free_cookie	= NULL;
	chunk->free_func	= NULL;

	chunk->next			= NULL;
	chunk->next_data	= NULL;

	return chunk;
}


// --------------------------------------------------
static net_buffer_chunk * find_buffer_chunk(net_buffer *buffer, uint32 offset, uint32 *offset_in_chunk, net_buffer_chunk **previous_chunk)
{
	net_buffer_chunk *previous;
	net_buffer_chunk *chunk;
	uint32			len;
	
	if (! buffer)
		return NULL;
	
	if (buffer->len <= offset)
		// this net_buffer don't hold enough data to reach this offset!
		return NULL;
	
	len = 0;
	chunk = buffer->data_chunks;
	previous = NULL;
	while (chunk)	{
		len += chunk->len;

		if(offset < len)
			break;
		
		previous = chunk;
		chunk = chunk->next_data;
	};

	if (offset_in_chunk)
		*offset_in_chunk = chunk->len - (len - offset);	
	if (previous_chunk)
		*previous_chunk = previous;

	return chunk;
}

// --------------------------------------------------
static status_t prepend_buffer(net_buffer *buffer, const void *data, uint32 bytes, buffer_chunk_free_func freethis)
{
	net_buffer_chunk *chunk;

	if (! buffer)
		return B_BAD_VALUE;

	if (bytes < 1)
		return B_BAD_VALUE;
	
	// create a new chunk to host the prepending data
	chunk = new_buffer_chunk(data, bytes);
	if (! chunk)
		return B_ERROR;
	
	chunk->free_func	= freethis; // can be NULL = don't free this chunk
	chunk->free_cookie	= NULL;	
	
	// add this chunk to all_chunks (chunks order don't matter in this list, so we do it the easy way :-)
	chunk->next	 		= buffer->all_chunks;
	buffer->all_chunks 	= chunk;
	
	// prepend this chunk to the data_chunks list:
	chunk->next_data	= buffer->data_chunks;
	buffer->data_chunks	= chunk;
	
	buffer->len += bytes;
	
	return B_OK;
}


// --------------------------------------------------
static status_t append_buffer(net_buffer *buffer, const void *data, uint32 bytes, buffer_chunk_free_func freethis)
{
	net_buffer_chunk *chunk;

	if (! buffer)
		return B_BAD_VALUE;
	
	if (bytes < 1)
		return B_BAD_VALUE;
	
	// create a new chunk to host the appending data
	chunk = new_buffer_chunk(data, bytes);
	if (! chunk)
		return B_ERROR;
	
	chunk->free_func	= freethis; // can be NULL = don't free this chunk
	chunk->free_cookie	= NULL;	
	
	// add this chunk to all_chunks (chunks order don't matter in this list, so we do it the easy way :-)
	chunk->next	 		= buffer->all_chunks;
	buffer->all_chunks 	= chunk;

	// Add this chunk to the end of data_chunks list
	chunk->next_data		= NULL;
	if (buffer->data_chunks) {
		net_buffer_chunk *tmp;
		
		tmp = buffer->data_chunks;
		while(tmp->next_data) // search the last net_buffer_chunk in list
			tmp = tmp->next_data;
		tmp->next_data = chunk;
	} else
		buffer->data_chunks = chunk;

	buffer->len += bytes;
	
	return B_OK;
}


// --------------------------------------------------
void dump_buffer(net_buffer *buffer)
{
	net_buffer_chunk *chunk;

	if (! buffer)
		return;

	DPRINTF("---- net_buffer %p: total len %ld\n", buffer, buffer->len);
	
	DPRINTF("data_chunks:\n");
	chunk = buffer->data_chunks;
	while (chunk) {
		DPRINTF("  * chunk %p: data %p, len %ld\n", chunk, chunk->data, chunk->len);
		dump_memory("      ", chunk->data, chunk->len);
		DPRINTF("    next: %p\n", chunk->next_data);

		chunk = chunk->next_data;
	};

	DPRINTF("all_chunks:\n");
	chunk = buffer->all_chunks;
	while (chunk) {
		DPRINTF("  * chunk %p: data %p, len %ld, ref_count %ld\n", chunk, chunk->data, chunk->len, chunk->ref_count);
		if (chunk->free_func)
			DPRINTF("    free_func: %p, free_cookie %p\n", chunk->free_func, chunk->free_cookie);
		DPRINTF("    next: %p\n", chunk->next);

		chunk = chunk->next;
	};

}

// #pragma mark [Packets Purgatory functions]

// --------------------------------------------------
static int32 buffers_purgatory_thread(void *data)
{
	while (true) {
		if (acquire_sem(g_buffers_purgatory_sync) != B_OK)
			break;
		
		// okay, time to cleanup some net_buffer(s)
		g_memory_pool->for_each_pool_node(g_buffers_pool, buffer_killer, NULL);
	};
	return 0;
}


// --------------------------------------------------
static status_t buffer_killer(memory_pool * pool, void * node, void * cookie)
{
	net_buffer *buffer = node;
	if (buffer->flags & BUFFER_TO_FREE)
		delete_buffer(buffer, false);
		
	return 0; // B_OK;
}


// --------------------------------------------------
static status_t buffer_queue_killer(memory_pool * pool, void * node, void * cookie)
{
	return delete_buffer_queue((net_buffer_queue *) node);
}

