/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "fssh_string.h"

#include <string.h>


void*
fssh_memchr(const void *source, int value, fssh_size_t length)
{
	return memchr((void*)source, value, length);
}


int
fssh_memcmp(const void *buffer1, const void *buffer2, fssh_size_t length)
{
	return memcmp(buffer1, buffer2, length);
}


void*
fssh_memcpy(void *dest, const void *source, fssh_size_t length)
{
	return memcpy(dest, source, length);
}


#if 0
void*
fssh_memccpy(void *dest, const void *source, int stopByte, fssh_size_t length)
{
	return memccpy(dest, source, stopByte, length);
}
#endif	// 0


void*
fssh_memmove(void *dest, const void *source, fssh_size_t length)
{
	return memmove(dest, source, length);
}


void*
fssh_memset(void *dest, int value, fssh_size_t length)
{
	return memset(dest, value, length);
}


char*
fssh_strcpy(char *dest, const char *source)
{
	return strcpy(dest, source);
}


char*
fssh_strncpy(char *dest, const char *source, fssh_size_t length)
{
	return strncpy(dest, source, length);
}


char*
fssh_strcat(char *dest, const char *source)
{
	return strcat(dest, source);
}


char*
fssh_strncat(char *dest, const char *source, fssh_size_t length)
{
	return strncat(dest, source, length);
}


fssh_size_t
fssh_strlen(const char *string)
{
	return strlen(string);
}


int
fssh_strcmp(const char *string1, const char *string2)
{
	return strcmp(string1, string2);
}


int
fssh_strncmp(const char *string1, const char *string2, fssh_size_t length)
{
	return strncmp(string1, string2, length);
}


char*
fssh_strchr(const char *string, int character)
{
	return strchr((char*)string, character);
}


char*
fssh_strrchr(const char *string, int character)
{
	return strrchr((char*)string, character);
}


char*
fssh_strstr(const char *string, const char *searchString)
{
	return strstr((char*)string, searchString);
}


#if 0
char*
fssh_strchrnul(const char *string, int character)
{
}
#endif	// 0


char*
fssh_strpbrk(const char *string, const char *set)
{
	return strpbrk((char*)string, set);
}


char*
fssh_strtok(char *string, const char *set)
{
	return strtok(string, set);
}


char*
fssh_strtok_r(char *string, const char *set, char **savePointer)
{
	return strtok_r(string, set, savePointer);
}


fssh_size_t
fssh_strspn(const char *string, const char *set)
{
	return strspn(string, set);
}


fssh_size_t
fssh_strcspn(const char *string, const char *set)
{
	return strcspn(string, set);
}


int
fssh_strcoll(const char *string1, const char *string2)
{
	return strcoll(string1, string2);
}


fssh_size_t
fssh_strxfrm(char *string1, const char *string2, fssh_size_t length)
{
	return strxfrm(string1, string2, length);
}


char*
fssh_strerror(int errorCode)
{
	return strerror(errorCode);
}


#if 0
int
fssh_strerror_r(int errorCode, char *buffer, fssh_size_t bufferSize)
{
	return strerror_r(errorCode, buffer, bufferSize);
}
#endif	// 0


int
fssh_strcasecmp(const char *string1, const char *string2)
{
	return strcasecmp(string1, string2);
}


int
fssh_strncasecmp(const char *string1, const char *string2, fssh_size_t length)
{
	return strncasecmp(string1, string2, length);
}


#if 0
char*
fssh_strcasestr(const char *string, const char *searchString)
{
	return strcasestr(string, searchString);
}
#endif	// 0


char*
fssh_strdup(const char *string)
{
	if (!string)
		return NULL;
	return strdup(string);
}


char*
fssh_stpcpy(char *dest, const char *source)
{
	return stpcpy(dest, source);
}


#if 0
const char *
fssh_strtcopy(char *dest, const char *source)
{
	return strtcopy(dest, source);
}
#endif	// 0


fssh_size_t
fssh_strlcat(char *dest, const char *source, fssh_size_t length)
{
	return strlcat(dest, source, length);
}


fssh_size_t
fssh_strlcpy(char *dest, const char *source, fssh_size_t length)
{
	return strlcpy(dest, source, length);
}


fssh_size_t
fssh_strnlen(const char *string, fssh_size_t count)
{
	return strnlen(string, count);
}


int
fssh_ffs(int i)
{
	return ffs(i);
}


#if 0
char*
fssh_index(const char *s, int c)
{
	return index(s, c);
}
#endif	// 0


#if 0
char*
fssh_rindex(char const *s, int c)
{
	return rindex(s, c);
}
#endif	// 0
