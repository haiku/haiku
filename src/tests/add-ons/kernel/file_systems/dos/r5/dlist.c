/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/*
directory vnode id list

We only add to this list as we encounter directories; there is no need to
scan through the directories ourselves since we aren't worried about preserving
vnid's across reboots.

We don't worry about aliases for directories since their cluster values will
always be the same -- searches are performed only on the starting cluster
number of the directories.

TODO:
	XXX: make this more efficient
*/

#define DPRINTF(a,b) if (debug_dlist > (a)) dprintf b

#include <KernelExport.h>
#include <fsproto.h>

#include <string.h>

#include "dosfs.h"
#include "dlist.h"
#include "util.h"
#include "vcache.h"

#if DEBUG
	#define DLIST_ENTRY_QUANTUM 1
#else
	#define DLIST_ENTRY_QUANTUM 0x20
#endif

status_t dlist_init(nspace *vol)
{
	DPRINTF(0, ("dlist_init called\n"));

	vol->dlist.entries = 0;
	vol->dlist.allocated = DLIST_ENTRY_QUANTUM;
	vol->dlist.vnid_list = malloc(sizeof(vnode_id) * vol->dlist.allocated);
	if (vol->dlist.vnid_list == NULL) {
		vol->dlist.allocated = 0;
		dprintf("dlist_init: out of core\n");
		return ENOMEM;
	}

	return B_OK;
}

status_t dlist_uninit(nspace *vol)
{
	DPRINTF(0, ("dlist_uninit called\n"));

	if (vol->dlist.vnid_list)
		free(vol->dlist.vnid_list);
	vol->dlist.entries = vol->dlist.allocated = 0;
	vol->dlist.vnid_list = NULL;

	return B_OK;
}

static status_t dlist_realloc(nspace *vol, uint32 allocate)
{
	vnode_id *vnid_list;

	DPRINTF(0, ("dlist_realloc %lx -> %lx\n", vol->dlist.allocated, allocate));

	ASSERT(allocate != vol->dlist.allocated);
	ASSERT(allocate > vol->dlist.entries);

	vnid_list = malloc(sizeof(vnode_id) * allocate);
	if (vnid_list == NULL) {
		dprintf("dlist_realloc: out of core\n");
		return ENOMEM;
	}

	memcpy(vnid_list, vol->dlist.vnid_list, sizeof(vnode_id) * vol->dlist.entries);
	free(vol->dlist.vnid_list);
	vol->dlist.vnid_list = vnid_list;
	vol->dlist.allocated = allocate;

	return B_OK;
}

status_t dlist_add(nspace *vol, vnode_id vnid)
{
	DPRINTF(0, ("dlist_add vnid %Lx\n", vnid));

	ASSERT(IS_DIR_CLUSTER_VNID(vnid) || IS_ARTIFICIAL_VNID(vnid));
	ASSERT(vnid != 0);
	// XXX: check for duplicate entries

	if (vol->dlist.entries == vol->dlist.allocated) {
		if (dlist_realloc(vol, vol->dlist.allocated + DLIST_ENTRY_QUANTUM) < 0)
			return B_ERROR;
	}
	vol->dlist.vnid_list[vol->dlist.entries++] = vnid;

	return B_OK;
}

status_t dlist_remove(nspace *vol, vnode_id vnid)
{
	uint32 i;

	DPRINTF(0, ("dlist_remove vnid %Lx\n", vnid));

	for (i=0;i<vol->dlist.entries;i++)
		if (vol->dlist.vnid_list[i] == vnid)
			break;
	ASSERT(i < vol->dlist.entries);
	if (i == vol->dlist.entries)
		return ENOENT;
	for (;i<vol->dlist.entries-1;i++)
		vol->dlist.vnid_list[i] = vol->dlist.vnid_list[i+1];
	vol->dlist.entries--;

	if (vol->dlist.allocated - vol->dlist.entries > 2*DLIST_ENTRY_QUANTUM)
		return dlist_realloc(vol, vol->dlist.allocated - DLIST_ENTRY_QUANTUM);

	return B_OK;
}

vnode_id dlist_find(nspace *vol, uint32 cluster)
{
	uint32 i;

	DPRINTF(1, ("dlist_find cluster %lx\n", cluster));

	ASSERT(((cluster >= 2) && (cluster < vol->total_clusters + 2)) || (cluster == 1));

	for (i=0;i<vol->dlist.entries;i++) {
		vnode_id loc;

		if (vcache_vnid_to_loc(vol, vol->dlist.vnid_list[i], &loc) < B_OK)
			loc = vol->dlist.vnid_list[i];
		ASSERT(IS_DIR_CLUSTER_VNID(loc));
		if (CLUSTER_OF_DIR_CLUSTER_VNID(loc) == cluster)
			return vol->dlist.vnid_list[i];
	}

	DPRINTF(1, ("dlist_find cluster %lx not found\n", cluster));

	return -1LL;
}

void dlist_dump(nspace *vol)
{
	uint32 i;

	dprintf("%lx/%lx dlist entries filled, QUANTUM = %x\n",
		vol->dlist.entries, vol->dlist.allocated, DLIST_ENTRY_QUANTUM);

	for (i=0;i<vol->dlist.entries;i++)
		dprintf("%s %Lx", ((i == 0) ? "entries:" : ","), vol->dlist.vnid_list[i]);

	dprintf("\n");
}
