/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/


#include "fat.h"

#include <stdlib.h>
#include <string.h>

#include <fs_cache.h>
#include <ByteOrder.h>
#include <KernelExport.h>

#include "dosfs.h"
#include "file.h"
#include "util.h"
#include "vcache.h"


#define END_FAT_ENTRY 0x0fffffff
#define BAD_FAT_ENTRY 0x0ffffff1

#define DPRINTF(a,b) if (debug_fat > (a)) dprintf b


static status_t
mirror_fats(nspace *vol, uint32 sector, uint8 *buffer)
{
	uint32 i;

	if (!vol->fat_mirrored)
		return B_OK;

	sector -= vol->active_fat * vol->sectors_per_fat;

	for (i = 0; i < vol->fat_count; i++) {
		char *blockData;
		status_t status;
		if (i == vol->active_fat)
			continue;

		status = block_cache_get_writable_etc(vol->fBlockCache,
			sector + i * vol->sectors_per_fat, 0, 1, -1, &blockData);
		if (status != B_OK)
			return status;

		memcpy(blockData, buffer, vol->bytes_per_sector);
		block_cache_put(vol->fBlockCache, sector + i * vol->sectors_per_fat);
	}

	return B_OK;
}


static int32
_count_free_clusters_fat32(nspace *vol)
{
	int32 count = 0;
	const uint8 *block;
	uint32 fat_sector;
	uint32 i;
	uint32 cur_sector;

	cur_sector = vol->reserved_sectors + vol->active_fat * vol->sectors_per_fat;

	for (fat_sector = 0; fat_sector < vol->sectors_per_fat; fat_sector++) {
		block = (uint8 *)block_cache_get(vol->fBlockCache, cur_sector);
		if(block == NULL)
			return B_IO_ERROR;

		for (i = 0; i < vol->bytes_per_sector; i += sizeof(uint32)) {
			uint32 val = read32(block, i);
			if ((val & 0x0fffffff) == 0)
				count++;
		}

		block_cache_put(vol->fBlockCache, cur_sector);
		cur_sector++;
	}

	return count;
}

// count free: no parameters. returns int32
// get_entry: cluster #. returns int32 entry/status
// set_entry: cluster #, value. returns int32 status
// allocate: # clusters in N, returns int32 status/starting cluster

enum {
	_IOCTL_COUNT_FREE_,
	_IOCTL_GET_ENTRY_,
	_IOCTL_SET_ENTRY_,
	_IOCTL_ALLOCATE_N_ENTRIES_
};

