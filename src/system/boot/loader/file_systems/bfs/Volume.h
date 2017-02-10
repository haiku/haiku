/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef VOLUME_H
#define VOLUME_H


#include <SupportDefs.h>

namespace boot {
	class Partition;
}


#include "bfs.h"

namespace BFS {

class Directory;

class Volume {
	public:
		Volume(boot::Partition *partition);
		~Volume();

		status_t			InitCheck();
		bool				IsValidSuperBlock();

		block_run			Root() const { return fSuperBlock.root_dir; }
		Directory			*RootNode() const { return fRootNode; }
		block_run			Indices() const { return fSuperBlock.indices; }
		const char			*Name() const { return fSuperBlock.name; }

		int					Device() const { return fDevice; }

		off_t				NumBlocks() const { return fSuperBlock.NumBlocks(); }
		off_t				UsedBlocks() const { return fSuperBlock.UsedBlocks(); }
		off_t				FreeBlocks() const { return NumBlocks() - UsedBlocks(); }

		uint32				BlockSize() const { return fSuperBlock.BlockSize(); }
		uint32				BlockShift() const { return fSuperBlock.BlockShift(); }
		uint32				InodeSize() const { return fSuperBlock.InodeSize(); }
		uint32				AllocationGroups() const { return fSuperBlock.AllocationGroups(); }
		uint32				AllocationGroupShift() const { return fSuperBlock.AllocationGroupShift(); }
		disk_super_block	&SuperBlock() { return fSuperBlock; }

		off_t				ToOffset(block_run run) const { return ToBlock(run) << BlockShift(); }
		off_t				ToOffset(off_t block) const { return block << BlockShift(); }
		off_t				ToBlock(block_run run) const { return ((off_t)run.AllocationGroup() << AllocationGroupShift()) | (uint32)run.Start(); }
		block_run			ToBlockRun(off_t block) const;
		status_t			ValidateBlockRun(block_run run);

		off_t				ToVnode(block_run run) const { return ToBlock(run); }
		off_t				ToVnode(off_t block) const { return block; }

	protected:
		int					fDevice;
		disk_super_block	fSuperBlock;

		BFS::Directory		*fRootNode;
};

}	// namespace BFS

#endif	/* VOLUME_H */
