#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/

#include <limits.h>

#define MAXPATHLEN      PATH_MAX
#define MAXSYMLINKS		SYMLOOP_MAX

#define NOFILE          OPEN_MAX

#ifndef MIN
  #define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

/* maximum possible length of this machine's hostname */
#ifndef MAXHOSTNAMELEN
  #define MAXHOSTNAMELEN 256
#endif

#endif	/* _SYS_PARAM_H */
