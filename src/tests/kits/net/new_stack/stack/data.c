#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <KernelExport.h>
#include <OS.h>

#include "memory_pool.h"
#include "net_stack.h"
#include "data.h"

#if 0
typedef struct net_data_node_info
{
	uint32					ref_count;		// this data is referenced by 'ref_count' net_data_node(s)
	const void *			data;			// address of data
	uint32					len;			// in bytes (could be 0, with *guest* add_free_element() node
	data_node_free_func		free_func;		// if NULL, don't free it!
	void *					free_cookie;	// if != NULL, free_func(cookie, data) is called... otherwise, free_func(data)
} net_data_node_info;


typedef struct net_data_node
{
	struct net_data_node *	next;
	net_data_node_info *	info;
} net_data_node;
#endif

typedef struct net_data_node
{
	struct net_data_node 	*next;
	struct net_data_node 	*next_data;
	uint32					ref_count;		// this data is referenced by 'ref_count' net_data_node(s)
	const void 				*data;			// address of data
	uint32					len;			// in bytes (could be 0, with *guest* add_data_free_element() node
	data_node_free_func		free_func;		// if NULL, don't free it!
	void *					free_cookie;	// if != NULL, free_func(cookie, data) is called... otherwise, free_func(data)
} net_data_node;

struct net_data {
	struct net_data 		*next;		// for chained datas, like fifo'ed ones...
	struct net_data_node 	*node_list;	// unordored but ref counted net_data_node(s) linked list, to free on delete_data()
	struct net_data_node 	*data_list;	// ordered net_data_node(s) linked list
	size_t					len;		// total bytes in this net_data
	uint32					flags;
		#define DATA_TO_FREE (1)
		#define DATA_IS_URGENT (2)
};

struct net_data_queue
{
  benaphore  		lock;
  sem_id        	sync;
  volatile int32  	waiting;
  volatile int32  	interrupt;
  
  size_t       		max_bytes;
  size_t        	current_bytes;
  
  net_data 			*head;
  net_data			*tail;
};


#define DPRINTF printf

#define DATAS_PER_POOL		(64)
#define DATA_NODES_PER_POOL	(256)

static memory_pool *	g_datas_pool 		= NULL;
static memory_pool *	g_datas_nodes_pool	= NULL;

static thread_id		g_datas_purgatory_thread = -1;
static sem_id			g_datas_purgatory_sync = -1;		// use to notify data purgatory thread that some net_data nodes need to be deleted (interrupt safely...)

#define DATA_QUEUES_PER_POOL	16

static memory_pool *	g_datas_queues_pool = NULL;

extern struct memory_pool_module_info *g_memory_pool;

// Privates prototypes
// -------------------

static net_data_node * 	new_data_node(const void *data, uint32 len);
static net_data_node * 	find_data_node(net_data *data, uint32 offset, uint32 *offset_in_node, net_data_node **previous_node);

static int32				datas_purgatory_thread(void * data);

static status_t		 		data_death(memory_pool * pool, void * node, void * cookie);
static status_t	 			data_queue_death(memory_pool * pool, void * node, void * cookie);


// LET'S GO FOR IMPLEMENTATION
// ------------------------------------


//	#pragma mark [Start/Stop Service functions]


// --------------------------------------------------
status_t start_data_service()
{
	g_datas_purgatory_sync = create_sem(0, "net data purgatory");
#ifdef _KERNEL_MODE
	set_sem_owner(g_datas_purgatory_sync, B_SYSTEM_TEAM);
#endif

	// fire data_purgatory_thread
	g_datas_purgatory_thread = spawn_kernel_thread(datas_purgatory_thread, "net data serial killer", B_LOW_PRIORITY, 0);
	if (g_datas_purgatory_thread < B_OK)
		return g_datas_purgatory_thread;

	puts("net data service started.");
		
	return resume_thread(g_datas_purgatory_thread);
}


