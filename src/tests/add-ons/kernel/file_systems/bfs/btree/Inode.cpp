/* Inode - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "Inode.h"
#include "Volume.h"
#include "Journal.h"


Inode::Inode(const char *name,int32 mode)
	:
	fMode(mode)
{
	rw_lock_init(&fLock, "inode lock");
	fFile.SetTo(name,B_CREATE_FILE | B_READ_WRITE | B_ERASE_FILE);
	fSize = 0;
	fVolume = new Volume(&fFile);
}


Inode::~Inode()
{
	delete fVolume;
}


status_t 
Inode::FindBlockRun(off_t pos, block_run &run, off_t &offset)
{
	// the whole file data is covered by this one block_run structure...
	run.SetTo(0,0,1);
	offset = 0;
	return B_OK;
}


status_t 
Inode::Append(Transaction& transaction, off_t bytes)
{
	return SetFileSize(transaction, Size() + bytes);
}


status_t 
Inode::SetFileSize(Transaction&, off_t bytes)
{
	//printf("set size = %ld\n",bytes);
	fSize = bytes;
	return fFile.SetSize(bytes);
}

