#ifndef _DIRENT_H
#define	_DIRENT_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/

#include <sys/types.h>


typedef struct dirent {
	dev_t			d_dev;
	dev_t			d_pdev;
	ino_t			d_ino;
	ino_t			d_pino;
	unsigned short	d_reclen;
	char			d_name[1];
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

