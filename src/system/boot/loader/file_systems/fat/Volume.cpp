/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Volume.h"
#include "Directory.h"
#include "CachedBlock.h"

#include <boot/partitions.h>
#include <boot/platform.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

//#define TRACE(x) dprintf x
#define TRACE(x) do {} while (0)

using namespace FATFS;


Volume::Volume(boot::Partition *partition)
	:
	fCachedBlock(NULL),
	fRoot(NULL)
{
	TRACE(("%s()\n", __FUNCTION__));
	if ((fDevice = open_node(partition, O_RDONLY)) < B_OK)
		return;

	fCachedBlock = new CachedBlock(*this);
	if (fCachedBlock == NULL)
		return;

	uint8 *buf;
	/* = (char *)malloc(4096);
	if (buf == NULL)
		return;*/

	fBlockSize = partition->block_size;
	switch (fBlockSize) {
	case 0x200:
		fBlockShift = 9;
		break;
	case 0x400:
		fBlockShift = 10;
		break;
	case 0x800:
		fBlockShift = 11;
		break;
	default:
		goto err1;
	}
	TRACE(("%s: reading bootsector\n", __FUNCTION__));
	// read boot sector
	buf = fCachedBlock->SetTo(0);
	if (buf == NULL)
		goto err1;

	TRACE(("%s: checking signature\n", __FUNCTION__));
	// check the signature
	if (((buf[0x1fe] != 0x55) || (buf[0x1ff] != 0xaa)) && (buf[0x15] == 0xf8))
		goto err1;

	if (!memcmp(buf+3, "NTFS    ", 8) || !memcmp(buf+3, "HPFS    ", 8))
		goto err1;

	TRACE(("%s: signature ok\n", __FUNCTION__));
	fBytesPerSector = read16(buf,0xb);
	switch (fBytesPerSector) {
	case 0x200:
		fSectorShift = 9;
		break;
	case 0x400:
		fSectorShift = 10;
		break;
	case 0x800:
		fSectorShift = 11;
		break;
	default:
		goto err1;
	}
	TRACE(("%s: block shift %d\n", __FUNCTION__, fBlockShift));

	fSectorsPerCluster = buf[0xd];
	switch (fSectorsPerCluster) {
	case 1:	case 2:	case 4:	case 8:
	case 0x10:	case 0x20:	case 0x40:	case 0x80:
		break;
	default:
		goto err1;
	}
	TRACE(("%s: sect/cluster %d\n", __FUNCTION__, fSectorsPerCluster));
	fClusterShift = fSectorShift;
	for (uint32 spc = fSectorsPerCluster; !(spc & 0x01); ) {
		spc >>= 1;
		fClusterShift += 1;
	}
	TRACE(("%s: cluster shift %d\n", __FUNCTION__, fClusterShift));

	fReservedSectors = read16(buf,0xe);
	fFatCount = buf[0x10];
	if ((fFatCount == 0) || (fFatCount > 8))
		goto err1;

	fMediaDesc = buf[0x15];
	if ((fMediaDesc != 0xf0) && (fMediaDesc < 0xf8))
		goto err1;

	fSectorsPerFat = read16(buf,0x16);
	if (fSectorsPerFat == 0) {
		// FAT32
		fFatBits = 32;
		fSectorsPerFat = read32(buf,0x24);
		fTotalSectors = read32(buf,0x20);
		bool lFatMirrored = !(buf[0x28] & 0x80);
		fActiveFat = (lFatMirrored) ? (buf[0x28] & 0xf) : 0;
		fDataStart = fReservedSectors + fFatCount * fSectorsPerFat;
		fTotalClusters = (fTotalSectors - fDataStart) / fSectorsPerCluster;
		fRootDirCluster = read32(buf,0x2c);
		if (fRootDirCluster >= fTotalClusters)
			goto err1;

		fFSInfoSector = read16(buf, 0x30);
		if (fFSInfoSector < 1 || fFSInfoSector > fTotalSectors)
			goto err1;
	} else {
		// FAT12/16
		// XXX:FIXME
		fFatBits = 16;
		goto err1;
	}
	TRACE(("%s: block size %d, sector size %d, sectors/cluster %d\n", __FUNCTION__,
		fBlockSize, fBytesPerSector, fSectorsPerCluster));
	TRACE(("%s: block shift %d, sector shift %d, cluster shift %d\n", __FUNCTION__,
		fBlockShift, fSectorShift, fClusterShift));
	TRACE(("%s: reserved %d, max root entries %d, media %02x, sectors/fat %d\n", __FUNCTION__,
		fReservedSectors, fMaxRootEntries, fMediaDesc, fSectorsPerFat));


	//if (fTotalSectors > partition->sectors_per_track * partition->cylinder_count * partition->head_count)
	if ((off_t)fTotalSectors * fBytesPerSector > partition->size)
		goto err1;

	TRACE(("%s: found fat%d filesystem, root dir at cluster %d\n", __FUNCTION__,
		fFatBits, fRootDirCluster));

	fRoot = new Directory(*this, 0, fRootDirCluster, "/");
	return;

err1:
	TRACE("fatfs: cannot mount (bad superblock ?)\n");
	// XXX !? this triple-faults in QEMU ..
	//delete fCachedBlock;
}


