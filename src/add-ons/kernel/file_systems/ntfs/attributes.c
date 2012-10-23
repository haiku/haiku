/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/* attributes.c
 * handles mime type information for ntfs
 * gets/sets mime information in vnode
 */


#define MIME_STRING_TYPE 'MIMS'

#include <KernelExport.h>
#include <SupportDefs.h>
#include <TypeConstants.h>

#include <dirent.h>
#include <fs_attr.h>
#include <stdlib.h>
#include <string.h>

#include "attributes.h"
#include "mime_table.h"
#include "ntfs.h"

//TODO: notify*()

status_t
fs_open_attrib_dir(fs_volume *_vol, fs_vnode *_node, void **_cookie)
{
	nspace *ns = (nspace*)_vol->private_volume;
	vnode *node = (vnode*)_node->private_node;
	attrdircookie *cookie = NULL;
	ntfs_inode *ni = NULL;
	ntfs_attr_search_ctx *ctx = NULL;

	status_t result = B_NO_ERROR;

	TRACE("%s - ENTER\n", __FUNCTION__);

	LOCK_VOL(ns);

	ni = ntfs_inode_open(ns->ntvol, node->vnid);
	if (ni == NULL) {
		result = errno;
		goto exit;
	}

	ctx = ntfs_attr_get_search_ctx(ni, NULL);
	if (ctx == NULL) {
		result = errno;
		goto exit;
	}

	cookie = (attrdircookie*)ntfs_calloc(sizeof(attrdircookie));
	if (cookie == NULL) {
		result = ENOMEM;
		goto exit;
	}

	cookie->inode = ni;
	cookie->ctx = ctx;
	ni = NULL;
	ctx = NULL;
	*_cookie = cookie;
	
exit:

	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	if (ni)
		ntfs_inode_close(ni);
	
	TRACE("%s - EXIT, result is %s\n", __FUNCTION__, strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}


status_t
fs_close_attrib_dir(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	return B_NO_ERROR;
}


status_t
fs_free_attrib_dir_cookie(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace *ns = (nspace *)_vol->private_volume;
	attrdircookie *cookie = (attrdircookie *)_cookie;

	LOCK_VOL(ns);

	if (cookie->ctx)
		ntfs_attr_put_search_ctx(cookie->ctx);
	if (cookie->inode)
		ntfs_inode_close(cookie->inode);

	UNLOCK_VOL(ns);

	free(cookie);
	return B_NO_ERROR;
}


status_t
fs_rewind_attrib_dir(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace *ns = (nspace*)_vol->private_volume;
	attrdircookie *cookie = (attrdircookie *)_cookie;
	status_t result = B_NO_ERROR;

	TRACE("%s - ENTER\n", __FUNCTION__);

	LOCK_VOL(ns);

	if (cookie->ctx)
		ntfs_attr_put_search_ctx(cookie->ctx);
	cookie->ctx = ntfs_attr_get_search_ctx(cookie->inode, NULL);
	if (cookie->ctx == NULL) {
		result = errno;
		//goto exit;
	}

//exit:

	TRACE("%s - EXIT, result is %s\n", __FUNCTION__, strerror(result));

	UNLOCK_VOL(ns);
	
	return result;
}


status_t
fs_read_attrib_dir(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	struct dirent *entry, size_t bufsize, uint32 *num)
{
	nspace *ns = (nspace *)_vol->private_volume;
	vnode *node = (vnode *)_node->private_node;
	char *name = NULL;
	attrdircookie *cookie = (attrdircookie *)_cookie;
	uint32 numEntries = 0;
	status_t result = B_NO_ERROR;

	if (cookie->ctx == NULL)
		panic("cookie->ctx == NULL");

	LOCK_VOL(ns);

	TRACE("%s - ENTER\n", __FUNCTION__);


	while (!(result = ntfs_attrs_walk(cookie->ctx))) {
		ATTR_RECORD *attr = cookie->ctx->attr;
		if (attr->type == AT_DATA) {
			// it's the actual file body
			if (attr->name_length == 0)
				continue;
			
			name = ntfs_attr_name_get((const ntfschar *)(((char *)attr)
				+ attr->name_offset), attr->name_length);
			dprintf("found AT_DATA '%s'\n", name);
			bufsize = MIN(bufsize, sizeof(struct dirent) + strlen(name) + 1);
			entry->d_ino = node->vnid;
			entry->d_dev = ns->id;
			entry->d_reclen = sizeof(struct dirent) + strlen(name);
			//XXX size
			strcpy(entry->d_name, name);
			ntfs_attr_name_free(&name);
			numEntries++;
			if (numEntries >= *num)
				break;
			break;
		}
	}
	if (result && errno != ENOENT) {
		result = errno;
		goto exit;
	} else
		result = B_OK;

exit:

	TRACE("%s - EXIT, result is %s, *num %d\n", __FUNCTION__,
		strerror(result), *num);

	UNLOCK_VOL(ns);
	
	if (result)
		*num = 0;
	else
		*num = numEntries;

	return result;
}


status_t
fs_create_attrib(fs_volume *_vol, fs_vnode *_node, const char* name,
	uint32 type, int openMode, void** _cookie)
{
	nspace *ns = (nspace*)_vol->private_volume;
	vnode *node = (vnode*)_node->private_node;
	attrcookie *cookie = NULL;
	ntfschar *uname = NULL;
	int ulen;
	ntfs_inode *ni = NULL;
	ntfs_attr *na = NULL;
	status_t result = B_NO_ERROR;

	TRACE("%s - ENTER\n", __FUNCTION__);

	LOCK_VOL(ns);

	if (node == NULL) {
		result = EINVAL;
		goto exit;
	}

	ni = ntfs_inode_open(ns->ntvol, node->vnid);
	if (ni == NULL) {
		result = errno;
		ERROR("%s - inode_open: %s\n", __FUNCTION__, strerror(result));
		goto exit;
	}

	// UXA demangling TODO

	// check for EA first... TODO: WRITEME


	// check for a named stream
	if (true) {
		uname = ntfs_calloc(MAX_PATH);
		ulen = ntfs_mbstoucs(name, &uname);
		if (ulen < 0) {
			result = EILSEQ;
			ERROR("%s - mb alloc: %s\n", __FUNCTION__, strerror(result));
			goto exit;
		}

		na = ntfs_attr_open(ni, AT_DATA, uname, ulen);
		if (na) {
			result = EEXIST;
			ERROR("%s - ntfs_attr_open: %s\n", __FUNCTION__,
				strerror(result));
			goto exit;
		}
		//if (ntfs_non_resident_attr_record_add(ni, AT_DATA, uname, ulen, 0, 32,
		//	0) < 0) {
		if (ntfs_attr_add(ni, AT_DATA, uname, ulen, NULL, 0) < 0) {
			result = errno;
			//ERROR("%s - ntfs_non_resident_attr_record_add: %s\n",
			ERROR("%s - ntfs_attr_add: %s\n",				__FUNCTION__, strerror(result));
			goto exit;
		}
		na = ntfs_attr_open(ni, AT_DATA, uname, ulen);
		if (!na) {
			result = errno;
			ERROR("%s - ntfs_attr_open: %s\n", __FUNCTION__,
				strerror(result));
			goto exit;
		}
	}


	cookie = (attrcookie*)ntfs_calloc(sizeof(attrcookie));

	if (cookie != NULL) {
		cookie->omode = openMode;
		*_cookie = (void*)cookie;
		cookie->inode = ni;
		cookie->stream = na;
		ni = NULL;
		na = NULL;
	} else
		result = ENOMEM;

exit:
	if (uname)
		free(uname);

	if (na)
		ntfs_attr_close(na);

	if (ni)
		ntfs_inode_close(ni);

	TRACE("%s - EXIT, result is %s\n", __FUNCTION__, strerror(result));

	UNLOCK_VOL(ns);

	return result;
}


status_t
fs_open_attrib(fs_volume *_vol, fs_vnode *_node, const char *name,
	int openMode, void **_cookie)
{
	nspace *ns = (nspace*)_vol->private_volume;
	vnode *node = (vnode*)_node->private_node;
	attrcookie *cookie = NULL;
	ntfschar *uname = NULL;
	int ulen;
	ntfs_inode *ni = NULL;
	ntfs_attr *na = NULL;
	status_t result = B_NO_ERROR;

	TRACE("%s - ENTER\n", __FUNCTION__);

	LOCK_VOL(ns);

	if (node == NULL) {
		result = EINVAL;
		goto exit;
	}

	ni = ntfs_inode_open(ns->ntvol, node->vnid);
	if (ni == NULL) {
		result = errno;
		goto exit;
	}

	// UXA demangling TODO

	// check for EA first... TODO: WRITEME


	// check for a named stream
	if (true) {
		uname = ntfs_calloc(MAX_PATH);
		ulen = ntfs_mbstoucs(name, &uname);
		if (ulen < 0) {
			result = EILSEQ;
			goto exit;
		}

		na = ntfs_attr_open(ni, AT_DATA, uname, ulen);
		if (na) {
			if (openMode & O_TRUNC) {
				if (ntfs_attr_truncate(na, 0))
					result = errno;
			}
		} else {
			result = ENOENT;
			goto exit;
		}
	}


	cookie = (attrcookie*)ntfs_calloc(sizeof(attrcookie));

	if (cookie != NULL) {
		cookie->omode = openMode;
		*_cookie = (void*)cookie;
		cookie->inode = ni;
		cookie->stream = na;
		ni = NULL;
		na = NULL;
	} else
		result = ENOMEM;

exit:
	if (uname)
		free(uname);

	if (na)
		ntfs_attr_close(na);

	if (ni)
		ntfs_inode_close(ni);

	TRACE("%s - EXIT, result is %s\n", __FUNCTION__, strerror(result));

	UNLOCK_VOL(ns);

	return result;
}


status_t
fs_close_attrib(fs_volume *_vol, fs_vnode *_node, void *cookie)
{
	return B_NO_ERROR;
}


status_t
fs_free_attrib_cookie(fs_volume *_vol, fs_vnode *_node, void *_cookie)
{
	nspace *ns = (nspace*)_vol->private_volume;
	attrcookie *cookie = (attrcookie *)_cookie;

	LOCK_VOL(ns);

	if (cookie->stream)
		ntfs_attr_close(cookie->stream);
	if (cookie->inode)
		ntfs_inode_close(cookie->inode);

	UNLOCK_VOL(ns);

	free(cookie);
	return B_NO_ERROR;
}


status_t 
fs_read_attrib_stat(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	struct stat *stat)
{
	nspace *ns = (nspace *)_vol->private_volume;
	//vnode *node = (vnode *)_node->private_node;
	attrcookie *cookie = (attrcookie *)_cookie;
	//ntfs_inode *ni = cookie->inode;
	ntfs_attr *na = cookie->stream;

	//status_t result = B_NO_ERROR;

	LOCK_VOL(ns);

	//ERRPRINT("%s - ENTER\n", __FUNCTION__);

	stat->st_type = B_XATTR_TYPE;
	stat->st_size = na ? na->data_size : 0;

//exit:

	UNLOCK_VOL(ns);
	
	return B_NO_ERROR;
}


status_t 
fs_read_attrib(fs_volume *_vol, fs_vnode *_node, void *_cookie, off_t pos,
	void *buffer, size_t *len)
{
	nspace *ns = (nspace *)_vol->private_volume;
	//vnode *node = (vnode *)_node->private_node;
	attrcookie *cookie = (attrcookie *)_cookie;
	ntfs_inode *ni = cookie->inode;
	ntfs_attr *na = cookie->stream;
	size_t size = *len;
	int total = 0;
	status_t result = B_NO_ERROR;

	if (pos < 0) {
		*len = 0;
		return EINVAL;
	}


	LOCK_VOL(ns);

	TRACE("%s - ENTER\n", __FUNCTION__);

	// it is a named stream
	if (na) {
		if (pos + size > na->data_size)
			size = na->data_size - pos;

		while (size) {
			off_t bytesRead = ntfs_attr_pread(na, pos, size, buffer);
			if (bytesRead < (s64)size) {
				ntfs_log_error("ntfs_attr_pread returned less bytes than "
					"requested.\n");
			}
			if (bytesRead <= 0) {
				*len = 0;
				result = EINVAL;
				goto exit;
			}
			size -= bytesRead;
			pos += bytesRead;
			total += bytesRead;
		}

		*len = total;
	} else {
		*len = 0;
		result = ENOENT; // TODO
	}

	fs_ntfs_update_times(_vol, ni, NTFS_UPDATE_ATIME); // XXX needed ?

exit:
	
	TRACE("%s - EXIT, result is %s\n", __FUNCTION__, strerror(result));

	UNLOCK_VOL(ns);
	
	return result;
}


status_t
fs_write_attrib(fs_volume *_vol, fs_vnode *_node, void *_cookie,off_t pos,
	const void *buffer, size_t *_length)
{
	nspace *ns = (nspace *)_vol->private_volume;
	//vnode *node = (vnode *)_node->private_node;
	attrcookie *cookie = (attrcookie *)_cookie;
	ntfs_inode *ni = cookie->inode;
	ntfs_attr *na = cookie->stream;
	size_t  size = *_length;
	int total = 0;
	status_t result = B_NO_ERROR;

	TRACE("%s - ENTER!!\n", __FUNCTION__);
	if (ns->flags & B_FS_IS_READONLY) {
		ERROR("ntfs is read-only\n");
		return EROFS;
	}

	if (pos < 0) {
		*_length = 0;
		return EINVAL;
	}


	LOCK_VOL(ns);

	TRACE("%s - ENTER\n", __FUNCTION__);

	// it is a named stream
	if (na) {
		if (cookie->omode & O_APPEND)
			pos = na->data_size;

		if (pos + size > na->data_size) {
			ntfs_mark_free_space_outdated(ns);
			if (ntfs_attr_truncate(na, pos + size))
				size = na->data_size - pos;
			else
				notify_stat_changed(ns->id, MREF(ni->mft_no), B_STAT_SIZE);
		}

		while (size) {
			off_t bytesWritten = ntfs_attr_pwrite(na, pos, size, buffer);
			if (bytesWritten < (s64)size)
				ERROR("%s - ntfs_attr_pwrite returned less bytes than "
					"requested.\n", __FUNCTION__);
			if (bytesWritten <= 0) {
				ERROR("%s - ntfs_attr_pwrite()<=0\n", __FUNCTION__);
				*_length = 0;
				result = EINVAL;
				goto exit;
			}
			size -= bytesWritten;
			pos += bytesWritten;
			total += bytesWritten;
		}

		*_length = total;
	} else {
		*_length = 0;
		return EINVAL;
	}

	
	if (total > 0)
		fs_ntfs_update_times(_vol, ni, NTFS_UPDATE_ATIME); // XXX needed ?

exit:
	
	TRACE("%s - EXIT, result is %s\n", __FUNCTION__, strerror(result));

	UNLOCK_VOL(ns);
	
	return result;
}

