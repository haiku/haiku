/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/* attr.c
 * handles mime type information for dosfs
 * gets/sets mime information in vnode
 */


#define MIME_STRING_TYPE 'MIMS'

#include <SupportDefs.h>
#include <KernelExport.h>

#include <dirent.h>
#include <fs_attr.h>
#include <string.h>
#include <malloc.h>

#include <fsproto.h>
#include <lock.h>

#include "dosfs.h"
#include "attr.h"
#include "mime_table.h"

#define DPRINTF(a,b) if (debug_attr > (a)) dprintf b

status_t set_mime_type(vnode *node, const char *filename)
{
	struct ext_mime *p;
	int32 namelen, ext_len;

	DPRINTF(0, ("get_mime_type of (%s)\n", filename));

	node->mime = NULL;

	namelen = strlen(filename);

	for (p=mimes;p->extension;p++) {
		ext_len = strlen(p->extension);

		if (namelen <= ext_len)
			continue;

		if (filename[namelen-ext_len-1] != '.')
			continue;
		
		if (!strcasecmp(filename + namelen - ext_len, p->extension))
			break;
	}

	node->mime = p->mime;

	return B_OK;
}

int dosfs_open_attrdir(void *_vol, void *_node, void **_cookie)
{
	nspace *vol = (nspace *)_vol;

	TOUCH(_node);

	DPRINTF(0, ("dosfs_open_attrdir called\n"));

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_open_attrdir")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	if ((*_cookie = malloc(sizeof(uint32))) == NULL) {
		UNLOCK_VOL(vol);
		return ENOMEM;
	}
	*(int32 *)(*_cookie) = 0;
	
	UNLOCK_VOL(vol);
	
	return 0;
}

int dosfs_close_attrdir(void *_vol, void *_node, void *_cookie)
{
	nspace *vol = (nspace *)_vol;

	TOUCH(_node);

	DPRINTF(0, ("dosfs_close_attrdir called\n"));

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_open_attrdir")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	*(int32 *)_cookie = 1;
	
	UNLOCK_VOL(vol);
	
	return 0;
}

int dosfs_free_attrcookie(void *_vol, void *_node, void *_cookie)
{
	TOUCH(_vol); TOUCH(_node);

	DPRINTF(0, ("dosfs_free_attrcookie called\n"));

	if (_cookie == NULL) {
		dprintf("error: dosfs_free_attrcookie called with null cookie\n");
		return EINVAL;
	}

	*(int32 *)_cookie = 0x87654321;
	free(_cookie);

	return 0;
}

int dosfs_rewind_attrdir(void *_vol, void *_node, void *_cookie)
{
	TOUCH(_vol); TOUCH(_node);

	DPRINTF(0, ("dosfs_rewind_attrdir called\n"));

	if (_cookie == NULL) {
		dprintf("error: dosfs_rewind_attrcookie called with null cookie\n");
		return EINVAL;
	}

	*(uint32 *)_cookie = 0;
	return 0;
}

int dosfs_read_attrdir(void *_vol, void *_node, void *_cookie, long *num, 
	struct dirent *entry, size_t bufsize)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;
	int32 *cookie = (int32 *)_cookie;

	TOUCH(bufsize);

	DPRINTF(0, ("dosfs_read_attrdir called\n"));

	*num = 0;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_read_attrdir") ||
		check_vnode_magic(node, "dosfs_read_attrdir")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	if ((*cookie == 0) && (node->mime)) {
		*num = 1;
		
		entry->d_ino = node->vnid;
		entry->d_dev = vol->id;
		entry->d_reclen = 10;
		strcpy(entry->d_name, "BEOS:TYPE");
	}

	*cookie = 1;

	UNLOCK_VOL(vol);
	
	return 0;
}

int dosfs_stat_attr(void *_vol, void *_node, const char *name, struct attr_info *buf)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	DPRINTF(0, ("dosfs_stat_attr (%s)\n", name));

	if (strcmp(name, "BEOS:TYPE"))
		return ENOENT;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_read_attr") ||
		check_vnode_magic(node, "dosfs_read_attr")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	if (node->mime == NULL) {
		UNLOCK_VOL(vol);
		return ENOENT;
	}
	
	buf->type = MIME_STRING_TYPE;
	buf->size = strlen(node->mime) + 1;

	UNLOCK_VOL(vol);
	return 0;
}

int dosfs_read_attr(void *_vol, void *_node, const char *name, int type, void *buf, 
	size_t *len, off_t pos)
{
	nspace *vol = (nspace *)_vol;
	vnode *node = (vnode *)_node;

	DPRINTF(0, ("dosfs_read_attr (%s)\n", name));

	if (strcmp(name, "BEOS:TYPE"))
		return ENOENT;
		
	if (type != MIME_STRING_TYPE)
		return ENOENT;

	LOCK_VOL(vol);

	if (check_nspace_magic(vol, "dosfs_read_attr") ||
		check_vnode_magic(node, "dosfs_read_attr")) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	if (node->mime == NULL) {
		UNLOCK_VOL(vol);
		return ENOENT;
	}

	if ((pos < 0) || (pos > strlen(node->mime))) {
		UNLOCK_VOL(vol);
		return EINVAL;
	}

	strncpy(buf, node->mime + pos, *len - 1);
	((char *)buf)[*len - 1] = 0;
	*len = strlen(buf) + 1;

	UNLOCK_VOL(vol);
	return 0;
}

// suck up application attempts to set mime types; this hides an unsightly
// error message printed out by zip
int dosfs_write_attr(void *_vol, void *_node, const char *name, int type,
	const void *buf, size_t *len, off_t pos)
{
	TOUCH(_vol); TOUCH(_node); TOUCH(name); TOUCH(type); TOUCH(buf);
	TOUCH(len); TOUCH(pos);

	DPRINTF(0, ("dosfs_write_attr (%s)\n", name));

	*len = 0;

	if (strcmp(name, "BEOS:TYPE"))
		return ENOSYS;
		
	if (type != MIME_STRING_TYPE)
		return ENOSYS;

	return 0;
}