static int32
_fat_ioctl_(nspace *vol, uint32 action, uint32 cluster, int32 N)
{
	int32 result = 0;
	uint32 n = 0, first = 0, last = 0;
	uint32 i;
	uint32 sector;
	uint32 offset, value = 0; /* quiet warning */
	uint8 *block1, *block2 = NULL; /* quiet warning */
	bool readOnly
		= action != _IOCTL_SET_ENTRY_ && action != _IOCTL_ALLOCATE_N_ENTRIES_;

	// mark end of chain for allocations
	uint32 endOfChainMarker = (action == _IOCTL_SET_ENTRY_) ? N : 0x0fffffff;

	ASSERT(action >= _IOCTL_COUNT_FREE_
		&& action <= _IOCTL_ALLOCATE_N_ENTRIES_);

	DPRINTF(3, ("_fat_ioctl_: action %" B_PRIu32 ", cluster %" B_PRIu32
		", N %" B_PRId32 "\n", action, cluster, N));

	if (action == _IOCTL_COUNT_FREE_) {
		if(vol->fat_bits == 32)
			// use a optimized version of the cluster counting algorithms
			return _count_free_clusters_fat32(vol);
		else
			cluster = 2;
	}

	if (action == _IOCTL_ALLOCATE_N_ENTRIES_)
		cluster = vol->last_allocated;

	if (action != _IOCTL_COUNT_FREE_) {
		if (!IS_DATA_CLUSTER(cluster)) {
			DPRINTF(0, ("_fat_ioctl_ called with invalid cluster (%" B_PRIu32
				")\n", cluster));
			return B_BAD_VALUE;
		}
	}

	offset = cluster * vol->fat_bits / 8;
	sector = vol->reserved_sectors + vol->active_fat * vol->sectors_per_fat +
		offset / vol->bytes_per_sector;
	offset %= vol->bytes_per_sector;

	if (readOnly) {
		block1 = (uint8 *)block_cache_get(vol->fBlockCache, sector);
	} else {
		block1 = (uint8 *)block_cache_get_writable(vol->fBlockCache, sector,
			-1);
	}

	if (block1 == NULL) {
		DPRINTF(0, ("_fat_ioctl_: error reading fat (sector %" B_PRIu32 ")\n",
			sector));
		return B_IO_ERROR;
	}

	for (i = 0; i < vol->total_clusters; i++) {
		ASSERT(IS_DATA_CLUSTER(cluster));
		ASSERT(offset == ((cluster * vol->fat_bits / 8)
			% vol->bytes_per_sector));

		if (vol->fat_bits == 12) {
			if (offset == vol->bytes_per_sector - 1) {
				if (readOnly) {
					block2 = (uint8 *)block_cache_get(vol->fBlockCache,
						++sector);
				} else {
					block2 = (uint8 *)block_cache_get_writable(vol->fBlockCache,
						++sector, -1);
				}

				if (block2 == NULL) {
					DPRINTF(0, ("_fat_ioctl_: error reading fat (sector %"
						B_PRIu32 ")\n", sector));
					result = B_IO_ERROR;
					sector--;
					goto bi;
				}
			}

			if (action != _IOCTL_SET_ENTRY_) {
				if (offset == vol->bytes_per_sector - 1)
					value = block1[offset] + 0x100 * block2[0];
				else
					value = block1[offset] + 0x100 * block1[offset + 1];

				if (cluster & 1)
					value >>= 4;
				else
					value &= 0xfff;

				if (value > 0xff0)
					value |= 0x0ffff000;
			}

			if (((action == _IOCTL_ALLOCATE_N_ENTRIES_) && (value == 0))
				|| action == _IOCTL_SET_ENTRY_) {
				uint32 andmask, ormask;
				if (cluster & 1) {
					ormask = (endOfChainMarker & 0xfff) << 4;
					andmask = 0xf;
				} else {
					ormask = endOfChainMarker & 0xfff;
					andmask = 0xf000;
				}

				block1[offset] &= (andmask & 0xff);
				block1[offset] |= (ormask & 0xff);
				if (offset == vol->bytes_per_sector - 1) {
					mirror_fats(vol, sector - 1, block1);
					block2[0] &= (andmask >> 8);
					block2[0] |= (ormask >> 8);
				} else {
					block1[offset + 1] &= (andmask >> 8);
					block1[offset + 1] |= (ormask >> 8);
				}
			}

			if (offset == vol->bytes_per_sector - 1) {
				offset = (cluster & 1) ? 1 : 0;
				block_cache_put(vol->fBlockCache, sector - 1);
				block1 = block2;
			} else
				offset += (cluster & 1) ? 2 : 1;

		} else if (vol->fat_bits == 16) {
			if (action != _IOCTL_SET_ENTRY_) {
				value = read16(block1, offset);
				if (value > 0xfff0)
					value |= 0x0fff0000;
			}

			if (((action == _IOCTL_ALLOCATE_N_ENTRIES_) && (value == 0))
				|| action == _IOCTL_SET_ENTRY_) {
				*(uint16 *)&block1[offset]
					= B_HOST_TO_LENDIAN_INT16(endOfChainMarker);
			}

			offset += 2;
		} else if (vol->fat_bits == 32) {
			if (action != _IOCTL_SET_ENTRY_)
				value = read32(block1, offset) & 0x0fffffff;

			if (((action == _IOCTL_ALLOCATE_N_ENTRIES_) && (value == 0))
				|| action == _IOCTL_SET_ENTRY_) {
				ASSERT((endOfChainMarker & 0xf0000000) == 0);
				*(uint32 *)&block1[offset]
					= B_HOST_TO_LENDIAN_INT32(endOfChainMarker);
			}

			offset += 4;
		} else
			ASSERT(0);

		if (action == _IOCTL_COUNT_FREE_) {
			if (value == 0)
				result++;
		} else if (action == _IOCTL_GET_ENTRY_) {
			result = value;
			goto bi;
		} else if (action == _IOCTL_SET_ENTRY_) {
			mirror_fats(vol, sector, block1);
			goto bi;
		} else if (action == _IOCTL_ALLOCATE_N_ENTRIES_ && value == 0) {
			vol->free_clusters--;
			mirror_fats(vol, sector, block1);

			if (n == 0) {
				ASSERT(first == 0);
				first = last = cluster;
			} else {
				ASSERT(IS_DATA_CLUSTER(first));
				ASSERT(IS_DATA_CLUSTER(last));
				// set last cluster to point to us

				result = _fat_ioctl_(vol, _IOCTL_SET_ENTRY_, last, cluster);
				if (result < 0) {
					ASSERT(0);
					goto bi;
				}

				last = cluster;
			}

			if (++n == N)
				goto bi;
		}

		// iterate cluster and sector if needed
		if (++cluster == vol->total_clusters + 2) {
			block_cache_put(vol->fBlockCache, sector);

			cluster = 2;
			offset = cluster * vol->fat_bits / 8;
			sector = vol->reserved_sectors + vol->active_fat
				* vol->sectors_per_fat;

			if (readOnly)
				block1 = (uint8 *)block_cache_get(vol->fBlockCache, sector);
			else {
				block1 = (uint8 *)block_cache_get_writable(vol->fBlockCache,
					sector, -1);
			}
		}

		if (offset >= vol->bytes_per_sector) {
			block_cache_put(vol->fBlockCache, sector);

			sector++;
			offset -= vol->bytes_per_sector;
			ASSERT(sector < vol->reserved_sectors + (vol->active_fat + 1)
				* vol->sectors_per_fat);

			if (readOnly)
				block1 = (uint8 *)block_cache_get(vol->fBlockCache, sector);
			else {
				block1 = (uint8 *)block_cache_get_writable(vol->fBlockCache,
					sector, -1);
			}
		}

		if (block1 == NULL) {
			DPRINTF(0, ("_fat_ioctl_: error reading fat (sector %" B_PRIu32
				")\n", sector));
			result = B_IO_ERROR;
			goto bi;
		}
	}

bi:
	if (block1 != NULL)
		block_cache_put(vol->fBlockCache, sector);

	if (action == _IOCTL_ALLOCATE_N_ENTRIES_) {
		if (result < 0) {
			DPRINTF(0, ("pooh. there is a problem. clearing chain (%" B_PRIu32
				")\n", first));
			if (first != 0)
				clear_fat_chain(vol, first, false);
		} else if (n != N) {
			DPRINTF(0, ("not enough free entries (%" B_PRId32 "/%" B_PRId32
				" found)\n", n, N));
			if (first != 0)
				clear_fat_chain(vol, first, false);
			result = B_DEVICE_FULL;
		} else if (result == 0) {
			vol->last_allocated = cluster;
			result = first;
			ASSERT(IS_DATA_CLUSTER(first));
		}
	}

	if (result < B_OK) {
		DPRINTF(0, ("_fat_ioctl_ error: action = %" B_PRIu32 " cluster = %"
			B_PRIu32 " N = %" B_PRId32 " (%s)\n", action, cluster, N,
			strerror(result)));
	}

	return result;
}


