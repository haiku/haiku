/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#include <KernelExport.h> 

#include <fsproto.h>
#include <lock.h>
#include <cache.h>

#include "iter.h"
#include "dosfs.h"
#include "fat.h"
#include "util.h"

#define DPRINTF(a,b) if (debug_iter > (a)) dprintf b

CHECK_MAGIC(diri,struct diri,DIRI_MAGIC)

static int _validate_cs_(nspace *vol, uint32 cluster, uint32 sector)
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

static off_t _csi_to_block_(struct csi *csi)
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

int init_csi(nspace *vol, uint32 cluster, uint32 sector, struct csi *csi)
{
	int ret;
	if ((ret = _validate_cs_(vol,cluster,sector)) != 0)
		return ret;
	
	csi->vol = vol; csi->cluster = cluster; csi->sector = sector;
	
	return 0;
}

int iter_csi(struct csi *csi, int sectors)
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
		csi->cluster = get_nth_fat_entry(csi->vol, csi->cluster, csi->sector / csi->vol->sectors_per_cluster);

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

uint8 *csi_get_block(struct csi *csi)
{
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return NULL;

	return get_block(csi->vol->fd, _csi_to_block_(csi), csi->vol->bytes_per_sector);
}

status_t csi_release_block(struct csi *csi)
{
	status_t err;

	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	err = release_block(csi->vol->fd, _csi_to_block_(csi));
	ASSERT(err == B_OK);
	return err;
}

status_t csi_mark_block_dirty(struct csi *csi)
{
	status_t err;

	ASSERT(_validate_cs_(csi->vol, csi->cluster, csi->sector) == 0);
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	err = mark_blocks_dirty(csi->vol->fd, _csi_to_block_(csi), 1);
	ASSERT(err == B_OK);
	return err;
}

/* XXX: not the most efficient implementation, but it gets the job done */
status_t csi_read_blocks(struct csi *csi, uint8 *buffer, ssize_t len)
{
	struct csi old_csi;
	uint32 sectors;
	off_t block;
	status_t err;

	ASSERT(len >= csi->vol->bytes_per_sector);

	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	sectors = 1;
	block = _csi_to_block_(csi);

	while (1) {
		old_csi = *csi;
		err = iter_csi(csi, 1);
		if (len < (sectors + 1) * csi->vol->bytes_per_sector)
			break;
		if ((err < B_OK) || (block + sectors != _csi_to_block_(csi)))
			break;
		sectors++;
	}

	err = cached_read(csi->vol->fd, block, buffer, sectors, csi->vol->bytes_per_sector);
	if (err < B_OK)
		return err;

	*csi = old_csi;

	return sectors * csi->vol->bytes_per_sector;
}

status_t csi_write_blocks(struct csi *csi, uint8 *buffer, ssize_t len)
{
	struct csi old_csi;
	uint32 sectors;
	off_t block;
	status_t err;

	ASSERT(len >= csi->vol->bytes_per_sector);

	ASSERT(_validate_cs_(csi->vol, csi->cluster, csi->sector) == 0);
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	sectors = 1;
	block = _csi_to_block_(csi);

	while (1) {
		old_csi = *csi;
		err = iter_csi(csi, 1);
		if (len < (sectors + 1) * csi->vol->bytes_per_sector)
			break;
		if ((err < B_OK) || (block + sectors != _csi_to_block_(csi)))
			break;
		sectors++;
	}

	err = cached_write(csi->vol->fd, block, buffer, sectors, csi->vol->bytes_per_sector);
	if (err < B_OK)
		return err;

	/* return the last state of the iterator because that's what dosfs_write
	 * expects. this lets it meaningfully cache the state even when it's
	 * writing to the end of the file. */
	*csi = old_csi;

	return sectors * csi->vol->bytes_per_sector;
}

status_t csi_write_block(struct csi *csi, uint8 *buffer)
{
	ASSERT(_validate_cs_(csi->vol, csi->cluster, csi->sector) == 0);
	if (_validate_cs_(csi->vol, csi->cluster, csi->sector) != 0)
		return EINVAL;

	return cached_write(csi->vol->fd, _csi_to_block_(csi), buffer, 1, csi->vol->bytes_per_sector);
}

static void _diri_release_current_block_(struct diri *diri)
{
	ASSERT(diri->current_block);
	if (diri->current_block == NULL)
		return;
	csi_release_block(&(diri->csi));
	diri->current_block = NULL;
}

uint8 *diri_init(nspace *vol, uint32 cluster, uint32 index, struct diri *diri)
{
	diri->magic = DIRI_MAGIC;
	diri->current_block = NULL;

	if (cluster >= vol->total_clusters + 2)
		return NULL;

	if (init_csi(vol,cluster,0,&(diri->csi)) != 0)
		return NULL;

	diri->starting_cluster = cluster;
	diri->current_index = index;
	if (index >= vol->bytes_per_sector / 0x20) {
		if (iter_csi(&(diri->csi), diri->current_index / (vol->bytes_per_sector / 0x20)) != 0)
			return NULL;
	}

	// get current sector
	diri->current_block = csi_get_block(&(diri->csi));

	if (diri->current_block == NULL)
		return NULL;

	return diri->current_block + (diri->current_index % (diri->csi.vol->bytes_per_sector / 0x20))*0x20;
}

int diri_free(struct diri *diri)
{
	if (check_diri_magic(diri, "diri_free")) return EINVAL;

	diri->magic = ~DIRI_MAGIC; // trash magic number

	if (diri->current_block)
		_diri_release_current_block_(diri);

	return 0;
}

uint8 *diri_current_entry(struct diri *diri)
{
	if (check_diri_magic(diri, "diri_current_entry")) return NULL;
	
	if (diri->current_block == NULL)
		return NULL;

	return diri->current_block + (diri->current_index % (diri->csi.vol->bytes_per_sector / 0x20))*0x20;
}

uint8 *diri_next_entry(struct diri *diri)
{
	if (check_diri_magic(diri, "diri_next_entry")) return NULL;
	
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
	
	return diri->current_block + (diri->current_index % (diri->csi.vol->bytes_per_sector / 0x20))*0x20;
}

uint8 *diri_rewind(struct diri *diri)
{
	if (check_diri_magic(diri, "diri_rewind")) return NULL;

	if (diri->current_index > (diri->csi.vol->bytes_per_sector / 0x20 - 1)) {
		if (diri->current_block)
			_diri_release_current_block_(diri);
		if (init_csi(diri->csi.vol, diri->starting_cluster, 0, &(diri->csi)) != 0)
			return NULL;
		diri->current_block = csi_get_block(&(diri->csi));
	}
	diri->current_index = 0;
	return diri->current_block;
}

void diri_mark_dirty(struct diri *diri)
{
	csi_mark_block_dirty(&(diri->csi));
}
