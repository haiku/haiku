#ifndef _DLFCN_H
#define	_DLFCN_H
/*
** Distributed under the terms of the OpenBeOS License.
*/

#include <sys/types.h>


#define RTLD_LAZY	0	/* relocations are performed as needed */
#define RTLD_NOW	1	/* the file gets relocated at load time */
#define RTLD_LOCAL	0	/* symbols are not available for relocating any other object */
#define RTLD_GLOBAL	2	/* all symbols are available */


#ifdef __cplusplus
extern "C" {
#endif

extern int	dlclose(void *image);
extern char	*dlerror(void);
extern void	*dlopen(const char *path, int mode);
extern void *dlsym(void *image, const char *symbolName);

#ifdef __cplusplus
}
#endif

#endif /* _DLFCN_H */