// --------------------------------------------------
status_t stop_data_service()
{
	status_t	status;

	// Free all data queues inner stuff (sem, locker, etc)
	g_memory_pool->for_each_pool_node(g_datas_queues_pool, data_queue_death, NULL);
	g_memory_pool->delete_pool(g_datas_queues_pool);
	g_datas_queues_pool	= NULL;

	// this would stop data_purgatory_thread
	delete_sem(g_datas_purgatory_sync);
	g_datas_purgatory_sync = -1;
	wait_for_thread(g_datas_purgatory_thread, &status);
	
	// As the purgatory thread stop, some net_data(s) still could be flagged
	// data_TO_FREE, but not deleted... yet.
	g_memory_pool->for_each_pool_node(g_datas_pool, data_death, NULL);

	// free datas-related pools
	g_memory_pool->delete_pool(g_datas_pool);
	g_memory_pool->delete_pool(g_datas_nodes_pool);
	
	g_datas_pool 		= NULL;
	g_datas_nodes_pool	= NULL;
	
	puts("net data service stopped.");
	
	return B_OK;
}


// #pragma mark [Public functions]

	
// --------------------------------------------------
net_data * new_data(void)
{
	net_data *nd;
	
	if (! g_datas_pool)
		g_datas_pool = g_memory_pool->new_pool(sizeof(net_data), DATAS_PER_POOL);
			
	if (! g_datas_pool)
		return NULL;
	
	nd = (net_data *) g_memory_pool->new_pool_node(g_datas_pool);
	if (! nd)
		return NULL;
	
	nd->next 		= NULL;
	nd->data_list 	= NULL;
	nd->node_list	= NULL;
	nd->len			= 0;
	nd->flags		= 0;
	
	return nd;
}



// --------------------------------------------------
status_t delete_data(net_data *nd, bool interrupt_safe)
{
	net_data_node *node;
	net_data_node *next_node;
	
	if (! nd)
		return B_BAD_VALUE;

	if (! g_datas_pool)
		// Uh? From where come this net_data!?!
		return B_ERROR;
		
	if (interrupt_safe) {
	  // We're called from a interrupt handler. Don't use any blocking calls (delete_pool_node() is!)
	  // We flag this net_data as to be free by the data purgatory thread
	  // Notify him that he have a new data purgatory member
      // (notice the B_DO_NOT_RESCHEDULE usage, the only release_sem_etc() interrupt-safe mode)
	  nd->flags |= DATA_TO_FREE;
      // release_sem_etc(g_data_purgatory_sync, 1, B_DO_NOT_RESCHEDULE); 
      return B_OK;
	};

	// Okay, we can free each data node in the node_list right now!
	node = nd->node_list;
	while (node) {
		// okay, one net_data_node less referencing this data
		node->ref_count--;
		next_node = node->next;
		
		if (node->ref_count == 0) {
			// it was the last net_data_node to reference this data, 
			// so free it now!

			// do we have to call the free_func() on this data?
			if (node->free_func) {
				// yes, please!!!
				// call the right free_func on this data
				if (node->free_cookie)
					node->free_func(node->free_cookie, (void *) node->data);
				else
					node->free_func((void *) node->data);
			};
		};
		// delete this net_data_node
		g_memory_pool->delete_pool_node(g_datas_nodes_pool, node);
			
		node = next_node;
	};
	
	// Last, delete the net_data itself
	return g_memory_pool->delete_pool_node(g_datas_pool, nd);
}


// --------------------------------------------------
net_data * duplicate_data(net_data *nd)
{
	return NULL; // B_UNSUPPORTED;
}


// --------------------------------------------------
net_data * clone_data(net_data *nd)
{
	return NULL; // B_UNSUPPORTED;
}


// --------------------------------------------------
status_t add_data_free_node(net_data *nd, void *arg1, void *arg2, data_node_free_func freethis)
{
	net_data_node *node;

	if (! nd)
		return B_BAD_VALUE;
	
	node = new_data_node(arg1, 0); // unknown data length
	if (! node)
		return B_ERROR;
	
	node->next_data		= NULL;	// not a data_list member, just a *guest star* node_list member
	node->free_func		= freethis;
	node->free_cookie	= arg2;	

	// add to node_list (nodes order don't matter in this list :-)
	node->next 		= nd->node_list;
	nd->node_list 	= node;
	
	return B_OK;
}


