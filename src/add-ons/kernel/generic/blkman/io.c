/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open block device manager

	Actual I/O.

	This is hardcode. Think twice before changing something
	in here. 

	Things could become a lot easier with the following restrictions:
	 - stricter data alignment that is sufficient for all controllers
	   (e.g. page-aligned should certainly be)
	 - no partial block access
	 - locked data
	 - always sufficient iovecs for entire transfer

	The last thing is a design problem of the devfs. The first two
	make sure that we need no buffer anymore, which would make much
	code unnecessary. The locked data would save much code too - having
	a sg list as input would make it even more sweeter.

	Obviously these restrictions cannot be enforced for user programs,
	but at least for transfers from/to disk cache, we could define
	extra functions with these properties.
*/


#include "blkman_int.h"
#include "KernelExport_ext.h"


/**	get sg list of iovecs, taking dma boundaries and maximum size of
 *	single s/g entry into account
 *	<vec_offset> must not point byond first iovec
 */

static int
blkman_map_iovecs(iovec *vec, size_t vec_count, size_t vec_offset, size_t len,
	phys_vecs *map, size_t max_phys_entries, size_t dma_boundary,
	size_t max_sg_block_size)
{
	status_t res;
	size_t total_len;
	size_t cur_idx;

	SHOW_FLOW0( 3, "" );

	if ((res = get_iovec_memory_map(vec, vec_count, vec_offset, len,
					map->vec, max_phys_entries, &map->num, &map->total_len)) < B_OK)
		return res;

	if (dma_boundary == ~0UL && max_sg_block_size >= map->total_len) 
		return B_OK;

	SHOW_FLOW(3, "Checking violation of dma boundary 0x%x and entry size 0x%x", 
		(int)dma_boundary, (int)max_sg_block_size);

	total_len = 0;

	for (cur_idx = 0; cur_idx < map->num; ++cur_idx) {
		addr_t max_len;

		// calculate space upto next dma boundary crossing
		max_len = (dma_boundary + 1) - ((addr_t)map->vec[cur_idx].address & dma_boundary);

		// restrict size per sg item
		max_len = min(max_len, max_sg_block_size);

		SHOW_FLOW(4, "addr=%p, size=%x, max_len=%x, idx=%d, num=%d", 
			map->vec[cur_idx].address, (int)map->vec[cur_idx].size,
			(int)max_len, (int)cur_idx, (int)map->num);

		if (max_len < map->vec[cur_idx].size) {
			// split sg block
			map->num = min(map->num + 1, max_phys_entries);
			memmove(&map->vec[cur_idx + 1], &map->vec[cur_idx], 
				(map->num - 1 - cur_idx) * sizeof(physical_entry));

			map->vec[cur_idx].size = max_len;
			map->vec[cur_idx + 1].address = (void *)((addr_t)map->vec[cur_idx + 1].address + max_len);
			map->vec[cur_idx + 1].size -= max_len;
		}

		total_len += map->vec[cur_idx].size;
	}

	// we really have to update total_len - due to block splitting,
	// some other blocks may got discarded if s/g list became too long
	map->total_len = total_len;
	return B_OK;
}


/**	check whether dma alignment restrictions are met
 *	returns true on success
 *	remark: if the user specifies too many iovecs and the unused iovecs
 *	        are mis-aligned, it's bad luck as we ignore this case and still
 *	        report an alignment problem
 */

static bool
blkman_check_alignment(struct iovec *vecs, uint num_vecs, 
	size_t vec_offset, uint alignment)
{
	if (alignment == 0)
		// no alignment - good boy
		return true;

	for (; num_vecs > 0; ++vecs, --num_vecs) {
		// check both begin and end of iovec
		if ((((addr_t)vecs->iov_base + vec_offset) & alignment) != 0) {
			SHOW_FLOW(1, "s/g entry not aligned (%p)", vecs->iov_base);
			return false;
		}

		if ((((addr_t)vecs->iov_base + vecs->iov_len) & alignment) != 0) {
			SHOW_FLOW(1, "end of s/g entry not aligned (%p)",
				(void *)((addr_t)vecs->iov_base + vecs->iov_len));
			return false;
		}

		vec_offset = 0;
	}

	return true;
}


/**	try to lock iovecs
 *	returns number of locked bytes
 *	(only entire iovecs are attempted to be locked)
 *	remark: if there are too few iovecs, you simply get fewer locked bytes
 */

