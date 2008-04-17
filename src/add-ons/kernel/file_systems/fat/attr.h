/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_ATTR_H_
#define _DOSFS_ATTR_H_

#include <fs_attr.h>

status_t set_mime_type(vnode *node, const char *filename);

status_t dosfs_open_attrdir(fs_volume *_vol, fs_vnode *_node, void **_cookie);
status_t dosfs_close_attrdir(fs_volume *_vol, fs_vnode *_node, void *_cookie);
status_t dosfs_free_attrdir_cookie(fs_volume *_vol, fs_vnode *_node,
	void *_cookie);
status_t dosfs_rewind_attrdir(fs_volume *_vol, fs_vnode *_node, void *_cookie);
status_t dosfs_read_attrdir(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	struct dirent *buf, size_t bufsize, uint32 *num);
status_t dosfs_open_attr(fs_volume *_vol, fs_vnode *_node, const char *name,
	int openMode, void **_cookie);
status_t dosfs_close_attr(fs_volume *_vol, fs_vnode *_node, void *cookie);
status_t dosfs_free_attr_cookie(fs_volume *_vol, fs_vnode *_node, void *cookie);
status_t dosfs_read_attr_stat(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	struct stat *stat);
status_t dosfs_read_attr(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	off_t pos, void *buffer, size_t *_length);
status_t dosfs_write_attr(fs_volume *_vol, fs_vnode *_node, void *_cookie,
	off_t pos, const void *buffer, size_t *_length);

#endif
