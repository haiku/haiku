// DiskScannerTest.cpp

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fs_device.h>
#include <KernelExport.h>
#include <disk_scanner/disk_scanner.h>

// The following code will be moved into libroot.

// get_nth_session_info
/*!	\brief Retrieves information about a session on a device.
	\param deviceFD File descriptor for the device in question.
	\param index The session index.
	\param sessionInfo Pointer to a pre-allocated session_info to be filled
		   out by the function.
	\return
	- \c B_OK: Everything went fine.
	- an error code: The contents of \a sessionInfo is undefined.
*/
status_t
get_nth_session_info(int deviceFD, int32 index, session_info *sessionInfo)
{
	status_t error = (sessionInfo ? B_OK : B_BAD_VALUE);
	disk_scanner_module_info *disk_scanner = NULL;
	// get the partition scanner module
	if (error == B_OK)
		error = get_module(DISK_SCANNER_MODULE_NAME, (module_info**)&disk_scanner);
	// get the session info
	if (error == B_OK)
		error = disk_scanner->get_nth_session_info(deviceFD, index, sessionInfo);
	// put the partition scanner module
	if (disk_scanner)
		put_module(disk_scanner->module.name);
	return error;
}

// get_nth_partition_info
/*!	\brief Retrieves information about a partion on a device.

	The fields \c device and \c mounted_at of \a partitionInfo are not set.

	\param deviceFD File descriptor for the device in question.
	\param sessionIndex The index of the session on which the partition
		   resides.
	\param partitionIndex The partition index.
	\param partitionInfo Pointer to a pre-allocated extended_partition_info
		   to be filled out by the function.
	\return
	- \c B_OK: Everything went fine.
	- an error code: The contents of \a partitionInfo is undefined.
*/
status_t
get_nth_partition_info(int deviceFD, int32 sessionIndex, int32 partitionIndex,
					   extended_partition_info *partitionInfo)
{
	status_t error = (partitionInfo ? B_OK : B_BAD_VALUE);
	session_info sessionInfo;
	disk_scanner_module_info *disk_scanner = NULL;
	// get the partition scanner module
	if (error == B_OK)
		error = get_module(DISK_SCANNER_MODULE_NAME, (module_info**)&disk_scanner);
	// get the session info
	if (error == B_OK) {
		error = disk_scanner->get_nth_session_info(deviceFD, sessionIndex,
											   &sessionInfo);
	}
	// get the partition info
	if (error == B_OK) {
		partitionInfo->info.logical_block_size
			= sessionInfo.logical_block_size;
		partitionInfo->info.session = sessionIndex;
		partitionInfo->info.partition = partitionIndex;
// NOTE: partitionInfo->info.device is not filled in!
// The user can this info via B_GET_PARTITION_INFO. We could get the dir
// of the raw device and construct the partition device name with session and
// partition ID.
partitionInfo->info.device[0] = '\0';
		error = disk_scanner->get_nth_partition_info(deviceFD, sessionInfo.offset,
			sessionInfo.size, partitionInfo);
	}
	// get the FS info
	if (error == B_OK) {
		bool hidden = (partitionInfo->flags & B_HIDDEN_PARTITION);
		if (!hidden) {
			error = disk_scanner->get_partition_fs_info(deviceFD,
														partitionInfo);
		}
		// in case the partition is no data partition or the FS is unknown,
		// we fill in the respective fields
		if (hidden || error == B_ENTRY_NOT_FOUND) {
			error = B_OK;
			partitionInfo->file_system_short_name[0] = '\0';
			partitionInfo->file_system_long_name[0] = '\0';
			partitionInfo->volume_name[0] = '\0';
			partitionInfo->mounted_at[0] = '\0';
		}
// NOTE: Where do we get mounted_at from?
	}
	// put the partition scanner module
	if (disk_scanner)
		put_module(disk_scanner->module.name);
	return error;
}


// the test code starts here

// print_session_info
static
void
print_session_info(const char *prefix, const session_info &info)
{
	printf("%soffset:     %lld\n", prefix, info.offset);
	printf("%ssize:       %lld\n", prefix, info.size);
	printf("%sblock size: %ld\n", prefix, info.logical_block_size);
	printf("%sindex:      %ld\n", prefix, info.index);
	printf("%sflags:      %lx\n", prefix, info.flags);
}

// print_partition_info
static
void
print_partition_info(const char *prefix, const extended_partition_info &info)
{
	printf("%soffset:         %lld\n", prefix, info.info.offset);
	printf("%ssize:           %lld\n", prefix, info.info.size);
	printf("%sblock size:     %ld\n", prefix, info.info.logical_block_size);
	printf("%ssession ID:     %ld\n", prefix, info.info.session);
	printf("%spartition ID:   %ld\n", prefix, info.info.partition);
	printf("%sdevice:         `%s'\n", prefix, info.info.device);
	printf("%sflags:          %lx\n", prefix, info.flags);
	printf("%spartition code: 0x%x\n", prefix, info.partition_code);
	printf("%spartition name: `%s'\n", prefix, info.partition_name);
	printf("%spartition type: `%s'\n", prefix, info.partition_type);
	printf("%sFS short name:  `%s'\n", prefix, info.file_system_short_name);
	printf("%sFS long name:   `%s'\n", prefix, info.file_system_long_name);
	printf("%svolume name:    `%s'\n", prefix, info.volume_name);
	printf("%smounted at:     `%s'\n", prefix, info.mounted_at);
}

// main
int
main()
{
//	const char *deviceName = "/dev/disk/virtual/0/raw";
	const char *deviceName = "/dev/disk/ide/ata/0/master/0/raw";
	int device = open(deviceName, 0);
	if (device >= 0) {
		printf("device: `%s'\n", deviceName);
		session_info sessionInfo;
		for (int32 i = 0;
			 get_nth_session_info(device, i, &sessionInfo) == B_OK;
			 i++) {
			printf("session %ld\n", i);
			print_session_info("  ", sessionInfo);
			extended_partition_info partitionInfo;
			for (int32 k = 0;
				 get_nth_partition_info(device, i, k, &partitionInfo) == B_OK;
				 k++) {
				printf("  partition %ld_%ld\n", i, k);
				print_partition_info("    ", partitionInfo);
			}
		}
		close(device);
	}
	return 0;
}

