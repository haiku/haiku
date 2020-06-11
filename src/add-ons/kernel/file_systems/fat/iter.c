/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/


#include "iter.h"

#include <string.h>

#include <fs_cache.h>
#include <fs_info.h>
#include <KernelExport.h>

#include "dosfs.h"
#include "fat.h"
#include "util.h"


#define DPRINTF(a,b) if (debug_iter > (a)) dprintf b


static int
_validate_cs_(nspace *vol, uint32 cluster, uint32 sector)
{
	if (sector < 0) return -1;

	if ((vol->fat_bits != 32) && IS_FIXED_ROOT(cluster)) { // fat12 or fat16 root
		if (sector >= vol->root_sectors)
			return -1;
		return 0;
	}

	if (sector >= vol->sectors_per_cluster) return -1;

	if (!IS_DATA_CLUSTER(cluster)) return -1;

	return 0;
}


off_t
csi_to_block(struct csi *csi)
{
	// presumes the caller has already called _validate_cs_ on the argument
	ASSERT(_validate_cs_(csi->vol, csi->cluster, csi->sector) == 0);
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	if (IS_FIXED_ROOT(csi->cluster))
		return csi->vol->root_start + csi->sector;

	return csi->vol->data_start +
		(off_t)(csi->cluster - 2)* csi->vol->sectors_per_cluster +
		csi->sector;
}


int
init_csi(nspace *vol, uint32 cluster, uint32 sector, struct csi *csi)
{
	int ret;
	if ((ret = _validate_cs_(vol,cluster,sector)) != 0)
		return ret;

	csi->vol = vol; csi->cluster = cluster; csi->sector = sector;

	return 0;
}


int
iter_csi(struct csi *csi, int sectors)
{
	if (csi->sector == 0xffff) // check if already at end of chain
		return -1;

	if (sectors < 0)
		return EINVAL;

	if (sectors == 0)
		return 0;

	if (IS_FIXED_ROOT(csi->cluster)) {
		csi->sector += sectors;
		if (csi->sector < csi->vol->root_sectors)
			return 0;
	} else {
		csi->sector += sectors;
		if (csi->sector < csi->vol->sectors_per_cluster)
			return 0;
		csi->cluster = get_nth_fat_entry(csi->vol, csi->cluster,
			csi->sector / csi->vol->sectors_per_cluster);

		if ((int32)csi->cluster < 0) {
			csi->sector = 0xffff;
			return csi->cluster;
		}

		if (vIS_DATA_CLUSTER(csi->vol,csi->cluster)) {
			csi->sector %= csi->vol->sectors_per_cluster;
			return 0;
		}
	}

	csi->sector = 0xffff;

	return -1;
}


uint8 *
csi_get_block(struct csi *csi)
{
	status_t status;
	const void* block;

	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return NULL;

	status = block_cache_get_etc(csi->vol->fBlockCache,
		csi_to_block(csi), 1, csi->vol->bytes_per_sector, &block);
	if (status != B_OK)
		return NULL;
	return (uint8 *)block;
}


status_t
csi_release_block(struct csi *csi)
{
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	block_cache_put(csi->vol->fBlockCache, csi_to_block(csi));
	return B_OK;
}


status_t
csi_make_writable(struct csi *csi)
{
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	return block_cache_make_writable(csi->vol->fBlockCache, csi_to_block(csi),
		-1);
}


/* XXX: not the most efficient implementation, but it gets the job done */
status_t
csi_read_blocks(struct csi *csi, uint8 *buffer, ssize_t len)
{
	struct csi old_csi;
	uint32 sectors;
	off_t block;
	status_t err;
	uint8 *buf = buffer;
	int32 i;

	ASSERT(len >= csi->vol->bytes_per_sector);

	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	sectors = 1;
	block = csi_to_block(csi);

	while (1) {
		old_csi = *csi;
		err = iter_csi(csi, 1);
		if (len < (sectors + 1) * csi->vol->bytes_per_sector)
			break;
		if ((err < B_OK) || (block + sectors != csi_to_block(csi)))
			break;
		sectors++;
	}

	for (i = block; i < block + sectors; i++) {
		char *blockData = (char *)block_cache_get(csi->vol->fBlockCache, i);
		memcpy(buf, blockData, csi->vol->bytes_per_sector);
		buf += csi->vol->bytes_per_sector;
		block_cache_put(csi->vol->fBlockCache, i);
	}

	*csi = old_csi;

	return sectors * csi->vol->bytes_per_sector;
}