int32
count_free_clusters(nspace *vol)
{
	return _fat_ioctl_(vol, _IOCTL_COUNT_FREE_, 0, 0);
}


static int32
get_fat_entry(nspace *vol, uint32 cluster)
{
	int32 value = _fat_ioctl_(vol, _IOCTL_GET_ENTRY_, cluster, 0);

	if (value < 0)
		return value;

	if (value == 0 || IS_DATA_CLUSTER(value))
		return value;

	if (value > 0x0ffffff7)
		return END_FAT_ENTRY;

	if (value > 0x0ffffff0)
		return BAD_FAT_ENTRY;

	DPRINTF(0, ("invalid fat entry: %" B_PRIu32 "\n", value));
	return BAD_FAT_ENTRY;
}


static status_t
set_fat_entry(nspace *vol, uint32 cluster, int32 value)
{
	return _fat_ioctl_(vol, _IOCTL_SET_ENTRY_, cluster, value);
}


//! Traverse n fat entries
int32
get_nth_fat_entry(nspace *vol, int32 cluster, uint32 n)
{
	while (n--) {
		cluster = get_fat_entry(vol, cluster);

		if (!IS_DATA_CLUSTER(cluster))
			break;
	}

	ASSERT(cluster != 0);
	return cluster;
}


// count number of clusters in fat chain starting at given cluster
// should only be used for calculating directory sizes because it doesn't
// return proper error codes
uint32
count_clusters(nspace *vol, int32 cluster)
{
	int32 count = 0;

	DPRINTF(2, ("count_clusters %" B_PRId32 "\n", cluster));

	// not intended for use on root directory
	if (!IS_DATA_CLUSTER(cluster)) {
		DPRINTF(0, ("count_clusters called on invalid cluster (%" B_PRId32
			")\n", cluster));
		return 0;
	}

	while (IS_DATA_CLUSTER(cluster)) {
		count++;

		// break out of circular fat chains in a sketchy manner
		if (count == vol->total_clusters)
			return 0;

		cluster = get_fat_entry(vol, cluster);
	}

	DPRINTF(2, ("count_clusters %" B_PRId32 " = %" B_PRId32 "\n", cluster,
		count));

	if (cluster == END_FAT_ENTRY)
		return count;

	dprintf("cluster = %" B_PRId32 "\n", cluster);
	ASSERT(0);
	return 0;
}


