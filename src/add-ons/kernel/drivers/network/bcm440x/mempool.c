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

spinlock chunk_pool_lock = 0;
void *chunk_pool = 0;
void *chunk_head = 0;

void *
area_malloc(size_t size)
{
	void *p;
	size = ROUNDUP(size, B_PAGE_SIZE);
	
	if (create_area("area_malloc", &p, B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, 0) < 0)
		return 0;
	return p;
}

void
area_free(void *p)
{
	delete_area(area_for(p));
}

int
mempool_init(int count)
{
	int chunk_size = 2048;

	int chunk_alloc_size = chunk_size * (count + 1);
	
	char *chunk_base;
	
	int i;
		
	chunk_pool = area_malloc(chunk_alloc_size);
	if (!chunk_pool) {
		return -1;
	}

	chunk_base = (char *)ROUNDUP((unsigned long)chunk_pool, 2048);
	
	for (i = 0; i < count; i++) {
		chunk_pool_put(chunk_base + i * chunk_size);
	}

	return 0;
}

void
mempool_exit(void)
{
	area_free(chunk_pool);
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
