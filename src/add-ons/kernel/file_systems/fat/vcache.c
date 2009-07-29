/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
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
*/


#include "vcache.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <KernelExport.h>

#include "dosfs.h"
#include "util.h"


#define DPRINTF(a,b) if (debug_vcache > (a)) dprintf b

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
	struct vcache_entry *next_vnid; /* next entry in vnid hash table */
	struct vcache_entry *next_loc;  /* next entry in location hash table */
};


void
dump_vcache(nspace *vol)
{
	uint32 i;
	struct vcache_entry *c;
	kprintf("vnid cache size %lx, cur vnid = %Lx\n"
		"vnid             loc\n",
		vol->vcache.cache_size, vol->vcache.cur_vnid);
	for (i = 0; i < vol->vcache.cache_size; i++) {
		for (c = vol->vcache.by_vnid[i]; c ; c = c->next_vnid)
			kprintf("%16Lx %16Lx\n", c->vnid, c->loc);
	}
}


status_t
init_vcache(nspace *vol)
{
	char name[16];
	DPRINTF(0, ("init_vcache called\n"));

	vol->vcache.cur_vnid = ARTIFICIAL_VNID_BITS;
#if DEBUG
	vol->vcache.cache_size = 1;
#else
	vol->vcache.cache_size = 512; /* must be power of 2 */
#endif

	vol->vcache.by_vnid = calloc(sizeof(struct vache_entry *),
		vol->vcache.cache_size);
	if (vol->vcache.by_vnid == NULL) {
		dprintf("init_vcache: out of memory\n");
		return ENOMEM;
	}

	vol->vcache.by_loc = calloc(sizeof(struct vache_entry *),
		vol->vcache.cache_size);
	if (vol->vcache.by_loc == NULL) {
		dprintf("init_vcache: out of memory\n");
		free(vol->vcache.by_vnid);
		vol->vcache.by_vnid = NULL;
		return ENOMEM;
	}

	sprintf(name, "fat cache %lx", vol->id);
	rw_lock_init(&vol->vcache.lock, "fat cache");

	DPRINTF(0, ("init_vcache: initialized vnid cache with %lx entries\n",
		vol->vcache.cache_size));

	return 0;
}


status_t
uninit_vcache(nspace *vol)
{
	uint32 i, count = 0;
	struct vcache_entry *c, *n;
	DPRINTF(0, ("uninit_vcache called\n"));

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

	DPRINTF(0, ("%lx vcache entries removed\n", count));

	free(vol->vcache.by_vnid); vol->vcache.by_vnid = NULL;
	free(vol->vcache.by_loc); vol->vcache.by_loc = NULL;

	rw_lock_destroy(&vol->vcache.lock);
	return B_OK;
}


ino_t
generate_unique_vnid(nspace *vol)
{
	DPRINTF(0, ("generate_unique_vnid\n"));
	/* only one thread per volume will be in here at any given time anyway
	 * due to volume locking */
	return vol->vcache.cur_vnid++;
}


