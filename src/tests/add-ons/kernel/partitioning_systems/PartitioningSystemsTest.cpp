/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <map>
#include <vector>

#include <module.h>

#include <disk_device_manager/ddm_modules.h>
#include <disk_device_manager.h>


struct partition_entry;
typedef std::vector<partition_entry*> PartitionVector;

struct partition_entry : partition_data {
	partition_entry*	parent;
	PartitionVector		children;
	char				path[PATH_MAX];
};

typedef std::map<partition_id, partition_entry*> PartitionMap;
typedef std::map<partition_id, disk_device_data*> DiskDeviceMap;


static PartitionMap sPartitions;
static DiskDeviceMap sDiskDevices;
static partition_id sNextID = 1;


static status_t
create_disk_device(int fd, const char* path, partition_id* _id)
{
	partition_entry* partition = new partition_entry;
	memset(partition, 0, sizeof(partition_entry));
	partition->id = sNextID++;
	strlcpy(partition->path, path, sizeof(partition->path));

	disk_device_data* device = new disk_device_data;
	device->id = partition->id;
	device->path = partition->path;
	device->flags = 0;

	if (ioctl(fd, B_GET_GEOMETRY, &device->geometry) < 0) {
		// maybe it's just a file
		struct stat stat;
		if (fstat(fd, &stat) < 0) {
			delete partition;
			delete device;
			return errno;
		}

		uint32 blockSize = 512;
		off_t blocks = stat.st_size / blockSize;
		uint32 heads = (blocks + ULONG_MAX - 1) / ULONG_MAX;
		if (heads == 0)
			heads = 1;
		device->geometry.bytes_per_sector = blockSize;
	    device->geometry.sectors_per_track = 1;
	    device->geometry.cylinder_count = blocks / heads;
	    device->geometry.head_count = heads;
	    device->geometry.device_type = B_DISK;
	    device->geometry.removable = false;
	    device->geometry.read_only = true;
	    device->geometry.write_once = false;
	}

	partition->offset = 0;
	partition->size = 1LL * device->geometry.head_count
		* device->geometry.cylinder_count * device->geometry.sectors_per_track
		* device->geometry.bytes_per_sector;
	partition->block_size = device->geometry.bytes_per_sector;

	sDiskDevices.insert(std::make_pair(partition->id, device));
	sPartitions.insert(std::make_pair(partition->id, partition));

	if (_id != NULL)
		*_id = partition->id;

	return B_OK;
}


static void
print_disk_device(partition_id id)
{
	disk_device_data* data = get_disk_device(id);

	printf("device ID %ld\n", id);
	printf("  path                 %s\n", data->path);
	printf("  geometry\n");
	printf("    bytes per sector   %lu\n", data->geometry.bytes_per_sector);
	printf("    sectors per track  %lu\n", data->geometry.sectors_per_track);
	printf("    cylinder count     %lu\n", data->geometry.cylinder_count);
	printf("    head count         %lu (size %lld bytes)\n",
		data->geometry.head_count, 1LL * data->geometry.head_count
			* data->geometry.cylinder_count * data->geometry.sectors_per_track
			* data->geometry.bytes_per_sector);
	printf("    device type        %d\n", data->geometry.device_type);
	printf("    removable          %d\n", data->geometry.removable);
	printf("    read only          %d\n", data->geometry.read_only);
	printf("    write once         %d\n\n", data->geometry.write_once);
}


static void
print_partition(partition_id id)
{
	partition_data* data = get_partition(id);

	printf("partition ID %ld\n", id);
	printf("  offset               %lld\n", data->offset);
	printf("  size                 %lld\n", data->size);
	printf("  content_size         %lld\n", data->content_size);
	printf("  block_size           %lu\n", data->block_size);
	printf("  child_count          %ld\n", data->child_count);
	printf("  index                %ld\n", data->index);
	printf("  status               %#lx\n", data->status);
	printf("  flags                %#lx\n", data->flags);
	printf("  name                 %s\n", data->name);
	printf("  type                 %s\n", data->type);
	printf("  content_name         %s\n", data->content_name);
	printf("  content_type         %s\n", data->content_type);
	printf("  parameters           %s\n", data->parameters);
	printf("  content_parameters   %s\n", data->content_parameters);
}


status_t
scan_partition(int fd, partition_id partitionID)
{
	partition_data* data = get_partition(partitionID);

	void* list = open_module_list("partitioning_systems");
	char name[PATH_MAX];
	size_t nameSize = sizeof(name);
	while (read_next_module_name(list, name, &nameSize) == B_OK) {
		partition_module_info* moduleInfo;
		if (get_module(name, (module_info**)&moduleInfo) == B_OK
			&& moduleInfo->identify_partition != NULL) {
			void* cookie;
			float priority = moduleInfo->identify_partition(fd, data, &cookie);

			printf("%s: %g\n", name, priority);

			if (priority >= 0) {
				// scan partitions
				moduleInfo->scan_partition(fd, data, cookie);
				moduleInfo->free_identify_partition_cookie(data, cookie);

				free((char*)data->content_type);
				data->content_type = strdup(moduleInfo->pretty_name);
			}

			put_module(name);
		}
		nameSize = sizeof(name);
	}

	return B_OK;
}


