/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FS_ATTR_H
#define	_FS_ATTR_H


#include <OS.h>
#include <dirent.h>


typedef struct attr_info {
	uint32	type;
	off_t	size;
} attr_info;


#ifdef  __cplusplus
extern "C" {
#endif


/* Since libroot_build is also used on Haiku and linked against the real
 * libroot which also has the fs*attr functions, these must be shadowed. */
#ifndef BUILDING_FS_ATTR_HAIKU
#define fs_read_attr		build_fs_read_attr
#define fs_write_attr		build_fs_write_attr
#define fs_remove_attr		build_fs_remove_attr
#define fs_stat_attr		build_fs_stat_attr
#define fs_open_attr		build_fs_open_attr
#define fs_fopen_attr		build_fs_fopen_attr
#define fs_close_attr		build_fs_close_attr
#define fs_open_attr_dir	build_fs_open_attr_dir
#define fs_fopen_attr_dir	build_fs_fopen_attr_dir
#define fs_close_attr_dir	build_fs_close_attr_dir
#define fs_read_attr_dir	build_fs_read_attr_dir
#define fs_rewind_attr_dir	build_fs_rewind_attr_dir
#endif


extern ssize_t	fs_read_attr(int fd, const char *attribute, uint32 type,
					off_t pos, void *buffer, size_t readBytes);
extern ssize_t	fs_write_attr(int fd, const char *attribute, uint32 type,
					off_t pos, const void *buffer, size_t readBytes);
extern int		fs_remove_attr(int fd, const char *attribute);
extern int		fs_stat_attr(int fd, const char *attribute,
					struct attr_info *attrInfo);

extern int		fs_open_attr(const char *path, const char *attribute,
					uint32 type, int openMode);
extern int		fs_fopen_attr(int fd, const char *attribute, uint32 type,
					int openMode);
extern int		fs_close_attr(int fd);

extern DIR		*fs_open_attr_dir(const char *path);
extern DIR		*fs_fopen_attr_dir(int fd);
extern int		fs_close_attr_dir(DIR *dir);
extern struct dirent *fs_read_attr_dir(DIR *dir);
extern void		fs_rewind_attr_dir(DIR *dir);

#ifdef  __cplusplus
}
#endif

#endif	/* _FS_ATTR_H */
