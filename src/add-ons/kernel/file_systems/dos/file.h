/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_FILE_H_
#define _DOSFS_FILE_H_

status_t write_vnode_entry(nspace *vol, vnode *node);

status_t	dosfs_get_vnode_name(void *_ns, void *_node, char *buffer, size_t bufferSize);
status_t	dosfs_release_vnode(void *_vol, void *_node, bool reenter);
status_t	dosfs_rstat(void *_vol, void *_node, struct stat *st);
status_t	dosfs_open(void *_vol, void *_node, int omode, void **cookie);
status_t	dosfs_read(void *_vol, void *_node, void *cookie, off_t pos,
					void *buf, size_t *len);
status_t	dosfs_free_cookie(void *vol, void *node, void *cookie);
status_t	dosfs_close(void *vol, void *node, void *cookie);

status_t	dosfs_remove_vnode(void *vol, void *node, bool reenter);
status_t	dosfs_create(void *vol, void *dir, const char *name,
					int omode, int perms, void **cookie, vnode_id *vnid);
status_t	dosfs_mkdir(void *vol, void *dir, const char *name, int perms, vnode_id *_vnid);
status_t	dosfs_rename(void *vol, void *olddir, const char *oldname,
					void *newdir, const char *newname);
status_t	dosfs_unlink(void *vol, void *dir, const char *name);
status_t	dosfs_rmdir(void *vol, void *dir, const char *name);
status_t	dosfs_wstat(void *vol, void *node, const struct stat *st, uint32 mask);
status_t	dosfs_write(void *vol, void *node, void *cookie, off_t pos,
						const void *buf, size_t *len);
status_t	dosfs_get_file_map(void *fs, void *node, off_t pos, size_t reqLen,
						struct file_io_vec *vecs, size_t *_count);
bool		dosfs_can_page(fs_volume _fs, fs_vnode _v, fs_cookie _cookie);
status_t	dosfs_read_pages(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, off_t pos,
				const iovec *vecs, size_t count, size_t *_numBytes, bool reenter);
status_t	dosfs_write_pages(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, off_t pos,
				const iovec *vecs, size_t count, size_t *_numBytes, bool reenter);

#endif
