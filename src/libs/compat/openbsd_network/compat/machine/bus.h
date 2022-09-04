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
	int nsegments, bus_size_t maxsegsz, bus_size_t boundary, bus_size_t alignment,
	int flags, bus_dmamap_t* dmamp, int no_alloc_map)
{
	*dmamp = calloc(sizeof(struct bus_dmamap_obsd) + (sizeof(bus_dma_segment_t) * nsegments), 1);
	if ((*dmamp) == NULL)
		return ENOMEM;

	int error = bus_dma_tag_create(tag, alignment, boundary,
		BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR, NULL, NULL,
		maxsize, nsegments, maxsegsz, flags, NULL, NULL,
		&(*dmamp)->_dmat);
	if (error != 0)
		return error;

	if (!no_alloc_map)
		error = bus_dmamap_create((*dmamp)->_dmat, flags, &(*dmamp)->_dmamp);
	return error;
}
#define bus_dmamap_create(tag, maxsize, nsegments, maxsegsz, boundary, flags, dmamp) \
	bus_dmamap_create_obsd(tag, maxsize, nsegments, maxsegsz, boundary, 1, flags, dmamp, 0)


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


#endif	/* _OBSD_COMPAT_MACHINE_BUS_H_ */
