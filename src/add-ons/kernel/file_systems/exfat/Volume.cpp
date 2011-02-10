/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//! Super block, mounting, etc.


#include "Volume.h"

#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs_cache.h>
#include <fs_volume.h>

#include <util/AutoLock.h>

#include "CachedBlock.h"
#include "encodings.h"
#include "Inode.h"


//#define TRACE_EXFAT
#ifdef TRACE_EXFAT
#	define TRACE(x...) dprintf("\33[34mexfat:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)


class DeviceOpener {
public:
								DeviceOpener(int fd, int mode);
								DeviceOpener(const char* device, int mode);
								~DeviceOpener();

			int					Open(const char* device, int mode);
			int					Open(int fd, int mode);
			void*				InitCache(off_t numBlocks, uint32 blockSize);
			void				RemoveCache(bool allowWrites);

			void				Keep();

			int					Device() const { return fDevice; }
			int					Mode() const { return fMode; }
			bool				IsReadOnly() const
									{ return _IsReadOnly(fMode); }

			status_t			GetSize(off_t* _size,
									uint32* _blockSize = NULL);

private:
	static	bool				_IsReadOnly(int mode)
									{ return (mode & O_RWMASK) == O_RDONLY;}
	static	bool				_IsReadWrite(int mode)
									{ return (mode & O_RWMASK) == O_RDWR;}

			int					fDevice;
			int					fMode;
			void*				fBlockCache;
};


DeviceOpener::DeviceOpener(const char* device, int mode)
	:
	fBlockCache(NULL)
{
	Open(device, mode);
}


DeviceOpener::DeviceOpener(int fd, int mode)
	:
	fBlockCache(NULL)
{
	Open(fd, mode);
}


DeviceOpener::~DeviceOpener()
{
	if (fDevice >= 0) {
		RemoveCache(false);
		close(fDevice);
	}
}


int
DeviceOpener::Open(const char* device, int mode)
{
	fDevice = open(device, mode | O_NOCACHE);
	if (fDevice < 0)
		fDevice = errno;

	if (fDevice < 0 && _IsReadWrite(mode)) {
		// try again to open read-only (don't rely on a specific error code)
		return Open(device, O_RDONLY | O_NOCACHE);
	}

	if (fDevice >= 0) {
		// opening succeeded
		fMode = mode;
		if (_IsReadWrite(mode)) {
			// check out if the device really allows for read/write access
			device_geometry geometry;
			if (!ioctl(fDevice, B_GET_GEOMETRY, &geometry)) {
				if (geometry.read_only) {
					// reopen device read-only
					close(fDevice);
					return Open(device, O_RDONLY | O_NOCACHE);
				}
			}
		}
	}

	return fDevice;
}


int
DeviceOpener::Open(int fd, int mode)
{
	fDevice = dup(fd);
	if (fDevice < 0)
		return errno;

	fMode = mode;

	return fDevice;
}


void*
DeviceOpener::InitCache(off_t numBlocks, uint32 blockSize)
{
	return fBlockCache = block_cache_create(fDevice, numBlocks, blockSize,
		IsReadOnly());
}


void
DeviceOpener::RemoveCache(bool allowWrites)
{
	if (fBlockCache == NULL)
		return;

	block_cache_delete(fBlockCache, allowWrites);
	fBlockCache = NULL;
}


void
DeviceOpener::Keep()
{
	fDevice = -1;
}


/*!	Returns the size of the device in bytes. It uses B_GET_GEOMETRY
	to compute the size, or fstat() if that failed.
*/
status_t
DeviceOpener::GetSize(off_t* _size, uint32* _blockSize)
{
	device_geometry geometry;
	if (ioctl(fDevice, B_GET_GEOMETRY, &geometry) < 0) {
		// maybe it's just a file
		struct stat stat;
		if (fstat(fDevice, &stat) < 0)
			return B_ERROR;

		if (_size)
			*_size = stat.st_size;
		if (_blockSize)	// that shouldn't cause us any problems
			*_blockSize = 512;

		return B_OK;
	}

	if (_size) {
		*_size = 1ULL * geometry.head_count * geometry.cylinder_count
			* geometry.sectors_per_track * geometry.bytes_per_sector;
	}
	if (_blockSize)
		*_blockSize = geometry.bytes_per_sector;

	return B_OK;
}


//	#pragma mark -


