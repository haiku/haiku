/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_VCACHE_H_
#define _DOSFS_VCACHE_H_


#include "dosfs.h"


void dump_vcache(nspace *vol);

status_t init_vcache(nspace *vol);
status_t uninit_vcache(nspace *vol);

ino_t generate_unique_vnid(nspace *vol);

status_t add_to_vcache(nspace *vol, ino_t vnid, ino_t loc);
status_t remove_from_vcache(nspace *vol, ino_t vnid);
status_t vcache_vnid_to_loc(nspace *vol, ino_t vnid, ino_t *loc);
status_t vcache_loc_to_vnid(nspace *vol, ino_t loc, ino_t *vnid);

status_t vcache_set_entry(nspace *vol, ino_t vnid, ino_t loc);

#define find_vnid_in_vcache(vol,vnid) vcache_vnid_to_loc(vol,vnid,NULL)
#define find_loc_in_vcache(vol,loc) vcache_loc_to_vnid(vol,loc,NULL)

#if DEBUG
	int debug_dfvnid(int argc, char **argv);
	int debug_dfloc(int argc, char **argv);
#endif

#endif
