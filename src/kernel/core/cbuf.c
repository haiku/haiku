/* This file contains the cbuf functions. Cbuf is a memory allocator, currently not used for anything in kernel land other than ports.*/


#include <kernel.h>
#include <memheap.h>
#include <OS.h>
#include <smp.h>
#include <arch/int.h>
#include <arch/debug.h>
#include <cbuf.h>
#include <arch/cpu.h>
#include <Errors.h>
#include <debug.h>
#include <int.h>
#include <vm.h>

#include <string.h>

#define ALLOCATE_CHUNK (PAGE_SIZE * 16)
#define CBUF_REGION_SIZE (4*1024*1024)
#define CBUF_BITMAP_SIZE (CBUF_REGION_SIZE / CBUF_LEN)

static cbuf *cbuf_free_list;
static sem_id free_list_sem;
static cbuf *cbuf_free_noblock_list;
static spinlock_t noblock_spin;

static spinlock_t cbuf_lowlevel_spinlock;
static region_id cbuf_region_id;
static cbuf *cbuf_region;
static region_id cbuf_bitmap_region_id;
static uint8 *cbuf_bitmap;

/* Declarations we need that aren't in header files */
uint16 ones_sum16(uint32, const void *, int);

static void initialize_cbuf(cbuf *buf)
{
	buf->len = sizeof(buf->dat);
	buf->data = buf->dat;
	buf->flags = 0;
	buf->total_len = 0;
}

static void *_cbuf_alloc(size_t *size)
{
	int state;
	void *buf;
	int i;
	int start;
	size_t len_found = 0;

//	dprintf("cbuf_alloc: asked to allocate size %d\n", *size);

	state = int_disable_interrupts();
	acquire_spinlock(&cbuf_lowlevel_spinlock);

	// scan through the allocation bitmap, looking for the first free block
	// XXX not optimal
	start = -1;
	for(i=0; i<CBUF_BITMAP_SIZE; i++) {
		if(!CHECK_BIT(cbuf_bitmap[i/8], i%8)) {
			cbuf_bitmap[i/8] = SET_BIT(cbuf_bitmap[i/8], i%8);
			if(start < 0)
				start = i;
			len_found += CBUF_LEN;
			if(len_found >= *size) {
				// we're done
				break;
			}
		} else if(start >= 0) {
			// we've found a start of a run before, so we're done now
			break;
		}
	}

	if(start < 0) {
		// couldn't find any memory
		buf = NULL;
		*size = 0;
	} else {
		buf = &cbuf_region[start];
		*size = len_found;
	}

	release_spinlock(&cbuf_lowlevel_spinlock);
	int_restore_interrupts(state);

	return buf;
}

static cbuf *allocate_cbuf_mem(size_t size)
{
	void *_buf;
	cbuf *buf = NULL;
	cbuf *last_buf = NULL;
	cbuf *head_buf = NULL;
	int i;
	int count;
	size_t found_size;

	size = PAGE_ALIGN(size);

	while(size > 0) {
		found_size = size;
		_buf = _cbuf_alloc(&found_size);
		if(!_buf) {
			// couldn't allocate, lets bail with what we have
			break;
		}
		size -= found_size;
		count = found_size / CBUF_LEN;
//		dprintf("allocate_cbuf_mem: returned %d of memory, %d left\n", found_size, size);

		buf = (cbuf *)_buf;
		if(!head_buf)
			head_buf = buf;
		for(i=0; i<count; i++) {
			initialize_cbuf(buf);
			if(last_buf)
				last_buf->next = buf;
			if(buf == head_buf) {
				buf->flags |= CBUF_FLAG_CHAIN_HEAD;
				buf->total_len = (size / CBUF_LEN) * sizeof(buf->dat);
			}
			last_buf = buf;
			buf++;
		}
	}
	if(buf) {
		buf->next = NULL;
		buf->flags |= CBUF_FLAG_CHAIN_TAIL;
	}

	return head_buf;
}

static void _clear_chain(cbuf *head, cbuf **tail)
{
	cbuf *buf;

	buf = head;
	*tail = NULL;
	while(buf) {
		initialize_cbuf(buf); // doesn't touch the next ptr
		*tail = buf;
		buf = buf->next;
	}

	return;
}

