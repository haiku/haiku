/* 
** Distributed under the terms of the MIT License.
*/
#ifndef _FSSH_DIRENT_H
#define _FSSH_DIRENT_H


#include "fssh_defs.h"


typedef struct fssh_dirent {
	fssh_dev_t			d_dev;		/* device */
	fssh_dev_t			d_pdev;		/* parent device (only for queries) */
	fssh_ino_t			d_ino;		/* inode number */
	fssh_ino_t			d_pino;		/* parent inode (only for queries) */
	unsigned short		d_reclen;	/* length of this record, not the name */
	char				d_name[1];	/* name of the entry (null byte terminated) */
} fssh_dirent_t;

typedef struct {
	int					fd;
	struct fssh_dirent	ent;
} fssh_DIR;

#ifdef __cplusplus
extern "C" {
#endif

fssh_DIR			*fssh_opendir(const char *dirname);
struct fssh_dirent	*fssh_readdir(fssh_DIR *dir);
int					fssh_readdir_r(fssh_DIR *dir, struct fssh_dirent *entry,
						struct fssh_dirent **_result);
int					fssh_closedir(fssh_DIR *dir);
void				fssh_rewinddir(fssh_DIR *dir);
void 				fssh_seekdir(fssh_DIR *dir, long int loc);
long int			fssh_telldir(fssh_DIR *);

#ifdef __cplusplus
}
#endif

#endif	/* _FSSH_DIRENT_H */
