/*
 * Copyright 2007-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Tyler Dauwalder.
 *
 * This file may be used under the terms of the MIT License.
 */

/*!
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

#include "iso9660_identify.h"

#ifndef FS_SHELL
#	include <errno.h>
#	include <stdlib.h>
#	include <string.h>
#	include <unistd.h>
#	include <stdio.h>

#	include <ByteOrder.h>
#	include <fs_info.h>
#	include <KernelExport.h>
#endif

#include "iso9660.h"

//#define TRACE(x) ;
#define TRACE(x) dprintf x


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
} iso9660_descriptor_type;

/*! \brief The portion of the volume descriptor common to all
    descriptor types.
*/
typedef struct iso9660_common_descriptor {
	uint8	type;
	char	standard_identifier[5];	// should be 'CD001'
	uint8	version;
	// Remaining bytes are unused
} __attribute__((packed)) iso9660_common_volume_descriptor;

typedef struct iso9660_volume_descriptor {
	iso9660_common_descriptor common;
	uint8	flags;
	char	system_identifier[32];
	char	identifier[ISO9660_VOLUME_IDENTIFIER_LENGTH];
	uint8	_reserved0[8];
	uint32	size;
	uint32	size_big_endian;
	char	escape_sequences[ISO9660_ESCAPE_SEQUENCE_LENGTH];
				// unused on primary descriptor
	uint16	set_size;
	uint16	set_size_big_endian;
	uint16	sequence_number;
	uint16	sequence_number_big_endian;
	uint16	logical_block_size;
	uint16	logical_block_size_big_endian;
	uint32	path_table_size;
	uint32	path_table_size_big_endian;
	uint32	_reserved1[4];
	uint8	root_directory_record[34];
	char	set_identifier[28];
	// Remaining bytes are disinteresting to us
} __attribute__((packed)) iso9660_volume_descriptor;

typedef struct iso9660_directory_record {
	uint8	length;
	uint8	extended_attribute_record_length;
	uint32	location;
	uint32	location_big_endian;
	uint32	data_length;
	uint8	_reserved[14];
	uint16	volume_space;
} __attribute__((packed)) iso9660_directory_record;


static void dump_directory_record(iso9660_directory_record *record,
	const char *indent);


//	#pragma mark -


/*! \brief Creates a new iso9660_info struct with empty volume names.

	\note Use the applicable set_XYZ_volume_name() functions rather than
	messing with the volume name data members directly.
*/
iso9660_info::iso9660_info()
	:
	iso9660_name(NULL),
	joliet_name(NULL)
{
}


iso9660_info::~iso9660_info()
{
	free(iso9660_name);
	free(joliet_name);
}


/*! \brief Returns true if a valid volume name exists.
*/
bool
iso9660_info::IsValid()
{
	return iso9660_name != NULL || joliet_name != NULL;
}


/*! \brief Sets the iso9660 volume name.

	\param name UTF-8 string containing the name.
	\param length The length (in bytes) of the string.
*/
void
iso9660_info::SetISO9660Name(const char *name, uint32 length)
{
	_SetString(&iso9660_name, name, length);
}


/*! \brief Sets the Joliet volume name.

	\param name UTF-8 string containing the name.
	\param length The length (in bytes) of the string.
*/
void
iso9660_info::SetJolietName(const char *name, uint32 length)
{
	_SetString(&joliet_name, name, length);
}


/*! \brief Returns the volume name of highest precedence.

	Currently, the ordering is (decreasingly):
	- Joliet
	- iso9660
*/
const char*
iso9660_info::PreferredName()
{
	if (joliet_name)
		return joliet_name;

	return iso9660_name;
}


/*! \brief Copies the given string into the old string, managing memory
	deallocation and allocation as necessary.
*/
void
iso9660_info::_SetString(char **string, const char *newString,
	uint32 newLength)
{
	if (string == NULL)
		return;

	TRACE(("iso9660_info::set_string(%p ('%s'), '%s', %u)\n", string,
		*string, newString, (unsigned)newLength));

	char *&oldString = *string;
	free(oldString);

	if (newString) {
		oldString = (char*)malloc(newLength + 1);
		if (oldString != NULL) {
			memcpy(oldString, newString, newLength);
			oldString[newLength] = '\0';
		}
	} else
		oldString = NULL;
}


//	#pragma mark - C functions