// --------------------------------------------------
status_t prepend_data(net_data *nd, const void *data, uint32 bytes, data_node_free_func freethis)
{
	net_data_node *node;

	if (! nd)
		return B_BAD_VALUE;

	if (bytes < 1)
		return B_BAD_VALUE;
	
	// create a new node to host the prepending data
	node = new_data_node(data, bytes);
	if (! node)
		return B_ERROR;
	
	node->free_func		= freethis; // can be NULL = don't free this chunk
	node->free_cookie	= NULL;	
	
	// add this node to node_list (nodes order don't matter in this list, so we do it the easy way :-)
	node->next	 	= nd->node_list;
	nd->node_list 	= node;
	
	// prepend this node to the data_list:
	node->next_data	= nd->data_list;
	nd->data_list	= node;
	
	nd->len += bytes;
	
	return B_OK;
}


// --------------------------------------------------
status_t append_data(net_data *nd, const void *data, uint32 bytes, data_node_free_func freethis)
{
	net_data_node *node;

	if (! nd)
		return B_BAD_VALUE;
	
	if (bytes < 1)
		return B_BAD_VALUE;
	
	// create a new node to host the appending data
	node = new_data_node(data, bytes);
	if (! node)
		return B_ERROR;
	
	node->free_func		= freethis; // can be NULL = don't free this chunk
	node->free_cookie	= NULL;	
	
	// add this node to node_list (nodes order don't matter in this list, so we do it the easy way :-)
	node->next	 	= nd->node_list;
	nd->node_list 	= node;

	// Add this node to the end of data_list
	node->next_data		= NULL;
	if (nd->data_list) {
		net_data_node *tmp;
		
		tmp = nd->data_list;
		while(tmp->next_data) // search the last net_data_node in list
			tmp = tmp->next_data;
		tmp->next_data = node;
	} else
		nd->data_list = node;

	nd->len += bytes;
	
	return B_OK;
}


// --------------------------------------------------
status_t insert_data(net_data *nd, uint32 offset, const void *data, uint32 bytes, data_node_free_func freethis)
{
	net_data_node 	*previous_node;
	net_data_node 	*next_node;
	net_data_node 	*split_node;
	net_data_node 	*new_node;
	uint32			offset_in_node;
	
	if (! nd)
		return B_BAD_VALUE;
	
	if (bytes < 1)
		return B_BAD_VALUE;

	if (offset == 0)
		return prepend_data(nd, data, bytes, freethis);

	next_node = find_data_node(nd, offset, &offset_in_node, &previous_node);
	if (! next_node)
		return B_BAD_VALUE;
	
	split_node	= NULL;
	new_node	= NULL;
	
	if (offset_in_node) {
		// we must split next_node data chunk in two parts :-(
		uint8 *split;

		split = (uint8 *) next_node->data;
		split += offset_in_node;
		split_node = new_data_node(split, next_node->len - offset_in_node);
		if (! split_node)
			goto error1;

		next_node->len = offset_in_node; // cut the data len of original node

		// as split_node data comes from 'next_node', we don't 
		// ask to free this node data, as it would be by the next_node node
		split_node->free_func = NULL;	
	
		// add the split_node to node_list (nodes order don't matter in this list, so we do it the easy way :-)
		split_node->next 	= nd->node_list;
		nd->node_list 		= split_node;
		
		// insert the split_node between the two part of the *splitted* next_node
		split_node->next_data = next_node->next_data;
		next_node->next_data = split_node;
		
		previous_node = next_node;
		next_node     = split_node;
	};

	// create a new node to host inserted data
	new_node = new_data_node(data, bytes);
	if (! new_node)
		goto error2;
	
	new_node->free_func		= freethis; // can be NULL = don't free this chunk
	new_node->free_cookie	= NULL;	
	
	// add the new_node to node_list (nodes order don't matter in this list, so we do it the easy way :-)
	new_node->next 	= nd->node_list;
	nd->node_list 	= new_node;

	// Insert this new_node between previous and next node
	previous_node->next_data 	= new_node;
	new_node->next_data			= next_node;

	nd->len += bytes;
	
	return B_OK;

error2:
	g_memory_pool->delete_pool_node(g_datas_nodes_pool, new_node);
error1:
	g_memory_pool->delete_pool_node(g_datas_nodes_pool, split_node);
	return B_ERROR;
}


