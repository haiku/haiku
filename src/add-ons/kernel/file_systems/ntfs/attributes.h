/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/* attributes.h
 * handles mime type information for ntfs
 * gets/sets mime information in vnode
 */
 
#ifndef _fs_ATTR_H_
#define _fs_ATTR_H_

#include <fs_attr.h>

status_t set_mime(vnode *node, const char *filename);

status_t fs_open_attrib_dir(void *_vol, void *_node, void **_cookie);
status_t fs_close_attrib_dir(void *_vol, void *_node, void *_cookie);
status_t fs_free_attrib_dir_cookie(void *_vol, void *_node, void *_cookie);
status_t fs_rewind_attrib_dir(void *_vol, void *_node, void *_cookie);
status_t fs_read_attrib_dir(void *_vol, void *_node, void *_cookie, struct dirent *buf, size_t bufsize, uint32 *num);
status_t fs_open_attrib(void *_vol, void *_node, const char *name, int openMode,fs_cookie *_cookie);
status_t fs_close_attrib(void *_vol, void *_node, fs_cookie cookie);
status_t fs_free_attrib_cookie(void *_vol, void *_node, fs_cookie cookie);
status_t fs_read_attrib_stat(void *_vol, void *_node, fs_cookie _cookie, struct stat *stat);
status_t fs_read_attrib(void *_vol, void *_node, fs_cookie _cookie, off_t pos,void *buffer, size_t *_length);
status_t fs_write_attrib(void *_vol, void *_node, fs_cookie _cookie, off_t pos,	const void *buffer, size_t *_length);

#endif
