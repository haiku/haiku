/* Stream - inode stream access functions
**
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef STREAM_H
#define STREAM_H


#include "Volume.h"

namespace BFS {

class Stream : public bfs_inode {
	public:
		Stream(Volume &volume, block_run run);
		~Stream();

		status_t InitCheck();
		Volume &GetVolume() { return fVolume; }

		status_t FindBlockRun(off_t pos, block_run &run, off_t &offset);
		status_t ReadAt(off_t pos, uint8 *buffer, size_t *length);

	private:
		Volume	&fVolume;
};

}	// namespace BFS

#endif	/* STREAM_H */