static size_t
blkman_lock_iovecs(struct iovec *vecs, uint num_vecs, 
	size_t vec_offset, size_t len, int flags)
{
	size_t orig_len = len;

	SHOW_FLOW(3, "len = %lu", len);

	for (; len > 0 && num_vecs > 0; ++vecs, --num_vecs) {
		size_t lock_len;
		status_t res;

		lock_len = min(vecs->iov_len - vec_offset, len);

		SHOW_FLOW(3, "pos = %p, len = %lu", vecs->iov_base, vecs->iov_len);

		res = lock_memory((void *)((addr_t)vecs->iov_base + vec_offset), lock_len, flags);
		if (res != B_OK) {
			SHOW_FLOW(3, "cannot lock: %s", strerror(res));
			break;
		}

		len -= lock_len;
		vec_offset = 0;
	}

	SHOW_FLOW( 3, "remaining len=%lu", len);

	return orig_len - len;
}


/** unlock iovecs */

static void
blkman_unlock_iovecs(struct iovec *vecs, uint num_vecs, 
	size_t vec_offset, size_t len, int flags)
{
	SHOW_FLOW(3, "len = %lu", len);

	for (; len > 0; ++vecs, --num_vecs) {
		size_t lock_len;

		lock_len = min(vecs->iov_len - vec_offset, len);

		if (unlock_memory((void *)((addr_t)vecs->iov_base + vec_offset), lock_len, 
				flags) != B_OK)
			panic( "Cannot unlock previously locked memory!" );

		len -= lock_len;
		vec_offset = 0;
	}
}


/**	copy data from/to transfer buffer;
 *	remark: if iovecs are missing, copying is aborted
 */

static void
blkman_copy_buffer(char *buffer, struct iovec *vecs, uint num_vecs, 
	size_t vec_offset, size_t len, bool to_buffer)
{
	for (; len > 0 && num_vecs > 0; ++vecs, --num_vecs) {
		size_t bytes;

		bytes = min(len, vecs->iov_len - vec_offset);

		if (to_buffer)
			memcpy(buffer, (void *)((addr_t)vecs->iov_base + vec_offset), bytes);
		else
			memcpy((void *)((addr_t)vecs->iov_base + vec_offset), buffer, bytes);

		buffer += bytes;
		vec_offset = 0;
	}
}


/** determine number of bytes described by iovecs */

static size_t
blkman_iovec_len(struct iovec *vecs, uint num_vecs, size_t vec_offset)
{
	size_t len = 0;

	for (; num_vecs > 0; ++vecs, --num_vecs) {
		len += vecs->iov_len - vec_offset;
		vec_offset = 0;
	}

	return len;
}


/**	main beast to execute i/o transfer
 *	as <need_locking> and <write> is usual const, we really want to inline 
 *	it - this makes this function much smaller in a given instance
 *	<need_locking> - data must be locked before transferring it
 *	<vec> - should be locked, though it's probably not really necessary
 */

