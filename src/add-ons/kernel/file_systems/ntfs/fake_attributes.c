/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/* fake_attributes.c
 * handles mime type information for ntfs
 * gets/sets mime information in vnode
 */

#define MIME_STRING_TYPE 'MIMS'

#include <SupportDefs.h>
#include <KernelExport.h>

#include <dirent.h>
#include <file_systems/mime_ext_table.h>
#include <fs_attr.h>
#include <string.h>
#include <malloc.h>

#include "ntfs.h"
#include "fake_attributes.h"

int32 kOpenTypeCookie = 0;
int32 kCloseTypeCookie = 1;
int32 kSetTypeCookie = 0x1234;
int32 kFreeTypeCookie = 0x87654321;


status_t set_mime_type(vnode *node, const char *filename)
{
	return set_mime(&node->mime, filename);
}


status_t
fake_open_attrib_dir(fs_volume *_vol, fs_vnode *_node, void **_cookie)
{
	nspace *ns = (nspace *)_vol->private_volume;

	int	result = B_NO_ERROR;

	TRACE("fake_open_attrdir - ENTER\n");

	LOCK_VOL(ns);

	if ((*_cookie = malloc(sizeof(uint32))) == NULL) {
		result = ENOMEM;
		goto	exit;
	}

	*(int32 *)(*_cookie) = kOpenTypeCookie;

exit:

	TRACE("fake_open_attrdir - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);

	return result;
}


status_t
fake_close_attrib_dir(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace *ns = (nspace *)_vol->private_volume;

	TRACE("fake_close_attrdir - ENTER\n");

	LOCK_VOL(ns);

	*(int32 *)_cookie = kCloseTypeCookie;

	TRACE("fake_close_attrdir - EXIT\n");

	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}


status_t
fake_free_attrib_dir_cookie(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace *ns = (nspace *)_vol->private_volume;

	int	result = B_NO_ERROR;

	LOCK_VOL(ns);

	TRACE("fake_free_attrib_dir_cookie - ENTER\n");

	if (_cookie == NULL) {
		TRACE("fake_free_attrib_dir_cookie - error:called with null cookie\n");
		result =  EINVAL;
		goto	exit;
	}

	*(int32 *)_cookie = kFreeTypeCookie;
	free(_cookie);

exit:

	TRACE("fake_free_attrib_dir_cookie - EXIT, result is %s\n",
		strerror(result));

	UNLOCK_VOL(ns);

	return result;
}


status_t
fake_rewind_attrib_dir(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace *ns = (nspace *)_vol->private_volume;

	int	result = B_NO_ERROR;

	LOCK_VOL(ns);

	TRACE("fake_rewind_attrcookie  - ENTER\n");

	if (_cookie == NULL) {
		TRACE("fake_rewind_attrcookie - error: fake_rewind_attrcookie"
			  "called with null cookie\n");
		result =  EINVAL;
		goto	exit;
	}

	*(uint32 *)_cookie = kOpenTypeCookie;

exit:

	TRACE("fake_rewind_attrcookie - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);

	return result;
}


status_t
fake_read_attrib_dir(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	struct dirent *entry, size_t bufsize, uint32 *num)
{
	nspace *ns = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;

	int32 *cookie = (int32 *)_cookie;

	LOCK_VOL(ns);

	TRACE("fake_read_attrdir - ENTER\n");

	*num = 0;

	if ((*cookie == kOpenTypeCookie) && (node->mime)) {
		*num = 1;

		entry->d_ino = node->vnid;
		entry->d_dev = ns->id;
		entry->d_reclen = sizeof(struct dirent) + strlen(kAttrMimeTypeName);
		strcpy(entry->d_name, kAttrMimeTypeName);
	}

	*cookie = kCloseTypeCookie;

	TRACE("fake_read_attrdir - EXIT\n");

	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}

status_t
fake_create_attrib(fs_volume *_vol, fs_vnode *_node, const char* name,
	uint32 type, int openMode, void** _cookie)
{
	nspace *ns = (nspace *)_vol->private_volume;

	LOCK_VOL(ns);

	TRACE("fake_create_attrib - ENTER (name = [%s])\n",name);

	if (strcmp(name, kAttrMimeTypeName) == 0)
		*_cookie = &kSetTypeCookie;

	TRACE("fake_create_attrib - EXIT\n");

	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}

status_t
fake_open_attrib(fs_volume *_vol, fs_vnode *_node, const char *name,
	int openMode, void **_cookie)
{
	nspace *ns = (nspace *)_vol->private_volume;
	status_t result = B_NO_ERROR;

	LOCK_VOL(ns);

	TRACE("fake_open_attrib - ENTER (name = [%s])\n",name);

	if (strcmp(name, kAttrMimeTypeName) == 0)
		*_cookie = &kSetTypeCookie;
	else
		result = ENOENT;

	TRACE("fake_open_attrib - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);

	return result;
}


status_t
fake_close_attrib(fs_volume *_vol, fs_vnode *_node, void *cookie)
{
	return B_NO_ERROR;
}


status_t
fake_free_attrib_cookie(fs_volume *_vol, fs_vnode *_node, void *cookie)
{
	return B_NO_ERROR;
}


status_t
fake_read_attrib_stat(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	struct stat *stat)
{
	nspace *ns = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;
	int	result = B_NO_ERROR;

	LOCK_VOL(ns);

	TRACE("fake_read_attr_stat - ENTER\n");

	if (_cookie != &kSetTypeCookie) {
		result = ENOENT;
		goto	exit;
	}

	stat->st_type = MIME_STRING_TYPE;
	if (node->mime == NULL)
		stat->st_size = 0;
	else
		stat->st_size = strlen(node->mime) + 1;

exit:

	TRACE("fake_read_attrib_stat - EXIT, result is %s\n",
		strerror(result));

	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}


status_t
fake_read_attrib(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	off_t pos,void *buffer, size_t *_length)
{
	nspace *ns = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;

	int	result = B_NO_ERROR;

	LOCK_VOL(ns);

	TRACE("fake_read_attr - ENTER\n");

	if (_cookie != &kSetTypeCookie) {
		result = ENOENT;
		goto	exit;
	}

	if (node->mime == NULL) {
		result = ENOENT;
		goto	exit;
	}

	if ((pos < 0) || (pos > strlen(node->mime))) {
		result = EINVAL;
		goto	exit;
	}

	strncpy(buffer, node->mime + pos, *_length - 1);
	((char *)buffer)[*_length - 1] = 0;
	*_length = strlen(buffer) + 1;

exit:

	TRACE("fake_read_attr - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);

	return result;
}


status_t
fake_write_attrib(fs_volume *_vol, fs_vnode *_node, void *_cookie, off_t pos,
	const void *buffer, size_t *_length)
{
	return B_NO_ERROR;
}
