/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */

extern "C" {
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/mbuf.h>

#include <machine/bus.h>
}


// #pragma mark - structures


struct bus_dma_tag {
	bus_dma_tag_t	parent;
	phys_size_t		alignment;
	phys_addr_t		boundary;
	phys_addr_t		lowaddr;
	phys_addr_t		highaddr;
	bus_dma_filter_t* filter;
	void*			filterarg;
	phys_size_t		maxsize;
	uint32			maxsegments;
	bus_dma_segment_t* segments;
	phys_size_t		maxsegsz;
	int32			ref_count;
};


// #pragma mark - functions


void
busdma_lock_mutex(void* arg, bus_dma_lock_op_t op)
{
	struct mtx* dmtx = (struct mtx*)arg;
	switch (op) {
	case BUS_DMA_LOCK:
		mtx_lock(dmtx);
	break;
	case BUS_DMA_UNLOCK:
		mtx_unlock(dmtx);
	break;
	default:
		panic("busdma_lock_mutex: unknown operation 0x%x", op);
	}
}


int
bus_dma_tag_create(bus_dma_tag_t parent, bus_size_t alignment, bus_size_t boundary,
	bus_addr_t lowaddr, bus_addr_t highaddr, bus_dma_filter_t* filter,
	void* filterarg, bus_size_t maxsize, int nsegments, bus_size_t maxsegsz,
	int flags, bus_dma_lock_t* lockfunc, void* lockfuncarg, bus_dma_tag_t* dmat)
{
	if (boundary != 0 && boundary < maxsegsz)
		maxsegsz = boundary;

	*dmat = NULL;

	bus_dma_tag_t newtag = (bus_dma_tag_t)kernel_malloc(sizeof(*newtag),
		M_DEVBUF, M_ZERO | M_NOWAIT);
	if (newtag == NULL)
		return ENOMEM;

	newtag->parent = parent;
	newtag->alignment = alignment;
	newtag->boundary = boundary;
	newtag->lowaddr = lowaddr;
	newtag->highaddr = highaddr;
	newtag->filter = filter;
	newtag->filterarg = filterarg;
	newtag->maxsize = maxsize;
	newtag->maxsegments = nsegments;
	newtag->maxsegsz = maxsegsz;
	newtag->ref_count = 1;

	if (newtag->parent != NULL) {
		atomic_add(&parent->ref_count, 1);

		newtag->lowaddr = max_c(parent->lowaddr, newtag->lowaddr);
		newtag->highaddr = min_c(parent->highaddr, newtag->highaddr);

		if (newtag->boundary == 0) {
			newtag->boundary = parent->boundary;
		} else if (parent->boundary != 0) {
			newtag->boundary = min_c(parent->boundary, newtag->boundary);
		}

		if (newtag->filter == NULL) {
			newtag->filter = parent->filter;
			newtag->filterarg = parent->filterarg;
		}
	}

	if (newtag->filter != NULL)
		panic("bus_dma_tag_create: error: filters not implemented!");

	*dmat = newtag;
	return 0;
}


int
bus_dma_tag_destroy(bus_dma_tag_t dmat)
{
	if (dmat == NULL)
		return 0;

	while (dmat != NULL) {
		bus_dma_tag_t parent;

		parent = dmat->parent;
		atomic_add(&dmat->ref_count, -1);
		if (dmat->ref_count == 0) {
			kernel_free(dmat->segments, M_DEVBUF);
			kernel_free(dmat, M_DEVBUF);

			// Last reference released, so release our reference on our parent.
			dmat = parent;
		} else
			dmat = NULL;
	}
	return 0;
}


int
bus_dmamap_create(bus_dma_tag_t dmat, int flags, bus_dmamap_t* mapp)
{
	// We never bounce, so we do not need maps.
	*mapp = NULL;
	return 0;
}


int
bus_dmamap_destroy(bus_dma_tag_t dmat, bus_dmamap_t map)
{
	// We never create maps, so we never need to destroy them.
	if (map)
		panic("map is not NULL!");
	return 0;
}


