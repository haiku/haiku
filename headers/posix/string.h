#ifndef _STRING_H_
#define _STRING_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/* memXXX() functions */
extern void		*memchr(const void *source, int value, size_t length);
extern int		memcmp(const void *buffer1, const void *buffer2, size_t length);
extern void		*memcpy(void *dest, const void *source, size_t length);
extern void		*memmove(void *dest, const void *source, size_t length);
extern void		*memset(void *dest, int value, size_t length);

/* string functions */
extern char		*strcpy(char *dest, const char *source);
extern char		*strncpy(char *dest, const char *source, size_t length);
extern char		*strcat(char *dest, const char *source);
extern char		*strncat(char *dest, const char *source, size_t length);

extern size_t	strlen(const char *string);
extern int		strcmp(const char *string1, const char *string2);
extern int		strncmp(const char *string1, const char *string2, size_t length);

extern char		*strchr(const char *string, int character);
extern char		*strrchr(const char *string, int character);
extern char		*strstr(const char *string, const char *searchString);

extern char		*strchrnul(const char *string, int character);
	// this is a GNU extension

extern char		*strpbrk(const char *string, const char *set);
extern char		*strtok(char *string, const char *set);
extern char		*strtok_r(char *string, const char *set, char **savePointer);
extern size_t	strspn(const char *string, const char *set);
extern size_t	strcspn(const char *string, const char *set);

extern int		strcoll(const char *string1, const char *string2);
extern size_t	strxfrm(char *string1, const char *string2, size_t length);

extern char		*strerror(int errorCode);
extern int		strerror_r(int errorCode, char *buffer, size_t bufferSize);

// ToDo: remove bzero(), bcopy() from here - we don't need those
//	implementations anyway, since BeOS defines them as macros
//	Of course, we could also just keep them (and bcmp() is missing)
//extern void  *bcopy(void const *source, void *dest, size_t length);
//extern void   bzero(void *buffer, size_t length);
#define bcopy(source, dest, length) memcpy(dest, source, length)
#define bzero(buffer, length) memset(buffer, 0, length)

/* non-standard string functions */
extern int		strcasecmp(const char *string1, const char *string2);
extern int		strncasecmp(const char *string1, const char *string2, size_t length);
#define	stricmp(a, b)	strcasecmp(a, b)
//#define strnicmp(a, b, length) strncasecmp(a, b, length)
// ToDo: remove the function, and only have the macro
extern int		strnicmp(const char *string1, const char *string2, size_t length);

extern char		*strcasestr(const char *string, const char *searchString);

extern char		*strdup(const char *string);
extern char		*stpcpy(char *dest, const char *source);
extern const char *strtcopy(char *dest, const char *source);

extern size_t	strlcat(char *dest, const char *source, size_t length);
extern size_t	strlcpy(char *dest, const char *source, size_t length);

extern size_t	strnlen(const char *string, size_t count);

//extern char		*strlwr(char *string);
//extern char		*strupr(char *string);

//extern char		*strsep(char **stringPointer, const char *delimiter);

extern const char	*strsignal(int signal);

#ifdef __cplusplus
}
#endif

#endif  /* _STRING_H_ */
