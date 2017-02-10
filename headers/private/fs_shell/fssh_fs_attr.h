/* File System attributes
**
** Distributed under the terms of the MIT License.
*/
#ifndef _FSSH_FS_ATTR_H
#define	_FSSH_FS_ATTR_H


#include "fssh_defs.h"
#include "fssh_dirent.h"


typedef struct fssh_attr_info {
	uint32_t	type;
	fssh_off_t	size;
} fssh_attr_info;


#ifdef  __cplusplus
extern "C" {
#endif

extern fssh_ssize_t	fssh_fs_read_attr(int fd, const char *attribute,
							uint32_t type, fssh_off_t pos, void *buffer,
							fssh_size_t readBytes);
extern fssh_ssize_t	fssh_fs_write_attr(int fd, const char *attribute,
							uint32_t type, fssh_off_t pos, const void *buffer,
							fssh_size_t readBytes);
extern int			fssh_fs_remove_attr(int fd, const char *attribute);
extern int			fssh_fs_stat_attr(int fd, const char *attribute,
							struct fssh_attr_info *attrInfo);

// ToDo: the following three functions are not part of the R5 API, and
// are only preliminary - they may change or be removed at any point
//extern int		fssh_fs_open_attr(const char *path, const char *attribute, uint32_t type, int openMode);
extern int			fssh_fs_open_attr(int fd, const char *attribute,
							uint32_t type, int openMode);
extern int			fssh_fs_close_attr(int fd);

extern fssh_DIR		*fssh_fs_open_attr_dir(const char *path);
extern fssh_DIR		*fssh_fs_fopen_attr_dir(int fd);
extern int			fssh_fs_close_attr_dir(fssh_DIR *dir);
extern struct fssh_dirent *fssh_fs_read_attr_dir(fssh_DIR *dir);
extern void			fssh_fs_rewind_attr_dir(fssh_DIR *dir);

#ifdef  __cplusplus
}
#endif

#endif	/* _FSSH_FS_ATTR_H */
