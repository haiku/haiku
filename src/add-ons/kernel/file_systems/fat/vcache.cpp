/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
*/


/*
The FAT file system has no good way of assigning unique persistent values to
nodes. The only obvious choice, storing the starting cluster number of the
file, is unusable because 0 byte files exist as directory entries only.
Further, even if it were usable, it would potentially require a full directory
tree traversal to locate an arbitrary node. We must resort to some ickiness
in order to make persistent vnode id's (at least across a given mount) work.

There are three ways to encode a vnode id:

1. Combine the starting cluster of the entry with the starting cluster of the
   directory it appears in. This is used for files with data.
2. Combine the starting cluster of the directory the entry appears in with the
   index of the entry in the directory. This is used for 0-byte files.
3. A unique number that doesn't match any possible values from encodings 1 or
   2.

With the first encoding, the vnode id is invalidated (i.e. no longer describes
the file's location) when the file moves to a different directory or when
its starting cluster changes (this can occur if the file is truncated and data
is subsequently written to it).

With the second encoding, the vnode id is invalidated when the file position
is moved within a directory (as a result of a renaming), when it's moved to a
different directory, or when data is written to it.

The third encoding doesn't describe the file's location on disk, and so it is
invalid from the start.

Since we can't change vnode id's once they are assigned, we have to create a
mapping table to translate vnode id's to locations. This file serves this
purpose.

It also allows the FS to check whether a node with a given inode number has been constructed,
without calling get_vnode().
*/


#include "vcache.h"

#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#else // !FS_SHELL
#include <new>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#endif // !FS_SHELL

#ifndef FS_SHELL
#include <fs_cache.h>
#include <fs_interface.h>
#endif // !FS_SHELL

#define _KERNEL
#include "sys/vnode.h"

#include "debug.h"
#include "dosfs.h"


#define LOCK_CACHE_R \
	rw_lock_read_lock(&vol->vcache.lock)

#define LOCK_CACHE_W \
	rw_lock_write_lock(&vol->vcache.lock)

#define UNLOCK_CACHE_R \
	rw_lock_read_unlock(&vol->vcache.lock)

#define UNLOCK_CACHE_W \
	rw_lock_write_unlock(&vol->vcache.lock)

#define hash(v) ((v) & (vol->vcache.cache_size-1))


struct vcache_entry {
	ino_t	vnid;		/* originally reported vnid */
	ino_t	loc;		/* where the file is now */
	bool	constructed;	/* has the node been set up by _dosfs_read_vnode */
	struct vcache_entry *next_vnid; /* next entry in vnid hash table */
	struct vcache_entry *next_loc;  /* next entry in location hash table */
#ifdef DEBUG
	struct vnode* node;
#endif
};


void
dump_vcache(const mount* vol)
{
	uint32 i;
	struct vcache_entry *c;
#if defined DEBUG && defined _KERNEL_MODE
	kprintf("vnid cache size %" B_PRIu32 ", cur vnid = %" B_PRIdINO "\n"
		"vnid                loc              %-*s\n", vol->vcache.cache_size, vol->vcache.cur_vnid,
		B_PRINTF_POINTER_WIDTH, "struct vnode");
	for (i = 0; i < vol->vcache.cache_size; i++) {
		for (c = vol->vcache.by_vnid[i]; c ; c = c->next_vnid)
			kprintf("%19" B_PRIdINO " %16" B_PRIdINO " %p\n", c->vnid, c->loc, c->node);
	}
#else
	kprintf("vnid cache size %" B_PRIu32 ", cur vnid = %" B_PRIdINO "\n"
		"vnid                loc\n", vol->vcache.cache_size, vol->vcache.cur_vnid);
	for (i = 0; i < vol->vcache.cache_size; i++) {
		for (c = vol->vcache.by_vnid[i]; c ; c = c->next_vnid)
			kprintf("%19" B_PRIdINO " %16" B_PRIdINO "\n", c->vnid, c->loc);
	}
#endif // !DEBUG
}


