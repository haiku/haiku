#ifndef _UTIME_H_
#define _UTIME_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/types.h>


struct utimbuf {
	time_t actime;		/* access time */
	time_t modtime;		/* modification time */
};


extern
#ifdef __cplusplus
"C"
#endif
int utime(const char *path, const struct utimbuf *buffer);

#endif	/* _UTIME_H_ */