status_t
clear_fat_chain(nspace *vol, uint32 cluster, bool discardBlockCache)
{
	int32 c;
	status_t result;

	if (!IS_DATA_CLUSTER(cluster)) {
		DPRINTF(0, ("clear_fat_chain called on invalid cluster (%" B_PRIu32
			")\n", cluster));
		return B_BAD_VALUE;
	}

	ASSERT(count_clusters(vol, cluster) != 0);

	DPRINTF(2, ("clearing fat chain: %" B_PRIu32, cluster));
	while (IS_DATA_CLUSTER(cluster)) {
		if ((c = get_fat_entry(vol, cluster)) < 0) {
			DPRINTF(0, ("clear_fat_chain: error clearing fat entry for cluster "
				"%" B_PRIu32 " (%s)\n", cluster, strerror(c)));
			return c;
		}

		if ((result = set_fat_entry(vol, cluster, 0)) != B_OK) {
			DPRINTF(0, ("clear_fat_chain: error clearing fat entry for cluster "
				"%" B_PRIu32 " (%s)\n", cluster, strerror(result)));
			return result;
		}

		if (discardBlockCache) {
			block_cache_discard(vol->fBlockCache,
				vol->data_start + (cluster - 2) * vol->sectors_per_cluster,
				vol->sectors_per_cluster);
		}

		vol->free_clusters++;
		cluster = c;
		DPRINTF(2, (", %" B_PRIu32, cluster));
	}
	DPRINTF(2, ("\n"));

	if (cluster != END_FAT_ENTRY) {
		dprintf("clear_fat_chain: fat chain terminated improperly with %"
			B_PRIu32 "\n", cluster);
	}

	return 0;
}