static status_t
_add_to_vcache_(nspace *vol, ino_t vnid, ino_t loc)
{
	int hash1 = hash(vnid), hash2 = hash(loc);
	struct vcache_entry *e, *c, *p;

	DPRINTF(0, ("add_to_vcache %Lx/%Lx\n", vnid, loc));

	ASSERT(vnid != loc);

	e = malloc(sizeof(struct vcache_entry));
	if (e == NULL)
		return ENOMEM;

	e->vnid = vnid; e->loc = loc; e->next_vnid = NULL; e->next_loc = NULL;

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
_remove_from_vcache_(nspace *vol, ino_t vnid)
{
	int hash1 = hash(vnid), hash2;
	struct vcache_entry *c, *p, *e;

	DPRINTF(0, ("remove_from_vcache %Lx\n", vnid));

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
_find_vnid_in_vcache_(nspace *vol, ino_t vnid)
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
_find_loc_in_vcache_(nspace *vol, ino_t loc)
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
add_to_vcache(nspace *vol, ino_t vnid, ino_t loc)
{
	status_t result;

	LOCK_CACHE_W;
	result = _add_to_vcache_(vol,vnid,loc);
	UNLOCK_CACHE_W;

	if (result < 0) DPRINTF(0, ("add_to_vcache failed (%s)\n", strerror(result)));
	return result;
}


/* XXX: do this in a smarter fashion */
static status_t
_update_loc_in_vcache_(nspace *vol, ino_t vnid, ino_t loc)
{
	status_t result;

	result = _remove_from_vcache_(vol, vnid);
	if (result == 0)
		result = _add_to_vcache_(vol, vnid, loc);

	return result;
}


status_t
remove_from_vcache(nspace *vol, ino_t vnid)
{
	status_t result;

	LOCK_CACHE_W;
	result = _remove_from_vcache_(vol, vnid);
	UNLOCK_CACHE_W;

	if (result < 0) DPRINTF(0, ("remove_from_vcache failed (%s)\n", strerror(result)));
	return result;
}


status_t
vcache_vnid_to_loc(nspace *vol, ino_t vnid, ino_t *loc)
{
	struct vcache_entry *e;

	DPRINTF(1, ("vcache_vnid_to_loc %Lx %lx\n", vnid, (uint32)loc));

	LOCK_CACHE_R;
	e = _find_vnid_in_vcache_(vol, vnid);
	if (loc && e)
			*loc = e->loc;
	UNLOCK_CACHE_R;

	return (e) ? B_OK : ENOENT;
}


status_t
vcache_loc_to_vnid(nspace *vol, ino_t loc, ino_t *vnid)
{
	struct vcache_entry *e;

	DPRINTF(1, ("vcache_loc_to_vnid %Lx %lx\n", loc, (uint32)vnid));

	LOCK_CACHE_R;
	e = _find_loc_in_vcache_(vol, loc);
	if (vnid && e)
			*vnid = e->vnid;
	UNLOCK_CACHE_R;

	return (e) ? B_OK : ENOENT;
}


status_t
vcache_set_entry(nspace *vol, ino_t vnid, ino_t loc)
{
	struct vcache_entry *e;
	status_t result = B_OK;

	DPRINTF(0, ("vcache_set_entry: %Lx -> %Lx\n", vnid, loc));

	/*if (is_vnode_removed(vol->id, vnid) > 0) {
		if (!IS_ARTIFICIAL_VNID(loc))
			return B_OK;
	} else {
		ASSERT(is_vnode_removed(vol->id, vnid) == 0);
	}*/

	LOCK_CACHE_W;

	e = _find_vnid_in_vcache_(vol, vnid);

	if (e) {
		if (e->vnid == loc)
			result = _remove_from_vcache_(vol, vnid);
		else
			result = _update_loc_in_vcache_(vol, vnid, loc);
	} else {
		if (vnid != loc)
			result = _add_to_vcache_(vol,vnid,loc);
	}

	UNLOCK_CACHE_W;

	return result;
}

#if DEBUG

int
debug_dfvnid(int argc, char **argv)
{
	int i;
	nspace *vol;

	if (argc < 3) {
		kprintf("dfvnid nspace vnid\n");
		return B_OK;
	}

	vol = (nspace *)strtoul(argv[1], NULL, 0);
	if (vol == NULL)
		return B_OK;

	for (i = 2; i < argc; i++) {
		ino_t vnid = strtoull(argv[i], NULL, 0);
		struct vcache_entry *e;
		if ((e = _find_vnid_in_vcache_(vol, vnid)) != NULL) {
			kprintf("vnid %Lx -> loc %Lx @ %p\n", vnid, e->loc, e);
		} else {
			kprintf("vnid %Lx not found in vnid cache\n", vnid);
		}
	}

	return B_OK;
}


int
debug_dfloc(int argc, char **argv)
{
	int i;
	nspace *vol;

	if (argc < 3) {
		kprintf("dfloc nspace vnid\n");
		return B_OK;
	}

	vol = (nspace *)strtoul(argv[1], NULL, 0);
	if (vol == NULL)
		return B_OK;

	for (i = 2; i < argc; i++) {
		ino_t loc = strtoull(argv[i], NULL, 0);
		struct vcache_entry *e;
		if ((e = _find_loc_in_vcache_(vol, loc)) != NULL) {
			kprintf("loc %Lx -> vnid %Lx @ %p\n", loc, e->vnid, e);
		} else {
			kprintf("loc %Lx not found in vnid cache\n", loc);
		}
	}

	return B_OK;
}

#endif
