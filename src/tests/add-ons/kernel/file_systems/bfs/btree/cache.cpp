/* cache - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "cache.h"

#include <File.h>
#include <List.h>

#include <malloc.h>
#include <stdio.h>


/*	A note from the author: this cache implementation can only be used
 *	with the test program, it really suites no other needs.
 *	It's very simple and not that efficient, and simple holds the whole
 *	file in memory, all the time.
 */


BList gBlocks;


void
init_cache(BFile */*file*/,int32 /*blockSize*/)
{
}


void
shutdown_cache(BFile *file,int32 blockSize)
{
	for (int32 i = 0;i < gBlocks.CountItems();i++) {
		void *buffer = gBlocks.ItemAt(i);
		if (buffer == NULL) {
			printf("cache is corrupt!\n");
			exit(-1);
		}
		file->WriteAt(i * blockSize,buffer,blockSize);
		free(buffer);
	}
}


static status_t
readBlocks(BFile *file,uint32 num,uint32 size)
{
	for (uint32 i = gBlocks.CountItems(); i <= num; i++) {
		void *buffer = malloc(size);
		if (buffer == NULL)
			return B_NO_MEMORY;

		gBlocks.AddItem(buffer);
		if (file->ReadAt(i * size,buffer,size) < B_OK)
			return B_IO_ERROR;
	}
	return B_OK;
}


int
cached_write(BFile *file, off_t num,const void *data,off_t numBlocks, int blockSize)
{
	//printf("cached_write(num = %Ld,data = %p,numBlocks = %Ld,blockSize = %ld)\n",num,data,numBlocks,blockSize);
	if (file == NULL)
		return B_BAD_VALUE;

	if (num >= gBlocks.CountItems())
		puts("Oh no!");

	void *buffer = gBlocks.ItemAt(num);
	if (buffer == NULL)
		return B_BAD_VALUE;

	if (buffer != data && numBlocks == 1)
		memcpy(buffer,data,blockSize);

	return B_OK;
}


void *
get_block(BFile *file, off_t num, int blockSize)
{
	//printf("get_block(num = %Ld,blockSize = %ld)\n",num,blockSize);
	if (file == NULL)
		return NULL;

	if (num >= gBlocks.CountItems())
		readBlocks(file,num,blockSize);

	return gBlocks.ItemAt(num);
}


int 
release_block(BFile *file, off_t num)
{
	//printf("release_block(num = %Ld)\n",num);
	return 0;
}


