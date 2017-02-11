/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/* fake_attributes.h
 * handles mime type information for ntfs
 * gets/sets mime information in vnode
 */

#ifndef NTFS_FAKE_ATTR_H_
#define NTFS_FAKE_ATTR_H_

#include <fs_attr.h>

status_t set_mime_type(vnode *node, const char *filename);

status_t fake_open_attrib_dir(fs_volume *_vol, fs_vnode *_node,
	void **_cookie);
status_t fake_close_attrib_dir(fs_volume *_vol, fs_vnode *_node,
	void *_cookie);
status_t fake_free_attrib_dir_cookie(fs_volume *_vol, fs_vnode *_node,
	void *_cookie);
status_t fake_rewind_attrib_dir(fs_volume *_vol, fs_vnode *_node,
	void *_cookie);
status_t fake_read_attrib_dir(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	struct dirent *buf, size_t bufsize, uint32 *num);
status_t fake_create_attrib(fs_volume *_vol, fs_vnode *_node, const char *name,
	uint32 type, int openMode, void** _cookie);
status_t fake_open_attrib(fs_volume *_vol, fs_vnode *_node, const char *name,
	int openMode, void **_cookie);
status_t fake_close_attrib(fs_volume *_vol, fs_vnode *_node, void *cookie);
status_t fake_free_attrib_cookie(fs_volume *_vol, fs_vnode *_node,
	void *cookie);
status_t fake_read_attrib_stat(fs_volume *_vol, fs_vnode *_node, void *cookie,
	struct stat *stat);
status_t fake_read_attrib(fs_volume *_vol, fs_vnode *_node, void *cookie,
	off_t pos,void *buffer, size_t *_length);
status_t fake_write_attrib(fs_volume *_vol, fs_vnode *_node, void *cookie,
	off_t pos,	const void *buffer, size_t *_length);

#endif //NTFS_FAKE_ATTR_H_