static inline status_t
blkman_readwrite(blkman_handle_info *handle, off_t pos, struct iovec *vec,
	int vec_count, size_t *total_len, bool need_locking, bool write)
{
	blkman_device_info *device = handle->device;
	uint32 block_size, ld_block_size;
	uint64 capacity;
	bool need_buffer;
	status_t res = B_OK;

	size_t len = *total_len;
	size_t orig_len = len;
	size_t vec_offset;

	phys_vecs *phys_vecs;

	SHOW_FLOW(3, "pos = %Ld, len = %lu, need_locking = %d, write = %d, vec_count = %d",
		pos, len, need_locking, write, vec_count);

	// general properties may get modified, so make a copy first
	/*device->interface->get_media_params( handle->handle_cookie, 
		&block_size, &ld_block_size, &capacity );*/
	ACQUIRE_BEN(&device->lock);
	block_size = device->block_size;
	ld_block_size = device->ld_block_size;
	capacity = device->capacity;
	RELEASE_BEN(&device->lock);

	if (capacity == 0) {
		res = B_DEV_NO_MEDIA;
		goto err;
	}

	if (block_size == 0) {
		res = B_DEV_CONFIGURATION_ERROR;
		goto err;
	}

	phys_vecs = locked_pool->alloc(device->phys_vecs_pool);

	SHOW_FLOW0(3, "got phys_vecs");

	// offset in active iovec (can span even byond first iovec)
	vec_offset = 0;

	while (len > 0) {
		//off_t block_pos;
		uint64 block_pos;
		uint32 block_ofs;
		size_t cur_len;

		size_t cur_blocks;
		struct iovec *cur_vecs;
		size_t cur_vec_count;
		size_t cur_vec_offset;

		size_t bytes_transferred;

		SHOW_FLOW(3, "current len = %lu", len);

		// skip handled iovecs
		while (vec_count > 0 && vec_offset >= vec->iov_len) {
			vec_offset -= vec->iov_len;
			++vec;
			--vec_count;
		}

		// having too few iovecs is handled in the following way:
		// 1. if no other problem occurs, lock_iovecs restrict transfer
		//    up to the last block fully described by iovecs
		// 2. if only a partial block is described, we fallback to 
		//    buffered transfer because lock_iovecs cannot give you 
		//    a whole block
		// 3. whenever buffered transfer is used, an explicit test for
		//    iovec shortage is done and transmission is restricted up to
		//    and including the last block that has an iovec; copying from/to
		//    buffer stops when no iovec is left (copy_buffer) and thus
		//    restricts transfer appropriately
		// 4. whenever all iovecs are consumed, we arrive at this piece of 
		//    code and abort
		if (vec_count == 0) {
			SHOW_FLOW0(3, "vec too short");
			res = B_BAD_VALUE;
			goto err2;
		}

		// get block index / start offset in block
		if (ld_block_size) {
			block_pos = pos >> ld_block_size;
			block_ofs = pos - (block_pos << ld_block_size);
		} else {
			block_pos = pos / block_size;
			block_ofs = pos - block_pos * block_size;
		}

		// read requests beyond end of volume must be ignored without notice
		if (block_pos >= capacity) {
			SHOW_FLOW0(1, "transfer starts beyond end of device");
			goto err2;
		}

		SHOW_FLOW(3, "block_pos = %Ld, block_ofs = %lu", block_pos, block_ofs);

		// check whether a buffered transfer is required:
		// 1. partial block transfer:
		// 1a. transfer starts within block
		// 1b. transfer finishes within block
		// 2. dma alignment problem

		// 1a and 2 is handled immediately, case 1b is delayed: we transmit 
		// whole blocks until the last (partial) block is reached and handle
		// it seperately; therefore we only check for len < block_size, e.g.
		// for a last partial block (we could do better, but you get what
		// you deserve)
		need_buffer = block_ofs != 0
			|| len < block_size
			|| !blkman_check_alignment(vec, vec_count, vec_offset, device->params.alignment);

retry:			
		if (need_buffer) {
			size_t tmp_len;

			// argh! - need buffered transfer
			SHOW_FLOW(1, "buffer required: len=%ld, block_ofs=%ld",
				len, block_ofs);

			acquire_sem(blkman_buffer_lock);

			// nobody helps us if there are too few iovecs, so test
			// for that explicitely
			// (case that tmp_len = 0 is already checked above; if not,
			//  we would get trouble when we try to round up to next
			//  block size, which would lead to zero, making trouble
			//  during lock)
			tmp_len = blkman_iovec_len(vec, vec_count, vec_offset);
			tmp_len = min(tmp_len, len);

			SHOW_FLOW(3, "tmp_len: %lu", tmp_len);

			if (write && (block_ofs != 0 || tmp_len < block_size)) {
				// partial block write - need to read block first
				// we always handle one block only to keep things simple
				cur_blocks = 1;

				SHOW_FLOW0(3, "partial write at beginning: reading content of first block");
				res = device->interface->read(handle->cookie, 
					&blkman_buffer_phys_vec, block_pos, 
					cur_blocks, block_size, &bytes_transferred);
			} else {
				// alignment problem or partial block read - find out how many
				// blocks are spanned by this transfer
				cur_blocks = (tmp_len + block_ofs + block_size - 1) / block_size;

				SHOW_FLOW(3, "cur_blocks: %ld", cur_blocks);

				// restrict block count to buffer size
				if (cur_blocks * block_size > blkman_buffer_size)
					cur_blocks = blkman_buffer_size / block_size;
			}

			// copy data into buffer before write
			// (calculate number of bytes to copy carefully!)
			if (write) {
				SHOW_FLOW(3, "copy data to buffer (%ld bytes)",
					cur_blocks * block_size - block_ofs);
				blkman_copy_buffer(blkman_buffer + block_ofs, 
					vec, vec_count, vec_offset, cur_blocks * block_size - block_ofs, 
					true);
			}

			cur_vecs = blkman_buffer_vec;
			cur_vec_count = 1;
			cur_vec_offset = 0;
		} else {
			// no buffer needed
			if (ld_block_size)
				cur_blocks = len >> ld_block_size;
			else
				cur_blocks = len / block_size;

			cur_vecs = vec;
			cur_vec_count = vec_count;
			cur_vec_offset = vec_offset;
		}

		SHOW_FLOW(3, "cur_blocks = %lu, cur_vec_offset = %lu, cur_vec_count = %lu",
			cur_blocks, cur_vec_offset, cur_vec_count);

		// restrict transfer size to device limits and media capacity
		cur_blocks = min(cur_blocks, device->params.max_blocks);

		if (block_pos + cur_blocks > capacity)
			cur_blocks = capacity - block_pos;

		SHOW_FLOW(3, "after applying size restriction: cur_blocks = %lu", cur_blocks);

		cur_len = cur_blocks * block_size;

		if (need_locking) {
			// lock data
			// side-node: we also lock the transfer buffer to simplify code

			// if n bytes are to be transferred, we try to lock
			// n bytes, then n/2, n/4 etc. until we succeed
			// this is needed because locking fails for entire iovecs only
			for (; cur_len > 0; cur_blocks >>= 1, cur_len = cur_blocks * block_size) {
				size_t locked_len;

				SHOW_FLOW(3, "trying to lock %lu bytes", cur_len);

				locked_len = blkman_lock_iovecs(cur_vecs, cur_vec_count, 
					cur_vec_offset, cur_len, B_DMA_IO | (write ? 0 : B_READ_DEVICE));

				if (locked_len == cur_len)
					break;

				// couldn't lock all we want				
				SHOW_FLOW0(3, "couldn't lock all bytes");

				if (locked_len > block_size) {
					// locked at least one block - we are happy
					SHOW_FLOW0(3, "transmission length restricted to locked bytes");
					cur_blocks = locked_len / block_size;
					cur_len = cur_blocks * block_size;
					break;
				}

				// got less then one block locked - unlock and retry
				SHOW_FLOW0(3, "too few bytes locked - trying again with fewer bytes");

				blkman_unlock_iovecs(cur_vecs, cur_vec_count, cur_vec_offset, locked_len,
					 B_DMA_IO | (write ? 0 : B_READ_DEVICE));
			}

			if (cur_len == 0) {
				// didn't manage to lock at least one block
				// -> fallback to buffered transfer
				SHOW_FLOW0(3, "locking failed");

				if (need_buffer) {
					// error locking transfer buffer? 
					// that's impossible - it is locked already!
					panic("Cannot lock scratch buffer\n");
					res = B_ERROR;
					goto err3;
				}

				need_buffer = true;
				goto retry;
			}
		}

		// data is locked and all restrictions are obeyed now;
		// time to setup sg list
		SHOW_FLOW0(3, "Creating SG list");

		res = blkman_map_iovecs(cur_vecs, cur_vec_count, cur_vec_offset, cur_len, 
				phys_vecs, device->params.max_sg_blocks, device->params.dma_boundary,
				device->params.max_sg_block_size);

		if (res < 0) {
			SHOW_FLOW(3, "failed - %s", strerror(res));
			goto cannot_map;
		}
	
		if (phys_vecs->total_len < cur_len) {
			// we hit some sg limit - restrict transfer appropriately
			cur_blocks = phys_vecs->total_len / block_size;

			SHOW_FLOW(3, "transmission to complex - restricted to %d blocks", (int)cur_blocks);

			if (cur_blocks == 0) {
				// oh no - not even one block is left; use transfer buffer instead
				SHOW_FLOW0(3, "SG too small to handle even one block");

				if (need_locking) {
					blkman_unlock_iovecs(cur_vecs, cur_vec_count, cur_vec_offset,
						cur_len, B_DMA_IO | (write ? 0 : B_READ_DEVICE));
				}

				if (need_buffer) {
					// we are already using the transfer buffer
					// this case is impossible as transfer buffer is linear!
					panic("Scratch buffer turned out to be too fragmented !?\n");
				}

				SHOW_FLOW0(3, "Falling back to buffered transfer");

				need_buffer = true;
				goto retry;
			}

			// reflect rounded len in sg list
			phys_vecs->total_len = cur_blocks * block_size;
		}

		// at least - let the bytes flow
		SHOW_FLOW(2, "Transmitting %d bytes @%Ld", 
			(int)phys_vecs->total_len, block_pos);

		if (write) {
			res = device->interface->write(handle->cookie,
					phys_vecs, block_pos, cur_blocks, block_size, &bytes_transferred);
		} else {
			res = device->interface->read(handle->cookie,
					phys_vecs, block_pos, cur_blocks, block_size, &bytes_transferred);
		}

		SHOW_FLOW(3, "Transfer of %d bytes completed (%s)", 
			(int)bytes_transferred, strerror(res));

cannot_map:
		// unlock data
		if (need_locking) {
			blkman_unlock_iovecs(cur_vecs, cur_vec_count, cur_vec_offset, 
				cur_len, B_DMA_IO | (write ? 0 : B_READ_DEVICE));
		}

		if (res < 0)
			goto err3;

		if (need_buffer) {
			// adjust transfer size by gap skipped at beginning of blocks
			bytes_transferred -= block_ofs;

			// if we had to round up to block size, adjust transfer as well
			if (bytes_transferred > len)
				bytes_transferred = len;

			// if transfer buffer is used for read, copy result from it
			if (!write) {
				SHOW_FLOW(3, "copying data back from buffer (%ld bytes)",
					bytes_transferred);
				blkman_copy_buffer(blkman_buffer + block_ofs,
					vec, vec_count, vec_offset, bytes_transferred, false);
			}

			release_sem(blkman_buffer_lock);
		}

		len -= bytes_transferred;
		vec_offset += bytes_transferred;
		pos += bytes_transferred;
	}

	locked_pool->free(device->phys_vecs_pool, phys_vecs);

	SHOW_FLOW0(3, "done");

	return B_OK;

err3:
	if (need_buffer)
		release_sem(blkman_buffer_lock);
err2:
	locked_pool->free(device->phys_vecs_pool, phys_vecs);
err:
	SHOW_FLOW(3, "done with error %s", strerror(res));

	// we haven't transferred all data - tell caller about
	*total_len = orig_len - len;
	return res;
}