// #pragma mark - disk device manager API


disk_device_data*
write_lock_disk_device(partition_id partitionID)
{
	// TODO: we could check if the device is properly unlocked again
	return get_disk_device(partitionID);
}


void
write_unlock_disk_device(partition_id partitionID)
{
}


disk_device_data*
read_lock_disk_device(partition_id partitionID)
{
	// TODO: we could check if the device is properly unlocked again
	return get_disk_device(partitionID);
}


void
read_unlock_disk_device(partition_id partitionID)
{
}


disk_device_data*
get_disk_device(partition_id partitionID)
{
	DiskDeviceMap::iterator found = sDiskDevices.find(partitionID);
	if (found == sDiskDevices.end())
		return NULL;

	return found->second;
}


partition_data*
get_partition(partition_id partitionID)
{
	PartitionMap::iterator found = sPartitions.find(partitionID);
	if (found == sPartitions.end())
		return NULL;

	return found->second;
}


partition_data*
get_parent_partition(partition_id partitionID)
{
	PartitionMap::iterator found = sPartitions.find(partitionID);
	if (found == sPartitions.end())
		return NULL;

	return found->second->parent;
}


partition_data*
get_child_partition(partition_id partitionID, int32 index)
{
	PartitionMap::iterator found = sPartitions.find(partitionID);
	if (found == sPartitions.end())
		return NULL;

	partition_entry* partition = found->second;

	if (index < 0 || index >= (int32)partition->children.size())
		return NULL;

	return partition->children[index];
}


int
open_partition(partition_id partitionID, int openMode)
{
	return -1;
}


partition_data*
create_child_partition(partition_id partitionID, int32 index, off_t offset,
	off_t size, partition_id childID)
{
	PartitionMap::iterator found = sPartitions.find(partitionID);
	if (found == sPartitions.end())
		return NULL;

	partition_entry* parent = found->second;

	partition_entry* child = new partition_entry();
	memset(child, 0, sizeof(partition_entry));

	child->id = sNextID++;
	child->offset = offset;
	child->size = size;
	child->index = parent->children.size();

	parent->children.push_back(child);
	parent->child_count++;
	sPartitions.insert(std::make_pair(child->id, child));

	printf("  new partition ID %ld (child of %ld)\n", child->id, parent->id);
	return child;
}


bool
delete_partition(partition_id partitionID)
{
	// TODO
	return false;
}


void
partition_modified(partition_id partitionID)
{
	// TODO: implemented
}


status_t
scan_partition(partition_id partitionID)
{
	PartitionMap::iterator found = sPartitions.find(partitionID);
	if (found == sPartitions.end())
		return B_ENTRY_NOT_FOUND;

	if (sDiskDevices.find(partitionID) == sDiskDevices.end()) {
		// TODO: we would need to fake child partitons
		return B_NOT_SUPPORTED;
	}

	partition_entry* partition = found->second;
	int fd = open(partition->path, O_RDONLY);
	if (fd < 0)
		return errno;

	scan_partition(fd, partitionID);
	void* list = open_module_list("partitioning_systems");
	char name[PATH_MAX];
	size_t nameSize = sizeof(name);
	while (read_next_module_name(list, name, &nameSize) == B_OK) {
		puts(name);
		nameSize = sizeof(name);
	}
	return B_ERROR;
}


bool
update_disk_device_job_progress(disk_job_id jobID, float progress)
{
	return false;
}


bool
set_disk_device_job_error_message(disk_job_id jobID, const char* message)
{
#if 0
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskDeviceJob* job = manager->FindJob(jobID)) {
			job->SetErrorMessage(message);
			return true;
		}
	}
#endif
	return false;
}


// #pragma mark -


static void
usage()
{
	extern const char* __progname;

	fprintf(stderr, "usage: %s <device>\n"
		"Must be started from the top-level Haiku directory to find its "
		"add-ons.\n", __progname);
	exit(1);
}


int
main(int argc, char** argv)
{
	if (argc != 2)
		usage();

	const char* deviceName = argv[1];

	int device = open(deviceName, O_RDONLY);
	if (device < 0) {
		fprintf(stderr, "Could not open device \"%s\": %s\n", deviceName,
			strerror(errno));
		return 1;
	}

	partition_id id;
	status_t status = create_disk_device(device, deviceName, &id);
	if (status != B_OK) {
		fprintf(stderr, "Could not get device size \"%s\": %s\n", deviceName,
			strerror(status));
		return 1;
	}

	print_disk_device(id);
	scan_partition(device, id);

	PartitionMap::iterator iterator = sPartitions.begin();
	for (; iterator != sPartitions.end(); iterator++) {
		print_partition(iterator->first);
	}

	close(device);
	return 0;
}

