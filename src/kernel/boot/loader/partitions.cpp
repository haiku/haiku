/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/partitions.h>
#include <boot/vfs.h>
#include <boot/stdio.h>
#include <ddm_modules.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>


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

extern list gPartitions;


class Partition : public partition_data, Node {
	public:
		Partition(int deviceFD);
		virtual ~Partition();

		virtual ssize_t ReadAt(void *cookie, off_t offset, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t offset, const void *buffer, size_t bufferSize);

		Partition *AddChild();
		status_t Scan();

		Partition *Parent() { return fParent; }

	private:
		void SetParent(Partition *parent) { fParent = parent; }

		int			fFD;
		Partition	*fParent;
};


Partition::Partition(int fd)
	:
	fParent(NULL)
{
	memset(this, 0, sizeof(partition_data));
	id = (partition_id)this;

	// it's safe to close the file
	fFD = dup(fd);
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

	return child;
}


status_t 
Partition::Scan()
{
	// ToDo: the scan algorithm should be recursive

	for (int32 i = 0; i < sNumPartitionModules; i++) {
		partition_module_info *module = sPartitionModules[i];
		void *cookie;

		puts(module->pretty_name);
		if (module->identify_partition(fFD, this, &cookie) <= 0.0)
			continue;

		status_t status = module->scan_partition(fFD, this, cookie);
		module->free_identify_partition_cookie(this, cookie);

		if (status == B_OK)
			return B_OK;
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
	partition.size = 1024*1024*1024; // ToDo: fix this!

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

	// ToDo: only add file systems
	list_add_item(&gPartitions, (void *)child);

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

