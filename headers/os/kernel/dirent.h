/**
 * @file dirent.h
 * @brief File Control functions and definitions
 */

#ifndef _DIRENT_H
#define	_DIRENT_H

// ToDo: should really be <sys/types>
// This file should reside in headers/posix, not here...
// needs ino_t (int64), and dev_t (long)
#include <ktypes.h>
typedef long dev_t;

typedef struct dirent {
	dev_t			d_dev;
	dev_t			d_pdev;
	ino_t			d_ino;
	ino_t			d_pino;
	unsigned short	d_reclen;
	char			d_name[1];
} dirent_t;

// ToDo: this structure is still incompatible with BeOS
typedef struct {
	int				fd;
	struct dirent  *ent;
	struct dirent   me;
} DIR;

#ifndef MAXNAMLEN
#	ifdef  NAME_MAX
#		define MAXNAMLEN NAME_MAX
#	else
#		define MAXNAMLEN 256
#	endif
#endif

#ifdef _cplusplus
extern "C" {
#endif

DIR			 	*opendir(const char *dirname);
struct dirent	*readdir(DIR *dirp);
int				 closedir(DIR *dirp);
void			 rewinddir(DIR *dirp);

#ifdef _cplusplus
}
#endif

#endif /* _DIRENT_H */

