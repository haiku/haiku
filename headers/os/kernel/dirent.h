/**
 * @file dirent.h
 * @brief File Control functions and definitions
 */

#ifndef _DIRENT_H
#define	_DIRENT_H

typedef struct dirent {
	vnode_id		d_ino;
	unsigned short	d_reclen;
	char			d_name[1];
} dirent_t;

typedef struct {
	int				fd;
	struct dirent  *ent;
	struct dirent   me;
} DIR;

#ifndef MAXNAMLEN
#ifdef  NAME_MAX
#define MAXNAMLEN NAME_MAX
#else
#define MAXNAMLEN 256
#endif
#endif

DIR			 	*opendir(const char *dirname);
struct dirent	*readdir(DIR *dirp);
int				 closedir(DIR *dirp);
void			 rewinddir(DIR *dirp);

#endif /* _DIRENT_H */

