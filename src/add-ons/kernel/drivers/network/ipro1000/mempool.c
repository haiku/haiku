/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#include <KernelExport.h>
#include <malloc.h>
#include "mempool.h"
#include "if_compat.h"
#include "debug.h"

spinlock pool_lock = 0;

void *mbuf_pool = 0;
void *chunk_pool = 0;

void *mbuf_head = 0;
void *chunk_head = 0;

void *
area_malloc(int size)
{
	void *p;
	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	
	if (create_area("area_malloc", &p, B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,0) < 0)
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
	int mbuf_size = (sizeof(struct mbuf) + 15) & ~15;

	int chunk_alloc_size = chunk_size * (count + 1);
	int mbuf_alloc_size = mbuf_size * (count + 1);
	
	char *chunk_base;
	char *mbuf_base;
	
	int i;
	
	TRACE("chunk_size %d, mbuf_size %d\n", chunk_size, mbuf_size);
	TRACE("chunk_alloc_size %d, mbuf_alloc_size %d\n", chunk_alloc_size, mbuf_alloc_size);
	
	chunk_pool = area_malloc(chunk_alloc_size);
	if (!chunk_pool) {
		TRACE("failed to allocate chunk storage of %d bytes\n", chunk_alloc_size);
		return -1;
	}
	mbuf_pool = area_malloc(mbuf_alloc_size);
	if (!mbuf_pool) {
		TRACE("failed to allocate mbuf storage of %d bytes\n", mbuf_alloc_size);
		area_free(chunk_pool);
		return -1;
	}

	chunk_base = (char *)(((unsigned long)chunk_pool + 2047) & ~2047);
	mbuf_base  = (char *)(((unsigned long)mbuf_pool + 15) & ~15);
	
	for (i = 0; i < count; i++) {
		chunk_pool_put(chunk_base + i * chunk_size);
		mbuf_pool_put(mbuf_base + i * mbuf_size);
	}

	TRACE("mempool init success\n");
	return 0;
}

void
mempool_exit()
{
	area_free(chunk_pool);
	area_free(mbuf_pool);
}

void *
mbuf_pool_get()
{
	void *p;
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&pool_lock);
	
	p = mbuf_head;
	if (p)
		mbuf_head = *(void **)p;

	release_spinlock(&pool_lock);
	restore_interrupts(s);
	return p;
}

void
mbuf_pool_put(void *p)
{
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&pool_lock);

	*(void **)p = mbuf_head;
	mbuf_head = p;

	release_spinlock(&pool_lock);
	restore_interrupts(s);
}

void *
chunk_pool_get()
{
	void *p;
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&pool_lock);

	p = chunk_head;
	if (p)
		chunk_head = *(void **)p;

	release_spinlock(&pool_lock);
	restore_interrupts(s);
	return p;
}

void
chunk_pool_put(void *p)
{
	cpu_status s;
	s = disable_interrupts();
	acquire_spinlock(&pool_lock);

	*(void **)p = chunk_head;
	chunk_head = p;

	release_spinlock(&pool_lock);
	restore_interrupts(s);
}
