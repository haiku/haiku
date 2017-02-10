/* File System indices
**
** Distributed under the terms of the MIT License.
*/
#ifndef _FSSH_FS_INDEX_H
#define	_FSSH_FS_INDEX_H


#include "fssh_defs.h"
#include "fssh_dirent.h"


typedef struct fssh_index_info {
	uint32_t	type;
	fssh_off_t	size;
	fssh_time_t	modification_time;
	fssh_time_t	creation_time;
	fssh_uid_t	uid;
	fssh_gid_t	gid;
} fssh_index_info;


#ifdef  __cplusplus
extern "C" {
#endif

extern int		fssh_fs_create_index(fssh_dev_t device, const char *name,
						uint32_t type, uint32_t flags);
extern int		fssh_fs_remove_index(fssh_dev_t device, const char *name);
extern int		fssh_fs_stat_index(fssh_dev_t device, const char *name,
						struct fssh_index_info *indexInfo);

extern fssh_DIR	*fssh_fs_open_index_dir(fssh_dev_t device);
extern int		fssh_fs_close_index_dir(fssh_DIR *indexDirectory);
extern struct fssh_dirent *fssh_fs_read_index_dir(fssh_DIR *indexDirectory);
extern void		fssh_fs_rewind_index_dir(fssh_DIR *indexDirectory);

#ifdef  __cplusplus
}
#endif

#endif	/* _FSSH_FS_INDEX_H */
