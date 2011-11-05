#ifndef VOLUME_H
#define VOLUME_H
/* Volume - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>

#include "bfs.h"


class BFile;


class Volume {
	public:
		Volume(BFile *file) : fFile(file) {}

		BFile *Device() { return fFile; }

		int32 BlockSize() const { return 1024; }
		off_t ToBlock(block_run run) const { return run.start; }
		block_run ToBlockRun(off_t block) const { return block_run::Run(0,0,block); }

		static void Panic();

	private:
		BFile	*fFile;
};


#endif	/* VOLUME_H */
