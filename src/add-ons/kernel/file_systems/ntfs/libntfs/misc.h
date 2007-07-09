#ifndef _NTFS_MISC_H_
#define _NTFS_MISC_H_

void *ntfs_calloc(size_t size);
void *ntfs_malloc(size_t size);

#if defined(__BEOS__) || defined(__HAIKU__)
#define BUF_SIZE 16384
int ntfs_snprintf(char *buff, size_t size, const char *format, ...);
#define snprintf ntfs_snprintf
#endif

#endif /* _NTFS_MISC_H_ */