// --------------------------------------------------
status_t remove_data(net_data *nd, uint32 offset, uint32 bytes)
{
#if 0
	// TODO!!!

	net_data_node *	ndn;
	net_data_node *	start_node;
	net_data_node *	end_node;
	uint32			start_node_offset;
	uint32			end_node_offset;
	net_data_node *	start_split_node;
	net_data_node *	end_split_node;
	uint8 *			data;
	
	if (bytes < 1)
		return B_BAD_VALUE;

	/*
	Possibles cases:
	[XXXXXXX|++++++]....	Triming at start of node (start and/or end nodes)
    [+++|XXXXXX|+++]....    Triming in the middle of *one* node
	[+++++|XXXXXXXX]....    Triming at the end of start node
	...[XXXXXXXXXXXXXX].... Triming one full node
	*/

	start_node = find_data_node(nd, offset, &start_node_offset, NULL);
	if (! start_node)
		return B_BAD_VALUE;

	end_node = find_data_node(nd, offset + bytes, &end_node_offset, NULL);
	if (! end_node)
		return B_BAD_VALUE;

	if ( (start_node == end_node) && 
		 (start_node_offset > 0) && (end_node_offset != end_node->len) )
		 // need to split one node into two parts, left & right, to be able to 
		 // trim data in the middle:
		 // [++++|XXXXXX|+++++++]
		 return B_OK;
	};

	// if the trimmed part is in one node, we know now that it's:
	// |++++++|XXXXXXXXXXXX|, or:
	// |XXXXXXXXX|+++++++++|

	if (start_node_offset)
		start_node->len = start_node_offset;

	if (end_node_offset) {
		data = (uint8 *) end_node->data;
		data += end_node_offset;
		end_node->data = data;
		end_node->len -= end_node_offset;
	};

		// start node must be split
		split = (uint8 *) start_node->data;
		split += start_node_offset;
		start_split_node = new_data_node(split, start_node->len - start_node_offset);
		if (! start_split_node)
			goto error1;
	};

	if (end_node_offset) {
		// end node must be split
		split = (uint8 *) end_node->data;
		split += end_node_offset;
		end_split_node = new_data_node(split, end_node->len - end_node_offset);
		if (! end_split_node)
			goto error2;
	};

	if (start_split_node) {
	};

	ndn = start_node->next_data;
	while (ndn->next_data != end_node) {
		if (ndn->next_data == end_node)
			break;

		ndn = ndn->next_data;
	};
	!= end_node

	if (end_split_node) {
	};


		// split start_node data chunk in two parts
		start_node->len = start_node_offset;
		
		start_split_node->free_func = NULL;

	};
	


	ndn->len = offset_in_node;
	
	split_node->free_func = NULL;	// as split_node data comes from 'ndn', we don't 
									// free this node data but only 'ndn' one...

	// create a new node to host inserted data
	new_node = new_data_node(data, bytes);
	if (! new_node)
		goto error2;
	
	new_node->free_func		= freethis; // can be NULL = don't free this chunk
	new_node->free_cookie	= NULL;	
	
	// add these nodes to node_list (nodes order don't matter in this list, so we do it the easy way :-)
	split_node->next 	= nd->node_list;
	new_node->next 	= split_node;
	nd->node_list 		= new_node;

	// Insert this new_node this node to the end of data_list
	split_node->next_data = ndn->next_data;
	ndn->next_data = new_node;
	new_node->next_data = split_node;

	nd->len -= bytes;
	
	return B_OK;

error2:
	g_memory_pool->delete_pool_node(g_datas_nodes_pool, end_split_node);
error1:
	if (start_split_node)
		g_memory_pool->delete_pool_node(g_datas_nodes_pool, start_split_node);
#endif
	return B_ERROR;
}


// --------------------------------------------------
uint32 copy_from_data(net_data *data, uint32 offset, void *copyinto, uint32 bytes)
{
	net_data_node 	*node;
	uint32			offset_in_node;
	uint32			len;
	uint32			chunk_len;
	uint8 			*from;
	uint8 			*to;
	
	to = (uint8 *) copyinto;
	len = 0;

	node = find_data_node(data, offset, &offset_in_node, NULL);
	while (node) {
		from  = (uint8 *) node->data;
		from += offset_in_node;
		chunk_len = min((node->len - offset_in_node), (bytes - len));
		
		memcpy(to, from, chunk_len);

		len += chunk_len;
		if (len >= bytes)
			break;

		to += chunk_len;
		offset_in_node = 0; // only the first node
		
		node = node->next_data;	// next node
	};
		
	return len;	
}

