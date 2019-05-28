/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
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

#include <vm/vm.h>

#include <string.h>


/*!	Check whether S/G list of request is supported DMA controller */
static bool
is_sg_list_dma_safe(scsi_ccb *request)
{
	scsi_bus_info *bus = request->bus;
	const physical_entry *sg_list = request->sg_list;
	uint32 sg_count = request->sg_count;
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
	if (dma_boundary == ~(uint32)0 && alignment == 0 && max_sg_block_size == 0)
		return true;

	// argh - controller is a bit picky, so make sure he likes us
	for (cur_idx = sg_count; cur_idx >= 1; --cur_idx, ++sg_list) {
		phys_addr_t max_len;

		// calculate space upto next dma boundary crossing and
		// verify that it isn't crossed
		max_len = (dma_boundary + 1) - (sg_list->address & dma_boundary);

		if (max_len < sg_list->size) {
			SHOW_FLOW(0, "S/G-entry crosses DMA boundary @%" B_PRIxPHYSADDR,
				sg_list->address + max_len);
			return false;
		}

		// check both begin and end of entry for alignment
		if ((sg_list->address & alignment) != 0) {
			SHOW_FLOW(0, "S/G-entry has bad alignment @%#" B_PRIxPHYSADDR,
				sg_list->address);
			return false;
		}

		if (((sg_list->address + sg_list->size) & alignment) != 0) {
			SHOW_FLOW(0, "end of S/G-entry has bad alignment @%" B_PRIxPHYSADDR,
				sg_list->address + sg_list->size);
			return false;
		}

		// verify entry size
		if (sg_list->size > max_sg_block_size) {
			SHOW_FLOW(0, "S/G-entry is too long (%" B_PRIuPHYSADDR "/%" B_PRIu32
				" bytes)", sg_list->size, max_sg_block_size);
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
	uint32 num_vecs = buffer->sg_count_orig;
	uchar *buffer_data = buffer->address;

	SHOW_FLOW(1, "to_buffer=%d, %" B_PRIu32 " bytes", to_buffer, size);

	// survive even if controller returned invalid data size
	size = min_c(size, request->data_length);

	// we have to use S/G list to original data; the DMA buffer
	// was allocated in kernel and is thus visible even if the thread
	// was changed
	for (; size > 0 && num_vecs > 0; ++sg_list, --num_vecs) {
		size_t bytes;

		bytes = min_c( size, sg_list->size );

		if (to_buffer) {
			vm_memcpy_from_physical(buffer_data, sg_list->address, bytes,
				false);
		} else
			vm_memcpy_to_physical(sg_list->address, buffer_data, bytes, false);

		buffer_data += bytes;
	}

	return true;
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
	// free old buffer first
	scsi_free_dma_buffer(buffer);

	// just in case alignment is ridiculously huge
	size = (size + dma_params->alignment) & ~dma_params->alignment;

	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// calculate worst case number of S/G entries, i.e. if they are non-continuous;
	// there is a controller limit and a limit by our own S/G manager to check
	if (size / B_PAGE_SIZE > dma_params->max_sg_blocks
		|| size / B_PAGE_SIZE > MAX_TEMP_SG_FRAGMENTS) {
		uint32 boundary = dma_params->dma_boundary;

		// alright - a contiguous buffer is required to keep S/G table short
		SHOW_INFO(1, "need to setup contiguous DMA buffer of size %" B_PRIu32,
			size);

		// verify that we don't get problems with dma boundary
		if (boundary != ~(uint32)0) {
			if (size > boundary + 1) {
				SHOW_ERROR(2, "data is longer then maximum DMA transfer len (%"
					 B_PRId32 "/%" B_PRId32 " bytes)", size, boundary + 1);
				return false;
			}
		}

		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions = {};
		if (dma_params->alignment != ~(uint32)0)
			physicalRestrictions.alignment = dma_params->alignment + 1;
		if (boundary != ~(uint32)0)
			physicalRestrictions.boundary = boundary + 1;
#if B_HAIKU_PHYSICAL_BITS > 32
		physicalRestrictions.high_address = 0x100000000ULL;
			// TODO: Use 64 bit addresses, if possible!
#endif
		buffer->area = create_area_etc(B_SYSTEM_TEAM, "DMA buffer", size,
			B_CONTIGUOUS, 0, 0, 0, &virtualRestrictions, &physicalRestrictions,
			(void**)&buffer->address);

		if (buffer->area < 0) {
			SHOW_ERROR(2, "Cannot create contignous DMA buffer of %" B_PRIu32
				" bytes", size);
			return false;
		}

		buffer->size = size;
	} else {
		// we can live with a fragmented buffer - very nice
		buffer->area = create_area("DMA buffer",
			(void **)&buffer->address, B_ANY_KERNEL_ADDRESS, size,
			B_32_BIT_FULL_LOCK, 0);
				// TODO: Use B_FULL_LOCK, if possible!
		if (buffer->area < 0) {
			SHOW_ERROR(2, "Cannot create DMA buffer of %" B_PRIu32 " bytes",
				size);
			return false;
		}

		buffer->size = size;
	}

	// create S/G list
	// worst case is one entry per page, and size is page-aligned
	size_t sg_list_size = buffer->size / B_PAGE_SIZE * sizeof( physical_entry );
	// create_area has page-granularity
	sg_list_size = (sg_list_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	buffer->sg_list_area = create_area("DMA buffer S/G table",
		(void **)&buffer->sg_list, B_ANY_KERNEL_ADDRESS, sg_list_size,
		B_32_BIT_FULL_LOCK, 0);
			// TODO: Use B_FULL_LOCK, if possible!
	if (buffer->sg_list_area < 0) {
		SHOW_ERROR( 2, "Cannot create DMA buffer S/G list of %" B_PRIuSIZE
			" bytes", sg_list_size );

		delete_area(buffer->area);
		buffer->area = 0;
		return false;
	}

	size_t sg_list_entries = sg_list_size / sizeof(physical_entry);

	{
		size_t mapped_len;
		status_t res;
		iovec vec = {
			buffer->address,
			buffer->size
		};

		res = get_iovec_memory_map(
			&vec, 1, 0, buffer->size,
			buffer->sg_list, sg_list_entries, &buffer->sg_count,
			&mapped_len );

		if( res != B_OK || mapped_len != buffer->size ) {
			SHOW_ERROR(0, "Error creating S/G list for DMA buffer (%s; wanted "
				"%" B_PRIuSIZE ", got %" B_PRIuSIZE " bytes)", strerror(res),
				mapped_len, buffer->size);
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
		buffer->sg_count_max_orig = 0;
	}
}


/** allocate S/G list to original data */

static bool
scsi_alloc_dma_buffer_sg_orig(dma_buffer *buffer, size_t size)
{
	// free old list first
	scsi_free_dma_buffer_sg_orig(buffer);

	size = (size * sizeof(physical_entry) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	buffer->sg_orig = create_area("S/G to original data",
		(void **)&buffer->sg_list_orig,
		B_ANY_KERNEL_ADDRESS, size,
		B_NO_LOCK, 0);
	if (buffer->sg_orig < 0) {
		SHOW_ERROR(2, "Cannot S/G list buffer to original data of %" B_PRIuSIZE
			" bytes", size);
		return false;
	}

	buffer->sg_count_max_orig = size / sizeof(physical_entry);

	SHOW_INFO(3, "Got up to %" B_PRIu32 " S/G entries to original data",
		buffer->sg_count_max_orig);

	return true;
}


/*! dump S/G table */
static void
dump_sg_table(const physical_entry *sg_list,
	uint32 sg_list_count)
{
	uint32 cur_idx;

	SHOW_FLOW(1, "count=%" B_PRIu32, sg_list_count);

	for (cur_idx = sg_list_count; cur_idx >= 1; --cur_idx, ++sg_list) {
		SHOW_FLOW(1, "addr=%" B_PRIxPHYSADDR ", size=%" B_PRIuPHYSADDR,
			sg_list->address, sg_list->size);
	}
}


/**	compose S/G list to original data of request */

static bool
scsi_dma_buffer_compose_sg_orig(dma_buffer *buffer, scsi_ccb *request)
{
	// enlarge buffer is required
	if (buffer->sg_count_max_orig < request->sg_count) {
		if (!scsi_alloc_dma_buffer_sg_orig(buffer, request->sg_count))
			return false;
	}

	SHOW_FLOW0(1, "copy S/G list");

	memcpy(buffer->sg_list_orig, request->sg_list,
		request->sg_count * sizeof(physical_entry));

	buffer->sg_count_orig = request->sg_count;
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

	dump_sg_table(request->sg_list, request->sg_count);

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
	if (buffer->size < request->data_length) {
		if (!scsi_alloc_dma_buffer(buffer, &device->bus->dma_params,
				request->data_length))
			goto err;
	}

	// create S/G to original data (necessary for copying from-buffer on end
	// of request, but also used during copying to-buffer in a second because
	// of lazyness)
	scsi_dma_buffer_compose_sg_orig(&device->dma_buffer, request);

	// copy data to buffer
	if ((request->flags & SCSI_DIR_MASK) == SCSI_DIR_OUT) {
		if (!scsi_copy_dma_buffer( request, request->data_length, true))
			goto err;
	}

	// replace data address, so noone notices that a buffer is used
	buffer->orig_data = request->data;
	buffer->orig_sg_list = request->sg_list;
	buffer->orig_sg_count = request->sg_count;

	request->data = buffer->address;
	request->sg_list = buffer->sg_list;
	request->sg_count = buffer->sg_count;

	SHOW_INFO(1, "bytes: %" B_PRIu32, request->data_length);
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


/*!	Copy data back and release DMA buffer;
	you must have called cleanup_tmp_sg before
*/
void
scsi_release_dma_buffer(scsi_ccb *request)
{
	scsi_device_info *device = request->device;
	dma_buffer *buffer = request->dma_buffer;

	SHOW_FLOW(1, "Buffering finished, %x, %" B_PRIx32,
		request->subsys_status & SCSI_SUBSYS_STATUS_MASK,
		(request->flags & SCSI_DIR_MASK));

	// copy data from buffer if required and if operation succeeded
	if ((request->subsys_status & SCSI_SUBSYS_STATUS_MASK) == SCSI_REQ_CMP
		&& (request->flags & SCSI_DIR_MASK) == SCSI_DIR_IN)
		scsi_copy_dma_buffer(request, request->data_length - request->data_resid, false);

	// restore request
	request->data = buffer->orig_data;
	request->sg_list = buffer->orig_sg_list;
	request->sg_count = buffer->orig_sg_count;

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
	scsi_device_info *device = (scsi_device_info*)dev;
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
	buffer->sg_count_max_orig = 0;
}
