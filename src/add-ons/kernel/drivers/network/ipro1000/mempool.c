/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <KernelExport.h>
#include <malloc.h>
#include "mempool.h"
#include "if_compat.h"
#include "util.h"
#include "debug.h"

spinlock mbuf_pool_lock = 0;
spinlock chunk_pool_lock = 0;

void *mbuf_pool = 0;
void *chunk_pool = 0;

void *mbuf_head = 0;
void *chunk_head = 0;

int
mempool_init(int count)
{
	int chunk_size = 2048;
	int mbuf_size = ROUNDUP(sizeof(struct mbuf), 16);

	int chunk_alloc_size = chunk_size * (count + 1);
	int mbuf_alloc_size = mbuf_size * (count + 1);
	
	char *chunk_base;
	char *mbuf_base;
	
	int i;
	
	INIT_DEBUGOUT2("chunk_size %d, mbuf_size %d", chunk_size, mbuf_size);
	INIT_DEBUGOUT2("chunk_alloc_size %d, mbuf_alloc_size %d", chunk_alloc_size, mbuf_alloc_size);
	
	chunk_pool = area_malloc(chunk_alloc_size);
	if (!chunk_pool) {
		ERROROUT1("failed to allocate chunk storage of %d bytes\n", chunk_alloc_size);
		return -1;
	}
	mbuf_pool = area_malloc(mbuf_alloc_size);
	if (!mbuf_pool) {
		ERROROUT1("failed to allocate mbuf storage of %d bytes\n", mbuf_alloc_size);
		area_free(chunk_pool);
		return -1;
	}

	chunk_base = (char *)ROUNDUP((unsigned long)chunk_pool, 2048);
	mbuf_base  = (char *)ROUNDUP((unsigned long)mbuf_pool, 16);
	
	for (i = 0; i < count; i++) {
		chunk_pool_put(chunk_base + i * chunk_size);
		mbuf_pool_put(mbuf_base + i * mbuf_size);
	}

	INIT_DEBUGOUT("mempool init success");
	return 0;
}

void
mempool_exit(void)
{
	area_free(chunk_pool);
	area_free(mbuf_pool);
}

void *
mbuf_pool_get(void)
{
	void *p;
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&mbuf_pool_lock);
	
	p = mbuf_head;
	if (p)
		mbuf_head = *(void **)p;

	release_spinlock(&mbuf_pool_lock);
	restore_interrupts(s);
	return p;
}

void
mbuf_pool_put(void *p)
{
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&mbuf_pool_lock);

	*(void **)p = mbuf_head;
	mbuf_head = p;

	release_spinlock(&mbuf_pool_lock);
	restore_interrupts(s);
}

void *
chunk_pool_get(void)
{
	void *p;
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&chunk_pool_lock);

	p = chunk_head;
	if (p)
		chunk_head = *(void **)p;

	release_spinlock(&chunk_pool_lock);
	restore_interrupts(s);
	return p;
}

void
chunk_pool_put(void *p)
{
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&chunk_pool_lock);

	*(void **)p = chunk_head;
	chunk_head = p;

	release_spinlock(&chunk_pool_lock);
	restore_interrupts(s);
}
