/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include "BFSAddOn.h"
#include "InitializeParameterEditor.h"

#include <new>

#include <Directory.h>
#include <List.h>
#include <Path.h>
#include <Volume.h>

#include <DiskDeviceTypes.h>
#include <MutablePartition.h>

#include <AutoDeleter.h>

#ifdef ASSERT
#	undef ASSERT
#endif

#include "bfs.h"
#include "bfs_control.h"
#include "bfs_disk_system.h"


using std::nothrow;


static const uint32 kDiskSystemFlags =
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING
//	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED
	| B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED
	| B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED
;


// #pragma mark - BFSAddOn


BFSAddOn::BFSAddOn()
	: BDiskSystemAddOn(kPartitionTypeBFS, kDiskSystemFlags)
{
}


BFSAddOn::~BFSAddOn()
{
}


status_t
BFSAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
	BFSPartitionHandle* handle = new(nothrow) BFSPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;

	status_t error = handle->Init();
	if (error != B_OK) {
		delete handle;
		return error;
	}

	*_handle = handle;
	return B_OK;
}


bool
BFSAddOn::CanInitialize(const BMutablePartition* partition)
{
	// TODO: Check partition size.
	return true;
}


status_t
BFSAddOn::GetInitializationParameterEditor(const BMutablePartition* partition,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;

	try {
		*editor = new InitializeBFSEditor();
	} catch (std::bad_alloc) {
		return B_NO_MEMORY;
	}
	return B_OK;
}


status_t
BFSAddOn::ValidateInitialize(const BMutablePartition* partition, BString* name,
	const char* parameterString)
{
	if (!CanInitialize(partition) || !name)
		return B_BAD_VALUE;

	// validate name

	// truncate, if it is too long
	if (name->Length() >= BFS_DISK_NAME_LENGTH)
		name->Truncate(BFS_DISK_NAME_LENGTH - 1);

	// replace '/' by '-'
	name->ReplaceAll('/', '-');

	// check parameters
	initialize_parameters parameters;
	status_t error = parse_initialize_parameters(parameterString, parameters);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
BFSAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameterString, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition) || check_volume_name(name) != B_OK)
		return B_BAD_VALUE;

	initialize_parameters parameters;
	status_t error = parse_initialize_parameters(parameterString, parameters);
	if (error != B_OK)
		return error;

	// create the handle
	BFSPartitionHandle* handle = new(nothrow) BFSPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;
	ObjectDeleter<BFSPartitionHandle> handleDeleter(handle);

	// init the partition
	error = partition->SetContentType(Name());
	if (error != B_OK)
		return error;
// TODO: The content type could as well be set by the caller.

	partition->SetContentName(name);
	partition->SetContentParameters(parameterString);
	uint32 blockSize = parameters.blockSize;
	partition->SetBlockSize(blockSize);
	partition->SetContentSize(partition->Size() / blockSize * blockSize);
	partition->Changed(B_PARTITION_CHANGED_INITIALIZATION);

	*_handle = handleDeleter.Detach();

	return B_OK;
}


// #pragma mark - BFSPartitionHandle


BFSPartitionHandle::BFSPartitionHandle(BMutablePartition* partition)
	: BPartitionHandle(partition)
{
}


BFSPartitionHandle::~BFSPartitionHandle()
{
}


status_t
BFSPartitionHandle::Init()
{
// TODO: Check parameters.
	return B_OK;
}


uint32
BFSPartitionHandle::SupportedOperations(uint32 mask)
{
	return kDiskSystemFlags & mask;
}


status_t
BFSPartitionHandle::Repair(bool checkOnly)
{
	BVolume volume(Partition()->VolumeID());
	BDirectory directory;
	volume.GetRootDirectory(&directory);
	BPath path;
	path.SetTo(&directory, ".");

	int fd = open(path.Path(), O_RDONLY);
	if (fd < 0)
	    return errno;

	struct check_control result;
	memset(&result, 0, sizeof(result));
	result.magic = BFS_IOCTL_CHECK_MAGIC;
	result.flags = !checkOnly ? BFS_FIX_BITMAP_ERRORS : 0;
	if (!checkOnly) {
		//printf("will fix any severe errors!\n");
		result.flags |= BFS_REMOVE_WRONG_TYPES | BFS_REMOVE_INVALID
			| BFS_FIX_NAME_MISMATCHES;
	}

	// start checking
	if (ioctl(fd, BFS_IOCTL_START_CHECKING, &result, sizeof(result)) < 0)
	    return errno;

	off_t attributeDirectories = 0, attributes = 0;
	off_t files = 0, directories = 0, indices = 0;
	off_t counter = 0;

	// check all files and report errors
	while (ioctl(fd, BFS_IOCTL_CHECK_NEXT_NODE, &result,
			sizeof(result)) == 0) {
		if (++counter % 50 == 0)
			printf("%9Ld nodes processed\x1b[1A\n", counter);

		if (result.errors) {
			printf("%s (inode = %lld)", result.name, result.inode);
			if (result.errors & BFS_MISSING_BLOCKS)
				printf(", some blocks weren't allocated");
			if (result.errors & BFS_BLOCKS_ALREADY_SET)
				printf(", has blocks already set");
			if (result.errors & BFS_INVALID_BLOCK_RUN)
				printf(", has invalid block run(s)");
			if (result.errors & BFS_COULD_NOT_OPEN)
				printf(", could not be opened");
			if (result.errors & BFS_WRONG_TYPE)
				printf(", has wrong type");
			if (result.errors & BFS_NAMES_DONT_MATCH)
				printf(", names don't match");
			putchar('\n');
		}
		if ((result.mode & (S_INDEX_DIR | 0777)) == S_INDEX_DIR)
			indices++;
		else if (result.mode & S_ATTR_DIR)
			attributeDirectories++;
		else if (result.mode & S_ATTR)
			attributes++;
		else if (S_ISDIR(result.mode))
			directories++;
		else
			files++;
	}

	// stop checking
	if (ioctl(fd, BFS_IOCTL_STOP_CHECKING, &result, sizeof(result)) != 0) {
		close(fd);
		return errno;
	}

	close(fd);

	printf("\t%lld nodes checked,\n\t%lld blocks not allocated,"
		"\n\t%lld blocks already set,\n\t%lld blocks could be freed\n\n",
		counter, result.stats.missing, result.stats.already_set,
		result.stats.freed);
	printf("\tfiles\t\t%lld\n\tdirectories\t%lld\n\tattributes\t%lld\n\tattr. "
		"dirs\t%lld\n\tindices\t\t%lld\n", files, directories, attributes,
		attributeDirectories, indices);

	if (result.status == B_ENTRY_NOT_FOUND)
		result.status = B_OK;

	return result.status;
}


// #pragma mark -


status_t
get_disk_system_add_ons(BList* addOns)
{
	BFSAddOn* addOn = new(nothrow) BFSAddOn;
	if (!addOn)
		return B_NO_MEMORY;

	if (!addOns->AddItem(addOn)) {
		delete addOn;
		return B_NO_MEMORY;
	}

	return B_OK;
}
