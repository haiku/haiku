/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2012, Axel Dörfler, axeld@pinc-software.de.
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
#include <StringForSize.h>

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


static BString
size_string(double size)
{
	BString string;
	char* buffer = string.LockBuffer(256);
	string_for_size(size, buffer, 256);

	string.UnlockBuffer();
	return string;
}


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


status_t
BFSAddOn::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;
	if (type == B_INITIALIZE_PARAMETER_EDITOR) {
		try {
			*editor = new InitializeBFSEditor();
		} catch (std::bad_alloc) {
			return B_NO_MEMORY;
		}
		return B_OK;
	}
	return B_NOT_SUPPORTED;
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

	FileDescriptorCloser closer(fd);

	struct check_control result;
	memset(&result, 0, sizeof(result));
	result.magic = BFS_IOCTL_CHECK_MAGIC;
	result.flags = 0;
	if (!checkOnly) {
		//printf("will fix any severe errors!\n");
		result.flags |= BFS_FIX_BITMAP_ERRORS | BFS_REMOVE_WRONG_TYPES
			| BFS_REMOVE_INVALID | BFS_FIX_NAME_MISMATCHES | BFS_FIX_BPLUSTREES;
	}

	// start checking
	if (ioctl(fd, BFS_IOCTL_START_CHECKING, &result, sizeof(result)) < 0)
	    return errno;

	uint64 attributeDirectories = 0, attributes = 0;
	uint64 files = 0, directories = 0, indices = 0;
	uint64 counter = 0;
	uint32 previousPass = result.pass;

	// check all files and report errors
	while (ioctl(fd, BFS_IOCTL_CHECK_NEXT_NODE, &result,
			sizeof(result)) == 0) {
		if (++counter % 50 == 0)
			printf("%9Ld nodes processed\x1b[1A\n", counter);

		if (result.pass == BFS_CHECK_PASS_BITMAP) {
			if (result.errors) {
				printf("%s (inode = %lld)", result.name, result.inode);
				if ((result.errors & BFS_MISSING_BLOCKS) != 0)
					printf(", some blocks weren't allocated");
				if ((result.errors & BFS_BLOCKS_ALREADY_SET) != 0)
					printf(", has blocks already set");
				if ((result.errors & BFS_INVALID_BLOCK_RUN) != 0)
					printf(", has invalid block run(s)");
				if ((result.errors & BFS_COULD_NOT_OPEN) != 0)
					printf(", could not be opened");
				if ((result.errors & BFS_WRONG_TYPE) != 0)
					printf(", has wrong type");
				if ((result.errors & BFS_NAMES_DONT_MATCH) != 0)
					printf(", names don't match");
				if ((result.errors & BFS_INVALID_BPLUSTREE) != 0)
					printf(", invalid b+tree");
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
		} else if (result.pass == BFS_CHECK_PASS_INDEX) {
			if (previousPass != result.pass) {
				printf("Recreating broken index b+trees...\n");
				previousPass = result.pass;
				counter = 0;
			}
		}
	}

	// stop checking
	if (ioctl(fd, BFS_IOCTL_STOP_CHECKING, &result, sizeof(result)) != 0)
		return errno;

	printf("        %" B_PRIu64 " nodes checked,\n\t%" B_PRIu64 " blocks not "
		"allocated,\n\t%" B_PRIu64 " blocks already set,\n\t%" B_PRIu64
		" blocks could be freed\n\n", counter, result.stats.missing,
		result.stats.already_set, result.stats.freed);
	printf("\tfiles\t\t%" B_PRIu64 "\n\tdirectories\t%" B_PRIu64 "\n"
		"\tattributes\t%" B_PRIu64 "\n\tattr. dirs\t%" B_PRIu64 "\n"
		"\tindices\t\t%" B_PRIu64 "\n", files, directories, attributes,
		attributeDirectories, indices);

	printf("\n\tdirect block runs\t\t%" B_PRIu64 " (%s)\n",
		result.stats.direct_block_runs, size_string(1.0
			* result.stats.blocks_in_direct
			* result.stats.block_size).String());
	printf("\tindirect block runs\t\t%" B_PRIu64 " (in %" B_PRIu64
		" array blocks, %s)\n", result.stats.indirect_block_runs,
		result.stats.indirect_array_blocks,
		size_string(1.0 * result.stats.blocks_in_indirect
			* result.stats.block_size).String());
	printf("\tdouble indirect block runs\t%" B_PRIu64 " (in %" B_PRIu64
		" array blocks, %s)\n", result.stats.double_indirect_block_runs,
		result.stats.double_indirect_array_blocks,
		size_string(1.0 * result.stats.blocks_in_double_indirect
			* result.stats.block_size).String());
	// TODO: this is currently not maintained correctly
	//printf("\tpartial block runs\t%" B_PRIu64 "\n",
	//	result.stats.partial_block_runs);

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