status_t
allocate_n_fat_entries(nspace *vol, int32 n, int32 *start)
{
	int32 c;

	ASSERT(n > 0);

	DPRINTF(2, ("allocating %" B_PRId32 " fat entries\n", n));

	c = _fat_ioctl_(vol, _IOCTL_ALLOCATE_N_ENTRIES_, 0, n);
	if (c < 0)
		return c;

	ASSERT(IS_DATA_CLUSTER(c));
	ASSERT(count_clusters(vol, c) == n);

	DPRINTF(2, ("allocated %" B_PRId32 " fat entries at %" B_PRId32 "\n", n,
		c));

	*start = c;
	return 0;
}


status_t
set_fat_chain_length(nspace *vol, vnode *node, uint32 clusters,
	bool discardBlockCache)
{
	status_t result;
	int32 i, c, n;

	DPRINTF(1, ("set_fat_chain_length: %" B_PRIdINO " to %" B_PRIu32
		" clusters (%" B_PRIu32 ")\n", node->vnid, clusters, node->cluster));

	if (IS_FIXED_ROOT(node->cluster)
		|| (!IS_DATA_CLUSTER(node->cluster) && (node->cluster != 0))) {
		DPRINTF(0, ("set_fat_chain_length called on invalid cluster (%" B_PRIu32
			")\n", node->cluster));
		return B_BAD_VALUE;
	}

	if (clusters == 0) {
		DPRINTF(1, ("truncating node to zero bytes\n"));
		if (node->cluster == 0)
			return B_OK;

		c = node->cluster;
		if ((result = clear_fat_chain(vol, c, discardBlockCache)) != B_OK)
			return result;

		node->cluster = 0;
		node->end_cluster = 0;

		// TODO: don't have to do this this way -- can clean up nicely
		do {
			result = vcache_set_entry(vol, node->vnid,
				GENERATE_DIR_INDEX_VNID(node->dir_vnid, node->sindex));
			// repeat until memory is freed up
			if (result != B_OK)
				snooze(5000LL);
		} while (result != B_OK);

		/* write to disk so that get_next_dirent doesn't barf */
		write_vnode_entry(vol, node);
		return result;
	}

	if (node->cluster == 0) {
		DPRINTF(1, ("node has no clusters. adding %" B_PRIu32 " clusters\n",
			clusters));

		if ((result = allocate_n_fat_entries(vol, clusters, &n)) != B_OK)
			return result;

		node->cluster = n;
		node->end_cluster = get_nth_fat_entry(vol, n, clusters - 1);

		// TODO: don't have to do this this way -- can clean up nicely
		do {
			result = vcache_set_entry(vol, node->vnid,
				GENERATE_DIR_CLUSTER_VNID(node->dir_vnid, node->cluster));
			// repeat until memory is freed up
			if (result != B_OK)
				snooze(5000LL);
		} while (result != B_OK);

		/* write to disk so that get_next_dirent doesn't barf */
		write_vnode_entry(vol, node);
		return result;
	}

	i = (node->st_size + vol->bytes_per_sector * vol->sectors_per_cluster - 1)
		/ vol->bytes_per_sector / vol->sectors_per_cluster;
	if (i == clusters)
		return B_OK;

	if (clusters > i) {
		// add new fat entries
		DPRINTF(1, ("adding %" B_PRIu32 " new fat entries\n", clusters - i));
		if ((result = allocate_n_fat_entries(vol, clusters - i, &n)) != B_OK)
			return result;

		ASSERT(IS_DATA_CLUSTER(n));

		result = set_fat_entry(vol, node->end_cluster, n);
		if (result < B_OK) {
			clear_fat_chain(vol, n, false);
			return result;
		}

		node->end_cluster = get_nth_fat_entry(vol, n, clusters - i - 1);
		return result;
	}

	// traverse fat chain
	c = node->cluster;
	n = get_fat_entry(vol, c);
	for (i = 1; i < clusters; i++) {
		if (!IS_DATA_CLUSTER(n))
			break;

		c = n;
		n = get_fat_entry(vol, c);
	}

	ASSERT(i == clusters);
	ASSERT(n != END_FAT_ENTRY);

	if (i == clusters && n == END_FAT_ENTRY)
		return B_OK;

	if (n < 0)
		return n;

	if (n != END_FAT_ENTRY && !IS_DATA_CLUSTER(n))
		return B_BAD_VALUE;

	// clear trailing fat entries
	DPRINTF(1, ("clearing trailing fat entries\n"));
	if ((result = set_fat_entry(vol, c, 0x0fffffff)) != B_OK)
		return result;

	node->end_cluster = c;
	return clear_fat_chain(vol, n, discardBlockCache);
}


