//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file iso9660.c
	disk_scanner filesystem module for iso9660 CD-ROM filesystems

	The standard to which this module is written is ECMA-119 second
	edition, a freely available iso9660 equivalent.
*/


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fs_device.h>
#include <KernelExport.h>
#include <disk_scanner/fs.h>

#define TRACE(x) ;
//#define TRACE(x) dprintf x

// misc constants
#define ISO9660_FS_MODULE_NAME "disk_scanner/fs/iso9660/v1"
static const char *kModuleDebugName = "fs/iso9660";
static const char *kISO9660Signature = "CD001";
static const uint32 kVolumeDescriptorLength = 2048;
#define ISO9660_VOLUME_IDENTIFIER_LENGTH 32

/*! \brief The portion of the volume descriptor common to all
    descriptor types.
*/
typedef struct iso9660_common_volume_descriptor {
	uchar volume_descriptor_type;
	uchar standard_identifier[5];	// should be 'CD001'
	uchar volume_descriptor_version;
	// Remaining bytes are unused
} iso9660_common_volume_descriptor;

/*! \brief Primary volume descriptor
*/
typedef struct iso9660_primary_volume_descriptor {
	iso9660_common_volume_descriptor info;
	uchar volume_flags;
	uchar system_identifier[32];
	uchar volume_identifier[ISO9660_VOLUME_IDENTIFIER_LENGTH];
	// Remaining bytes are disinteresting to us
} iso9660_primary_volume_descriptor;

//! Volume descriptor types
typedef enum {
	ISO9660VD_BOOT,
	ISO9660VD_PRIMARY,
	ISO9660VD_SUPPLEMENTARY,
	ISO9660VD_PARTITION,
	ISO9660VD_TERMINATOR = 255
} iso9660_volume_descriptor_type;

static
const char*
volume_descriptor_type_to_string(iso9660_volume_descriptor_type type)
{
	switch (type) {
		case ISO9660VD_BOOT:          return "boot";
		case ISO9660VD_PRIMARY:       return "primary";
		case ISO9660VD_SUPPLEMENTARY: return "supplementary";
		case ISO9660VD_PARTITION:     return "partiton";
		case ISO9660VD_TERMINATOR:    return "terminator";
		default:                      return "invalid";
	}
}

static
void
dump_common_volume_descriptor(iso9660_common_volume_descriptor *common, const char *indent,
                              bool print_header)
{
	if (print_header)
		TRACE(("%siso9660_common_volume_descriptor:\n", indent));
	
	TRACE(("%s  volume_descriptor_type    == %d (%s)\n", indent,
	       common->volume_descriptor_type,
	       volume_descriptor_type_to_string(common->volume_descriptor_type)));
	TRACE(("%s  standard_identifier       == %.5s (%s)\n", indent, common->standard_identifier,
	       strncmp(common->standard_identifier, kISO9660Signature, 5) == 0 ? "valid" : "INVALID"));
	TRACE(("%s  volume_descriptor_version == %d\n", indent, common->volume_descriptor_version));
}

static
void
dump_primary_volume_descriptor(iso9660_primary_volume_descriptor *primary, const char *indent,
                              bool print_header)
{
	if (print_header)
		TRACE(("%siso9660_primary_volume_descriptor:\n", indent));
	
	dump_common_volume_descriptor(&(primary->info), indent, false);
	TRACE(("%s  volume_identifier         == %.32s\n", indent,
	       primary->volume_identifier));
}

static
status_t
check_common_volume_descriptor(iso9660_common_volume_descriptor *common)
{
	status_t error = common ? B_OK : B_BAD_VALUE;
	if (!error)
		error = strncmp(common->standard_identifier, kISO9660Signature,
		                5) == 0 ? B_OK : B_BAD_DATA;
	return error;
}

