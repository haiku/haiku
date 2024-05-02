/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_VCACHE_H_
#define _DOSFS_VCACHE_H_


#include <sys/mount.h>

#include "dosfs.h"


void dump_vcache(const mount* vol);

status_t init_vcache(mount* vol);
status_t uninit_vcache(mount* vol);

ino_t generate_unique_vnid(mount *vol);

status_t add_to_vcache(mount* vol, ino_t vnid, ino_t loc);
status_t remove_from_vcache(mount* vol, ino_t vnid);
status_t vcache_vnid_to_loc(mount* vol, ino_t vnid, ino_t* loc);
status_t vcache_loc_to_vnid(mount* vol, ino_t loc, ino_t* vnid);

status_t vcache_set_entry(mount* vol, ino_t vnid, ino_t loc);
status_t vcache_set_constructed(mount* vol, ino_t vnid, bool constructed = true);
status_t vcache_get_constructed(mount* vol, ino_t vnid, bool* constructed);

status_t sync_all_files(mount* vol);


#define find_vnid_in_vcache(vol,vnid) vcache_vnid_to_loc(vol,vnid,NULL)
#define find_loc_in_vcache(vol,loc) vcache_loc_to_vnid(vol,loc,NULL)

#if DEBUG
	status_t vcache_set_node(mount* vol, ino_t vnid, struct vnode* node);
	status_t vcache_get_node(mount* vol, ino_t vnid, struct vnode** node);

	int debug_dfvnid(int argc, char **argv);
	int debug_dfloc(int argc, char **argv);
#endif


#endif // _DOSFS_VCACHE_H
