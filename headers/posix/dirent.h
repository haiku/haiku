/* 
** Distributed under the terms of the Haiku License.
*/
#ifndef _DIRENT_H
#define _DIRENT_H


#include <sys/types.h>


typedef struct dirent {
	dev_t			d_dev;		/* device */
	dev_t			d_pdev;		/* parent device (only for queries) */
	ino_t			d_ino;		/* inode number */
	ino_t			d_pino;		/* parent inode (only for queries) */
	unsigned short	d_reclen;	/* length of this record, not the name */
	char			d_name[1];	/* name of the entry (null byte terminated) */
} dirent_t;

typedef struct {
	int				fd;
	struct dirent	ent;
} DIR;

#ifndef MAXNAMLEN
#	ifdef  NAME_MAX
#		define MAXNAMLEN NAME_MAX
#	else
#		define MAXNAMLEN 256
#	endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

DIR				*opendir(const char *dirname);
struct dirent	*readdir(DIR *dir);
int				readdir_r(DIR *dir, struct dirent *entry, struct dirent **_result);
int				closedir(DIR *dir);
void			rewinddir(DIR *dir);

#ifdef __cplusplus
}
#endif

#endif	/* _DIRENT_H */
