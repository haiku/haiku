/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_FAT_H_
#define _DOSFS_FAT_H_

#define vIS_DATA_CLUSTER(vol,cluster) (((cluster) >= 2) && ((cluster) < vol->total_clusters + 2))
#define IS_DATA_CLUSTER(cluster) vIS_DATA_CLUSTER(vol,cluster)

// cluster 1 represents root directory for fat12 and fat16
#define IS_FIXED_ROOT(cluster) ((cluster) == 1)

int32		count_free_clusters(nspace *vol);
int32		get_nth_fat_entry(nspace *vol, int32 cluster, uint32 n);
uint32		count_clusters(nspace *vol, int32 cluster);

/* remember to update vnode iteration after calling this function */
status_t	clear_fat_chain(nspace *vol, uint32 cluster);

/* remember to set end of chain field when merging into a vnode */
status_t	allocate_n_fat_entries(nspace *vol, int32 n, int32 *start);

/* remember to update vnode iteration after calling this function */
status_t	set_fat_chain_length(nspace *vol, vnode *node, uint32 clusters);

void		dump_fat_chain(nspace *vol, uint32 cluster);

status_t	fragment(nspace *vol, uint32 *pattern);

#endif