//	#pragma mark [data(s) queues functions]

// --------------------------------------------------
net_data_queue * new_data_queue(size_t max_bytes)
{
	net_data_queue *queue;
	
	if (! g_datas_queues_pool)
		g_datas_queues_pool = g_memory_pool->new_pool(sizeof(net_data_queue), DATA_QUEUES_PER_POOL);
			
	if (! g_datas_queues_pool)
		return NULL;
	
	queue = (net_data_queue *) g_memory_pool->new_pool_node(g_datas_queues_pool);
	if (! queue)
		return NULL;
	
	create_benaphore(&queue->lock, "net_data_queue lock");
	queue->sync = create_sem(0, "net_data_queue sem");
	
	queue->max_bytes 		= max_bytes;
	queue->current_bytes 	= 0;
	
	queue->head = queue->tail = NULL;
	
	return queue;
}


// --------------------------------------------------
status_t	delete_data_queue(net_data_queue *queue)
{
	status_t status;
	
	if (! queue)
		return B_BAD_VALUE;
		
	if (! g_datas_queues_pool)
		// Uh? From where come this queue then!?!
		return B_ERROR;

	// free the net_data's (sill) in this queue
	status = empty_data_queue(queue);
	if (status != B_OK)
		return status;
	
	delete_sem(queue->sync);
	delete_benaphore(&queue->lock);

	return g_memory_pool->delete_pool_node(g_datas_queues_pool, queue);
}


// --------------------------------------------------
status_t	empty_data_queue(net_data_queue *queue)
{
	net_data *data;
	net_data *tmp = NULL;
		
	if (! queue)
		return B_BAD_VALUE;
	
	lock_benaphore(&queue->lock);
	
	data = queue->head;
	while (data) {
		tmp = data;
		data = data->next;
		delete_data(data, true);
	};
	
	queue->head = NULL;
	queue->tail = NULL;

	delete_sem(queue->sync);
	queue->sync = create_sem(0, "net_data_queue sem");
	
	unlock_benaphore(&queue->lock);
	return B_OK;
}


// --------------------------------------------------
status_t enqueue_data(net_data_queue *queue, net_data *data)
{
	if (! queue)
		return B_BAD_VALUE;
	
	if (! data)
		return B_BAD_VALUE;
	
	data->next = NULL;
	
/*
	if (queue->current_bytes + data->len > queue->max_bytes)
		// what to do? dump some enqueued data(s) to free space? drop the new net_data?
		// TODO: dequeue enought net_data(s) to free space to queue this one...
		NULL;
*/
	lock_benaphore(&queue->lock);
		
	if (! queue->head)
		queue->head = data;
	if (queue->tail)
		queue->tail->next = data;
	queue->tail =  data;
	
	queue->current_bytes += data->len;

	unlock_benaphore(&queue->lock);
	
	return release_sem_etc(queue->sync, 1, B_DO_NOT_RESCHEDULE);
}


// --------------------------------------------------
size_t  dequeue_data(net_data_queue *queue, net_data **data, bigtime_t timeout, bool peek)
{
	status_t	status;
	net_data	*head_data;
	
	if (! queue)
		return B_BAD_VALUE;

	status = acquire_sem_etc(queue->sync, 1, B_RELATIVE_TIMEOUT, timeout);
	if (status != B_OK)
		return status;
	
	lock_benaphore(&queue->lock);
	
	head_data = queue->head;
	
	if (! peek) {
		// detach the head net_data from this fifo
		queue->head = head_data->next;
		if (queue->tail == head_data)
			queue->tail = queue->head;

		queue->current_bytes -= head_data->len;

		head_data->next = NULL;	// we never know :-)
	};
	
	unlock_benaphore(&queue->lock);
	
	*data = head_data;
	
	return head_data->len;
}	


//	#pragma mark [Helper functions]

