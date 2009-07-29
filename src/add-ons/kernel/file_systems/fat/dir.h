/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_DIR_H_
#define _DOSFS_DIR_H_


#include "dosfs.h"


bool is_filename_legal(const char *name);
status_t	check_dir_empty(nspace *vol, vnode *dir);
status_t 	findfile_case(nspace *vol, vnode *dir, const char *file,
				ino_t *vnid, vnode **node);
status_t 	findfile_nocase(nspace *vol, vnode *dir, const char *file,
				ino_t *vnid, vnode **node);
status_t 	findfile_nocase_duplicates(nspace *vol, vnode *dir, const char *file,
				ino_t *vnid, vnode **node, bool *dups_exist);
status_t 	findfile_case_duplicates(nspace *vol, vnode *dir, const char *file,
				ino_t *vnid, vnode **node, bool *dups_exist);
status_t	erase_dir_entry(nspace *vol, vnode *node);
status_t	compact_directory(nspace *vol, vnode *dir);
status_t	create_volume_label(nspace *vol, const char name[11], uint32 *index);
status_t	create_dir_entry(nspace *vol, vnode *dir, vnode *node,
				const char *name, uint32 *ns, uint32 *ne);

status_t	dosfs_read_vnode(fs_volume *_vol, ino_t vnid, fs_vnode *_node,
				int *_type, uint32 *_flags, bool reenter);
status_t	dosfs_walk(fs_volume *_vol, fs_vnode *_dir, const char *file,
				ino_t *_vnid);
status_t	dosfs_access(fs_volume *_vol, fs_vnode *_node, int mode);
status_t	dosfs_readlink(fs_volume *_vol, fs_vnode *_node, char *buf,
				size_t *bufsize);
status_t	dosfs_opendir(fs_volume *_vol, fs_vnode *_node, void **cookie);
status_t	dosfs_readdir(fs_volume *_vol, fs_vnode *_node, void *cookie,
				struct dirent *buf, size_t bufsize, uint32 *num);
status_t	dosfs_rewinddir(fs_volume *_vol, fs_vnode *_node, void *cookie);
status_t	dosfs_closedir(fs_volume *_vol, fs_vnode *_node, void *cookie);
status_t	dosfs_free_dircookie(fs_volume *_vol, fs_vnode *_node,
				void *cookie);

#endif