// std_ops
static
status_t
std_ops(int32 op, ...)
{
	TRACE(("%s: std_ops(0x%lx)\n", kModuleDebugName, op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

// read_block
static
status_t
read_block(int fd, off_t offset, size_t size, uchar **block)
{
	status_t error = (block && size > 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		*block = malloc(size);
		if (*block) {
			if (read_pos(fd, offset, *block, size) != (ssize_t)size) {
				error = errno;
				if (error == B_OK)
					error = B_ERROR;
				free(*block);
				*block = NULL;
			}
		} else
			error = B_NO_MEMORY;
	}
	return error;
}

// iso9660_fs_identify
/*! \brief Returns true if the given partition is a valid iso9660 partition.

	See fs_identify_hook() for more information.
	
	\todo Fill in partitionInfo->mounted_at with something useful.
*/
static
bool
iso9660_fs_identify(int deviceFD, struct extended_partition_info *partitionInfo,
                    float *priority)
{
	bool result = false;
	uchar *buffer = NULL;
	uint32 blockSize = partitionInfo->info.logical_block_size;
	bool exit = false;
	// The first 16 blocks are for "system use" only, and thus are
	// irrelevant to us and generally just zeros
	off_t offset = partitionInfo->info.offset + 16*blockSize;
	status_t error = B_OK;
	
	TRACE(("%s: identify(%d, %p)\n", kModuleDebugName, deviceFD,
		   partitionInfo));
		   
	// Read through the volume descriptors looking for a primary descriptor.
	// If for some reason there are more than one primary descriptor, the
	// volume name from the last encountered descriptor will be used.
	while (!error && !exit) {
		iso9660_common_volume_descriptor *common = NULL;
		
		// Read the block containing the current descriptor
		error = read_block(deviceFD, offset, blockSize, &buffer);
		offset += blockSize;
		if (!error) {
			common = (iso9660_common_volume_descriptor*)buffer;
			error = check_common_volume_descriptor(common);
		}
		
		// Handle each type of descriptor appropriately
		if (!error) {
			TRACE(("%s: found %s descriptor\n", kModuleDebugName,
			       volume_descriptor_type_to_string(common->volume_descriptor_type)));

			switch (common->volume_descriptor_type) {
				case ISO9660VD_BOOT:
					break;
					
				case ISO9660VD_PRIMARY:
				{
					int i;
					iso9660_primary_volume_descriptor *primary = (iso9660_primary_volume_descriptor*)buffer;
					
					dump_primary_volume_descriptor(primary, "  ", true);
					
					if (partitionInfo->file_system_short_name)
						strcpy(partitionInfo->file_system_short_name, "iso9660");
					if (partitionInfo->file_system_long_name)
						strcpy(partitionInfo->file_system_long_name, "iso9660 CD-ROM File System");

					// Cut off any trailing spaces from the volume id. Note
					// that this allows for spaces INSIDE the volume id, even
					// though that's not technically allowed by the standard;
					// this was necessary to support certain RedHat 6.2 CD-ROMs
					// from a certain Linux company who shall remain unnamed. ;-)
					for (i = ISO9660_VOLUME_IDENTIFIER_LENGTH-1; i >= 0; i--) {
						if (primary->volume_identifier[i] != 0x20)
							break;
					}
					if (partitionInfo->volume_name) {
						strncpy(partitionInfo->volume_name, primary->volume_identifier, i+1);
						partitionInfo->volume_name[i+1] = 0;
					}

					if (priority)
						*priority = 0;
					result = true;
					break;
				}
					
				case ISO9660VD_SUPPLEMENTARY:
					break;
					
				case ISO9660VD_PARTITION:
					break;
					
				case ISO9660VD_TERMINATOR:
					exit = true;
					break;
					
				default:
					break;
			}
		}
		if (buffer) {
			free(buffer);
			buffer = NULL;
		}	
	}
	
	switch (error) {
		case B_OK:
			break;
			
		case B_BAD_DATA:
			TRACE(("%s: identify: bad signature\n", kModuleDebugName));
			break;
		
		default:
			TRACE(("%s: identify error: 0x%lx\n", kModuleDebugName,
		           error));
		    break;
	}

	return result;
}

static fs_module_info iso9660_fs_module = 
{
	// module_info
	{
		ISO9660_FS_MODULE_NAME,
		0,	// better B_KEEP_LOADED?
		std_ops
	},
	iso9660_fs_identify,
};

_EXPORT fs_module_info *modules[] =
{
	&iso9660_fs_module,
	NULL
};