static status_t
blkman_readv_int(blkman_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, size_t *len, bool need_locking)
{
	return blkman_readwrite(handle, pos, vec, vec_count, len, need_locking, false);
}


static status_t
blkman_writev_int(blkman_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, size_t *len, bool need_locking)
{
	return blkman_readwrite(handle, pos, vec, vec_count, len, need_locking, true);
}


/**	generic read(v)/write(v) routine; 
 *	iovecs are locked during transfer
 *	inlining it leads to overall code reduction as <write> is const
 */

static inline status_t
blkman_readwritev(blkman_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, size_t *len, bool write)

{
	status_t res;
	struct iovec *cur_vec;
	size_t left;
	size_t total_len;

	if ((res = lock_memory(vec, vec_count * sizeof(vec[0]), 0)) < 0)
		return res;

	// there is an error in the BeBook: *len does _not_ contain correct
	// total length on call - you have to calculate that yourself
	total_len = 0;

	for (cur_vec = vec, left = vec_count; left > 0; ++cur_vec, --left) {
		total_len += cur_vec->iov_len;
	}

	*len = total_len;

	if (write)
		res = blkman_writev_int(handle, pos, vec, vec_count, len, true);
	else
		res = blkman_readv_int(handle, pos, vec, vec_count, len, true);

	unlock_memory(vec, vec_count * sizeof(vec[0]), 0);

	return res;
}