// --------------------------------------------------
static net_data_node * new_data_node(const void *data, uint32 len)
{
	net_data_node *	node;
	
	if (! g_datas_nodes_pool)
		g_datas_nodes_pool = g_memory_pool->new_pool(sizeof(net_data_node), DATA_NODES_PER_POOL);
			
	if (! g_datas_nodes_pool)
		return NULL;
	
	node = (net_data_node *) g_memory_pool->new_pool_node(g_datas_nodes_pool);
	
	node->data = data;
	node->len  = len;

	node->ref_count = 1;

	node->free_cookie	= NULL;
	node->free_func		= NULL;

	node->next		= NULL;
	node->next_data	= NULL;

	return node;
}


// --------------------------------------------------
static net_data_node * find_data_node(net_data *data, uint32 offset, uint32 *offset_in_node, net_data_node **previous_node)
{
	net_data_node *previous;
	net_data_node *node;
	uint32			len;
	
	if (! data)
		return NULL;
	
	if (data->len <= offset)
		// this net_data don't hold enough data to reach this offset!
		return NULL;
	
	len = 0;
	node = data->data_list;
	previous = NULL;
	while (node)	{
		len += node->len;

		if(offset < len)
			break;
		
		previous = node;
		node = node->next_data;
	};

	if (offset_in_node)
		*offset_in_node = node->len - (len - offset);	
	if (previous_node)
		*previous_node = previous;

	return node;
}


// --------------------------------------------------
void dump_memory
	(
	const char *	prefix,
	const void *	data,
	uint32			len
	)
{
	uint32	i,j;
  	char	text[96];	// only 3*16 + 16 max by line needed
	uint8 *	byte;
	char *	ptr;

	byte = (uint8 *) data;

	for ( i = 0; i < len; i += 16 )
		{
		ptr = text;

      	for ( j = i; j < i+16 ; j++ )
			{
			if ( j < len )
				sprintf(ptr, "%02x ",byte[j]);
			else
				sprintf(ptr, "   ");
			ptr += 3;
			};
			
		for (j = i; j < len && j < i+16;j++)
			{
			if ( byte[j] >= ' ' && byte[j] <= 0x7e )
				*ptr = byte[j];
			else
				*ptr = '.';
				
			ptr++;
			};
		*ptr = '\n';
		ptr++;
		*ptr = '\0';
		
		if (prefix)
			DPRINTF(prefix);
		DPRINTF(text);

		// next line
		};
}


// --------------------------------------------------
void dump_data(net_data *data)
{
	net_data_node * node;

	if (! data)
		return;

	DPRINTF("---- net_data %p: total len %ld\n", data, data->len);
	
	DPRINTF("data_list:\n");
	node = data->data_list;
	while (node) {
		DPRINTF("  * node %p: data %p, len %ld\n", node, node->data, node->len);
		dump_memory("      ", node->data, node->len);
		DPRINTF("    next: %p\n", node->next_data);

		node = node->next_data;
	};

	DPRINTF("node_list:\n");
	node = data->node_list;
	while (node) {
		DPRINTF("  * node %p: data %p, len %ld, ref_count %ld\n", node, node->data, node->len, node->ref_count);
		if (node->free_func)
			DPRINTF("    free_func: %p, free_cookie %p\n", node->free_func, node->free_cookie);
		DPRINTF("    next: %p\n", node->next);

		node = node->next;
	};

}

// #pragma mark [data Purgatory functions]

// --------------------------------------------------
static int32 datas_purgatory_thread(void *data)
{
	while (true) {
		if (acquire_sem(g_datas_purgatory_sync) != B_OK)
			break;
		
		// okay, time to cleanup some net_data(s)
		g_memory_pool->for_each_pool_node(g_datas_pool, data_death, NULL);
	};
	return 0;
}


// --------------------------------------------------
static status_t data_death(memory_pool * pool, void * node, void * cookie)
{
	net_data *data;
	
	data = (net_data *) node;
	if (data->flags & DATA_TO_FREE)
		delete_data(data, false);
		
	return 0; // B_OK;
}


// --------------------------------------------------
static status_t data_queue_death(memory_pool * pool, void * node, void * cookie)
{
	return delete_data_queue((net_data_queue *) node);
}

