/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/partitions.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <boot/FileMapDisk.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <boot/vfs.h>
#include <ddm_modules.h>

#include "RootFileSystem.h"


using namespace boot;

#define TRACE_PARTITIONS
#ifdef TRACE_PARTITIONS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/* supported partition modules */

static const partition_module_info *sPartitionModules[] = {
#ifdef BOOT_SUPPORT_PARTITION_AMIGA
	&gAmigaPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_EFI
	&gEFIPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_INTEL
	&gIntelPartitionMapModule,
	&gIntelExtendedPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_APPLE
	&gApplePartitionModule,
#endif
};
static const int32 sNumPartitionModules = sizeof(sPartitionModules)
	/ sizeof(partition_module_info *);

/* supported file system modules */

static file_system_module_info *sFileSystemModules[] = {
#ifdef BOOT_SUPPORT_FILE_SYSTEM_BFS
	&gBFSFileSystemModule,
#endif
#ifdef BOOT_SUPPORT_FILE_SYSTEM_AMIGA_FFS
	&gAmigaFFSFileSystemModule,
#endif
#ifdef BOOT_SUPPORT_FILE_SYSTEM_FAT
	&gFATFileSystemModule,
#endif
#ifdef BOOT_SUPPORT_FILE_SYSTEM_HFS_PLUS
	&gHFSPlusFileSystemModule,
#endif
#ifdef BOOT_SUPPORT_FILE_SYSTEM_TARFS
	&gTarFileSystemModule,
#endif
};
static const int32 sNumFileSystemModules = sizeof(sFileSystemModules)
	/ sizeof(file_system_module_info *);

extern NodeList gPartitions;


namespace boot {

/*! A convenience class to automatically close a
	file descriptor upon deconstruction.
*/
class NodeOpener {
public:
	NodeOpener(Node *node, int mode)
	{
		fFD = open_node(node, mode);
	}

	~NodeOpener()
	{
		close(fFD);
	}

	int Descriptor() const { return fFD; }

private:
	int		fFD;
};


static int32 sIdCounter = 0;


//	#pragma mark -


Partition::Partition(int fd)
	:
	fParent(NULL),
	fIsFileSystem(false),
	fIsPartitioningSystem(false)
{
	TRACE(("%p Partition::Partition\n", this));

	memset((partition_data *)this, 0, sizeof(partition_data));

	id = atomic_add(&sIdCounter, 1);

	// it's safe to close the file
	fFD = dup(fd);
}


Partition::~Partition()
{
	TRACE(("%p Partition::~Partition\n", this));

	// Tell the children that their parent is gone

	NodeIterator iterator = gPartitions.GetIterator();
	Partition *child;

	while ((child = (Partition *)iterator.Next()) != NULL) {
		if (child->Parent() == this)
			child->SetParent(NULL);
	}

	close(fFD);
}


Partition *
Partition::Lookup(partition_id id, NodeList *list)
{
	Partition *p;

	if (list == NULL)
		list = &gPartitions;

	NodeIterator iterator = list->GetIterator();

	while ((p = (Partition *)iterator.Next()) != NULL) {
		if (p->id == id)
			return p;
		if (!p->fChildren.IsEmpty()) {
			Partition *c = Lookup(id, &p->fChildren);
			if (c)
				return c;
		}
	}
	return NULL;
}


void
Partition::SetParent(Partition *parent)
{
	TRACE(("%p Partition::SetParent %p\n", this, parent));
	fParent = parent;
}


Partition *
Partition::Parent() const
{
	//TRACE(("%p Partition::Parent is %p\n", this, fParent));
	return fParent;
}


ssize_t
Partition::ReadAt(void *cookie, off_t position, void *buffer, size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		return B_BAD_VALUE;

	if (position + (off_t)bufferSize > this->size)
		bufferSize = this->size - position;

	ssize_t result = read_pos(fFD, this->offset + position, buffer, bufferSize);
	return result < 0 ? errno : result;
}


ssize_t
Partition::WriteAt(void *cookie, off_t position, const void *buffer,
	size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		return B_BAD_VALUE;

	if (position + (off_t)bufferSize > this->size)
		bufferSize = this->size - position;

	ssize_t result = write_pos(fFD, this->offset + position, buffer,
		bufferSize);
	return result < 0 ? errno : result;
}


off_t
Partition::Size() const
{
	struct stat stat;
	if (fstat(fFD, &stat) == B_OK)
		return stat.st_size;

	return Node::Size();
}


int32
Partition::Type() const
{
	struct stat stat;
	if (fstat(fFD, &stat) == B_OK)
		return stat.st_mode;

	return Node::Type();
}


Partition *
Partition::AddChild()
{
	Partition *child = new(nothrow) Partition(fFD);
	TRACE(("%p Partition::AddChild %p\n", this, child));
	if (child == NULL)
		return NULL;

	child->SetParent(this);
	child_count++;
	fChildren.Add(child);

	return child;
}


status_t
Partition::_Mount(file_system_module_info *module, Directory **_fileSystem)
{
	TRACE(("%p Partition::_Mount check for file_system: %s\n",
		this, module->pretty_name));

	Directory *fileSystem;
	if (module->get_file_system(this, &fileSystem) == B_OK) {
		gRoot->AddVolume(fileSystem, this);
		if (_fileSystem)
			*_fileSystem = fileSystem;

		// remember the module that mounted us
		fModuleName = module->module_name;
		this->content_type = module->pretty_name;

		fIsFileSystem = true;

#ifdef BOOT_SUPPORT_FILE_MAP_DISK
		static int fileMapDiskDepth = 0;
		// if we aren't already mounting an image
		if (!fileMapDiskDepth++) {
			// see if it contains an image file we could mount in turn
			FileMapDisk *disk = FileMapDisk::FindAnyFileMapDisk(fileSystem);
			if (disk) {
				TRACE(("%p Partition::_Mount: found FileMapDisk\n", this));
				disk->RegisterFileMapBootItem();
				add_partitions_for(disk, true, false);
			}
		}
		fileMapDiskDepth--;
#endif

		return B_OK;
	}

	return B_BAD_VALUE;
}


status_t
Partition::Mount(Directory **_fileSystem, bool isBootDevice)
{
	if (isBootDevice && gBootVolume.GetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE,
			false)) {
		return _Mount(&gTarFileSystemModule, _fileSystem);
	}

