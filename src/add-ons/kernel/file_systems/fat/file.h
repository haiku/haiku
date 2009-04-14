/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef FAT_FILE_H
#define FAT_FILE_H

mode_t		make_mode(nspace *volume, vnode *node);
status_t	write_vnode_entry(nspace *vol, vnode *node);

status_t	dosfs_get_vnode_name(fs_volume *_vol, fs_vnode *_node,
				char *buffer, size_t bufferSize);
status_t	dosfs_release_vnode(fs_volume *_vol, fs_vnode *_node,
				bool reenter);
status_t	dosfs_rstat(fs_volume *_vol, fs_vnode *_node, struct stat *st);
status_t	dosfs_open(fs_volume *_vol, fs_vnode *_node, int omode,
				void **cookie);
status_t	dosfs_read(fs_volume *_vol, fs_vnode *_node, void *cookie,
				off_t pos, void *buf, size_t *len);
status_t	dosfs_free_cookie(fs_volume *vol, fs_vnode *node, void *cookie);
status_t	dosfs_close(fs_volume *vol, fs_vnode *node, void *cookie);

status_t	dosfs_remove_vnode(fs_volume *vol, fs_vnode *node, bool reenter);
status_t	dosfs_create(fs_volume *vol, fs_vnode *dir, const char *name,
				int omode, int perms, void **cookie, ino_t *vnid);
status_t	dosfs_mkdir(fs_volume *vol, fs_vnode *dir, const char *name,
				int perms);
status_t	dosfs_rename(fs_volume *vol, fs_vnode *olddir, const char *oldname,
				fs_vnode *newdir, const char *newname);
status_t	dosfs_unlink(fs_volume *vol, fs_vnode *dir, const char *name);
status_t	dosfs_rmdir(fs_volume *vol, fs_vnode *dir, const char *name);
status_t	dosfs_wstat(fs_volume *vol, fs_vnode *node, const struct stat *st,
				uint32 mask);
status_t	dosfs_write(fs_volume *vol, fs_vnode *node, void *cookie,
				off_t pos, const void *buf, size_t *len);
status_t	dosfs_get_file_map(fs_volume *_vol, fs_vnode *_node, off_t pos,
				size_t reqLen, struct file_io_vec *vecs, size_t *_count);
bool		dosfs_can_page(fs_volume *_vol, fs_vnode *_node, void *_cookie);
status_t	dosfs_read_pages(fs_volume *_vol, fs_vnode *_node, void *_cookie,
				off_t pos, const iovec *vecs, size_t count, size_t *_numBytes);
status_t	dosfs_write_pages(fs_volume *_vol, fs_vnode *_node, void *_cookie,
				off_t pos, const iovec *vecs, size_t count, size_t *_numBytes);

#endif	/* FAT_FILE_H */