status_t
blkman_readv(blkman_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, size_t *len)
{	
/*	SHOW_FLOW( 4, "len=%d", (int)*len );

	for( cur_vec = vec, left = vec_count; left > 0; ++cur_vec, --left ) {
		SHOW_FLOW( 4, "pos=%x, size=%d",
			(int)cur_vec->iov_base, (int)cur_vec->iov_len );
	}*/

	return blkman_readwritev(handle, pos, vec, vec_count, len, false);
}


status_t
blkman_read(blkman_handle_info *handle, off_t pos, void *buf, size_t *len)
{
	iovec vec[1];

	vec[0].iov_base = buf;
	vec[0].iov_len = *len;

	SHOW_FLOW0( 3, "" );

	// TBD: this assumes that the thread stack is not paged,
	//      else you want to use blkman_readv
	return blkman_readv_int(handle, pos, vec, 1, len, true);
}


ssize_t
blkman_writev(blkman_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, ssize_t *len)
{
	return blkman_readwritev(handle, pos, vec, vec_count, len, true);
}


ssize_t
blkman_write(blkman_handle_info *handle, off_t pos, void *buf, size_t *len)
{
	iovec vec[1];

	vec[0].iov_base = buf;
	vec[0].iov_len = *len;

	// see blkman_read
	return blkman_writev_int(handle, pos, vec, 1, len, true);
}


void
blkman_set_media_params(blkman_device_info *device, uint32 block_size,
	uint32 ld_block_size, uint64 capacity)
{
	SHOW_FLOW(3, "block_size = %lu, ld_block_size = %lu, capacity = %Lu\n", block_size,
		ld_block_size, capacity);

	ACQUIRE_BEN(&device->lock);

	device->block_size = block_size;
	device->ld_block_size = ld_block_size;
	device->capacity = capacity;	

	RELEASE_BEN(&device->lock);
}

