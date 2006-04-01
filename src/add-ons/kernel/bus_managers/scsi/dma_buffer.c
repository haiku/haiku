/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	DMA buffer handling.

	If the peripheral driver hasn't made sure that the data of a request 
	is DMA safe, we check that and copy data to a buffer if needed.
	The buffer is enlarged on demand and destroyed after a time-out
	by a daemon. Obviously, it's a good idea to avoid all this, therefore 
	blkman takes care of that for read/write requests.
	
	To be able to copy data back after the request was finished, we need a 
	S/G list to the original data as the copying is done in a different
	thread/process context (namely the service thread).
	
	Currently, there is only one buffer per device; in the future,
	we may support multiple buffers, especially if we want to support
	more then 4 GB memory, which leads to trouble with 32-bit PCI cards.
*/


#include "scsi_internal.h"
#include "KernelExport_ext.h"

#include <vm.h>

#include <string.h>


/**	check whether S/G list of request is supported DMA controller */

static bool
is_sg_list_dma_safe(scsi_ccb *request) 
{
	scsi_bus_info *bus = request->bus;
	const physical_entry *sg_list = request->sg_list;
	uint32 sg_count = request->sg_cnt;
	uint32 dma_boundary = bus->dma_params.dma_boundary;
	uint32 alignment = bus->dma_params.alignment;
	uint32 max_sg_block_size = bus->dma_params.max_sg_block_size;
	uint32 cur_idx;

	// not too many S/G list entries
	if (sg_count > bus->dma_params.max_sg_blocks) {
		SHOW_FLOW0(1, "S/G-list too long");
		return false;
	}
		
	// if there are no further restrictions - be happy
	if (dma_boundary == ~0UL && alignment == 0 && max_sg_block_size == 0)
		return true;

	// argh - controller is a bit picky, so make sure he likes us
	for (cur_idx = sg_count; cur_idx >= 1; --cur_idx, ++sg_list) {
		addr_t max_len;

		// calculate space upto next dma boundary crossing and
		// verify that it isn't crossed
		max_len = (dma_boundary + 1) - 
			((addr_t)sg_list->address & dma_boundary);

		if (max_len < sg_list->size) {
			SHOW_FLOW(0, "S/G-entry crosses DMA boundary @0x%x", 
				(int)sg_list->address + (int)max_len);
			return false;
		}

		// check both begin and end of entry for alignment
		if (((addr_t)sg_list->address & alignment) != 0) {
			SHOW_FLOW(0, "S/G-entry has bad alignment @0x%x",
				(int)sg_list->address);
			return false;
		}

		if ((((addr_t)sg_list->address + sg_list->size) & alignment) != 0) {
			SHOW_FLOW(0, "end of S/G-entry has bad alignment @0x%x",
				(int)sg_list->address + (int)sg_list->size);
			return false;
		}

		// verify entry size
		if (sg_list->size > max_sg_block_size) {
			SHOW_FLOW(0, "S/G-entry is too long (%d/%d bytes)",
				(int)sg_list->size, (int)max_sg_block_size);
			return false;
		}
	}

	return true;
}


/** copy data from/to DMA buffer */

static bool
scsi_copy_dma_buffer(scsi_ccb *request, uint32 size, bool to_buffer)
{
	dma_buffer *buffer = request->dma_buffer;
	const physical_entry *sg_list = buffer->sg_list_orig;
	uint32 num_vecs = buffer->sg_cnt_orig;
	char *buffer_data = buffer->address;

	SHOW_FLOW(1, "to_buffer=%d, %d bytes", to_buffer, (int)size);

	// survive even if controller returned invalid data size
	size = min(size, request->data_len);

	// we have to use S/G list to original data; the DMA buffer
	// was allocated in kernel and is thus visible even if the thread
	// was changed
	for (; size > 0 && num_vecs > 0; ++sg_list, --num_vecs) {
		addr_t virtualAddress;
		size_t bytes;
		
		bytes = min( size, sg_list->size );

		if (vm_get_physical_page((addr_t)sg_list->address, &virtualAddress,
				PHYSICAL_PAGE_CAN_WAIT) != B_OK)
			return false;

		if (to_buffer)
			memcpy(buffer_data, (void *)virtualAddress, bytes);
		else
			memcpy((void *)virtualAddress, buffer_data, bytes);

		vm_put_physical_page(virtualAddress);

		buffer_data += bytes;
	}
	
	return true;
}


