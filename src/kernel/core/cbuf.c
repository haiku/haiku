/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


/* This file contains the cbuf functions. Cbuf is a memory allocator,
 * currently not used for anything in kernel land other than ports.
 */


#include <OS.h>
#include <KernelExport.h>
#include <cbuf.h>
#include <vm.h>

#include <string.h>


#define CBUF_LENGTH 2048

#define CBUF_FLAG_CHAIN_HEAD 1
#define CBUF_FLAG_CHAIN_TAIL 2

struct cbuf {
	cbuf	*next;
	size_t	length;
	size_t	total_length;
	void	*data;
	int		flags;

	// fill the bytes to let this structure be exactly CBUF_LENGTH bytes large
	char	dat[CBUF_LENGTH - sizeof(struct cbuf *) -
				2*sizeof(int) - sizeof(void *) - sizeof(int)];
};

#define ALLOCATE_CHUNK (B_PAGE_SIZE * 16)
#define CBUF_REGION_SIZE (4*1024*1024)
#define CBUF_BITMAP_SIZE (CBUF_REGION_SIZE / CBUF_LENGTH)

static cbuf *sFreeBufferList;
static mutex sFreeBufferListMutex;
static cbuf *sFreeBufferNoBlockList;
static spinlock sNoBlockSpinlock;

static spinlock sLowlevelSpinlock;
static region_id sBufferArea;
static cbuf *sBuffer;
static region_id sBitmapArea;
static uint8 *sBitmap;


/* Declarations we need that aren't in header files */
uint16 ones_sum16(uint32, const void *, int);


//	private part of the API implementation


static void
initialize_cbuf(cbuf *buf)
{
	buf->length = sizeof(buf->dat);
	buf->data = buf->dat;
	buf->flags = 0;
	buf->total_length = 0;
}


static void *
allocate_cbuf(size_t *_size)
{
	size_t lengthFound = 0;
	void *buffer;
	int state;
	int i;
	int start = -1;

//	dprintf("cbuf_alloc: asked to allocate size %d\n", *size);

	state = disable_interrupts();
	acquire_spinlock(&sLowlevelSpinlock);

	// scan through the allocation bitmap, looking for the first free block
	// XXX not optimal
	for (i = 0; i < CBUF_BITMAP_SIZE; i++) {
		if (!CHECK_BIT(sBitmap[i/8], i%8)) {
			sBitmap[i/8] = SET_BIT(sBitmap[i/8], i%8);
			if (start < 0)
				start = i;

			lengthFound += CBUF_LENGTH;
			if (lengthFound >= *_size) {
				// we're done
				break;
			}
		} else if (start >= 0) {
			// we've found a start of a run before, so we're done now
			break;
		}
	}

	if (start < 0) {
		// couldn't find any memory
		buffer = NULL;
		*_size = 0;
	} else {
		buffer = &sBuffer[start];
		*_size = lengthFound;
	}

	release_spinlock(&sLowlevelSpinlock);
	restore_interrupts(state);

	return buffer;
}


static cbuf *
allocate_cbuf_mem(size_t size)
{
	void *_buffer;
	cbuf *buffer = NULL;
	cbuf *lastBuffer = NULL;
	cbuf *headBuffer = NULL;
	int i;
	int count;
	size_t foundSize;

	size = PAGE_ALIGN(size);

	while (size > 0) {
		foundSize = size;
		_buffer = allocate_cbuf(&foundSize);
		if (!_buffer) {
			// couldn't allocate, lets bail with what we have
			break;
		}
		size -= foundSize;
		count = foundSize / CBUF_LENGTH;
//		dprintf("allocate_cbuf_mem: returned %d of memory, %d left\n", found_size, size);

		buffer = (cbuf *)_buffer;
		if (headBuffer == NULL) {
			headBuffer = buffer;
			headBuffer->flags |= CBUF_FLAG_CHAIN_HEAD;
		}

		for (i = 0; i < count; i++) {
			initialize_cbuf(buffer);
			if (lastBuffer)
				lastBuffer->next = buffer;
			if (headBuffer)
				buffer->total_length += buffer->length;

			lastBuffer = buffer;
			buffer++;
		}
	}

	if (lastBuffer) {
		lastBuffer->next = NULL;
		lastBuffer->flags |= CBUF_FLAG_CHAIN_TAIL;
	}

	return headBuffer;
}


