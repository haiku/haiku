/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_MACHINE_BUS_H_
#define _OBSD_COMPAT_MACHINE_BUS_H_


#include_next <machine/bus.h>
#include <sys/bus_dma.h>


#define	BUS_DMA_READ		(BUS_DMA_NOWRITE)
#define	BUS_DMA_WRITE		(0)

struct bus_dmamap_obsd {
	bus_dma_tag_t _dmat;
	bus_dmamap_t _dmamp;
	int _error;

	bus_size_t dm_mapsize;
	int dm_nsegs;
	bus_dma_segment_t dm_segs[];
};
typedef struct bus_dmamap_obsd* bus_dmamap_obsd_t;
#define bus_dmamap_t bus_dmamap_obsd_t


static int
bus_dmamap_create_obsd(bus_dma_tag_t tag, bus_size_t maxsize,
	int nsegments, bus_size_t maxsegsz, bus_size_t boundary,
	int flags, bus_dmamap_t* dmamp)
{
	*dmamp = calloc(sizeof(struct bus_dmamap_obsd) + (sizeof(bus_dma_segment_t) * nsegments), 1);
	if ((*dmamp) == NULL)
		return ENOMEM;

	int error = bus_dma_tag_create(tag, 1, boundary,
		BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR, NULL, NULL,
		maxsize, nsegments, maxsegsz, flags, NULL, NULL,
		&(*dmamp)->_dmat);
	if (error != 0)
		return error;

	error = bus_dmamap_create((*dmamp)->_dmat, flags, &(*dmamp)->_dmamp);
	return error;
}
#define bus_dmamap_create bus_dmamap_create_obsd


static void
bus_dmamap_destroy_obsd(bus_dma_tag_t tag, bus_dmamap_t dmam)
{
	bus_dmamap_destroy(dmam->_dmat, dmam->_dmamp);
	bus_dma_tag_destroy(dmam->_dmat);
	_kernel_free(dmam);
}
#define bus_dmamap_destroy bus_dmamap_destroy_obsd


static void
bus_dmamap_load_obsd_callback(void* arg, bus_dma_segment_t* segs, int nseg, int error)
{
	bus_dmamap_t dmam = (bus_dmamap_t)arg;
	dmam->_error = error;
	dmam->dm_nsegs = nseg;
	memcpy(dmam->dm_segs, segs, nseg * sizeof(bus_dma_segment_t));
}

static int
bus_dmamap_load_obsd(bus_dma_tag_t tag, bus_dmamap_t dmam, void *buf, bus_size_t buflen, struct proc *p, int flags)
{
	int error = bus_dmamap_load(dmam->_dmat, dmam->_dmamp, buf, buflen,
		bus_dmamap_load_obsd_callback, dmam, flags | BUS_DMA_NOWAIT);
	if (error != 0)
		return error;
	dmam->dm_mapsize = buflen;
	return dmam->_error;
}
#define bus_dmamap_load bus_dmamap_load_obsd


static int
bus_dmamap_load_mbuf_obsd(bus_dma_tag_t tag, bus_dmamap_t dmam, struct mbuf *chain, int flags)
{
	dmam->dm_mapsize = chain->m_pkthdr.len;
	return bus_dmamap_load_mbuf_sg(dmam->_dmat, dmam->_dmamp, chain,
		dmam->dm_segs, &dmam->dm_nsegs, flags);
}
#define bus_dmamap_load_mbuf bus_dmamap_load_mbuf_obsd


static void
bus_dmamap_unload_obsd(bus_dma_tag_t tag, bus_dmamap_t dmam)
{
	bus_dmamap_unload(dmam->_dmat, dmam->_dmamp);
	dmam->dm_mapsize = 0;
	dmam->dm_nsegs = 0;
}
#define bus_dmamap_unload bus_dmamap_unload_obsd


static void
bus_dmamap_sync_obsd(bus_dma_tag_t tag, bus_dmamap_t dmam,
	bus_addr_t offset, bus_size_t length, int ops)
{
	bus_dmamap_sync_etc(dmam->_dmat, dmam->_dmamp, offset, length, ops);
}
#define bus_dmamap_sync bus_dmamap_sync_obsd


static int
bus_dmamem_alloc_obsd(bus_dma_tag_t tag, bus_size_t size, bus_size_t alignment, bus_size_t boundary,
	bus_dma_segment_t* segs, int nsegs, int* rsegs, int flags)
{
	// OpenBSD distinguishes between three different types of addresses:
	//     1. virtual addresses (caddr_t)
	//     2. opaque "bus" addresses
	//     3. physical addresses
	// This function returns the second type. We simply return the virtual address for it.

	bus_dma_tag_t local;
	int error = bus_dma_tag_create(tag, alignment, boundary,
		BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR, NULL, NULL,
		size, nsegs, size, flags, NULL, NULL, &local);
	if (error)
		return error;

	error = bus_dmamem_alloc(local, (void**)&segs[0].ds_addr, flags, NULL);
	if (error) {
		bus_dma_tag_destroy(local);
		return error;
	}
	segs[0].ds_len = size;

	error = bus_dma_tag_destroy(local);
	if (error)
		return error;

	*rsegs = 1;
	return 0;
}
#define bus_dmamem_alloc bus_dmamem_alloc_obsd


static void
bus_dmamem_free_obsd(bus_dma_tag_t tag, bus_dma_segment_t* segs, int nsegs)
{
	for (int i = 0; i < nsegs; i++)
		bus_dmamem_free_tagless((void*)segs[i].ds_addr, segs[i].ds_len);
}
#define bus_dmamem_free bus_dmamem_free_obsd


static int
bus_dmamem_map_obsd(bus_dma_tag_t tag, bus_dma_segment_t* segs, int nsegs, size_t size, caddr_t* kvap, int flags)
{
	if (nsegs != 1)
		return EINVAL;

	*kvap = (caddr_t)segs[0].ds_addr;
	return 0;
}
#define bus_dmamem_map bus_dmamem_map_obsd


static void
bus_dmamem_unmap_obsd(bus_dma_tag_t tag, caddr_t kva, size_t size)
{
	// Nothing to do.
}
#define bus_dmamem_unmap bus_dmamem_unmap_obsd


#endif	/* _OBSD_COMPAT_MACHINE_BUS_H_ */
