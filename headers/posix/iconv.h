#ifndef _ICONV_H_
#define _ICONV_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/
#include <size_t.h>

typedef void *iconv_t;


#ifdef __cplusplus
extern "C" {
#endif

extern iconv_t	iconv_open(const char *toCode, const char *fromCode);
extern int		iconv_close(iconv_t cd);
extern size_t	iconv(iconv_t cd, char **inBuffer, size_t *inBytesLeft,
					char **outBuffer, size_t *outBytesLeft);

#ifdef __cplusplus
}
#endif

#endif	/* _ICONV_H_ */