status_t
csi_write_blocks(struct csi *csi, uint8 *buffer, ssize_t len)
{
	struct csi old_csi;
	uint32 sectors;
	off_t block;
	status_t err;
	uint8 *buf = buffer;
	int32 i;

	ASSERT(len >= csi->vol->bytes_per_sector);

	ASSERT(_validate_cs_(csi->vol, csi->cluster, csi->sector) == 0);
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	sectors = 1;
	block = csi_to_block(csi);

	while (1) {
		old_csi = *csi;
		err = iter_csi(csi, 1);
		if (len < (sectors + 1) * csi->vol->bytes_per_sector)
			break;
		if ((err < B_OK) || (block + sectors != csi_to_block(csi)))
			break;
		sectors++;
	}

	for (i = block; i < block + sectors; i++) {
		char *blockData;
		status_t status = block_cache_get_writable_etc(csi->vol->fBlockCache,
			i, 0, 1, -1, &blockData);
		if (status != B_OK)
			return status;

		memcpy(blockData, buf, csi->vol->bytes_per_sector);
		buf += csi->vol->bytes_per_sector;
		block_cache_put(csi->vol->fBlockCache, i);
	}

	/* return the last state of the iterator because that's what dosfs_write
	 * expects. this lets it meaningfully cache the state even when it's
	 * writing to the end of the file. */
	*csi = old_csi;

	return sectors * csi->vol->bytes_per_sector;
}


status_t
csi_write_block(struct csi *csi, uint8 *buffer)
{
	status_t status;
	off_t block;
	char *blockData;

	block = csi_to_block(csi);

	ASSERT(_validate_cs_(csi->vol, csi->cluster, csi->sector) == 0);
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	status = block_cache_get_writable_etc(csi->vol->fBlockCache, block, 0, 1,
		-1, &blockData);
	if (status != B_OK)
		return status;

	memcpy(blockData, buffer, csi->vol->bytes_per_sector);
	block_cache_put(csi->vol->fBlockCache, block);

	return B_OK;
}


//	#pragma mark -


static void
_diri_release_current_block_(struct diri *diri)
{
	ASSERT(diri->current_block);
	if (diri->current_block == NULL)
		return;
	csi_release_block(&(diri->csi));
	diri->current_block = NULL;
}


uint8 *
diri_init(nspace *vol, uint32 cluster, uint32 index, struct diri *diri)
{
	diri->current_block = NULL;

	if (cluster >= vol->total_clusters + 2)
		return NULL;

	if (init_csi(vol,cluster,0,&(diri->csi)) != 0)
		return NULL;

	diri->starting_cluster = cluster;
	diri->current_index = index;
	if (index >= vol->bytes_per_sector / 0x20
		&& iter_csi(&(diri->csi), diri->current_index
				/ (vol->bytes_per_sector / 0x20)) != 0)
		return NULL;

	// get current sector
	diri->current_block = csi_get_block(&diri->csi);

	if (diri->current_block == NULL)
		return NULL;

	// now the diri is valid
	return diri->current_block
		+ (diri->current_index % (diri->csi.vol->bytes_per_sector / 0x20))*0x20;
}


int
diri_free(struct diri *diri)
{
	if (diri->current_block)
		_diri_release_current_block_(diri);

	return 0;
}


uint8 *
diri_current_entry(struct diri *diri)
{
	if (diri->current_block == NULL)
		return NULL;

	return diri->current_block
		+ (diri->current_index % (diri->csi.vol->bytes_per_sector / 0x20))*0x20;
}


uint8 *
diri_next_entry(struct diri *diri)
{
	if (diri->current_block == NULL)
		return NULL;

	if ((++diri->current_index % (diri->csi.vol->bytes_per_sector / 0x20)) == 0) {
		_diri_release_current_block_(diri);
		if (iter_csi(&(diri->csi), 1) != 0)
			return NULL;
		diri->current_block = csi_get_block(&(diri->csi));
		if (diri->current_block == NULL)
			return NULL;
	}

	return diri->current_block
		+ (diri->current_index % (diri->csi.vol->bytes_per_sector / 0x20))*0x20;
}


uint8 *
diri_rewind(struct diri *diri)
{
	if (diri->current_index > (diri->csi.vol->bytes_per_sector / 0x20 - 1)) {
		if (diri->current_block)
			_diri_release_current_block_(diri);
		if (init_csi(diri->csi.vol, diri->starting_cluster, 0, &(diri->csi)) != 0)
			return NULL;
		diri->current_block = csi_get_block(&diri->csi);
	}
	diri->current_index = 0;
	return diri->current_block;
}


void
diri_make_writable(struct diri *diri)
{
	csi_make_writable(&diri->csi);
}
