//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file iso9660.cpp
	\brief disk_scanner filesystem module for iso9660 CD-ROM filesystems

	<h5>iso9660</h5>
	The standard to which this module is written is ECMA-119 second
	edition, a freely available iso9660 equivalent.
	
	<h5>Joliet</h5>
	Joliet support comes courtesy of the following document:
	
	http://www-plateau.cs.berkeley.edu/people/chaffee/jolspec.htm
	
	As specified there, the existence of any of the following escape
	sequences in a supplementary volume descriptor's "escape sequences"
	field denotes a Joliet volume descriptor using unicode ucs-2
	character encoding (2-byte characters, big-endian):
	
	- UCS-2 Level 1: 0x252F40 == "%/@"
	- UCS-2 Level 2: 0x252F43 == "%/C"
	- UCS-2 Level 3: 0x252F45 == "%/E"

	The following UCS-2 characters are considered illegal (we allow them,
	printing out a warning if encountered):
	
	- All values between 0x0000 and 0x001f inclusive == control chars
	- 0x002A == '*'
	- 0x002F == '/'
	- 0x003A == ':'
	- 0x003B == ';'
	- 0x003F == '?'
	- 0x005C == '\'	
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <ByteOrder.h>
#include <disk_scanner.h>
#include <fs_info.h>
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
#define ISO9660_ESCAPE_SEQUENCE_LENGTH 32

//! Volume descriptor types
typedef enum {
	ISO9660VD_BOOT,
	ISO9660VD_PRIMARY,
	ISO9660VD_SUPPLEMENTARY,
	ISO9660VD_PARTITION,
	ISO9660VD_TERMINATOR = 255
} iso9660_volume_descriptor_type;

/*! \brief The portion of the volume descriptor common to all
    descriptor types.
*/
typedef struct iso9660_common_volume_descriptor {
	uchar volume_descriptor_type;
	char standard_identifier[5];	// should be 'CD001'
	uchar volume_descriptor_version;
	// Remaining bytes are unused
} __attribute__((packed)) iso9660_common_volume_descriptor;

/*! \brief Primary volume descriptor
*/
typedef struct iso9660_primary_volume_descriptor {
	iso9660_common_volume_descriptor info;
	uchar volume_flags;
	char system_identifier[32];
	char volume_identifier[ISO9660_VOLUME_IDENTIFIER_LENGTH];
	uchar unused01[8];
	uint32 volume_space_size_little_endian;
	uint32 volume_space_size_big_endian;
	uchar unused02[ISO9660_ESCAPE_SEQUENCE_LENGTH];	
	uint16 volume_set_size_little_endian;
	uint16 volume_set_size_big_endian;
	uint16 volume_sequence_number_little_endian;
	uint16 volume_sequence_number_big_endian;
	uint16 logical_block_size_little_endian;
	uint16 logical_block_size_big_endian;
	uint32 path_table_size_little_endian;
	uint32 path_table_size_big_endian;
	uint32 ignored02[4];
	uchar root_directory_record[34];
	char volume_set_identifier[28];
	// Remaining bytes are disinteresting to us
} __attribute__((packed)) iso9660_primary_volume_descriptor;

typedef struct iso9660_supplementary_volume_descriptor {
	iso9660_common_volume_descriptor info;
	uchar volume_flags;
	char system_identifier[32];
	char volume_identifier[ISO9660_VOLUME_IDENTIFIER_LENGTH];
	uchar unused01[8];
	uint32 volume_space_size_little_endian;
	uint32 volume_space_size_big_endian;
	char escape_sequences[ISO9660_ESCAPE_SEQUENCE_LENGTH];	
	uint16 volume_set_size_little_endian;
	uint16 volume_set_size_big_endian;
	uint16 volume_sequence_number_little_endian;
	uint16 volume_sequence_number_big_endian;
	uint16 logical_block_size_little_endian;
	uint16 logical_block_size_big_endian;
	uint32 path_table_size_little_endian;
	uint32 path_table_size_big_endian;
	uint32 ignored02[4];
	uchar root_directory_record[34];
	char volume_set_identifier[28];
	// Remaining bytes are disinteresting to us
} __attribute__((packed)) iso9660_supplementary_volume_descriptor;

typedef struct iso9660_directory_record {
	uint8 length;
	uint8 extended_attribute_record_length;
	uint32 location_le;
	uint32 location_be;
	uint32 data_length;
	uchar ignored[14];
	uint16 volume_space_le;
} __attribute__((packed)) iso9660_directory_record;