void cbuf_free_chain_noblock(cbuf *buf)
{
	cbuf *head, *last;
	int state;

	if(buf == NULL)
		return;

	head = buf;
	_clear_chain(head, &last);

	state = int_disable_interrupts();
	acquire_spinlock(&noblock_spin);

	last->next = cbuf_free_noblock_list;
	cbuf_free_noblock_list = head;

	release_spinlock(&noblock_spin);
	int_restore_interrupts(state);
}

void cbuf_free_chain(cbuf *buf)
{
	cbuf *head, *last;

	if(buf == NULL)
		return;

	head = buf;
	_clear_chain(head, &last);

	acquire_sem(free_list_sem);

	last->next = cbuf_free_list;
	cbuf_free_list = head;

	release_sem(free_list_sem);
}

cbuf *cbuf_get_chain(size_t len)
{
	cbuf *chain = NULL;
	cbuf *tail = NULL;
	cbuf *temp;
	size_t chain_len = 0;

	if(len == 0)
		panic("cbuf_get_chain: passed size 0\n");

	acquire_sem(free_list_sem);

	while(chain_len < len) {
		if(cbuf_free_list == NULL) {
			// we need to allocate some more cbufs
			release_sem(free_list_sem);
			temp = allocate_cbuf_mem(ALLOCATE_CHUNK);
			if(!temp) {
				// no more ram
				if(chain)
					cbuf_free_chain(chain);
				return NULL;
			}
			cbuf_free_chain(temp);
			acquire_sem(free_list_sem);
			continue;
		}

		temp = cbuf_free_list;
		cbuf_free_list = cbuf_free_list->next;
		temp->next = chain;
		if(chain == NULL)
			tail = temp;
		chain = temp;

		chain_len += chain->len;
	}
	release_sem(free_list_sem);

	// now we have a chain, fixup the first and last entry
	chain->total_len = len;
	chain->flags |= CBUF_FLAG_CHAIN_HEAD;
	tail->len -= chain_len - len;
	tail->flags |= CBUF_FLAG_CHAIN_TAIL;

	return chain;
}

cbuf *cbuf_get_chain_noblock(size_t len)
{
	cbuf *chain = NULL;
	cbuf *tail = NULL;
	cbuf *temp;
	size_t chain_len = 0;
	int state;

	state = int_disable_interrupts();
	acquire_spinlock(&noblock_spin);

	while(chain_len < len) {
		if(cbuf_free_noblock_list == NULL) {
			dprintf("cbuf_get_chain_noblock: not enough cbufs\n");
			release_spinlock(&noblock_spin);
			int_restore_interrupts(state);

			if(chain != NULL)
				cbuf_free_chain_noblock(chain);

			return NULL;
		}

		temp = cbuf_free_noblock_list;
		cbuf_free_noblock_list = cbuf_free_noblock_list->next;
		temp->next = chain;
		if(chain == NULL)
			tail = temp;
		chain = temp;

		chain_len += chain->len;
	}
	release_spinlock(&noblock_spin);
	int_restore_interrupts(state);

	// now we have a chain, fixup the first and last entry
	chain->total_len = len;
	chain->flags |= CBUF_FLAG_CHAIN_HEAD;
	tail->len -= chain_len - len;
	tail->flags |= CBUF_FLAG_CHAIN_TAIL;

	return chain;
}