status_t
init_vcache(mount* vol)
{
	char name[16];
	FUNCTION();

	vol->vcache.cur_vnid = ARTIFICIAL_VNID_BITS;
#if DEBUG
	vol->vcache.cache_size = 1;
#else
	vol->vcache.cache_size = 512; /* must be power of 2 */
#endif

	vol->vcache.by_vnid = (vcache_entry**)calloc(sizeof(struct vache_entry *),
		vol->vcache.cache_size);
	if (vol->vcache.by_vnid == NULL) {
		INFORM("init_vcache: out of memory\n");
		return ENOMEM;
	}

	vol->vcache.by_loc = (vcache_entry**)calloc(sizeof(struct vache_entry *),
		vol->vcache.cache_size);
	if (vol->vcache.by_loc == NULL) {
		INFORM("init_vcache: out of memory\n");
		free(vol->vcache.by_vnid);
		vol->vcache.by_vnid = NULL;
		return ENOMEM;
	}

	sprintf(name, "fat cache %" B_PRIdDEV, vol->mnt_fsvolume->id);
	rw_lock_init(&vol->vcache.lock, "fat cache");

	PRINT("init_vcache: initialized vnid cache with %" B_PRIu32
		" entries\n", vol->vcache.cache_size);

	return 0;
}


status_t
uninit_vcache(mount* vol)
{
	uint32 i, count = 0;
	struct vcache_entry *c, *n;
	FUNCTION();

	LOCK_CACHE_W;

	/* free entries */
	for (i = 0; i < vol->vcache.cache_size; i++) {
		c = vol->vcache.by_vnid[i];
		while (c) {
			count++;
			n = c->next_vnid;
			free(c);
			c = n;
		}
	}

	PRINT("%" B_PRIu32 " vcache entries removed\n", count);

	free(vol->vcache.by_vnid); vol->vcache.by_vnid = NULL;
	free(vol->vcache.by_loc); vol->vcache.by_loc = NULL;

	rw_lock_destroy(&vol->vcache.lock);
	return B_OK;
}


ino_t
generate_unique_vnid(mount* vol)
{
	FUNCTION();

#ifdef FS_SHELL
	LOCK_CACHE_W;
	ino_t current = vol->vcache.cur_vnid++;
	UNLOCK_CACHE_W;

	return current;
#else
	return atomic_add64(&vol->vcache.cur_vnid, 1);
#endif // !FS_SHELL
}


static status_t
_add_to_vcache_(mount* vol, ino_t vnid, ino_t loc)
{
	int hash1 = hash(vnid), hash2 = hash(loc);
	struct vcache_entry *e, *c, *p;

	FUNCTION_START("%" B_PRIdINO "/%" B_PRIdINO "\n", vnid, loc);

	e = (vcache_entry*)malloc(sizeof(struct vcache_entry));
	if (e == NULL)
		return ENOMEM;

	e->vnid = vnid; e->loc = loc; e->constructed = false; e->next_vnid = NULL; e->next_loc = NULL;

	c = p = vol->vcache.by_vnid[hash1];
	while (c) {
		if (vnid < c->vnid)
			break;
		ASSERT(vnid != c->vnid); ASSERT(loc != c->loc);
		p = c;
		c = c->next_vnid;
	}
	ASSERT(!c || (vnid != c->vnid));

	e->next_vnid = c;
	if (p == c)
		vol->vcache.by_vnid[hash1] = e;
	else
		p->next_vnid = e;

	c = p = vol->vcache.by_loc[hash2];
	while (c) {
		if (loc < c->loc)
			break;
		ASSERT(vnid != c->vnid); ASSERT(loc != c->loc);
		p = c;
		c = c->next_loc;
	}
	ASSERT(!c || (loc != c->loc));

	e->next_loc = c;
	if (p == c)
		vol->vcache.by_loc[hash2] = e;
	else
		p->next_loc = e;

	return B_OK;
}


static status_t
_remove_from_vcache_(mount* vol, ino_t vnid)
{
	int hash1 = hash(vnid), hash2;
	struct vcache_entry *c, *p, *e;

	FUNCTION_START("%" B_PRIdINO "\n", vnid);

	c = p = vol->vcache.by_vnid[hash1];
	while (c) {
		if (vnid == c->vnid)
			break;
		ASSERT(c->vnid < vnid);
		p = c;
		c = c->next_vnid;
	}
	ASSERT(c);
	if (!c) return ENOENT;

	if (p == c)
		vol->vcache.by_vnid[hash1] = c->next_vnid;
	else
		p->next_vnid = c->next_vnid;

	e = c;

	hash2 = hash(c->loc);
	c = p = vol->vcache.by_loc[hash2];

	while (c) {
		if (vnid == c->vnid)
			break;
		ASSERT(c->loc < e->loc);
		p = c;
		c = c->next_loc;
	}
	ASSERT(c);
	if (!c) return ENOENT;
	if (p == c)
		vol->vcache.by_loc[hash2] = c->next_loc;
	else
		p->next_loc = c->next_loc;

	free(c);

	return 0;
}


