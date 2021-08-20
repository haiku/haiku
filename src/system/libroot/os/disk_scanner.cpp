//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#include <disk_scanner.h>
#include <disk_scanner/disk_scanner.h>
#include <KernelExport.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
	disk_scanner_module_info *diskScanner = NULL;
	// get the disk scanner module
	if (error == B_OK) {
		error = get_module(DISK_SCANNER_MODULE_NAME,
						   (module_info**)&diskScanner);
	}
	// get the session info
	if (error == B_OK) {
		error = diskScanner->get_nth_session_info(deviceFD, index,
												  sessionInfo, NULL);
	}
	// put the partition scanner module
	if (diskScanner)
		put_module(diskScanner->module.name);
	return error;
}

// get_nth_partition_info
/*!	\brief Retrieves information about a partition on a device.

	The fields \c device and \c mounted_at of \a partitionInfo are not set.

	\param deviceFD File descriptor for the device in question.
	\param sessionIndex The index of the session on which the partition
		   resides.
	\param partitionIndex The partition index.
	\param partitionInfo Pointer to a pre-allocated extended_partition_info
		   to be filled out by the function.
	\param partitionMapName Pointer to a pre-allocated char buffer of minimal
		   size \c B_FILE_NAME_LENGTH, into which the short name of the
		   partitioning system shall be written. The result is the empty
		   string (""), if no partitioning system is used (e.g. for floppies).
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- an error code: The contents of \a partitionInfo is undefined.
*/
status_t
get_nth_partition_info(int deviceFD, int32 sessionIndex, int32 partitionIndex,
					   extended_partition_info *partitionInfo,
					   char *partitionMapName)
{
	if (partitionInfo == NULL)
		return B_BAD_VALUE;

	session_info sessionInfo;
	disk_scanner_module_info *diskScanner = NULL;

	// get the disk scanner module
	status_t error = get_module(DISK_SCANNER_MODULE_NAME,
						(module_info**)&diskScanner);
	if (diskScanner == NULL)
		return error;

	// get the session info
	error = diskScanner->get_nth_session_info(deviceFD, sessionIndex,
											  &sessionInfo, NULL);

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
// Update: No, we can neither get the name of the raw device nor of the
// directory it lives in. We only have a FD and I see no way to get a path
// from it. Since deviceFD might represent an image file, we can't even get
// the its path by recursively searching the /dev/disk directory.
partitionInfo->info.device[0] = '\0';
		error = diskScanner->get_nth_partition_info(deviceFD, &sessionInfo,
			partitionIndex, partitionInfo, partitionMapName, NULL);
	}
	// get the FS info
	if (error == B_OK) {
		bool hidden = (partitionInfo->flags & B_HIDDEN_PARTITION);
		if (!hidden) {
			error = diskScanner->get_partition_fs_info(deviceFD,
													   partitionInfo);
		}
		// in case the partition is no data partition or the FS is unknown,
		// we fill in the respective fields
		if (hidden || error == B_ENTRY_NOT_FOUND) {
			error = B_OK;
			partitionInfo->file_system_short_name[0] = '\0';
			partitionInfo->file_system_long_name[0] = '\0';
			partitionInfo->volume_name[0] = '\0';
//			partitionInfo->mounted_at[0] = '\0';
			partitionInfo->file_system_flags = 0;
		}
// NOTE: Where do we get mounted_at from?
// Update: Actually, it looks, like it is really hard. We could traverse the
// list of mounted devices, build for each one the raw device path from its
// partition device path, check, if the raw device is the same one as deviceFD.
// Then, with the path of the raw device we have more options.
	}
	// put the partition scanner module
	if (diskScanner)
		put_module(diskScanner->module.name);
	return error;
}

