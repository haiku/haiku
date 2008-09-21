/* Stream - inode stream access functions
**
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef STREAM_H
#define STREAM_H

#include <sys/stat.h>

#include <boot/vfs.h>

#include "Volume.h"


class Node;


namespace BFS {

class Stream {
	public:
		Stream(Volume &volume, block_run run);
		Stream(Volume &volume, off_t id);
		Stream(const Stream& other);
		~Stream();

		status_t InitCheck();
		Volume &GetVolume() const { return fVolume; }

		status_t FindBlockRun(off_t pos, block_run &run, off_t &offset);
		status_t ReadAt(off_t pos, uint8 *buffer, size_t *length);

		status_t GetName(char *name, size_t size) const;
		off_t Size() const { return fInode->data.Size(); }
		off_t ID() const { return fVolume.ToVnode(fInode->inode_num); }
		inode_addr InodeNum() const { return fInode->inode_num; }
		int32 Mode() const { return fInode->Mode(); }
		status_t ReadLink(char *buffer, size_t bufferSize);

		bool IsContainer() const { return Mode() & (S_IFDIR | S_INDEX_DIR | S_ATTR_DIR); }
		bool IsSymlink() const { return S_ISLNK(Mode()); }

		static Node *NodeFactory(Volume &volume, ::Directory* parent, off_t id);

	private:
		Stream& operator=(const Stream& other);

		status_t GetNextSmallData(const small_data **_smallData) const;

		status_t _LoadInode(off_t offset);
		void _UseInode(bfs_inode* inode);

		Volume&		fVolume;
		bfs_inode*	fInode;
};

}	// namespace BFS

#endif	/* STREAM_H */
