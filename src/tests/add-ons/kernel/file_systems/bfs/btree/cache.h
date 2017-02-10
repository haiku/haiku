#ifndef CACHE_H
#define CACHE_H
/* cache - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include <SupportDefs.h>

class BFile;


extern void init_cache(BFile* file, int32 blockSize);
extern void shutdown_cache(BFile* file, int32 blockSize);

extern status_t cached_write(void* cache, off_t num, const void* _data,
	off_t numBlocks);


// Block Cache API

extern "C" {

extern const void* block_cache_get(void* _cache, off_t blockNumber);

extern status_t block_cache_make_writable(void* _cache, off_t blockNumber,
	int32 transaction);
extern void* block_cache_get_writable(void* _cache, off_t blockNumber,
	int32 transaction);

extern status_t block_cache_set_dirty(void* _cache, off_t blockNumber,
	bool dirty, int32 transaction);
extern void block_cache_put(void* _cache, off_t blockNumber);

} // extern "C"

#endif	/* CACHE_H */
