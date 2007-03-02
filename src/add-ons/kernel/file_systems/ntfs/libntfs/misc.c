#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "misc.h"
#include "logging.h"

/**
 * ntfs_calloc
 * 
 * Return a pointer to the allocated memory or NULL if the request fails.
 */
void *ntfs_calloc(size_t size)
{
	void *p;
	
	p = calloc(1, size);
	if (!p)
		ntfs_log_perror("Failed to calloc %lld bytes", (long long)size);
	return p;
}

void *ntfs_malloc(size_t size)
{
	void *p;
	
	p = malloc(size);
	if (!p)
		ntfs_log_perror("Failed to malloc %lld bytes", (long long)size);
	return p;
}

#if defined(__BEOS__) || defined(__HAIKU__)
int ntfs_snprintf(char *buff, size_t size, const char *format, ...)
{
 	int ret;
 	char buffer[BUFSIZ];	
 	va_list args;
 	va_start(args, format);
 	memset(buffer,0,BUFSIZ);
 	ret = sprintf(buffer, format, args);
 	va_end(args);
 	strncpy(buff,buffer,size);
 	return ret;
 }
#endif