static void
clear_chain(cbuf *head, cbuf **tail)
{
	cbuf *buffer;

	buffer = head;
	*tail = NULL;
	while (buffer) {
		initialize_cbuf(buffer); // doesn't touch the next ptr
		*tail = buffer;
		buffer = buffer->next;
	}
}


//	#pragma mark -
//	public part of the API


/** Frees the specified buffer chain. Unlike cbuf_free_chain(),
 *	it doesn't block on a semaphore, but disables interrupts
 *	to update the non-block list.
 */

void
cbuf_free_chain_noblock(cbuf *buffer)
{
	cbuf *head, *last;
	int state;

	if (buffer == NULL)
		return;

	head = buffer;
	clear_chain(head, &last);

	state = disable_interrupts();
	acquire_spinlock(&sNoBlockSpinlock);

	last->next = sFreeBufferNoBlockList;
	sFreeBufferNoBlockList = head;

	release_spinlock(&sNoBlockSpinlock);
	restore_interrupts(state);
}


void
cbuf_free_chain(cbuf *buffer)
{
	cbuf *head, *last;

	if (buffer == NULL)
		return;

	head = buffer;
	clear_chain(head, &last);

	mutex_lock(&sFreeBufferListMutex);

	last->next = sFreeBufferList;
	sFreeBufferList = head;

	mutex_unlock(&sFreeBufferListMutex);
}


cbuf *
cbuf_get_chain(size_t length)
{
	size_t chainLength = 0;
	cbuf *chain = NULL;
	cbuf *tail = NULL;

	if (length == 0)
		panic("cbuf_get_chain(): passed size 0\n");

	mutex_lock(&sFreeBufferListMutex);

	while (chainLength < length) {
		cbuf *tempBuffer;

		if (sFreeBufferList == NULL) {
			// we need to allocate some more cbufs
			mutex_unlock(&sFreeBufferListMutex);

			tempBuffer = allocate_cbuf_mem(ALLOCATE_CHUNK);
			if (tempBuffer == NULL) {
				// no more ram
				if (chain)
					cbuf_free_chain(chain);

				return NULL;
			}
			cbuf_free_chain(tempBuffer);

			mutex_lock(&sFreeBufferListMutex);
			continue;
		}

		tempBuffer = sFreeBufferList;
		sFreeBufferList = sFreeBufferList->next;
		tempBuffer->next = chain;
		if (chain == NULL)
			tail = tempBuffer;
		chain = tempBuffer;

		chainLength += chain->length;
	}
	mutex_unlock(&sFreeBufferListMutex);

	// now we have a chain, fixup the first and last entry
	chain->total_length = length;
	chain->flags |= CBUF_FLAG_CHAIN_HEAD;
	tail->length -= chainLength - length;
	tail->flags |= CBUF_FLAG_CHAIN_TAIL;

	return chain;
}


cbuf *
cbuf_get_chain_noblock(size_t length)
{
	size_t chainLength = 0;
	cbuf *chain = NULL;
	cbuf *tail = NULL;
	int state;

	state = disable_interrupts();
	acquire_spinlock(&sNoBlockSpinlock);

	while (chainLength < length) {
		cbuf *tempBuffer;

		if (sFreeBufferNoBlockList == NULL) {
			dprintf("cbuf_get_chain_noblock: not enough cbufs\n");
			release_spinlock(&sNoBlockSpinlock);
			restore_interrupts(state);

			if (chain != NULL)
				cbuf_free_chain_noblock(chain);

			return NULL;
		}

		tempBuffer = sFreeBufferNoBlockList;
		sFreeBufferNoBlockList = sFreeBufferNoBlockList->next;
		tempBuffer->next = chain;
		if (chain == NULL)
			tail = tempBuffer;
		chain = tempBuffer;

		chainLength += chain->length;
	}
	release_spinlock(&sNoBlockSpinlock);
	restore_interrupts(state);

	// now we have a chain, fixup the first and last entry
	chain->total_length = length;
	chain->flags |= CBUF_FLAG_CHAIN_HEAD;
	tail->length -= chainLength - length;
	tail->flags |= CBUF_FLAG_CHAIN_TAIL;

	return chain;
}


