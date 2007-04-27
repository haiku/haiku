/*
 * Copyright 2004-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_STRING_H
#define _FSSH_STRING_H


#include "fssh_defs.h"


#ifdef __cplusplus
extern "C" {
#endif


/* memXXX() functions */
extern void		*fssh_memchr(const void *source, int value, fssh_size_t length);
extern int		fssh_memcmp(const void *buffer1, const void *buffer2,
						fssh_size_t length);
extern void		*fssh_memcpy(void *dest, const void *source,
						fssh_size_t length);
extern void		*fssh_memccpy(void *dest, const void *source, int stopByte,
						fssh_size_t length);
extern void		*fssh_memmove(void *dest, const void *source,
						fssh_size_t length);
extern void		*fssh_memset(void *dest, int value, fssh_size_t length);

/* string functions */
extern char		*fssh_strcpy(char *dest, const char *source);
extern char		*fssh_strncpy(char *dest, const char *source,
						fssh_size_t length);
extern char		*fssh_strcat(char *dest, const char *source);
extern char		*fssh_strncat(char *dest, const char *source,
						fssh_size_t length);

extern fssh_size_t	fssh_strlen(const char *string);
extern int		fssh_strcmp(const char *string1, const char *string2);
extern int		fssh_strncmp(const char *string1, const char *string2,
						fssh_size_t length);

extern char		*fssh_strchr(const char *string, int character);
extern char		*fssh_strrchr(const char *string, int character);
extern char		*fssh_strstr(const char *string, const char *searchString);

extern char		*fssh_strchrnul(const char *string, int character);
	// this is a GNU extension

extern char		*fssh_strpbrk(const char *string, const char *set);
extern char		*fssh_strtok(char *string, const char *set);
extern char		*fssh_strtok_r(char *string, const char *set,
						char **savePointer);
extern fssh_size_t	fssh_strspn(const char *string, const char *set);
extern fssh_size_t	fssh_strcspn(const char *string, const char *set);

extern int		fssh_strcoll(const char *string1, const char *string2);
extern fssh_size_t	fssh_strxfrm(char *string1, const char *string2,
						fssh_size_t length);

extern char		*fssh_strerror(int errorCode);
extern int		fssh_strerror_r(int errorCode, char *buffer,
						fssh_size_t bufferSize);

/* non-standard string functions */
extern int		fssh_strcasecmp(const char *string1, const char *string2);
extern int		fssh_strncasecmp(const char *string1, const char *string2,
						fssh_size_t length);

extern char		*fssh_strcasestr(const char *string, const char *searchString);

extern char		*fssh_strdup(const char *string);
extern char		*fssh_stpcpy(char *dest, const char *source);
extern const char *fssh_strtcopy(char *dest, const char *source);

extern fssh_size_t	fssh_strlcat(char *dest, const char *source,
						fssh_size_t length);
extern fssh_size_t	fssh_strlcpy(char *dest, const char *source,
						fssh_size_t length);

extern fssh_size_t	fssh_strnlen(const char *string, fssh_size_t count);

extern int fssh_ffs(int i);
extern char 		*fssh_index(const char *s, int c);
extern char 		*fssh_rindex(char const *s, int c);


#ifdef __cplusplus
}
#endif

#endif  /* _FSSH_STRING_H */
