/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_ATTR_H_
#define _DOSFS_ATTR_H_

status_t set_mime_type(vnode *node, const char *filename);

int dosfs_open_attrdir(void *_vol, void *_node, void **_cookie);
int dosfs_close_attrdir(void *_vol, void *_node, void *_cookie);
int dosfs_free_attrcookie(void *_vol, void *_node, void *_cookie);
int dosfs_rewind_attrdir(void *_vol, void *_node, void *_cookie);
int dosfs_read_attrdir(void *_vol, void *_node, void *_cookie, long *num, 
	struct dirent *buf, size_t bufsize);
int dosfs_stat_attr(void *_vol, void *_node, const char *name, struct attr_info *buf);
int dosfs_read_attr(void *_vol, void *_node, const char *name, int type, void *buf, 
	size_t *len, off_t pos);
int dosfs_write_attr(void *_vol, void *_node, const char *name, int type,
	const void *buf, size_t *len, off_t pos);

#endif
