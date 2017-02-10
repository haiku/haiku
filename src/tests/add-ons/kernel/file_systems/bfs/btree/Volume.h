#ifndef VOLUME_H
#define VOLUME_H
/* Volume - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include <SupportDefs.h>

#include "bfs.h"


class BFile;


class Volume {
	public:
		Volume(BFile *file) : fFile(file) {}

		bool IsInitializing() const { return false; }
		bool IsValidInodeBlock(off_t) const { return true; }

		BFile *Device() { return fFile; }
		void *BlockCache() { return fFile; }
		fs_volume *FSVolume() { return NULL; }

		uint32 BlockSize() const { return 1024; }
		uint32 BlockShift() const { return 10; /* 2^BlockShift == BlockSize */ }

		off_t ToBlock(block_run run) const { return run.start; }
		block_run ToBlockRun(off_t block) const
		{
			return block_run::Run(0, 0, block);
		}

		static void Panic();

		int32 GenerateTransactionID();

	private:
		BFile	*fFile;
};


#endif	/* VOLUME_H */