status_t
cbuf_memcpy_to_chain(cbuf *chain, size_t offset, const void *_source, size_t length)
{
	cbuf *buffer;
	char *source = (char *)_source;
	int bufferOffset;

	if (chain == NULL)
		return B_BAD_VALUE;

	if ((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_to_chain: chain at %p not head\n", chain);
		return B_BAD_VALUE;
	}

	if (length + offset > chain->total_length) {
		dprintf("cbuf_memcpy_to_chain: length + offset > size of cbuf chain\n");
		return B_BAD_VALUE;
	}

	// find the starting cbuf in the chain to copy to
	buffer = chain;
	bufferOffset = 0;

	while (offset > 0) {
		if (buffer == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return B_ERROR;
		}

		if (offset < buffer->length) {
			// this is the one
			bufferOffset = offset;
			break;
		}

		offset -= buffer->length;
		buffer = buffer->next;
	}

	while (length > 0) {
		int toCopy;

		if (buffer == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return B_ERROR;
		}

		toCopy = min(length, buffer->length - bufferOffset);
		memcpy((char *)buffer->data + bufferOffset, source, toCopy);

		bufferOffset = 0;
		length -= toCopy;
		source += toCopy;
		buffer = buffer->next;
	}

	return B_OK;
}


status_t
cbuf_user_memcpy_to_chain(cbuf *chain, size_t offset, const void *_source, size_t length)
{
	cbuf *buffer;
	char *source = (char *)_source;
	int bufferOffset;
	int err;

	if (chain == NULL)
		return B_BAD_VALUE;

	if ((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_to_chain: chain at %p not head\n", chain);
		return B_BAD_VALUE;
	}

	if (length + offset > chain->total_length) {
		dprintf("cbuf_memcpy_to_chain: length + offset > size of cbuf chain\n");
		return B_BAD_VALUE;
	}

	// find the starting cbuf in the chain to copy to
	buffer = chain;
	bufferOffset = 0;

	while (offset > 0) {
		if (buffer == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return B_ERROR;
		}

		if (offset < buffer->length) {
			// this is the one
			bufferOffset = offset;
			break;
		}

		offset -= buffer->length;
		buffer = buffer->next;
	}

	err = B_NO_ERROR;

	while (length > 0) {
		int toCopy;

		if (buffer == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return B_ERROR;
		}
		toCopy = min(length, buffer->length - bufferOffset);

		err = user_memcpy((char *)buffer->data + bufferOffset, source, toCopy);
		if (err < 0)
			break; // memory exception

		bufferOffset = 0;
		length -= toCopy;
		source += toCopy;
		buffer = buffer->next;
	}

	return err;
}


status_t
cbuf_memcpy_from_chain(void *_dest, cbuf *chain, size_t offset, size_t length)
{
	cbuf *buffer;
	char *dest = (char *)_dest;
	int bufferOffset;

	if (chain == NULL)
		return B_BAD_VALUE;

	if ((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_from_chain: chain at %p not head\n", chain);
		return B_BAD_VALUE;
	}

	if (length + offset > chain->total_length) {
		dprintf("cbuf_memcpy_from_chain: length + offset > size of cbuf chain\n");
		return B_BAD_VALUE;
	}

	// find the starting cbuf in the chain to copy from
	buffer = chain;
	bufferOffset = 0;

	while (offset > 0) {
		if (buffer == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return B_ERROR;
		}

		if (offset < buffer->length) {
			// this is the one
			bufferOffset = offset;
			break;
		}
		offset -= buffer->length;
		buffer = buffer->next;
	}

	while (length > 0) {
		int toCopy;

		if (buffer == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return B_ERROR;
		}

		toCopy = min(length, buffer->length - bufferOffset);
		memcpy(dest, (char *)buffer->data + bufferOffset, toCopy);

		bufferOffset = 0;
		length -= toCopy;
		dest += toCopy;
		buffer = buffer->next;
	}

	return B_OK;
}


status_t
cbuf_user_memcpy_from_chain(void *_dest, cbuf *chain, size_t offset, size_t length)
{
	cbuf *buffer;
	char *dest = (char *)_dest;
	int bufferOffset;
	int err;

	if (chain == NULL)
		return B_BAD_VALUE;

	if ((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_from_chain: chain at %p not head\n", chain);
		return B_BAD_VALUE;
	}

	if (length + offset > chain->total_length) {
		dprintf("cbuf_memcpy_from_chain: length + offset > size of cbuf chain\n");
		return B_BAD_VALUE;
	}

	// find the starting cbuf in the chain to copy from
	buffer = chain;
	bufferOffset = 0;

	while (offset > 0) {
		if (buffer == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return B_ERROR;
		}

		if (offset < buffer->length) {
			// this is the one
			bufferOffset = offset;
			break;
		}
		offset -= buffer->length;
		buffer = buffer->next;
	}

	err = B_NO_ERROR;

	while (length > 0) {
		int toCopy;

		if (buffer == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return B_ERROR;
		}

		toCopy = min(length, buffer->length - bufferOffset);

		err = user_memcpy(dest, (char *)buffer->data + bufferOffset, toCopy);
		if (err < 0)
			break;

		bufferOffset = 0;
		length -= toCopy;
		dest += toCopy;
		buffer = buffer->next;
	}

	return err;
}


cbuf *
cbuf_duplicate_chain(cbuf *chain, size_t offset, size_t length)
{
	cbuf *buffer;
	cbuf *newBuffer;
	cbuf *destBuffer;
	int destBufferOffset;
	int bufferOffset;

	if (chain == NULL)
		return NULL;

	if ((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return NULL;
	if (offset >= chain->total_length)
		return NULL;

	length = min(length, chain->total_length - offset);

	newBuffer = cbuf_get_chain(length);
	if (!newBuffer)
		return NULL;

	// find the starting cbuf in the chain to copy from
	buffer = chain;
	bufferOffset = 0;
	while (offset > 0) {
		if (buffer == NULL) {
			cbuf_free_chain(newBuffer);
			dprintf("cbuf_duplicate_chain: end of chain reached too early!\n");
			return NULL;
		}
		if (offset < buffer->length) {
			// this is the one
			bufferOffset = offset;
			break;
		}
		offset -= buffer->length;
		buffer = buffer->next;
	}

	destBuffer = newBuffer;
	destBufferOffset = 0;

	while (length > 0) {
		size_t toCopy;

		if (buffer == NULL) {
			cbuf_free_chain(newBuffer);
			dprintf("cbuf_duplicate_chain: end of source chain reached too early!\n");
			return NULL;
		}
		if (destBuffer == NULL) {
			cbuf_free_chain(newBuffer);
			dprintf("cbuf_duplicate_chain: end of destination chain reached too early!\n");
			return NULL;
		}

		toCopy = min(destBuffer->length - destBufferOffset, buffer->length - bufferOffset);
		toCopy = min(toCopy, length);
		memcpy((char *)destBuffer->data + destBufferOffset, (char *)buffer->data + bufferOffset, toCopy);

		length -= toCopy;
		if (toCopy + bufferOffset == buffer->length) {
			buffer = buffer->next;
			bufferOffset = 0;
		} else
			bufferOffset += toCopy;

		if (toCopy + destBufferOffset == destBuffer->length) {
			destBuffer = destBuffer->next;
			destBufferOffset = 0;
		} else
			destBufferOffset += toCopy;
	}

	return newBuffer;
}


cbuf *
cbuf_merge_chains(cbuf *chain1, cbuf *chain2)
{
	cbuf *buffer;

	if (!chain1 && !chain2)
		return NULL;

	if (!chain1)
		return chain2;
	if (!chain2)
		return chain1;

	if ((chain1->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_merge_chain: chain at %p not head\n", chain1);
		return NULL;
	}

	if ((chain2->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_merge_chain: chain at %p not head\n", chain2);
		return NULL;
	}

	// walk to the end of the first chain and tag the second one on
	buffer = chain1;
	while (buffer->next)
		buffer = buffer->next;

	buffer->next = chain2;

	// modify the flags on the chain headers
	buffer->flags &= ~CBUF_FLAG_CHAIN_TAIL;
	chain1->total_length += chain2->total_length;
	chain2->flags &= ~CBUF_FLAG_CHAIN_HEAD;

	return chain1;
}


size_t
cbuf_get_length(cbuf *buffer)
{
	if (buffer == NULL)
		return 0;

	if (buffer->flags & CBUF_FLAG_CHAIN_HEAD) {
		return buffer->total_length;
	} else {
		int length = 0;
		while (buffer) {
			length += buffer->length;
			buffer = buffer->next;
		}
		return length;
	}
}


void *
cbuf_get_ptr(cbuf *buffer, size_t offset)
{
	while (buffer) {
		if (buffer->length > offset)
			return (void *)((int)buffer->data + offset);

		offset -= buffer->length;
		buffer = buffer->next;
	}
	return NULL;
}


/** Returns true if the buffer chain is contiguous over
 *	the specified range.
 */

bool
cbuf_is_contig_region(cbuf *buffer, size_t start, size_t end)
{
	while (buffer) {
		if (buffer->length > start)
			return buffer->length - start >= end;

		start -= buffer->length;
		end -= buffer->length;
		buffer = buffer->next;
	}
	return 0;
}


uint16
cbuf_ones_cksum16(cbuf *buffer, size_t offset, size_t length)
{
	uint16 sum = 0;
	int swapped = 0;

	if (buffer == NULL
		|| (buffer->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return 0;

	// find the start ptr
	while (buffer) {
		if (buffer->length > offset)
			break;

		offset -= buffer->length;
		buffer = buffer->next;
	}

	// start checksumming
	while (buffer && length > 0) {
		void *ptr = (void *)((addr_t)buffer->data + offset);
		size_t plen = min(length, buffer->length - offset);

		sum = ones_sum16(sum, ptr, plen);

		length -= plen;
		buffer = buffer->next;

		// if the pointer was odd, or the length was odd, but not both,
		// the checksum was swapped
		if ((buffer && length > 0)
			&& (((offset % 2) && (plen % 2) == 0) || (((offset % 2) == 0) && (plen % 2)))) {
			swapped ^= 1;
			sum = ((sum & 0xff) << 8) | ((sum >> 8) & 0xff);
		}
		offset = 0;
	}

	if (swapped)
		sum = ((sum & 0xff) << 8) | ((sum >> 8) & 0xff);

	return ~sum;
}


/** Truncates the head of the buffer chain about truncBytes.
 */

status_t
cbuf_truncate_head(cbuf *buffer, size_t truncBytes)
{
	cbuf *head = buffer;

	if (!buffer || (buffer->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return B_BAD_VALUE;

	while (buffer && truncBytes > 0) {
		int toTrunc = min(truncBytes, buffer->length);

		buffer->length -= toTrunc;
		buffer->data = (void *)((int)buffer->data + toTrunc);

		truncBytes -= toTrunc;
		head->total_length -= toTrunc;
		buffer = buffer->next;
	}

	return B_OK;
}


/** Truncate the tail of the buffer chain about truncBytes.
 */

status_t
cbuf_truncate_tail(cbuf *buffer, size_t truncBytes)
{
	cbuf *head = buffer;
	size_t bufferOffset;
	size_t offset;

	if (!buffer || (buffer->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return B_BAD_VALUE;

	offset = buffer->total_length - truncBytes;
		// ToDo: what happens if truncBytes >= total_length??
	bufferOffset = 0;
	while (buffer && offset > 0) {
		if (offset < buffer->length) {
			// this is the one
			bufferOffset = offset;
			break;
		}
		offset -= buffer->length;
		buffer = buffer->next;
	}
	if (!buffer)
		return B_ERROR;

	head->total_length -= buffer->length - bufferOffset;
	buffer->length -= bufferOffset;
		// ToDo: is this correct?
		// Isn't bufferOffset the part that has to stay in the buffer?
		// can't we just have head->total_length -= truncBytes?

	// clear out the rest of the buffers in this chain
	while ((buffer = buffer->next) != NULL) {
		head->total_length -= buffer->length;
		buffer->length = 0;
	}

	return B_OK;
}


//	#pragma mark -


static int
dbg_dump_cbuf_freelists(int argc, char **argv)
{
	cbuf *buffer;

	dprintf("sFreeBufferList:\n");
	for (buffer = sFreeBufferList; buffer; buffer = buffer->next)
		dprintf("%p ", buffer);
	dprintf("\n");

	dprintf("sFreeBufferNoBlockList:\n");
	for (buffer = sFreeBufferNoBlockList; buffer; buffer = buffer->next)
		dprintf("%p ", buffer);
	dprintf("\n");

	return 0;
}


void
cbuf_test(void)
{
	cbuf *buffer, *buffer2;
	char temp[1024];
	unsigned int i;

	dprintf("starting cbuffer test\n");

	buffer = cbuf_get_chain(32);
	if (!buffer)
		panic("cbuf_test: failed allocation of 32\n");

	buffer2 = cbuf_get_chain(3*1024*1024);
	if (!buffer2)
		panic("cbuf_test: failed allocation of 3mb\n");

	buffer = cbuf_merge_chains(buffer2, buffer);

	cbuf_free_chain(buffer);

	dprintf("allocating too much...\n");

	buffer = cbuf_get_chain(128*1024*1024);
	if (buffer)
		panic("cbuf_test: should have failed to allocate 128mb\n");

	dprintf("touching memory allocated by cbuf\n");

	buffer = cbuf_get_chain(7*1024*1024);
	if (!buffer)
		panic("cbuf_test: failed allocation of 7mb\n");

	for (i = 0; i < sizeof(temp); i++)
		temp[i] = i;
	for (i = 0; i < 7*1024*1024 / sizeof(temp); i++) {
		if (i % 128 == 0)
			dprintf("%Lud\n", (long long)(i*sizeof(temp)));
		cbuf_memcpy_to_chain(buffer, i*sizeof(temp), temp, sizeof(temp));
	}
	cbuf_free_chain(buffer);

	dprintf("finished cbuffer test\n");
}


status_t
cbuf_init(void)
{
	cbuf *buffer;
	int i;

/*
	sFreeBufferList = NULL;
	sFreeBufferNoBlockList = NULL;
	sNoBlockSpinlock = 0;
	sLowlevelSpinlock = 0;
*/

	// add the debug command
	add_debugger_command("cbuf_freelist", &dbg_dump_cbuf_freelists, "Dumps the cbuf free lists");

	if (mutex_init(&sFreeBufferListMutex, "cbuf_free_list") < B_OK) {
		panic("cbuf_init: error creating cbuf_free_list mutex\n");
		return B_NO_MEMORY;
	}

	// errors are fatal, that's why we don't clean up here

	sBufferArea = create_area("cbuf region", (void **)&sBuffer, B_ANY_KERNEL_ADDRESS,
		CBUF_REGION_SIZE, B_NO_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (sBufferArea < 0) {
		panic("cbuf_init: error creating cbuf region\n");
		return B_NO_MEMORY;
	}

	sBitmapArea = create_area("cbuf bitmap region", (void **)&sBitmap, B_ANY_KERNEL_ADDRESS,
		CBUF_BITMAP_SIZE / 8, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (sBitmapArea < 0) {
		panic("cbuf_init: error creating cbuf bitmap region\n");
		return B_NO_MEMORY;
	}

	// initialize the bitmap
	for (i = 0; i < CBUF_BITMAP_SIZE / 8; i++)
		sBitmap[i] = 0;

	buffer = allocate_cbuf_mem(ALLOCATE_CHUNK);
	if (buffer == NULL)
		return B_NO_MEMORY;
	cbuf_free_chain_noblock(buffer);

	buffer = allocate_cbuf_mem(ALLOCATE_CHUNK);
	if (buffer == NULL)
		return B_NO_MEMORY;
	cbuf_free_chain(buffer);

	return B_OK;
}