Volume::~Volume()
{
	delete fRoot;
	delete fCachedBlock;
	close(fDevice);
}


status_t
Volume::InitCheck()
{
	if (fCachedBlock == NULL)
		return B_ERROR;
	if (fRoot == NULL)
		return B_ERROR;

	return B_OK;
}


status_t
Volume::GetName(char *name, size_t size) const
{
	//TODO: WRITEME
	return strlcpy(name, "UNKNOWN", size);
}


off_t
Volume::ClusterToOffset(uint32 cluster) const
{
	return (fDataStart << SectorShift()) + ((cluster - 2) << ClusterShift());
}

uint32
Volume::NextCluster(uint32 cluster, uint32 skip)
{
	//TRACE(("%s(%d, %d)\n", __FUNCTION__, cluster, skip));
	// lookup the FAT for next cluster in chain
	off_t offset;
	uint8 *buf;
	int32 next;
	int fatBytes = (FatBits() + 7) / 8;

	switch (fFatBits) {
		case 32:
		case 16:
			break;
		//XXX handle FAT12
		default:
			return InvalidClusterID();
	}

again:
	offset = fBytesPerSector * fReservedSectors;
	//offset += fActiveFat * fTotalClusters * fFatBits / 8;
	offset += cluster * fatBytes;

	if (!IsValidCluster(cluster))
		return InvalidClusterID();

	buf = fCachedBlock->SetTo(ToBlock(offset));
	if (!buf)
		return InvalidClusterID();

	offset %= BlockSize();

	switch (fFatBits) {
		case 32:
			next = read32(buf, offset);
			next &= 0x0fffffff;
			break;
		case 16:
			next = read16(buf, offset);
			break;
		default:
			return InvalidClusterID();
	}
	if (skip--) {
		cluster = next;
		goto again;
	}
	return next;
}


bool
Volume::IsValidCluster(uint32 cluster) const
{
	if (cluster > 1 && cluster < fTotalClusters)
		return true;
	return false;
}


bool
Volume::IsLastCluster(uint32 cluster) const
{
	if (cluster >= fTotalClusters && ((cluster & 0xff8) == 0xff8))
		return true;
	return false;
}