static int
log2(uint32 x)
{
	int y;

	for (y = 31; y >= 0; --y) {
		if (x == ((uint32)1 << y))
			break;
	}

	return y;
}


static void
scsi_free_dma_buffer(dma_buffer *buffer)
{
	if (buffer->area > 0) {
		SHOW_FLOW0(1, "Destroying buffer");

		delete_area(buffer->area);
		buffer->area = 0;
		buffer->size = 0;
	}

	if (buffer->sg_list_area > 0) {
		delete_area(buffer->sg_list_area);
		buffer->sg_list_area = 0;
	}
}


/**	allocate dma buffer for given device, deleting old one
 *	size - buffer size in bytes
 */

static bool
scsi_alloc_dma_buffer(dma_buffer *buffer, dma_params *dma_params, uint32 size)
{
	size_t sg_list_size, sg_list_entries;
	
	// free old buffer first	
	scsi_free_dma_buffer( buffer );

	// just in case alignment is redicuously huge
	size = (size + dma_params->alignment) & ~dma_params->alignment;
	
	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	
	// calculate worst case number of S/G entries, i.e. if they are non-continuous;
	// there is a controller limit and a limit by our own S/G manager to check
	if (size / B_PAGE_SIZE > dma_params->max_sg_blocks
		|| size / B_PAGE_SIZE > MAX_TEMP_SG_FRAGMENTS) {
		uint32 boundary = dma_params->dma_boundary;
		uchar *dma_buffer_address_unaligned;

		// alright - a contiguous buffer is required to keep S/G table short
		SHOW_INFO(1, "need to setup contiguous DMA buffer of size %d", 
			(int)size);

		// verify that we don't get problems with dma boundary
		if (boundary != ~0UL) {
			if (size > boundary + 1) {
				SHOW_ERROR(2, "data is longer then maximum DMA transfer len (%d/%d bytes)", 
					(int)size, (int)boundary + 1);
				return false;
			}

			// round up to next power of two and allocate a buffer double the 
			// needed size so we can cut out an area that doesn't cross 
			// dma boundary
			size = (1 << log2( size )) * 2;
		}

		buffer->area = create_area("DMA buffer", 
			(void **)&dma_buffer_address_unaligned, 
			B_ANY_KERNEL_ADDRESS, size,
			B_FULL_LOCK | B_CONTIGUOUS, 0 );
		if (buffer->area < 0) {
			SHOW_ERROR(2, "Cannot create contignous DMA buffer of %d bytes", 
				(int)size);
			return false;
		}

		if (boundary != ~0UL) {
			uchar *next_boundary;

			// boundary case: cut out piece aligned on "size"
			buffer->address = (uchar *)(
				((addr_t)dma_buffer_address_unaligned + size - 1) & ~(size - 1));

			// determine how many bytes are available until next DMA boundary
			next_boundary = (uchar *)(((addr_t)buffer->address + boundary - 1) &
				~(boundary - 1));

			// adjust next boundary if outside allocated area
			if( next_boundary > dma_buffer_address_unaligned + size )
				next_boundary = dma_buffer_address_unaligned + size;
				
			buffer->size = next_boundary - buffer->address;
		} else {
			// non-boundary case: use buffer directly
			buffer->address = dma_buffer_address_unaligned;
			buffer->size = size;
		}
	} else {
		// we can live with a fragmented buffer - very nice
		buffer->area = create_area( "DMA buffer", 
			(void **)&buffer->address, 
			B_ANY_KERNEL_ADDRESS, size,
			B_FULL_LOCK, 0 );
		if (buffer->area < 0) {
			SHOW_ERROR(2, "Cannot create DMA buffer of %d bytes", 
				(int)size);
			return false;
		}
		
		buffer->size = size;
	}

	// create S/G list	
	// worst case is one entry per page, and size is page-aligned
	sg_list_size = buffer->size / B_PAGE_SIZE * sizeof( physical_entry );
	// create_area has page-granularity
	sg_list_size = (sg_list_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	
	buffer->sg_list_area = create_area("DMA buffer S/G table",
		(void **)&buffer->sg_list,
		B_ANY_KERNEL_ADDRESS, sg_list_size,
		B_FULL_LOCK, 0);
	if (buffer->sg_list_area < 0) {
		SHOW_ERROR( 2, "Cannot craete DMA buffer S/G list of %d bytes",
			(int)sg_list_size );

		delete_area(buffer->area);
		buffer->area = 0;
		return false;
	}
	
	sg_list_entries = sg_list_size / sizeof( physical_entry );

	{
		size_t mapped_len;
		status_t res;
		iovec vec = {
			buffer->address,
			buffer->size
		};
		
		res = get_iovec_memory_map( 
			&vec, 1, 0, buffer->size, 
			buffer->sg_list, sg_list_entries, &buffer->sg_cnt, 
			&mapped_len );
			
		if( res != B_OK || mapped_len != buffer->size ) {
			SHOW_ERROR(0, "Error creating S/G list for DMA buffer (%s; wanted %d, got %d bytes)",
				strerror(res), (int)mapped_len, (int)buffer->size);
		}
	}

	return true;
}


static void
scsi_free_dma_buffer_sg_orig(dma_buffer *buffer)
{
	if (buffer->sg_orig > 0) {
		delete_area(buffer->sg_orig);
		buffer->sg_orig = 0;
		buffer->sg_cnt_max_orig = 0;
	}
}


/** allocate S/G list to original data */

static bool
scsi_alloc_dma_buffer_sg_orig(dma_buffer *buffer, int size)
{
	// free old list first	
	scsi_free_dma_buffer_sg_orig(buffer);

	size = (size * sizeof(physical_entry) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	buffer->sg_orig = create_area("S/G to original data", 
		(void **)&buffer->sg_list_orig, 
		B_ANY_KERNEL_ADDRESS, size,
		B_NO_LOCK, 0);
	if (buffer->sg_orig < 0) {
		SHOW_ERROR(2, "Cannot S/G list buffer to original data of %d bytes", 
			(int)size);
		return false;
	}

	buffer->sg_cnt_max_orig = size / sizeof(physical_entry);

	SHOW_INFO(3, "Got up to %d S/G entries to original data",
		(int)buffer->sg_cnt_max_orig);

	return true;
}


/** helper: dump S/G table */

static void
dump_sg_table(const physical_entry *sg_list,
	uint32 sg_list_count)
{
	uint32 cur_idx;

	SHOW_FLOW(1, "count=%d", (int)sg_list_count);

	for (cur_idx = sg_list_count; cur_idx >= 1; --cur_idx, ++sg_list) {
		SHOW_FLOW(1, "addr=%x, size=%d", (int)sg_list->address,
			(int)sg_list->size);
	}
}


/**	compose S/G list to original data of request */

static bool
scsi_dma_buffer_compose_sg_orig(dma_buffer *buffer, scsi_ccb *request)
{
	// enlarge buffer is required
	if (buffer->sg_cnt_max_orig < request->sg_cnt) {
		if (!scsi_alloc_dma_buffer_sg_orig(buffer, request->sg_cnt))
			return false;
	}

	SHOW_FLOW0(1, "copy S/G list");

	memcpy(buffer->sg_list_orig, request->sg_list, 
		request->sg_cnt * sizeof(physical_entry));

	buffer->sg_cnt_orig = request->sg_cnt;
	return true;		
}


/**	init DMA buffer and copy data to it if required
 *	note: S/G list of request must already be setup
 */

bool
scsi_get_dma_buffer(scsi_ccb *request)
{
	scsi_device_info *device = request->device;
	dma_buffer *buffer;
	
	request->buffered = false;

	// perhaps we have luck and no buffering is needed
	if( is_sg_list_dma_safe( request ))		
		return true;

	SHOW_FLOW0(1, "Buffer is not DMA safe" );

	dump_sg_table(request->sg_list, request->sg_cnt);

	// only one buffer at a time	
	acquire_sem(device->dma_buffer_owner);

	// make sure, clean-up daemon doesn't bother us
	ACQUIRE_BEN(&device->dma_buffer_lock);

	// there is only one buffer, so no further management
	buffer = &device->dma_buffer;
	
	buffer->inuse = true;	
	
	RELEASE_BEN(&device->dma_buffer_lock);

	// memorize buffer for cleanup
	request->dma_buffer = buffer;

	// enlarge buffer if too small
	if (buffer->size < request->data_len) {
		if (!scsi_alloc_dma_buffer(buffer, &device->bus->dma_params,
				request->data_len))
			goto err;
	}

	// create S/G to original data (necessary for copying from-buffer on end
	// of request, but also used during copying to-buffer in a second because
	// of lazyness)
	scsi_dma_buffer_compose_sg_orig(&device->dma_buffer, request);

	// copy data to buffer
	if ((request->flags & SCSI_DIR_MASK) == SCSI_DIR_OUT) {
		if (!scsi_copy_dma_buffer( request, request->data_len, true))
			goto err;
	}

	// replace data address, so noone notices that a buffer is used
	buffer->orig_data = request->data;
	buffer->orig_sg_list = request->sg_list;
	buffer->orig_sg_cnt = request->sg_cnt;
	
	request->data = buffer->address;
	request->sg_list = buffer->sg_list;
	request->sg_cnt = buffer->sg_cnt;

	SHOW_INFO(1, "bytes: %d", (int)request->data_len);
	SHOW_INFO0(3, "we can start now");

	request->buffered = true;
	return true;

err:
	SHOW_INFO0(3, "error setting up DMA buffer");

	ACQUIRE_BEN(&device->dma_buffer_lock);

	// some of this is probably not required, but I'm paranoid
	buffer->inuse = false;

	RELEASE_BEN(&device->dma_buffer_lock);
	release_sem(device->dma_buffer_owner);

	return false;	
}


/**	copy data back and release DMA buffer;
 *	you must have called cleanup_tmp_sg before
 */

void
scsi_release_dma_buffer(scsi_ccb *request)
{
	scsi_device_info *device = request->device;
	dma_buffer *buffer = request->dma_buffer;
	
	SHOW_FLOW(1, "Buffering finished, %x, %x", 
		request->subsys_status & SCSI_SUBSYS_STATUS_MASK,
		(int)(request->flags & SCSI_DIR_MASK));

	// copy data from buffer if required and if operation succeeded
	if ((request->subsys_status & SCSI_SUBSYS_STATUS_MASK) == SCSI_REQ_CMP
		&& (request->flags & SCSI_DIR_MASK) == SCSI_DIR_IN)
		scsi_copy_dma_buffer(request, request->data_len - request->data_resid, false );
		
	// restore request	
	request->data = buffer->orig_data;
	request->sg_list = buffer->orig_sg_list;
	request->sg_cnt = buffer->orig_sg_cnt;
	
	// free buffer
	ACQUIRE_BEN(&device->dma_buffer_lock);

	buffer->last_use = system_time();
	buffer->inuse = false;

	RELEASE_BEN(&device->dma_buffer_lock);

	release_sem(device->dma_buffer_owner);

	request->buffered = false;
}


/** dameon that deletes DMA buffer if not used for some time */

void
scsi_dma_buffer_daemon(void *dev, int counter)
{
	scsi_device_info *device = dev;
	dma_buffer *buffer;

	ACQUIRE_BEN(&device->dma_buffer_lock);

	buffer = &device->dma_buffer;

	if (!buffer->inuse
		&& buffer->last_use - system_time() > SCSI_DMA_BUFFER_CLEANUP_DELAY) {
		scsi_free_dma_buffer(buffer);
		scsi_free_dma_buffer_sg_orig(buffer);
	}

	RELEASE_BEN(&device->dma_buffer_lock);
}


void
scsi_dma_buffer_free(dma_buffer *buffer)
{
	scsi_free_dma_buffer(buffer);
	scsi_free_dma_buffer_sg_orig(buffer);
}


void
scsi_dma_buffer_init(dma_buffer *buffer)
{
	buffer->area = 0;
	buffer->size = 0;
	buffer->sg_orig = 0;
	buffer->sg_cnt_max_orig = 0;
}
