/* File System File and Block Caches
** 
** Distributed under the terms of the Haiku License.
*/
#ifndef _FS_CACHE_H
#define _FS_CACHE_H


#include <fs_interface.h>


#ifdef __cplusplus
extern "C" {
#endif 

/* file cache */
extern void *file_cache_create(mount_id mountID, vnode_id vnodeID, off_t size, int openMode);
extern void file_cache_delete(void *_cacheRef);
extern status_t file_cache_set_size(void *_cacheRef, off_t size);

extern status_t file_cache_read_pages(void *_cacheRef, off_t offset,
					const iovec *vecs, size_t count, size_t *_numBytes);
extern status_t file_cache_write_pages(void *_cacheRef, off_t offset,
					const iovec *vecs, size_t count, size_t *_numBytes);
extern status_t file_cache_read(void *_cacheRef, off_t offset, void *bufferBase, size_t *_size);
extern status_t file_cache_write(void *_cacheRef, off_t offset, const void *buffer, size_t *_size);

#ifdef __cplusplus
}
#endif 

#endif	/* _FS_CACHE_H */