int
bus_dmamem_alloc(bus_dma_tag_t dmat, void** vaddr, int flags,
	bus_dmamap_t* mapp)
{
	int mflags;
	if (flags & BUS_DMA_NOWAIT)
		mflags = M_NOWAIT;
	else
		mflags = M_WAITOK;

	if (flags & BUS_DMA_ZERO)
		mflags |= M_ZERO;

	// We never need to map/bounce.
	*mapp = NULL;

	// FreeBSD uses standard malloc() for the case where maxsize <= PAGE_SIZE,
	// however, our malloc() has no guarantees that the allocated memory will
	// not be swapped out, which obviously is a requirement here. So we must
	// always use kernel_contigmalloc().

	// The range specified by lowaddr, highaddr is an *exclusion* range,
	// not an inclusion range. So we want to at least start with the low end,
	// if possible. (The most common exclusion range is 32-bit only,
	// and ones other than that are very rare, so typically this will
	// succeed.)
	if (dmat->lowaddr > B_PAGE_SIZE) {
		*vaddr = kernel_contigmalloc(dmat->maxsize, M_DEVBUF, mflags,
			0, dmat->lowaddr,
			dmat->alignment ? dmat->alignment : 1ul, dmat->boundary);
		if (*vaddr == NULL)
			dprintf("bus_dmamem_alloc: failed to allocate with lowaddr "
				"0x%" B_PRIxPHYSADDR "\n", dmat->lowaddr);
	}
	if (*vaddr == NULL && dmat->highaddr < BUS_SPACE_MAXADDR) {
		*vaddr = kernel_contigmalloc(dmat->maxsize, M_DEVBUF, mflags,
			dmat->highaddr, BUS_SPACE_MAXADDR,
			dmat->alignment ? dmat->alignment : 1ul, dmat->boundary);
	}

	if (*vaddr == NULL) {
		dprintf("bus_dmamem_alloc: failed to allocate for tag (size %d, "
			"low 0x%" B_PRIxPHYSADDR ", high 0x%" B_PRIxPHYSADDR ", "
		    "boundary 0x%" B_PRIxPHYSADDR ")\n",
			(int)dmat->maxsize, dmat->lowaddr, dmat->highaddr, dmat->boundary);
		return ENOMEM;
	} else if (vtophys(*vaddr) & (dmat->alignment - 1)) {
		dprintf("bus_dmamem_alloc: failed to align memory: wanted %#x, got %#x\n",
			dmat->alignment, vtophys(vaddr));
		bus_dmamem_free(dmat, *vaddr, *mapp);
		return ENOMEM;
	}
	return 0;
}


void
bus_dmamem_free(bus_dma_tag_t dmat, void* vaddr, bus_dmamap_t map)
{
	// We never bounce, so map should be NULL.
	if (map != NULL)
		panic("bus_dmamem_free: map is not NULL!");

	kernel_contigfree(vaddr, dmat->maxsize, M_DEVBUF);
}


static int
_bus_dmamap_load_buffer(bus_dma_tag_t dmat, bus_dmamap_t /* map */, void* buf,
	bus_size_t buflen, int flags, bus_addr_t* lastaddrp, bus_dma_segment_t* segs,
	int& seg, bool first)
{
	vm_offset_t virtual_addr = (vm_offset_t)buf;
	bus_addr_t last_phys_addr = *lastaddrp;
	const bus_addr_t boundary_mask = ~(dmat->boundary - 1);

	while (buflen > 0) {
		const bus_addr_t phys_addr = pmap_kextract(virtual_addr);

		bus_size_t segment_size = B_PAGE_SIZE - (phys_addr & (B_PAGE_SIZE - 1));
		if (segment_size > buflen)
			segment_size = buflen;

		if (dmat->boundary > 0) {
			// Make sure we don't cross a boundary.
			bus_addr_t boundary_addr = (phys_addr + dmat->boundary) & boundary_mask;
			if (segment_size > (boundary_addr - phys_addr))
				segment_size = (boundary_addr - phys_addr);
		}

		// Insert chunk into a segment.
		if (first) {
			segs[seg].ds_addr = phys_addr;
			segs[seg].ds_len = segment_size;
			first = false;
		} else {
			// If possible, coalesce into the previous segment.
			if (phys_addr == last_phys_addr
			        && (segs[seg].ds_len + segment_size) <= dmat->maxsegsz
					&& (dmat->boundary == 0
						|| (segs[seg].ds_addr & boundary_mask)
							== (phys_addr & boundary_mask))) {
				segs[seg].ds_len += segment_size;
			} else {
				if (++seg >= dmat->maxsegments)
					break;
				segs[seg].ds_addr = phys_addr;
				segs[seg].ds_len = segment_size;
			}
		}

		last_phys_addr = phys_addr + segment_size;
		virtual_addr += segment_size;
		buflen -= segment_size;
	}

	*lastaddrp = last_phys_addr;
	return (buflen != 0 ? EFBIG : 0);
}