static struct vcache_entry *
_find_vnid_in_vcache_(mount* vol, ino_t vnid)
{
	int hash1 = hash(vnid);
	struct vcache_entry *c;
	c = vol->vcache.by_vnid[hash1];
	while (c) {
		if (c->vnid == vnid)
			break;
		if (c->vnid > vnid)
			return NULL;
		c = c->next_vnid;
	}

	return c;
}


static struct vcache_entry *
_find_loc_in_vcache_(mount* vol, ino_t loc)
{
	int hash2 = hash(loc);
	struct vcache_entry *c;
	c = vol->vcache.by_loc[hash2];
	while (c) {
		if (c->loc == loc)
			break;
		if (c->loc > loc)
			return NULL;
		c = c->next_loc;
	}

	return c;
}


status_t
add_to_vcache(mount* vol, ino_t vnid, ino_t loc)
{
	status_t result;

	LOCK_CACHE_W;
	result = _add_to_vcache_(vol,vnid,loc);
	UNLOCK_CACHE_W;

	if (result < 0) INFORM("add_to_vcache failed (%s)\n", strerror(result));
	return result;
}


static status_t
_update_loc_in_vcache_(mount* vol, ino_t vnid, ino_t loc)
{
	status_t result;
	bool constructed;
	result = vcache_get_constructed(vol, vnid, &constructed);
	if (result != B_OK)
		return result;
#if DEBUG
	struct vnode *node;
	result = vcache_get_node(vol, vnid, &node);
	if (result != B_OK)
		return result;
#endif // DEBUG

	result = _remove_from_vcache_(vol, vnid);
	if (result == 0) {
		result = _add_to_vcache_(vol, vnid, loc);
		if (result == B_OK && constructed == true)
			result = vcache_set_constructed(vol, vnid);
#if DEBUG
		if (result == B_OK)
			result = vcache_set_node(vol, vnid, node);
#endif // DEBUG
	}

	return result;
}


status_t
remove_from_vcache(mount* vol, ino_t vnid)
{
	status_t result;

	LOCK_CACHE_W;
	result = _remove_from_vcache_(vol, vnid);
	UNLOCK_CACHE_W;

	if (result < 0) INFORM("remove_from_vcache failed (%s)\n", strerror(result));
	return result;
}


status_t
vcache_vnid_to_loc(mount* vol, ino_t vnid, ino_t* loc)
{
	struct vcache_entry *e;

	FUNCTION_START("%" B_PRIdINO " %p\n", vnid,	loc);

	LOCK_CACHE_R;
	e = _find_vnid_in_vcache_(vol, vnid);
	if (loc && e)
			*loc = e->loc;
	UNLOCK_CACHE_R;

	return (e) ? B_OK : ENOENT;
}


status_t
vcache_loc_to_vnid(mount* vol, ino_t loc, ino_t* vnid)
{
	struct vcache_entry *e;

	FUNCTION_START("%" B_PRIdINO " %p\n", loc, vnid);

	LOCK_CACHE_R;
	e = _find_loc_in_vcache_(vol, loc);
	if (vnid && e)
			*vnid = e->vnid;
	UNLOCK_CACHE_R;

	return (e) ? B_OK : ENOENT;
}


status_t
vcache_set_entry(mount* vol, ino_t vnid, ino_t loc)
{
	struct vcache_entry *e;
	status_t result = B_OK;

	FUNCTION_START("vcache_set_entry: %" B_PRIdINO " -> %" B_PRIdINO "\n", vnid, loc);

	LOCK_CACHE_W;

	e = _find_vnid_in_vcache_(vol, vnid);

	if (e)
		result = _update_loc_in_vcache_(vol, vnid, loc);
	else
		result = _add_to_vcache_(vol,vnid,loc);

	UNLOCK_CACHE_W;

	return result;
}


status_t
vcache_set_constructed(mount* vol, ino_t vnid, bool constructed)
{
	struct vcache_entry* cEntry;
	status_t result = B_OK;

	FUNCTION_START("%" B_PRIdINO "\n", vnid);

	LOCK_CACHE_W;

	cEntry = _find_vnid_in_vcache_(vol, vnid);

	if (cEntry != NULL)
		cEntry->constructed = constructed;
	else
		result = B_BAD_VALUE;

	UNLOCK_CACHE_W;

	return result;
}