/*! \brief Converts the given unicode character to utf8.
*/
static void
unicode_to_utf8(uint32 c, char **out)
{
	char *s = *out;

	if (c < 0x80)
		*(s++) = c;
	else if (c < 0x800) {
		*(s++) = 0xc0 | (c >> 6);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c < 0x10000) {
		*(s++) = 0xe0 | (c >> 12);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c <= 0x10ffff) {
		*(s++) = 0xf0 | (c >> 18);
		*(s++) = 0x80 | ((c >> 12) & 0x3f);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	}
	*out = s;
}


static const char*
descriptor_type_to_string(iso9660_descriptor_type type)
{
	switch (type) {
		case ISO9660VD_BOOT:
			return "boot";
		case ISO9660VD_PRIMARY:
			return "primary";
		case ISO9660VD_SUPPLEMENTARY:
			return "supplementary";
		case ISO9660VD_PARTITION:
			return "partiton";
		case ISO9660VD_TERMINATOR:
			return "terminator";
		default:
			return "invalid";
	}
}


static void
dump_common_descriptor(iso9660_common_descriptor *common,
	const char *indent, bool printHeader)
{
	if (printHeader)
		TRACE(("%siso9660_common_descriptor:\n", indent));

	TRACE(("%s  volume descriptor type: %d (%s)\n", indent,
		common->type, descriptor_type_to_string(
			(iso9660_descriptor_type)common->type)));
	TRACE(("%s  standard identifier:    %.5s (%s)\n", indent,
		common->standard_identifier,
		strncmp(common->standard_identifier, kISO9660Signature, 5) == 0
			? "valid" : "INVALID"));
	TRACE(("%s  version:                %d\n", indent, common->version));
}


static void
dump_primary_descriptor(iso9660_volume_descriptor *primary,
	const char *indent, bool printHeader)
{
	if (printHeader)
		TRACE(("%siso9660_primary_descriptor:\n", indent));

	dump_common_descriptor(&primary->common, indent, false);
	TRACE(("%s  identifier:             '%.32s'\n", indent,
		primary->identifier));
	TRACE(("%s  size:                   %d\n", indent,
		(int)B_LENDIAN_TO_HOST_INT32(primary->size)));
	TRACE(("%s  set size:               %d\n", indent,
		(int)B_LENDIAN_TO_HOST_INT32(primary->set_size)));
	TRACE(("%s  sequence number:        %d\n", indent,
		(int)B_LENDIAN_TO_HOST_INT32(primary->sequence_number)));
	TRACE(("%s  logical block size:     %d\n", indent,
		(int)B_LENDIAN_TO_HOST_INT32(primary->logical_block_size)));
	TRACE(("%s  path table size:        %d\n", indent,
		(int)B_LENDIAN_TO_HOST_INT32(primary->path_table_size)));
	TRACE(("%s  set identifier:         %.28s\n", indent,
		primary->set_identifier));
	dump_directory_record((iso9660_directory_record*)
		primary->root_directory_record, indent);
}


static void
dump_supplementary_descriptor(iso9660_volume_descriptor *supplementary,
	const char *indent, bool printHeader)
{
	if (printHeader)
		TRACE(("%siso9660_supplementary_descriptor:\n", indent));

	dump_primary_descriptor(supplementary, indent, false);
	TRACE(("%s  escape sequences:      ", indent));
	for (int i = 0; i < ISO9660_ESCAPE_SEQUENCE_LENGTH; i++) {
		TRACE((" %2x", supplementary->escape_sequences[i]));
		if (i == ISO9660_ESCAPE_SEQUENCE_LENGTH / 2 - 1)
			TRACE(("\n                          "));
	}
	TRACE(("\n"));
}


static void
dump_directory_record(iso9660_directory_record *record, const char *indent)
{
	TRACE(("%s  root directory record:\n", indent));
	TRACE(("%s    length:               %d\n", indent, record->length));
	TRACE(("%s    location:             %d\n", indent,
		(int)B_LENDIAN_TO_HOST_INT32(record->location)));
	TRACE(("%s    data length:          %d\n", indent,
		(int)B_LENDIAN_TO_HOST_INT32(record->data_length)));
	TRACE(("%s    volume space:         %d\n", indent,
		B_LENDIAN_TO_HOST_INT16(record->volume_space)));
}


static status_t
check_common_descriptor(iso9660_common_descriptor *common)
{
	if (common == NULL)
		return B_BAD_VALUE;

	return strncmp(common->standard_identifier, kISO9660Signature, 5) == 0
		? B_OK : B_BAD_DATA;
}


//	#pragma mark - Public functions


