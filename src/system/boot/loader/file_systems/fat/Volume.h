/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef VOLUME_H
#define VOLUME_H


#include "fatfs.h"

#include <SupportDefs.h>

namespace boot {
	class Partition;
}


namespace FATFS {

class CachedBlock;
class Directory;

class Volume {
	public:
		Volume(boot::Partition *partition);
		~Volume();

		status_t			InitCheck();
		status_t			GetName(char *name, size_t size) const;

		int					Device() const { return fDevice; }
		Directory			*Root() { return fRoot; }
		int32				FatBits() const { return fFatBits; }
		uint32				DataStart() const { return fDataStart; }

		int32				BlockSize() const { return fBlockSize; }
		int32				ClusterSize() const { return fSectorsPerCluster * fBytesPerSector; }

		int32				BlockShift() const { return fBlockShift; }
		int32				SectorShift() const { return fSectorShift; }
		int32				ClusterShift() const { return fClusterShift; }

		int32				NumBlocks() const { return (int32)((off_t)fTotalSectors * fBytesPerSector / fBlockSize); }
		int32				NumSectors() const { return fTotalSectors; }
		int32				NumClusters() const { return fTotalClusters; }

		uint32				NextCluster(uint32 cluster, uint32 skip=0);
		bool				IsValidCluster(uint32 cluster) const;
		bool				IsLastCluster(uint32 cluster) const;
		uint32				InvalidClusterID() const { return (1 << fFatBits) - 1; }

		status_t			AllocateCluster(uint32 previousCluster,
								uint32& _newCluster);

		off_t				ClusterToOffset(uint32 cluster) const;
//		uint32				ToCluster(off_t offset) const { return offset >> ClusterShift(); }
		off_t				BlockToOffset(off_t block) const
								{ return block << BlockShift(); }
		uint32				ToBlock(off_t offset) const { return offset >> BlockShift(); }

	private:
		status_t			_UpdateCluster(uint32 cluster, uint32 value);
		status_t			_ClusterAllocated(uint32 cluster);

	protected:
		int					fDevice;
		int32				fBlockShift;
		int32				fSectorShift;
		int32				fClusterShift;
		uint32				fBlockSize;
		// from the boot/fsinfo sectors
		uint32				fBytesPerSector;
		uint32				fSectorsPerCluster;
		uint32				fReservedSectors;
		uint8				fMediaDesc;
		uint32				fSectorsPerFat;
		uint32				fTotalSectors;
		uint8				fFatCount;
		uint16				fMaxRootEntries;
		uint8				fActiveFat;
		uint8				fFatBits;
		uint32				fDataStart;
		uint32				fTotalClusters;
		uint32				fRootDirCluster;
		uint16				fFSInfoSector;

		CachedBlock			*fCachedBlock;
		Directory			*fRoot;
};

}	// namespace FATFS

#endif	/* VOLUME_H */