/*! \brief Contains all the info of interest pertaining to an
	iso9660 volume.
	
	Currently supported character set encoding styles (in decreasing
	order of precedence):
	- Joliet (UCS-12 (16-bit unicode), which is converted to UTF-8)
	- iso9660 (some absurdly tiny character set, but we actually allow UTF-8)
*/
struct iso9660_info {
	iso9660_info();
	~iso9660_info();

	bool is_valid();
	
	void set_iso9660_volume_name(const char *name, uint32 length);
	void set_joliet_volume_name(const char *name, uint32 length);

	const char* get_preferred_volume_name();
		
	char *iso9660_volume_name;
	char *joliet_volume_name;
	
	void set_string(char **string, const char *new_string, uint32 new_length);
};

//----------------------------------------------------------------------------
// iso9660_info
//----------------------------------------------------------------------------

/*! \brief Creates a new iso9660_info struct with empty volume names.

	\note Use the applicable set_XYZ_volume_name() functions rather than
	messing with the volume name data members directly.
*/
iso9660_info::iso9660_info()
	: iso9660_volume_name(NULL)
	, joliet_volume_name(NULL)
{
}

/*! \brief Destroys the struct, freeing the volume name strings.
*/
iso9660_info::~iso9660_info()
{
	if (iso9660_volume_name) {
		free(iso9660_volume_name);
		iso9660_volume_name = NULL;
	}
	if (joliet_volume_name) {
		free(joliet_volume_name);
		joliet_volume_name = NULL;
	}
}

/*! \brief Returns true if a valid volume name exists.
*/
bool
iso9660_info::is_valid()
{
	return iso9660_volume_name || joliet_volume_name;
}

/*! \brief Sets the iso9660 volume name.

	\param name UTF-8 string containing the name.
	\param length The length (in bytes) of the string.
*/
void
iso9660_info::set_iso9660_volume_name(const char *name, uint32 length)
{
	set_string(&iso9660_volume_name, name, length);
}

/*! \brief Sets the Joliet volume name.

	\param name UTF-8 string containing the name.
	\param length The length (in bytes) of the string.
*/
void
iso9660_info::set_joliet_volume_name(const char *name, uint32 length)
{
	set_string(&joliet_volume_name, name, length);
}

/*! \brief Returns the volume name of highest precedence.

	Currently, the ordering is (decreasingly):
	- Joliet
	- iso9660
*/
const char*
iso9660_info::get_preferred_volume_name()
{
	if (joliet_volume_name)
		return joliet_volume_name;
	else
		return iso9660_volume_name;
}

/*! \brief Copies the given string into the old string, managing memory
	deallocation and allocation as necessary.
*/
void
iso9660_info::set_string(char **string, const char *new_string, uint32 new_length)
{
	TRACE(("%s: iso9660_info::set_string(%p (`%s'), `%s', %ld)\n", kModuleDebugName,
	       string, *string, new_string, new_length));
	if (string) {
		char *&old_string = *string; 
		if (old_string)
			free(old_string);
		if (new_string) {
			old_string = (char*)malloc(new_length+1);
			if (old_string) {
				strncpy(old_string, new_string, new_length);
				old_string[new_length] = 0;
			}
		} else {
			old_string = NULL;
		}
	}			
}

//----------------------------------------------------------------------------
// C functions
//----------------------------------------------------------------------------