// get_partitioning_parameters
/*!	\brief Returns parameters for partitioning a session.

	The partitioning system (module) identified by \a identifier is asked to
	return parameters for the session. If the session is already partitioned
	using this system, then the parameters describing the current layout will
	be returned, otherwise default values.

	If the supplied buffer is too small for the parameters, the function
	returns \c B_OK, but doesn't fill in the buffer; the required buffer
	size is returned in \a actualSize. If the buffer is large enough,
	\a actualSize is set to the actually used size. The size includes the
	terminating null.

	\param deviceFD The device the session to be partitioned resides on.
	\param sessionIndex The index of the session to be partitioned.
	\param identifier A string identifying the partitioning system to be used.
	\param buffer Pointer to a pre-allocated buffer of size \a bufferSize.
	\param bufferSize The size of \a buffer.
	\param actualSize Pointer to a pre-allocated size_t to be set to the
		   actually needed buffer size.
	\return
	- \c B_OK: The parameters could be retrieved or the buffer is too
	  small. \a actualSize has to be checked!
	- another error code, if something went wrong
*/
status_t
get_partitioning_parameters(int deviceFD, int32 sessionIndex,
							const char *identifier, char *buffer,
							size_t bufferSize, size_t *actualSize)
{
	status_t error = (identifier && buffer && actualSize ? B_OK : B_BAD_VALUE);
	disk_scanner_module_info *diskScanner = NULL;
	session_info sessionInfo;
	// get the disk scanner module
	if (error == B_OK) {
		error = get_module(DISK_SCANNER_MODULE_NAME,
						   (module_info**)&diskScanner);
	}
	// get the session info
	if (error == B_OK) {
		error = diskScanner->get_nth_session_info(deviceFD, sessionIndex,
												  &sessionInfo, NULL);
	}
	// get the parameters
	if (error == B_OK) {
		error = diskScanner->get_partitioning_params(deviceFD, &sessionInfo,
			identifier, buffer, bufferSize, actualSize);
	}
	// put the partition scanner module
	if (diskScanner)
		put_module(diskScanner->module.name);
	return error;
}

// get_fs_initialization_parameters
/*!	\brief Returns parameters for initializing a volume.

	The FS identified by \a fileSystem is asked to return parameters for
	the volume. If the volume is already initialized with this FS, then the
	parameters describing the current state will be returned, otherwise
	default values.

	If the supplied buffer is too small for the parameters, the function
	returns \c B_OK, but doesn't fill in the buffer; the required buffer
	size is returned in \a actualSize. If the buffer is large enough,
	\a actualSize is set to the actually used size. The size includes the
	terminating null.

	\param deviceFD The device the partition to be initialized resides on.
	\param sessionIndex The index of the session the partition to be
						initialized resides on.
	\param partitionIndex The index of the partition to be initialized.
	\param fileSystem A string identifying the file system to be used.
	\param buffer Pointer to a pre-allocated buffer of size \a bufferSize.
	\param bufferSize The size of \a buffer.
	\param actualSize Pointer to a pre-allocated size_t to be set to the
		   actually needed buffer size.
	\return
	- \c B_OK: The parameters could be retrieved or the buffer is too
	  small. \a actualSize has to be checked!
	- another error code, if something went wrong
*/
status_t
get_fs_initialization_parameters(int deviceFD, int32 sessionIndex,
								 int32 partitionIndex, const char *fileSystem,
								 char *buffer, size_t bufferSize,
								 size_t *actualSize)
{
	// not yet implemented
	return B_UNSUPPORTED;
}

// partition_session
/*!	\brief Partitions a specified session on a device using the paritioning
		   system identified by \a identifier and according to supplied
		   parameters.
	\param deviceFD The device the session to be partitioned resides on.
	\param sessionIndex The index of the session to be partitioned.
	\param identifier A string identifying the partitioning system to be used.
	\param parameters Parameters according to which the session shall be
		   partitioned. May be \c NULL, depending on the concerned partition
		   module.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
partition_session(int deviceFD, int32 sessionIndex, const char *identifier,
				  const char *parameters)
{
	status_t error = (identifier ? B_OK : B_BAD_VALUE);
	disk_scanner_module_info *diskScanner = NULL;
	session_info sessionInfo;
	// get the disk scanner module
	if (error == B_OK) {
		error = get_module(DISK_SCANNER_MODULE_NAME,
						   (module_info**)&diskScanner);
	}
	// get the session info
	if (error == B_OK) {
		error = diskScanner->get_nth_session_info(deviceFD, sessionIndex,
												  &sessionInfo, NULL);
	}
	// partition the session
	if (error == B_OK) {
		error = diskScanner->partition(deviceFD, &sessionInfo, identifier,
									   parameters);
	}
	// put the partition scanner module
	if (diskScanner)
		put_module(diskScanner->module.name);
	return error;
}

// initialize_volume
/*!	\brief Initializes a specified device using a certain file system.
	\param where The path to the device to be initialized.
	\param fileSystem The identifier of the file system to be used for that
		   partition.
	\param volumeName The name to be given to the initialized volume.
	\param parameters Parameters according to which the session shall be
		   initialized.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
initialize_volume(const char *where, const char *fileSystem,
				  const char *volumeName, const char *parameters)
{
	// not yet implemented
	return B_UNSUPPORTED;
}