void
dump_fat_chain(nspace *vol, uint32 cluster)
{
	dprintf("fat chain: %" B_PRIu32, cluster);
	while (IS_DATA_CLUSTER(cluster)) {
		cluster = get_fat_entry(vol, cluster);
		dprintf(" %" B_PRIu32, cluster);
	}

	dprintf("\n");
}

/*
status_t fragment(nspace *vol, uint32 *pattern)
{
	uint32 sector, offset, previous_entry, i, val;
	uchar *buffer;
	bool dirty = FALSE;

	srand(time(NULL)|1);

	if (vol->fat_bits == 16)
		previous_entry = 0xffff;
	else if (vol->fat_bits == 32)
		previous_entry = 0x0fffffff;
	else {
		dprintf("fragment: only for FAT16 and FAT32\n");
		return ENOSYS;
	}

	sector = vol->reserved_sectors + vol->active_fat * vol->sectors_per_fat +
			((vol->total_clusters + 2 - 1) * (vol->fat_bits / 8)) /
					vol->bytes_per_sector;
	offset = ((vol->total_clusters + 2 - 1) * (vol->fat_bits / 8)) %
			vol->bytes_per_sector;

	buffer = (uchar *)get_block(vol->fd, sector, vol->bytes_per_sector);
	if (!buffer) {
		dprintf("fragment: error getting fat block %lx\n", sector);
		return EINVAL;
	}

	val = pattern ? *pattern : rand();

	for (i=vol->total_clusters+1;i>=2;i--) {
		if (val & (1 << (i & 31))) {
			if (vol->fat_bits == 16) {
				if (read16(buffer, offset) == 0) {
					buffer[offset+0] = (previous_entry     ) & 0xff;
					buffer[offset+1] = (previous_entry >> 8) & 0xff;
					previous_entry = i;
					dirty = TRUE;
					vol->free_clusters--;
				}
			} else {
				if (read32(buffer, offset) == 0) {
					buffer[offset+0] = (previous_entry      ) & 0xff;
					buffer[offset+1] = (previous_entry >>  8) & 0xff;
					buffer[offset+2] = (previous_entry >> 16) & 0xff;
					buffer[offset+3] = (previous_entry >> 24) & 0xff;
					previous_entry = i;
					dirty = TRUE;
					vol->free_clusters--;
				}
			}
		}

		if (!offset) {
			if (dirty) {
				mark_blocks_dirty(vol->fd, sector, 1);
				mirror_fats(vol, sector, buffer);
			}
			release_block(vol->fd, sector);

			dirty = FALSE;
			sector--;

			buffer = (uchar *)get_block(vol->fd, sector,
					vol->bytes_per_sector);
			if (!buffer) {
				dprintf("fragment: error getting fat block %lx\n", sector);
				return EINVAL;
			}
		}

		offset = (offset - vol->fat_bits / 8 + vol->bytes_per_sector) %
				vol->bytes_per_sector;

		if (!pattern && ((i & 31) == 31))
			val = rand();
	}

	if (dirty) {
		mark_blocks_dirty(vol->fd, sector, 1);
		mirror_fats(vol, sector, buffer);
	}
	release_block(vol->fd, sector);

	vol->last_allocated = (rand() % vol->total_clusters) + 2;

	return B_OK;
}
*/