int cbuf_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len)
{
	cbuf *buf;
	char *src = (char *)_src;
	int buf_offset;

	if((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_to_chain: chain at %p not head\n", chain);
		return ERR_INVALID_ARGS;
	}

	if(len + offset > chain->total_len) {
		dprintf("cbuf_memcpy_to_chain: len + offset > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy to
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		to_copy = min(len, buf->len - buf_offset);
		memcpy((char *)buf->data + buf_offset, src, to_copy);

		buf_offset = 0;
		len -= to_copy;
		src += to_copy;
		buf = buf->next;
	}

	return B_NO_ERROR;
}

int cbuf_user_memcpy_to_chain(cbuf *chain, size_t offset, const void *_src, size_t len)
{
	cbuf *buf;
	char *src = (char *)_src;
	int buf_offset;
	int err;

	if((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_to_chain: chain at %p not head\n", chain);
		return ERR_INVALID_ARGS;
	}

	if(len + offset > chain->total_len) {
		dprintf("cbuf_memcpy_to_chain: len + offset > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy to
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	err = B_NO_ERROR;
	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			dprintf("cbuf_memcpy_to_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		to_copy = min(len, buf->len - buf_offset);
		if ((err = user_memcpy((char *)buf->data + buf_offset, src, to_copy) < 0))
			break; // memory exception

		buf_offset = 0;
		len -= to_copy;
		src += to_copy;
		buf = buf->next;
	}

	return err;
}


int cbuf_memcpy_from_chain(void *_dest, cbuf *chain, size_t offset, size_t len)
{
	cbuf *buf;
	char *dest = (char *)_dest;
	int buf_offset;

	if((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_from_chain: chain at %p not head\n", chain);
		return ERR_INVALID_ARGS;
	}

	if(len + offset > chain->total_len) {
		dprintf("cbuf_memcpy_from_chain: len + offset > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy from
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}

		to_copy = min(len, buf->len - buf_offset);
		memcpy(dest, (char *)buf->data + buf_offset, to_copy);

		buf_offset = 0;
		len -= to_copy;
		dest += to_copy;
		buf = buf->next;
	}

	return B_NO_ERROR;
}

int cbuf_user_memcpy_from_chain(void *_dest, cbuf *chain, size_t offset, size_t len)
{
	cbuf *buf;
	char *dest = (char *)_dest;
	int buf_offset;
	int err;

	if((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_memcpy_from_chain: chain at %p not head\n", chain);
		return ERR_INVALID_ARGS;
	}

	if(len + offset > chain->total_len) {
		dprintf("cbuf_memcpy_from_chain: len + offset > size of cbuf chain\n");
		return ERR_INVALID_ARGS;
	}

	// find the starting cbuf in the chain to copy from
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	err = B_NO_ERROR;
	while(len > 0) {
		int to_copy;

		if(buf == NULL) {
			dprintf("cbuf_memcpy_from_chain: end of chain reached too early!\n");
			return ERR_GENERAL;
		}

		to_copy = min(len, buf->len - buf_offset);
		if ((err = user_memcpy(dest, (char *)buf->data + buf_offset, to_copy) < 0))
			break;

		buf_offset = 0;
		len -= to_copy;
		dest += to_copy;
		buf = buf->next;
	}

	return err;
}

cbuf *cbuf_duplicate_chain(cbuf *chain, size_t offset, size_t len)
{
	cbuf *buf;
	cbuf *newbuf;
	cbuf *destbuf;
	int dest_buf_offset;
	int buf_offset;

	if(!chain)
		return NULL;
	if((chain->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return NULL;
	if(offset >= chain->total_len)
		return NULL;
	len = min(len, chain->total_len - offset);

	newbuf = cbuf_get_chain(len);
	if(!newbuf)
		return NULL;

	// find the starting cbuf in the chain to copy from
	buf = chain;
	buf_offset = 0;
	while(offset > 0) {
		if(buf == NULL) {
			cbuf_free_chain(newbuf);
			dprintf("cbuf_duplicate_chain: end of chain reached too early!\n");
			return NULL;
		}
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}

	destbuf = newbuf;
	dest_buf_offset = 0;
	while(len > 0) {
		size_t to_copy;

		if(buf == NULL) {
			cbuf_free_chain(newbuf);
			dprintf("cbuf_duplicate_chain: end of source chain reached too early!\n");
			return NULL;
		}
		if(destbuf == NULL) {
			cbuf_free_chain(newbuf);
			dprintf("cbuf_duplicate_chain: end of destination chain reached too early!\n");
			return NULL;
		}

		to_copy = min(destbuf->len - dest_buf_offset, buf->len - buf_offset);
		to_copy = min(to_copy, len);
		memcpy((char *)destbuf->data + dest_buf_offset, (char *)buf->data + buf_offset, to_copy);

		len -= to_copy;
		if(to_copy + buf_offset == buf->len) {
			buf = buf->next;
			buf_offset = 0;
		} else {
			buf_offset += to_copy;
		}
		if(to_copy + dest_buf_offset == destbuf->len) {
			destbuf = destbuf->next;
			dest_buf_offset = 0;
		} else {
			dest_buf_offset += to_copy;
		}
	}

	return newbuf;
}


cbuf *cbuf_merge_chains(cbuf *chain1, cbuf *chain2)
{
	cbuf *buf;

	if(!chain1 && !chain2)
		return NULL;
	if(!chain1)
		return chain2;
	if(!chain2)
		return chain1;

	if((chain1->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_merge_chain: chain at %p not head\n", chain1);
		return NULL;
	}

	if((chain2->flags & CBUF_FLAG_CHAIN_HEAD) == 0) {
		dprintf("cbuf_merge_chain: chain at %p not head\n", chain2);
		return NULL;
	}

	// walk to the end of the first chain and tag the second one on
	buf = chain1;
	while(buf->next)
		buf = buf->next;

	buf->next = chain2;

	// modify the flags on the chain headers
	buf->flags &= ~CBUF_FLAG_CHAIN_TAIL;
	chain1->total_len += chain2->total_len;
	chain2->flags &= ~CBUF_FLAG_CHAIN_HEAD;

	return chain1;
}

size_t cbuf_get_len(cbuf *buf)
{
	if(!buf)
		return ERR_INVALID_ARGS;

	if(buf->flags & CBUF_FLAG_CHAIN_HEAD) {
		return buf->total_len;
	} else {
		int len = 0;
		while(buf) {
			len += buf->len;
			buf = buf->next;
		}
		return len;
	}
}

void *cbuf_get_ptr(cbuf *buf, size_t offset)
{
	while(buf) {
		if(buf->len > offset)
			return (void *)((int)buf->data + offset);
		if(buf->len > offset)
			return NULL;
		offset -= buf->len;
		buf = buf->next;
	}
	return NULL;
}

int cbuf_is_contig_region(cbuf *buf, size_t start, size_t end)
{
	while(buf) {
		if(buf->len > start) {
			if(buf->len - start >= end)
				return 1;
			else
				return 0;
		}
		start -= buf->len;
		end -= buf->len;
		buf = buf->next;
	}
	return 0;
}

uint16 cbuf_ones_cksum16(cbuf *buf, size_t offset, size_t len)
{
	uint16 sum = 0;
	int swapped = 0;

	if(!buf)
		return 0;
	if((buf->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return 0;

	// find the start ptr
	while(buf) {
		if(buf->len > offset)
			break;
		if(buf->len > offset)
			return 0;
		offset -= buf->len;
		buf = buf->next;
	}

	// start checksumming
	while(buf && len > 0) {
		void *ptr = (void *)((addr)buf->data + offset);
		size_t plen = min(len, buf->len - offset);

		sum = ones_sum16(sum, ptr, plen);

		len -= plen;
		buf = buf->next;

		// if the pointer was odd, or the length was odd, but not both,
		// the checksum was swapped
		if((buf && len > 0) && (((offset % 2) && ((plen % 2) == 0)) || (((offset % 2) == 0) && (plen % 2)))) {
			swapped ^= 1;
			sum = ((sum & 0xff) << 8) | ((sum >> 8) & 0xff);
		}
		offset = 0;
	}

	if (swapped)
		sum = ((sum & 0xff) << 8) | ((sum >> 8) & 0xff);

	return ~sum;
}

int cbuf_truncate_head(cbuf *buf, size_t trunc_bytes)
{
	cbuf *head = buf;

	if(!buf)
		return ERR_INVALID_ARGS;
	if((buf->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return ERR_INVALID_ARGS;

	while(buf && trunc_bytes > 0) {
		int to_trunc;

		to_trunc = min(trunc_bytes, buf->len);

		buf->len -= to_trunc;
		buf->data = (void *)((int)buf->data + to_trunc);

		trunc_bytes -= to_trunc;
		head->total_len -= to_trunc;
		buf = buf->next;
	}

	return 0;
}

int cbuf_truncate_tail(cbuf *buf, size_t trunc_bytes)
{
	size_t offset;
	size_t buf_offset;
	cbuf *head = buf;

	if(!buf)
		return ERR_INVALID_ARGS;
	if((buf->flags & CBUF_FLAG_CHAIN_HEAD) == 0)
		return ERR_INVALID_ARGS;

	offset = buf->total_len - trunc_bytes;
	buf_offset = 0;
	while(buf && offset > 0) {
		if(offset < buf->len) {
			// this is the one
			buf_offset = offset;
			break;
		}
		offset -= buf->len;
		buf = buf->next;
	}
	if(!buf)
		return ERR_GENERAL;

	head->total_len -= buf->len - buf_offset;
	buf->len -= buf->len - buf_offset;

	// clear out the rest of the buffers in this chain
	buf = buf->next;
	while(buf) {
		head->total_len -= buf->len;
		buf->len = 0;
		buf = buf->next;
	}

	return B_NO_ERROR;
}

static void dbg_dump_cbuf_freelists(int argc, char **argv)
{
	cbuf *buf;

	dprintf("cbuf_free_list:\n");
	for(buf = cbuf_free_list; buf; buf = buf->next)
		dprintf("%p ", buf);
	dprintf("\n");

	dprintf("cbuf_free_noblock_list:\n");
	for(buf = cbuf_free_noblock_list; buf; buf = buf->next)
		dprintf("%p ", buf);
	dprintf("\n");
}

void cbuf_test()
{
	cbuf *buf, *buf2;
	char temp[1024];
	unsigned int i;

	dprintf("starting cbuffer test\n");

	buf = cbuf_get_chain(32);
	if(!buf)
		panic("cbuf_test: failed allocation of 32\n");

	buf2 = cbuf_get_chain(3*1024*1024);
	if(!buf2)
		panic("cbuf_test: failed allocation of 3mb\n");

	buf = cbuf_merge_chains(buf2, buf);

	cbuf_free_chain(buf);

	dprintf("allocating too much...\n");

	buf = cbuf_get_chain(128*1024*1024);
	if(buf)
		panic("cbuf_test: should have failed to allocate 128mb\n");

	dprintf("touching memory allocated by cbuf\n");

	buf = cbuf_get_chain(7*1024*1024);
	if(!buf)
		panic("cbuf_test: failed allocation of 7mb\n");

	for(i=0; i < sizeof(temp); i++)
		temp[i] = i;
	for(i=0; i<7*1024*1024 / sizeof(temp); i++) {
		if(i % 128 == 0) dprintf("%Lud\n", (long long)(i*sizeof(temp)));
		cbuf_memcpy_to_chain(buf, i*sizeof(temp), temp, sizeof(temp));
	}
	cbuf_free_chain(buf);

	dprintf("finished cbuffer test\n");
}

int cbuf_init()
{
	cbuf *buf;
	int i;

	cbuf_free_list = NULL;
	cbuf_free_noblock_list = NULL;
	noblock_spin = 0;
	cbuf_lowlevel_spinlock = 0;

	// add the debug command
	dbg_add_command(&dbg_dump_cbuf_freelists, "cbuf_freelist", "Dumps the cbuf free lists");

	free_list_sem = create_sem(1, "cbuf_free_list_sem");
	if(free_list_sem < 0) {
		panic("cbuf_init: error creating cbuf_free_list_sem\n");
		return ENOMEM;
	}

	cbuf_region_id = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "cbuf region",
		(void **)&cbuf_region, REGION_ADDR_ANY_ADDRESS, CBUF_REGION_SIZE, REGION_WIRING_LAZY, LOCK_RW|LOCK_KERNEL);
	if(cbuf_region_id < 0) {
		panic("cbuf_init: error creating cbuf region\n");
		return ENOMEM;
	}

	cbuf_bitmap_region_id = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "cbuf bitmap region",
		(void **)&cbuf_bitmap, REGION_ADDR_ANY_ADDRESS,
		CBUF_BITMAP_SIZE / 8, REGION_WIRING_WIRED, LOCK_RW|LOCK_KERNEL);
	if(cbuf_region_id < 0) {
		panic("cbuf_init: error creating cbuf bitmap region\n");
		return ENOMEM;
	}

	// initialize the bitmap
	for(i=0; i<CBUF_BITMAP_SIZE/8; i++)
		cbuf_bitmap[i] = 0;

	buf = allocate_cbuf_mem(ALLOCATE_CHUNK);
	if(buf == NULL)
		return ENOMEM;
	cbuf_free_chain_noblock(buf);

	buf = allocate_cbuf_mem(ALLOCATE_CHUNK);
	if(buf == NULL)
		return ENOMEM;
	cbuf_free_chain(buf);

	return B_NO_ERROR;
}