	for (int32 i = 0; i < sNumFileSystemModules; i++) {
		status_t status = _Mount(sFileSystemModules[i], _fileSystem);
		if (status == B_OK)
			return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
Partition::Scan(bool mountFileSystems, bool isBootDevice)
{
	// scan for partitions first (recursively all eventual children as well)

	TRACE(("%p Partition::Scan()\n", this));

	// if we were not booted from the real boot device, we won't scan
	// the device we were booted from (which is likely to be a slow
	// floppy or CD)
	if (isBootDevice && gBootVolume.GetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE,
			false)) {
		return B_ENTRY_NOT_FOUND;
	}

	const partition_module_info *bestModule = NULL;
	void *bestCookie = NULL;
	float bestPriority = -1;

	for (int32 i = 0; i < sNumPartitionModules; i++) {
		const partition_module_info *module = sPartitionModules[i];
		void *cookie = NULL;
		NodeOpener opener(this, O_RDONLY);

		TRACE(("check for partitioning_system: %s\n", module->pretty_name));

		float priority
			= module->identify_partition(opener.Descriptor(), this, &cookie);
		if (priority < 0.0)
			continue;

		TRACE(("  priority: %" B_PRId32 "\n", (int32)(priority * 1000)));
		if (priority <= bestPriority) {
			// the disk system recognized the partition worse than the currently
			// best one
			module->free_identify_partition_cookie(this, cookie);
			continue;
		}

		// a new winner, replace the previous one
		if (bestModule)
			bestModule->free_identify_partition_cookie(this, bestCookie);
		bestModule = module;
		bestCookie = cookie;
		bestPriority = priority;
	}

	// find the best FS module
	file_system_module_info *bestFSModule = NULL;
	float bestFSPriority = -1;
	for (int32 i = 0; i < sNumFileSystemModules; i++) {
		if (sFileSystemModules[i]->identify_file_system == NULL)
			continue;

		float priority = sFileSystemModules[i]->identify_file_system(this);
		if (priority <= 0)
			continue;

		if (priority > bestFSPriority) {
			bestFSModule = sFileSystemModules[i];
			bestFSPriority = priority;
		}
	}

	// now let the best matching disk system scan the partition
	if (bestModule && bestPriority >= bestFSPriority) {
		NodeOpener opener(this, O_RDONLY);
		status_t status = bestModule->scan_partition(opener.Descriptor(), this,
			bestCookie);
		bestModule->free_identify_partition_cookie(this, bestCookie);

		if (status != B_OK) {
			dprintf("Partitioning module `%s' recognized the partition, but "
				"failed to scan it\n", bestModule->pretty_name);
			return status;
		}

		fIsPartitioningSystem = true;

		content_type = bestModule->pretty_name;
		flags |= B_PARTITION_PARTITIONING_SYSTEM;

		// now that we've found something, check our children
		// out as well!

		NodeIterator iterator = fChildren.GetIterator();
		Partition *child = NULL;

		while ((child = (Partition *)iterator.Next()) != NULL) {
			TRACE(("%p Partition::Scan(): scan child %p (start = %" B_PRIdOFF
				", size = %" B_PRIdOFF ", parent = %p)!\n", this, child,
				child->offset, child->size, child->Parent()));

			child->Scan(mountFileSystems);

			if (!mountFileSystems || child->IsFileSystem()) {
				// move the partitions containing file systems to the partition
				// list
				fChildren.Remove(child);
				gPartitions.Add(child);
			}
		}

		// remove all unused children (we keep only file systems)

		while ((child = (Partition *)fChildren.Head()) != NULL) {
			fChildren.Remove(child);
			delete child;
		}

		// remember the name of the module that identified us
		fModuleName = bestModule->module.name;

		return B_OK;
	}

	// scan for file systems
	if (mountFileSystems && bestFSModule != NULL)
		return _Mount(bestFSModule, NULL);

	return B_ENTRY_NOT_FOUND;
}

}	// namespace boot


