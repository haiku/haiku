#ifndef _NTFS_MISC_H_
#define _NTFS_MISC_H_

void *ntfs_calloc(size_t size);
void *ntfs_malloc(size_t size);

#if defined(__BEOS__) || defined(__HAIKU__)
#define snprintf ntfs_snprintf
#endif /* defined(__BEOS__) || defined(__HAIKU__) */
#endif /* _NTFS_MISC_H_ */

