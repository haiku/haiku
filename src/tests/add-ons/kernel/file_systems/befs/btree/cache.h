#ifndef CACHE_H
#define CACHE_H
/* cache - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>

class BFile;


extern void init_cache(BFile *file, int32 blockSize);
extern void shutdown_cache(BFile *file, int32 blockSize);

extern int cached_write(BFile *file, off_t bnum, const void *data,off_t num_blocks, int bsize);
extern void *get_block(BFile *file, off_t bnum, int bsize);
extern int release_block(BFile *file, off_t bnum);

#endif	/* CACHE_H */