status_t
vcache_get_constructed(mount* vol, ino_t vnid, bool* constructed)
{
	struct vcache_entry* cEntry;
	status_t result = B_OK;

	FUNCTION_START("%" B_PRIdINO "\n", vnid);

	LOCK_CACHE_W;

	cEntry = _find_vnid_in_vcache_(vol, vnid);

	if (cEntry != NULL)
		*constructed = cEntry->constructed;
	else
		result = B_BAD_VALUE;

	UNLOCK_CACHE_W;

	return result;
}


status_t
sync_all_files(mount* vol)
{
	uint32 count = 0, errors = 0;
	vnode* bsdNode = NULL;
	status_t status = B_OK;

	FUNCTION_START("volume @ %p\n", vol);

	LOCK_CACHE_R;

	for (uint32 i = 0; i < vol->vcache.cache_size; i++) {
		for (vcache_entry* cEntry = vol->vcache.by_vnid[i]; cEntry != NULL ;
			cEntry = cEntry->next_vnid) {
			if (cEntry->constructed == true) {
				status = get_vnode(vol->mnt_fsvolume, cEntry->vnid,
					reinterpret_cast<void**>(&bsdNode));
				if (status != B_OK) {
					INFORM("get_vnode:  %s\n", strerror(status));
					errors++;
				} else {
					if (bsdNode->v_type == VREG) {
						status = file_cache_sync(bsdNode->v_cache);
						if (status != B_OK)
							errors++;
						else
							count++;
					}
					put_vnode(vol->mnt_fsvolume, cEntry->vnid);
				}
			}
		}
	}

	UNLOCK_CACHE_R;

	PRINT("sync'd %" B_PRIu32 " files with %" B_PRIu32 " errors\n", count, errors);

	return (errors == 0) ? B_OK : B_ERROR;
}


#if DEBUG
status_t
vcache_set_node(mount* vol, ino_t vnid, struct vnode* node)
{
	vcache_entry* cEntry;
	status_t result = B_OK;

	FUNCTION_START("%" B_PRIdINO "\n", vnid);

	LOCK_CACHE_W;

	cEntry = _find_vnid_in_vcache_(vol, vnid);

	if (cEntry != NULL)
		cEntry->node = node;
	else
		result = B_BAD_VALUE;

	UNLOCK_CACHE_W;

	return result;
}


status_t
vcache_get_node(mount* vol, ino_t vnid, struct vnode** node)
{
	vcache_entry* cEntry;
	status_t result = B_OK;

	FUNCTION_START("%" B_PRIdINO "\n", vnid);

	LOCK_CACHE_W;

	cEntry = _find_vnid_in_vcache_(vol, vnid);

	if (cEntry != NULL)
		*node = cEntry->node;
	else
		result = B_BAD_VALUE;

	UNLOCK_CACHE_W;

	return result;
}


int
debug_dfvnid(int argc, char **argv)
{
	int i;
	mount* vol;

	if (argc < 3) {
		kprintf("dfvnid volume vnid\n");
		return B_OK;
	}

	vol = reinterpret_cast<mount*>(strtoul(argv[1], NULL, 0));
	if (vol == NULL)
		return B_OK;

	for (i = 2; i < argc; i++) {
		ino_t vnid = strtoull(argv[i], NULL, 0);
		struct vcache_entry *e;
		if ((e = _find_vnid_in_vcache_(vol, vnid)) != NULL) {
			kprintf("vnid %" B_PRIdINO " -> loc %" B_PRIdINO " @ %p\n", vnid,
				e->loc, e);
		} else {
			kprintf("vnid %" B_PRIdINO " not found in vnid cache\n", vnid);
		}
	}

	return B_OK;
}


int
debug_dfloc(int argc, char **argv)
{
	int i;
	mount* vol;

	if (argc < 3) {
		kprintf("dfloc volume vnid\n");
		return B_OK;
	}

	vol = reinterpret_cast<mount*>(strtoul(argv[1], NULL, 0));
	if (vol == NULL)
		return B_OK;

	for (i = 2; i < argc; i++) {
		ino_t loc = strtoull(argv[i], NULL, 0);
		struct vcache_entry *e;
		if ((e = _find_loc_in_vcache_(vol, loc)) != NULL) {
			kprintf("loc %" B_PRIdINO " -> vnid %" B_PRIdINO " @ %p\n", loc,
				e->vnid, e);
		} else {
			kprintf("loc %" B_PRIdINO " not found in vnid cache\n", loc);
		}
	}

	return B_OK;
}
#endif // DEBUG
