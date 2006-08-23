/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_FILE_H_
#define _DOSFS_FILE_H_

status_t write_vnode_entry(nspace *vol, vnode *node);

int		dosfs_write_vnode(void *_vol, void *_node, char r);
int		dosfs_rstat(void *_vol, void *_node, struct stat *st);
int		dosfs_open(void *_vol, void *_node, int omode, void **cookie);
int		dosfs_read(void *_vol, void *_node, void *cookie, off_t pos,
					void *buf, size_t *len);
int		dosfs_free_cookie(void *vol, void *node, void *cookie);
int		dosfs_close(void *vol, void *node, void *cookie);

int		dosfs_remove_vnode(void *vol, void *node, char r);
int		dosfs_create(void *vol, void *dir, const char *name,
					int perms, int omode, vnode_id *vnid, void **cookie);
int		dosfs_mkdir(void *vol, void *dir, const char *name, int perms);
int		dosfs_rename(void *vol, void *olddir, const char *oldname,
					void *newdir, const char *newname);
int		dosfs_unlink(void *vol, void *dir, const char *name);
int		dosfs_rmdir(void *vol, void *dir, const char *name);
int		dosfs_wstat(void *vol, void *node, struct stat *st, long mask);
int		dosfs_write(void *vol, void *node, void *cookie, off_t pos,
						const void *buf, size_t *len);

#endif