int
bus_dmamap_load(bus_dma_tag_t dmat, bus_dmamap_t map, void *buf,
	bus_size_t buflen, bus_dmamap_callback_t *callback,
	void *callback_arg, int flags)
{
	bus_addr_t lastaddr = 0;
	int error, nsegs = 0;

	if (dmat->segments == NULL) {
		dmat->segments = (bus_dma_segment_t*)kernel_malloc(
			sizeof(bus_dma_segment_t) * dmat->maxsegments, M_DEVBUF,
			M_ZERO | M_NOWAIT);
		if (dmat->segments == NULL)
			return ENOMEM;
	}

	error = _bus_dmamap_load_buffer(dmat, map, buf, buflen, flags,
		&lastaddr, dmat->segments, nsegs, true);

	if (error)
		(*callback)(callback_arg, dmat->segments, 0, error);
	else
		(*callback)(callback_arg, dmat->segments, nsegs + 1, 0);

	// ENOMEM is returned; all other errors are only sent to the callback.
	if (error == ENOMEM)
		return error;
	return 0;
}


int
bus_dmamap_load_mbuf(bus_dma_tag_t dmat, bus_dmamap_t map, struct mbuf* mb,
	bus_dmamap_callback2_t* callback, void* callback_arg, int flags)
{
	M_ASSERTPKTHDR(mb);

	if (dmat->segments == NULL) {
		dmat->segments = (bus_dma_segment_t*)kernel_malloc(
			sizeof(bus_dma_segment_t) * dmat->maxsegments, M_DEVBUF,
			M_ZERO | M_NOWAIT);
		if (dmat->segments == NULL)
			return ENOMEM;
	}

	int nsegs = 0, error = 0;
	if (mb->m_pkthdr.len <= dmat->maxsize) {
		bool first = true;
		bus_addr_t lastaddr = 0;
		for (struct mbuf* m = mb; m != NULL && error == 0; m = m->m_next) {
			if (m->m_len <= 0)
				continue;

			error = _bus_dmamap_load_buffer(dmat, map, m->m_data, m->m_len,
				flags, &lastaddr, dmat->segments, nsegs, first);
			first = false;
		}
	} else {
		error = EINVAL;
	}

	if (error) {
		(*callback)(callback_arg, dmat->segments, 0, 0, error);
	} else {
		(*callback)(callback_arg, dmat->segments, nsegs + 1, mb->m_pkthdr.len,
			error);
	}
	return error;
}


int
bus_dmamap_load_mbuf_sg(bus_dma_tag_t dmat, bus_dmamap_t map, struct mbuf* mb,
	bus_dma_segment_t* segs, int* nsegs, int flags)
{
	M_ASSERTPKTHDR(mb);

	*nsegs = 0;
	int error = 0;
	if (mb->m_pkthdr.len <= dmat->maxsize) {
		bool first = true;
		bus_addr_t lastaddr = 0;

		for (struct mbuf* m = mb; m != NULL && error == 0; m = m->m_next) {
			if (m->m_len <= 0)
				continue;

			error = _bus_dmamap_load_buffer(dmat, map, m->m_data, m->m_len,
				flags, &lastaddr, segs, *nsegs, first);
			first = false;
		}
	} else {
		error = EINVAL;
	}

	++*nsegs;
	return error;
}


void
_bus_dmamap_unload(bus_dma_tag_t dmat, bus_dmamap_t map)
{
	// We never allocate bounce pages; nothing to do.
}


void
_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_dmasync_op_t)
{
	// We never bounce; nothing to do.
}
