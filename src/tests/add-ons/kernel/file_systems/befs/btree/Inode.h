#ifndef INODE_H
#define INODE_H
/* Inode - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>
#include <File.h>

#include "Lock.h"
#include "bfs.h"


class Volume;
class Transaction;


class Inode {
	public:
		Inode(const char *name,int32 mode = S_STR_INDEX | S_ALLOW_DUPS);
		~Inode();

		ReadWriteLock &Lock() { return fLock; }

		status_t FindBlockRun(off_t pos,block_run &run,off_t &offset);
		status_t Append(Transaction *,off_t bytes);
		status_t SetFileSize(Transaction *,off_t bytes);

		Volume *GetVolume() const { return fVolume; }
		off_t ID() const { return 0; }
		int32 Mode() const { return fMode; }
		char *Name() const { return "whatever"; }
		block_run BlockRun() const { return block_run::Run(0,0,0); }
		block_run Parent() const { return block_run::Run(0,0,0); }
		off_t BlockNumber() const { return 0; }
		bfs_inode *Node() { return (bfs_inode *)1; }

		off_t Size() const { return fSize; }
		bool IsDirectory() const { return true; }

	private:
		Volume	*fVolume;
		BFile	fFile;
		off_t	fSize;
		ReadWriteLock	fLock;
		int32	fMode;
};

#endif	/* INODE_H */
