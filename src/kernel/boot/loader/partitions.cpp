/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Partition.h"

#include <boot/partitions.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <ddm_modules.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>


/* supported partition modules */

static partition_module_info *sPartitionModules[] = {
#ifdef BOOT_SUPPORT_PARTITION_AMIGA
	&gAmigaPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_INTEL
	&gIntelPartitionMapModule,
	&gIntelExtendedPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_APPLE
	&gApplePartitionModule,
#endif
};
static const int32 sNumPartitionModules = sizeof(sPartitionModules) / sizeof(partition_module_info *);

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
};
static const int32 sNumFileSystemModules = sizeof(sFileSystemModules) / sizeof(file_system_module_info *);

extern list gPartitions;


Partition::Partition(int fd)
	:
	fParent(NULL),
	fIsFileSystem(false)
{
	memset(this, 0, sizeof(partition_data));
	id = (partition_id)this;

	// it's safe to close the file
	fFD = dup(fd);
	list_init_etc(&fChildren, Partition::NextOffset());
}


Partition::~Partition()
{
	close(fFD);
}


ssize_t 
Partition::ReadAt(void *cookie, off_t position, void *buffer, size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		position = 0;

	if (position + bufferSize > this->size)
		bufferSize = this->size - position;

	return read_pos(fFD, this->offset + position, buffer, bufferSize);
}


ssize_t 
Partition::WriteAt(void *cookie, off_t position, const void *buffer, size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		position = 0;

	if (position + bufferSize > this->size)
		bufferSize = this->size - position;

	return write_pos(fFD, this->offset + position, buffer, bufferSize);
}


Partition *
Partition::AddChild()
{
	Partition *child = new Partition(volume);
	if (child == NULL)
		return NULL;

	child->SetParent(this);
	child_count++;
	list_add_item(&fChildren, (void *)child);

	return child;
}


status_t 
Partition::Scan()
{
	// scan for partitions first (recursively all eventual children as well)

	for (int32 i = 0; i < sNumPartitionModules; i++) {
		partition_module_info *module = sPartitionModules[i];
		void *cookie;

		puts(module->pretty_name);
		if (module->identify_partition(fFD, this, &cookie) <= 0.0)
			continue;

		status_t status = module->scan_partition(fFD, this, cookie);
		module->free_identify_partition_cookie(this, cookie);

		if (status == B_OK) {
			// now that we've found something, check our children
			// out as well!

			Partition *child = NULL, *last = NULL;

			while ((child = (Partition *)list_get_next_item(&fChildren, child)) != NULL) {
				child->Scan();

				if (child->IsFileSystem()) {
					// move the file systems to the partition list
					list_remove_item(&fChildren, child);
					list_add_item(&gPartitions, child);

					child = last;
						// skip this item
				}

				last = child;
			}

			// remove all unused children (we keep only file systems)

			while ((child = (Partition *)list_remove_head_item(&fChildren)) != NULL)
				delete child;

			return B_OK;
		}
	}

	// scan for file systems

	for (int32 i = 0; i < sNumFileSystemModules; i++) {
		file_system_module_info *module = sFileSystemModules[i];

		puts(module->pretty_name);

		Directory *fileSystem;
		if (module->get_file_system(this, &fileSystem) == B_OK) {
			gRoot->AddNode(fileSystem);

			fIsFileSystem = true;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND; 
}


//	#pragma mark -


status_t
add_partitions_for(int fd)
{
	Partition partition(fd);

	// set some magic/default values
	partition.block_size = 512;
	partition.size = partition.Size();

	return partition.Scan();
}


partition_data *
create_child_partition(partition_id id, int32 index, partition_id childID)
{
	Partition &partition = *(Partition *)id;
	Partition *child = partition.AddChild();
	if (child == NULL) {
		printf("creating partition failed: no memory\n");
		return NULL;
	}

	// we cannot do anything with the child here, because it was not
	// yet initialized by the partition module.

	return child;
}


partition_data *
get_child_partition(partition_id id, int32 index)
{
	//Partition &partition = *(Partition *)id;

	// ToDo: do we really have to implement this?
	//	The intel partition module doesn't really needs this for our mission...

	return NULL;
}


partition_data *
get_parent_partition(partition_id id)
{
	Partition &partition = *(Partition *)id;

	return partition.Parent();
}