// iso9660_fs_identify
/*! \brief Returns true if the given partition is a valid iso9660 partition.

	See fs_identify_hook() for more information.

	\todo Fill in partitionInfo->mounted_at with something useful.
*/
status_t
iso9660_fs_identify(int deviceFD, iso9660_info *info)
{
	char buffer[ISO_PVD_SIZE];
	bool exit = false;
	bool found = false;
	status_t error = B_OK;

	TRACE(("identify(%d, %p)\n", deviceFD, info));
	off_t offset = 0x8000;

	// Read through the volume descriptors looking for a primary descriptor.
	// If for some reason there are more than one primary descriptor, the
	// volume name from the last encountered descriptor will be used.
	while (!error && !exit) {// && count++ < 10) {
		iso9660_common_descriptor *common = NULL;

		// Read the block containing the current descriptor
		error = read_pos(deviceFD, offset, (void *)&buffer, ISO_PVD_SIZE);
		offset += ISO_PVD_SIZE;
		if (error < ISO_PVD_SIZE)
			break;

		common = (iso9660_common_descriptor*)buffer;
		error = check_common_descriptor(common);
		if (error < B_OK)
			break;

//		dump_common_descriptor(common, "", true);

		// Handle each type of descriptor appropriately
		TRACE(("found %s descriptor\n", descriptor_type_to_string(
			(iso9660_descriptor_type)common->type)));
		found = true;

		switch (common->type) {
			case ISO9660VD_BOOT:
				break;

			case ISO9660VD_PRIMARY:
			{
				iso9660_volume_descriptor *primary
					= (iso9660_volume_descriptor*)buffer;
				int i;

				dump_primary_descriptor(primary, "  ", true);

				// Cut off any trailing spaces from the volume id. Note
				// that this allows for spaces INSIDE the volume id, even
				// though that's not technically allowed by the standard;
				// this was necessary to support certain RedHat 6.2 CD-ROMs
				// from a certain Linux company who shall remain unnamed. ;-)
				for (i = ISO9660_VOLUME_IDENTIFIER_LENGTH - 1; i >= 0;
						i--) {
					if (primary->identifier[i] != 0x20)
						break;
				}

				// Give a holler if the iso9660 name is already set
				if (info->iso9660_name) {
					char name[ISO9660_VOLUME_IDENTIFIER_LENGTH + 1];
					strlcpy(name, primary->identifier, i + 1);
					TRACE(("duplicate iso9660 volume name found, using "
						"latter (`%s') instead of former (`%s')\n", name,
						info->iso9660_name));
				}

				info->SetISO9660Name(primary->identifier, i + 1);
				info->max_blocks = B_LENDIAN_TO_HOST_INT32(primary->set_size);
				break;
			}

			case ISO9660VD_SUPPLEMENTARY:
			{
				iso9660_volume_descriptor *supplementary
					= (iso9660_volume_descriptor*)buffer;
				dump_supplementary_descriptor(supplementary, "  ", true);

				// Copy and null terminate the escape sequences
				char escapes[ISO9660_ESCAPE_SEQUENCE_LENGTH + 1];
				strlcpy(escapes, supplementary->escape_sequences,
					ISO9660_ESCAPE_SEQUENCE_LENGTH + 1);

				// Check for a Joliet VD
				if (strstr(escapes, "%/@") || strstr(escapes, "%/C")
					|| strstr(escapes, "%/E")) {
					char name[(ISO9660_VOLUME_IDENTIFIER_LENGTH * 3 / 2) + 1];
						// Since we're dealing with 16-bit Unicode, each
						// UTF-8 sequence will be at most 3 bytes long.
					char *pos = name;
					uint16 ch;

					// Walk thru the unicode volume name, converting to utf8 as we go.
					for (int i = 0; (ch = B_BENDIAN_TO_HOST_INT16(
								((uint16*)supplementary->identifier)[i]))
							&& i < ISO9660_VOLUME_IDENTIFIER_LENGTH; i++) {
						// Give a warning if the character is technically
						// illegal
						if (ch <= 0x001F || ch == '*' || ch == '/'
						    || ch == ':' || ch == ';'
						    || ch == '?' || ch == '\\') {
							TRACE(("warning: illegal Joliet character "
								"found: 0%4x\n", ch));
						}

						// Convert to utf-8
						unicode_to_utf8(ch, &pos);
					}
					pos[0] = '\0';

					// Give a holler if the joliet name is already set
					if (info->joliet_name) {
						TRACE(("duplicate joliet volume name found, using "
							"latter (`%s') instead of former (`%s')\n",
							name, info->joliet_name));
					}

					info->SetJolietName(name, pos - name);
				}
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

	return found ? B_OK : error;
}

