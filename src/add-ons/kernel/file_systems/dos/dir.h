/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_DIR_H_
#define _DOSFS_DIR_H_

bool is_filename_legal(const char *name);
status_t	check_dir_empty(nspace *vol, vnode *dir);
status_t 	findfile_case(nspace *vol, vnode *dir, const char *file,
				vnode_id *vnid, vnode **node);
status_t 	findfile_nocase(nspace *vol, vnode *dir, const char *file,
				vnode_id *vnid, vnode **node);
status_t 	findfile_nocase_duplicates(nspace *vol, vnode *dir, const char *file,
				vnode_id *vnid, vnode **node, bool *dups_exist);				
status_t 	findfile_case_duplicates(nspace *vol, vnode *dir, const char *file,
				vnode_id *vnid, vnode **node, bool *dups_exist);				
status_t	erase_dir_entry(nspace *vol, vnode *node);
status_t	compact_directory(nspace *vol, vnode *dir);
status_t	create_volume_label(nspace *vol, const char name[11], uint32 *index);
status_t	create_dir_entry(nspace *vol, vnode *dir, vnode *node, 
				const char *name, uint32 *ns, uint32 *ne);

int			dosfs_read_vnode(void *_vol, vnode_id vnid, char r, void **node);
int			dosfs_walk(void *_vol, void *_dir, const char *file,
				char **newpath, vnode_id *vnid);
int			dosfs_access(void *_vol, void *_node, int mode);
int			dosfs_readlink(void *_vol, void *_node, char *buf, size_t *bufsize);
int			dosfs_opendir(void *_vol, void *_node, void **cookie);
int			dosfs_readdir(void *_vol, void *_node, void *cookie,
				long *num, struct dirent *buf, size_t bufsize);
int			dosfs_rewinddir(void *_vol, void *_node, void *cookie);
int			dosfs_closedir(void *_vol, void *_node, void *cookie);
int			dosfs_free_dircookie(void *_vol, void *_node, void *cookie);

#endif