/*!	Allocates a free cluster.
	If \a previousCluster is a valid cluster idnex, its chain pointer is
	changed to point to the newly allocated cluster.
*/
status_t
Volume::AllocateCluster(uint32 previousCluster, uint32& _newCluster)
{
	if (fFatBits != 32)
		return B_UNSUPPORTED;
		// TODO: Support FAT16 and FAT12.

	const int fatBytes = FatBits() / 8;
	const uint32 blockOffsetMask = (uint32)BlockSize() - 1;

	// Iterate through the FAT to find a free cluster.
	off_t offset = fBytesPerSector * fReservedSectors;
	offset += 2 * fatBytes;
	for (uint32 i = 2; i < fTotalClusters; i++, offset += fatBytes) {
		uint8* buffer = fCachedBlock->SetTo(ToBlock(offset));
		if (buffer == NULL)
			return B_ERROR;

		uint32 value = read32(buffer, offset & blockOffsetMask);
		if (value == 0) {
			// found one -- mark it used (end of file)
			status_t error = _UpdateCluster(i, 0x0ffffff8);
			if (error != B_OK)
				return error;

			// If a previous cluster was given, update its list link.
			if (IsValidCluster(previousCluster)) {
				error = _UpdateCluster(previousCluster, i);
				if (error != B_OK) {
					_UpdateCluster(i, 0);
					return error;
				}
			}

			_ClusterAllocated(i);

			_newCluster = i;
			return B_OK;
		}
	}

	return B_DEVICE_FULL;
}


status_t
Volume::_UpdateCluster(uint32 cluster, uint32 value)
{
	if (fFatBits != 32 && fFatBits != 16)
		return B_UNSUPPORTED;
		// TODO: Support FAT12.

	if (!IsValidCluster(cluster))
		return InvalidClusterID();

	// get the buffer we need to change
	const int fatBytes = FatBits() / 8;

	for (uint8 i = 0; i < fFatCount; i++) {
		off_t offset
			= fBytesPerSector * (fReservedSectors + i * fSectorsPerFat);
		offset += cluster * fatBytes;

		uint8* buffer = fCachedBlock->SetTo(ToBlock(offset));
		if (buffer == NULL)
			return InvalidClusterID();

		offset %= BlockSize();

		// set the value
		switch (fFatBits) {
			case 32:
				*(uint32*)(buffer + offset) = B_HOST_TO_LENDIAN_INT32(value);
				break;
			case 16:
				*(uint16*)(buffer + offset) = B_HOST_TO_LENDIAN_INT16(value);
				break;
			default:
				return InvalidClusterID();
		}

		// write the block back to disk
		status_t error = fCachedBlock->Flush();
		if (error != B_OK) {
			fCachedBlock->Unset();
			return error;
		}
	}

	return B_OK;
}


status_t
Volume::_ClusterAllocated(uint32 cluster)
{
	// update the FS info

	// exists only for FAT32
	if (fFatBits != 32)
		return B_OK;

	off_t offset = fBytesPerSector * fFSInfoSector;

	status_t error = fCachedBlock->SetTo(offset / BlockSize(),
		CachedBlock::READ);
	if (error != B_OK)
		return error;

	uint8* buffer = fCachedBlock->Block() + offset % BlockSize();

	// update number of free cluster
	int32 freeClusters = read32(buffer, 0x1e8);
	if (freeClusters != -1)
		write32(buffer, 0x1e8, freeClusters - 1);

	// update number of most recently allocated cluster
	write32(buffer, 0x1ec, cluster);

	// write the block back
	error = fCachedBlock->Flush();
	if (error != B_OK)
		fCachedBlock->Unset();

	return error;
}


//	#pragma mark -


float
dosfs_identify_file_system(boot::Partition *partition)
{
	TRACE(("%s()\n", __FUNCTION__));
	Volume volume(partition);

	return volume.InitCheck() < B_OK ? 0 : 0.8;
}


static status_t
dosfs_get_file_system(boot::Partition *partition, ::Directory **_root)
{
	TRACE(("%s()\n", __FUNCTION__));
	Volume *volume = new Volume(partition);
	if (volume == NULL)
		return B_NO_MEMORY;

	if (volume->InitCheck() < B_OK) {
		delete volume;
		return B_ERROR;
	}

	*_root = volume->Root();
	return B_OK;
}


file_system_module_info gFATFileSystemModule = {
	"file_systems/dosfs/v1",
	kPartitionTypeFAT32, // XXX:FIXME: FAT16 too ?
	dosfs_identify_file_system,
	dosfs_get_file_system
};