/*! \brief Converts the given unicode character to utf8.

	Courtesy Mr. Axel Dörfler.
	
	\todo Once OpenTracker's locale kit is done, perhaps that functionality
	should be used rather than outright stealing the code.
*/
static
void
unicode_to_utf8(uint32 c, char **out)
{
	char *s = *out;

	if (c < 0x80)
		*(s++) = c;
	else if (c < 0x800) {
		*(s++) = 0xc0 | (c>>6);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c < 0x10000) {
		*(s++) = 0xe0 | (c>>12);
		*(s++) = 0x80 | ((c>>6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c <= 0x10ffff) {
		*(s++) = 0xf0 | (c>>18);
		*(s++) = 0x80 | ((c>>12) & 0x3f);
		*(s++) = 0x80 | ((c>>6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	}
	*out = s;
}

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
	
	TRACE(("%s  volume descriptor type    == %d (%s)\n", indent,
	       common->volume_descriptor_type,
	       volume_descriptor_type_to_string((iso9660_volume_descriptor_type)common->volume_descriptor_type)));
	TRACE(("%s  standard identifier       == %.5s (%s)\n", indent, common->standard_identifier,
	       strncmp(common->standard_identifier, kISO9660Signature, 5) == 0 ? "valid" : "INVALID"));
	TRACE(("%s  volume descriptor version == %d\n", indent, common->volume_descriptor_version));
}

static
void
dump_directory_record(iso9660_directory_record *record, const char *indent);

static
void
dump_primary_volume_descriptor(iso9660_primary_volume_descriptor *primary, const char *indent,
                              bool print_header)
{
	if (print_header)
		TRACE(("%siso9660_primary_volume_descriptor:\n", indent));
	
	dump_common_volume_descriptor(&(primary->info), indent, false);
	TRACE(("%s  volume identifier         == `%.32s'\n", indent,
	       primary->volume_identifier));
	TRACE(("%s  volume space size         == %ld\n", indent,
	       primary->volume_space_size_little_endian));
	TRACE(("%s  volume set size           == %d\n", indent,
	       primary->volume_set_size_little_endian));
	TRACE(("%s  volume sequence number    == %d\n", indent,
	       primary->volume_sequence_number_little_endian));
	TRACE(("%s  logical block size        == %d\n", indent,
	       primary->logical_block_size_little_endian));
	TRACE(("%s  path table size           == %ld\n", indent,
	       primary->path_table_size_little_endian));
	TRACE(("%s  volume set identifier     == %.28s\n", indent,
	       primary->volume_set_identifier));
	dump_directory_record((iso9660_directory_record*)primary->root_directory_record, indent);
}

static
void
dump_supplementary_volume_descriptor(iso9660_supplementary_volume_descriptor *supplementary, const char *indent,
                              bool print_header)
{
	if (print_header)
		TRACE(("%siso9660_supplementary_volume_descriptor:\n", indent));
		
	dump_primary_volume_descriptor((iso9660_primary_volume_descriptor*)supplementary, indent, false);
	TRACE(("%s  escape sequences          ==", indent));
	for (int i = 0; i < ISO9660_ESCAPE_SEQUENCE_LENGTH; i++) {
		TRACE((" %2x", supplementary->escape_sequences[i]));
		if (i == ISO9660_ESCAPE_SEQUENCE_LENGTH/2-1)
			TRACE(("\n                                "));
	}
	TRACE(("\n"));
}
		
static
void
dump_directory_record(iso9660_directory_record *record, const char *indent)
{
	TRACE(("%s  root directory record:\n", indent));
	TRACE(("%s    length                    == %d\n", indent, record->length));
	TRACE(("%s    location                  == %ld\n", indent, record->location_le));
	TRACE(("%s    data length               == %ld\n", indent, record->data_length));
	TRACE(("%s    volume sequence number    == %d\n", indent, record->volume_space_le));
}

static
status_t
check_common_volume_descriptor(iso9660_common_volume_descriptor *common)
{
	status_t error = common ? B_OK : B_BAD_VALUE;
	if (!error) {
		error = strncmp(common->standard_identifier, kISO9660Signature,
		                5) == 0 ? B_OK : B_BAD_DATA;
	}
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
read_block(fs_get_buffer get_buffer, struct fs_buffer_cache *cache,
		   off_t offset, size_t size, uchar **block)
{
	size_t actualSize = 0;
	status_t error = get_buffer(cache, offset, size, (void**)block,
								&actualSize);
	if (error == B_OK && actualSize != size) {
		error = B_ERROR;
		free(*block);
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
                    float *priority, fs_get_buffer get_buffer,
					struct fs_buffer_cache *cache)
{
	bool result = false;
	uchar *buffer = NULL;
	uint32 blockSize = partitionInfo->info.logical_block_size;
	bool exit = false;
	// The first 16 blocks are for "system use" only, and thus are
	// irrelevant to us and generally just zeros
	off_t offset = 16 * blockSize;
	status_t error = B_OK;
	
	TRACE(("%s: identify(%d, %p)\n", kModuleDebugName, deviceFD,
		   partitionInfo));
		   
	iso9660_info info;
		   
	// Read through the volume descriptors looking for a primary descriptor.
	// If for some reason there are more than one primary descriptor, the
	// volume name from the last encountered descriptor will be used.
	while (!error && !exit) {// && count++ < 10) {
		iso9660_common_volume_descriptor *common = NULL;
		
		// Read the block containing the current descriptor
		error = read_block(get_buffer, cache, offset, blockSize, &buffer);
		offset += blockSize;
		if (!error) {
			common = (iso9660_common_volume_descriptor*)buffer;
			error = check_common_volume_descriptor(common);
//			dump_common_volume_descriptor(common, "", true);
		}
		
		// Handle each type of descriptor appropriately
		if (!error) {
			TRACE(("%s: found %s descriptor\n", kModuleDebugName,
			       volume_descriptor_type_to_string((iso9660_volume_descriptor_type)common->volume_descriptor_type)));

			switch (common->volume_descriptor_type) {
				case ISO9660VD_BOOT:
					break;
					
				case ISO9660VD_PRIMARY:
				{
					int i;
					iso9660_primary_volume_descriptor *primary = (iso9660_primary_volume_descriptor*)buffer;
					
					dump_primary_volume_descriptor(primary, "  ", true);
					
					// Cut off any trailing spaces from the volume id. Note
					// that this allows for spaces INSIDE the volume id, even
					// though that's not technically allowed by the standard;
					// this was necessary to support certain RedHat 6.2 CD-ROMs
					// from a certain Linux company who shall remain unnamed. ;-)
					for (i = ISO9660_VOLUME_IDENTIFIER_LENGTH-1; i >= 0; i--) {
						if (primary->volume_identifier[i] != 0x20)
							break;
					}

					// Give a holler if the iso9660 name is already set
					if (info.iso9660_volume_name) {
						char str[ISO9660_VOLUME_IDENTIFIER_LENGTH+1];
						strncpy(str, primary->volume_identifier, i+1);
						str[i+1] = 0;
						TRACE(("%s: duplicate iso9660 volume name found, using latter (`%s') "
						       "instead of former (`%s')\n", kModuleDebugName, str,
						       info.iso9660_volume_name));
					}
					
					info.set_iso9660_volume_name(primary->volume_identifier, i+1);
					break;
				}
					
				case ISO9660VD_SUPPLEMENTARY:
				{
					iso9660_supplementary_volume_descriptor *supplementary = (iso9660_supplementary_volume_descriptor*)buffer;
					dump_supplementary_volume_descriptor((iso9660_supplementary_volume_descriptor*)supplementary, "  ", true);
					
					// Copy and null terminate the escape sequences
					char escapes[ISO9660_ESCAPE_SEQUENCE_LENGTH+1];
					strncpy(escapes, supplementary->escape_sequences, ISO9660_ESCAPE_SEQUENCE_LENGTH);
					escapes[ISO9660_ESCAPE_SEQUENCE_LENGTH] = 0;
					
					// Check for a Joliet VD
					if (strstr(escapes, "%/@") || strstr(escapes, "%/C") || strstr(escapes, "%/E")) {
						char str[(ISO9660_VOLUME_IDENTIFIER_LENGTH*3/2)+1];
							// Since we're dealing with 16-bit Unicode, each UTF-8 sequence
							// will be at most 3 bytes long. So we need 3/2 as many chars as
							// we start out with. 
						char *str_iterator = str;
						uint16 ch;
						
						// Walk thru the unicode volume name, converting to utf8 as we go.
						for (int i = 0;
						       (ch = B_BENDIAN_TO_HOST_INT16(((uint16*)supplementary->volume_identifier)[i]))
						       && i < ISO9660_VOLUME_IDENTIFIER_LENGTH;
						         i++) {
							// Give a warning if the character is technically illegal
							if (   ch <= 0x001F
							    || ch == 0x002A
							    || ch == 0x002F
							    || ch == 0x003A
							    || ch == 0x003B
							    || ch == 0x003F
							    || ch == 0x005C)
							{
								TRACE(("%s: warning: illegal Joliet character found: 0%4x\n",
								       kModuleDebugName, ch));
							}
	
							// Convert to utf-8
							unicode_to_utf8(ch, &str_iterator);
						}
						*str_iterator = 0;
						
						// Give a holler if the joliet name is already set
						if (info.joliet_volume_name) {
							TRACE(("%s: duplicate joliet volume name found, using latter (`%s') "
							       "instead of former (`%s')\n", kModuleDebugName, str,
							       info.joliet_volume_name));
						}
						
						info.set_joliet_volume_name(str, strlen(str));							
					} // end "if Joliet VD"
					break;
				}
					
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
			if (info.is_valid()) {
				result = true;
				if (partitionInfo->file_system_short_name)
					strcpy(partitionInfo->file_system_short_name, "iso9660");
				if (partitionInfo->file_system_long_name)
					strcpy(partitionInfo->file_system_long_name, "iso9660 CD-ROM File System");
				partitionInfo->file_system_flags = B_FS_IS_PERSISTENT;
				if (priority)
					*priority = 0;
				// Copy the volume name of highest precedence
				if (partitionInfo->volume_name) {
					TRACE(("%s: iso9660 name: `%s'\n", kModuleDebugName, info.iso9660_volume_name));
					TRACE(("%s: joliet name:  `%s'\n", kModuleDebugName, info.joliet_volume_name));
					const char *name = info.get_preferred_volume_name();
					int length = strlen(name);
					if (length > B_FILE_NAME_LENGTH-1)
						length = B_FILE_NAME_LENGTH-1;
					strncpy(partitionInfo->volume_name, name, length);
					partitionInfo->volume_name[length] = 0;
				}
			}			
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

