#ifndef _INTTYPES_H_
#define _INTTYPES_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/


#include <stdint.h>


typedef struct {
	intmax_t	quot;	/* quotient */
	intmax_t	rem;	/* remainder */
} imaxdiv_t;


#ifdef __cplusplus
extern "C" {
#endif

extern intmax_t		imaxabs(intmax_t num);
extern imaxdiv_t	imaxdiv(intmax_t numer, intmax_t denom);

extern intmax_t		strtoimax(const char *string, char **_end, int base);
extern uintmax_t	strtoumax(const char *string, char **_end, int base);
//extern intmax_t		wcstoimax(const __wchar_t *, __wchar_t **, int);
//extern uintmax_t	wcstoumax(const __wchar_t *, __wchar_t **, int);

#ifdef __cplusplus
}
#endif

#endif	/* _INTTYPES_H_ */
