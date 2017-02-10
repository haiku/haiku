/* Stream - inode stream access functions
**
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef STREAM_H
#define STREAM_H


#include "Volume.h"

#include <sys/stat.h>

class Node;


namespace BFS {

class Stream : public bfs_inode {
	public:
		Stream(Volume &volume, block_run run);
		Stream(Volume &volume, off_t id);
		~Stream();

		status_t InitCheck();
		Volume &GetVolume() const { return fVolume; }

		status_t FindBlockRun(off_t pos, block_run &run, off_t &offset);
		status_t ReadAt(off_t pos, uint8 *buffer, size_t *length);

		status_t GetName(char *name, size_t size) const;
		off_t Size() const { return data.Size(); }
		off_t ID() const { return fVolume.ToVnode(inode_num); }
		status_t ReadLink(char *buffer, size_t bufferSize);

		bool IsContainer() const { return Mode() & (S_IFDIR | S_INDEX_DIR | S_ATTR_DIR); }
		bool IsSymlink() const { return S_ISLNK(Mode()); }
		bool IsIndex() const { return false; }

		static Node *NodeFactory(Volume &volume, off_t id);

	private:
		status_t GetNextSmallData(const small_data **_smallData) const;

		Volume	&fVolume;
};

}	// namespace BFS

#endif	/* STREAM_H */