class LabelVisitor : public EntryVisitor {
public:
								LabelVisitor(Volume* volume);
			bool				VisitLabel(struct exfat_entry*);
private:
			Volume*				fVolume;
};


LabelVisitor::LabelVisitor(Volume* volume)
	:
	fVolume(volume)
{
}


bool
LabelVisitor::VisitLabel(struct exfat_entry* entry)
{
	dprintf("LabelVisitor::VisitLabel()\n");
	char utfName[30];
	size_t utfLength = 30;
	unicode_to_utf8((const uchar*)entry->name_label.name,
		entry->name_label.length * 2, (uint8*)utfName, &utfLength);
	fVolume->SetName(utfName);
	return true;
}


//	#pragma mark -


bool
exfat_super_block::IsValid()
{
	// TODO: check some more values!
	if (strncmp(filesystem, EXFAT_SUPER_BLOCK_MAGIC, sizeof(filesystem)) != 0)
		return false;
	if (signature != 0xaa55)
		return false;
	if (jump_boot[0] != 0xeb || jump_boot[1] != 0x76 || jump_boot[2] != 0x90)
		return false;
	if (version_minor != 0 || version_major != 1)
		return false;
	
	return true;
}


//	#pragma mark -


Volume::Volume(fs_volume* volume)
	:
	fFSVolume(volume),
	fFlags(0),
	fRootNode(NULL),
	fNextId(1)
{
	mutex_init(&fLock, "exfat volume");
	fInodesClusterTree = new InodesClusterTree;
	fInodesInoTree = new InodesInoTree;
	memset(fName, 0, sizeof(fName));
}


Volume::~Volume()
{
	TRACE("Volume destructor.\n");
	delete fInodesClusterTree;
	delete fInodesInoTree;
}


bool
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}


const char*
Volume::Name() const
{
	return fName;
}


status_t
Volume::Mount(const char* deviceName, uint32 flags)
{
	flags |= B_MOUNT_READ_ONLY;
		// we only support read-only for now
	
	if ((flags & B_MOUNT_READ_ONLY) != 0) {
		TRACE("Volume::Mount(): Read only\n");
	} else {
		TRACE("Volume::Mount(): Read write\n");
	}

	DeviceOpener opener(deviceName, (flags & B_MOUNT_READ_ONLY) != 0
		? O_RDONLY : O_RDWR);
	fDevice = opener.Device();
	if (fDevice < B_OK) {
		ERROR("Volume::Mount(): couldn't open device\n");
		return fDevice;
	}

	if (opener.IsReadOnly())
		fFlags |= VOLUME_READ_ONLY;

	// read the super block
	status_t status = Identify(fDevice, &fSuperBlock);
	if (status != B_OK) {
		ERROR("Volume::Mount(): Identify() failed\n");
		return status;
	}
	
	fBlockSize = 1 << fSuperBlock.BlockShift();
	TRACE("block size %ld\n", fBlockSize);
	fEntriesPerBlock = (fBlockSize / sizeof(struct exfat_entry));
		
	// check if the device size is large enough to hold the file system
	off_t diskSize;
	status = opener.GetSize(&diskSize);
	if (status != B_OK)
		return status;
	if (diskSize < (off_t)fSuperBlock.NumBlocks() << fSuperBlock.BlockShift())
		return B_BAD_VALUE;

	fBlockCache = opener.InitCache(fSuperBlock.NumBlocks(), fBlockSize);
	if (fBlockCache == NULL)
		return B_ERROR;
	
	TRACE("Volume::Mount(): Initialized block cache: %p\n", fBlockCache);

	ino_t rootIno;
	// ready
	{
		Inode rootNode(this, fSuperBlock.RootDirCluster(), 0);
		rootIno = rootNode.ID();
	}

	status = get_vnode(fFSVolume, rootIno, (void**)&fRootNode);
	if (status != B_OK) {
		ERROR("could not create root node: get_vnode() failed!\n");
		return status;
	}

	TRACE("Volume::Mount(): Found root node: %lld (%s)\n", fRootNode->ID(),
		strerror(fRootNode->InitCheck()));

	// all went fine
	opener.Keep();

	DirectoryIterator iterator(fRootNode);
	LabelVisitor visitor(this);
	iterator.Iterate(visitor);

	if (fName[0] == '\0') {
		// generate a more or less descriptive volume name
		off_t divisor = 1ULL << 40;
		char unit = 'T';
		if (diskSize < divisor) {
			divisor = 1UL << 30;
			unit = 'G';
			if (diskSize < divisor) {
				divisor = 1UL << 20;
				unit = 'M';
			}
		}

		double size = double((10 * diskSize + divisor - 1) / divisor);
			// %g in the kernel does not support precision...

		snprintf(fName, sizeof(fName), "%g %cB ExFAT Volume",
			size / 10, unit);
	}

	return B_OK;
}