//	#pragma mark -


/*!	Scans the device passed in for partitioning systems. If none are found,
	a partition containing the whole device is created.
	All created partitions are added to the gPartitions list.
*/
status_t
add_partitions_for(int fd, bool mountFileSystems, bool isBootDevice)
{
	TRACE(("add_partitions_for(fd = %d, mountFS = %s)\n", fd,
		mountFileSystems ? "yes" : "no"));

	Partition *partition = new(nothrow) Partition(fd);

	// set some magic/default values
	partition->block_size = 512;
	partition->size = partition->Size();

	// add this partition to the list of partitions
	// temporarily for Lookup() to work
	gPartitions.Add(partition);

	// keep it, if it contains or might contain a file system
	if ((partition->Scan(mountFileSystems, isBootDevice) == B_OK
			&& partition->IsFileSystem())
		|| (!partition->IsPartitioningSystem() && !mountFileSystems)) {
		return B_OK;
	}

	// if not, we no longer need the partition
	gPartitions.Remove(partition);
	delete partition;
	return B_OK;
}


status_t
add_partitions_for(Node *device, bool mountFileSystems, bool isBootDevice)
{
	TRACE(("add_partitions_for(%p, mountFS = %s)\n", device,
		mountFileSystems ? "yes" : "no"));

	int fd = open_node(device, O_RDONLY);
	if (fd < B_OK)
		return fd;

	status_t status = add_partitions_for(fd, mountFileSystems, isBootDevice);
	if (status < B_OK)
		dprintf("add_partitions_for(%d) failed: %" B_PRIx32 "\n", fd, status);

	close(fd);
	return B_OK;
}


partition_data *
create_child_partition(partition_id id, int32 index, off_t offset, off_t size,
	partition_id childID)
{
	Partition *partition = Partition::Lookup(id);
	if (partition == NULL) {
		dprintf("creating partition failed: could not find partition.\n");
		return NULL;
	}

	Partition *child = partition->AddChild();
	if (child == NULL) {
		dprintf("creating partition failed: no memory\n");
		return NULL;
	}

	child->offset = offset;
	child->size = size;

	// we cannot do anything with the child here, because it was not
	// yet initialized by the partition module.
	TRACE(("new child partition!\n"));

	return child;
}


partition_data *
get_child_partition(partition_id id, int32 index)
{
	// TODO: do we really have to implement this?
	//	The intel partition module doesn't really need this for our mission...
	TRACE(("get_child_partition(id = %" B_PRId32 ", index = %" B_PRId32 ")\n",
		id, index));

	return NULL;
}


partition_data *
get_parent_partition(partition_id id)
{
	Partition *partition = Partition::Lookup(id);
	if (partition == NULL) {
		dprintf("could not find parent partition.\n");
		return NULL;
	}
	return partition->Parent();
}