status_t
Volume::Unmount()
{
	TRACE("Volume::Unmount()\n");

	TRACE("Volume::Unmount(): Putting root node\n");
	put_vnode(fFSVolume, RootNode()->ID());
	TRACE("Volume::Unmount(): Deleting the block cache\n");
	block_cache_delete(fBlockCache, !IsReadOnly());
	TRACE("Volume::Unmount(): Closing device\n");
	close(fDevice);

	TRACE("Volume::Unmount(): Done\n");
	return B_OK;
}


status_t
Volume::LoadSuperBlock()
{
	CachedBlock cached(this);
	const uint8* block = cached.SetTo(EXFAT_SUPER_BLOCK_OFFSET / fBlockSize);

	if (block == NULL)
		return B_IO_ERROR;

	memcpy(&fSuperBlock, block + EXFAT_SUPER_BLOCK_OFFSET % fBlockSize,
		sizeof(fSuperBlock));

	return B_OK;
}


status_t
Volume::ClusterToBlock(cluster_t cluster, fsblock_t &block)
{
	block = ((cluster - 2) << SuperBlock().BlocksPerClusterShift())
		+ SuperBlock().FirstDataBlock();
	TRACE("Volume::ClusterToBlock() cluster %lu %u %lu: %llu, %lu\n", cluster, 
		SuperBlock().BlocksPerClusterShift(), SuperBlock().FirstDataBlock(),
		block, SuperBlock().FirstFatBlock());
	return B_OK;
}


cluster_t
Volume::NextCluster(cluster_t _cluster)
{
	uint32 clusterPerBlock = fBlockSize / sizeof(cluster_t);
	CachedBlock block(this);
	fsblock_t blockNum = SuperBlock().FirstFatBlock()
		+ _cluster / clusterPerBlock;
	cluster_t *cluster = (cluster_t *)block.SetTo(blockNum);
	cluster += _cluster % clusterPerBlock;
	TRACE("Volume::NextCluster() cluster %lu next %lu\n", _cluster, *cluster);
	return *cluster;
}


Inode*
Volume::FindInode(ino_t id)
{
	return fInodesInoTree->Lookup(id);
}


Inode*
Volume::FindInode(cluster_t cluster)
{
	return fInodesClusterTree->Lookup(cluster);
}


ino_t
Volume::GetIno(cluster_t cluster, uint32 offset, ino_t parent)
{
	struct node_key key;
	key.cluster = cluster;
	key.offset = offset;
	struct node* node = fNodeTree.Lookup(key);
	if (node != NULL) {
		TRACE("Volume::GetIno() cached cluster %lu offset %lu ino %" B_PRIdINO
			"\n", cluster, offset, node->ino);
		return node->ino;
	}
	node = new struct node();
	node->key = key;
	node->ino = _NextID();
	node->parent = parent;
	fNodeTree.Insert(node);
	fInoTree.Insert(node);
	TRACE("Volume::GetIno() new cluster %lu offset %lu ino %" B_PRIdINO "\n",
		cluster, offset, node->ino);
	return node->ino;
}


struct node_key*
Volume::GetNode(ino_t ino, ino_t &parent)
{
	struct node* node = fInoTree.Lookup(ino);
	if (node != NULL) {
		parent = node->parent;
		return &node->key;
	}
	return NULL;
}


//	#pragma mark - Disk scanning and initialization


/*static*/ status_t
Volume::Identify(int fd, exfat_super_block* superBlock)
{
	if (read_pos(fd, EXFAT_SUPER_BLOCK_OFFSET, superBlock,
			sizeof(exfat_super_block)) != sizeof(exfat_super_block))
		return B_IO_ERROR;

	if (!superBlock->IsValid()) {
		ERROR("invalid super block!\n");
		return B_BAD_VALUE;
	}

	return B_OK;
}

