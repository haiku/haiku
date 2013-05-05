/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
 * This file may be used under the terms of the MIT License.
 */
#ifndef _UDF_DISK_STRUCTURES_H
#define _UDF_DISK_STRUCTURES_H

#include <string.h>

#include <ByteOrder.h>
#include <SupportDefs.h>

#include "UdfDebug.h"
#include "Utils.h"

#include "Array.h"

#include <util/kernel_cpp.h>

/*! \file UdfStructures.h

	\brief UDF on-disk data structure declarations
	
	UDF is a specialization of the ECMA-167 standard. For the most part,
	ECMA-167 structures are used by UDF with special restrictions. In a
	few instances, UDF introduces its own structures to augment those
	supplied by ECMA-167; those structures are clearly marked.
	
	For UDF info: <a href='http://www.osta.org'>http://www.osta.org</a>
	For ECMA info: <a href='http://www.ecma-international.org'>http://www.ecma-international.org</a>

	For lack of a better place to store this info, the structures that
	are allowed to have length greater than the logical block size are
	as follows (other length restrictions may be found in UDF-2.01 5.1):
	- \c logical_volume_descriptor
	- \c unallocated_space_descriptor
	- \c logical_volume_integrity_descriptor
	- \c space_bitmap_descriptor

	Other links of interest:
	- <a href='http://www.extra.research.philips.com/udf/'>Philips UDF verifier</a>
	- <a href='http://www.hi-ho.ne.jp/y-komachi/committees/fpro/fpro.htm'>Possible test disc image generator (?)</a>
*/

//----------------------------------------------------------------------
// ECMA-167 Part 1
//----------------------------------------------------------------------

/*! \brief Character set specifications

	The character_set_info field shall be set to the ASCII string
	"OSTA Compressed Unicode" (padded right with NULL chars).
	
	See also: ECMA 167 1/7.2.1, UDF-2.01 2.1.2
*/
struct charspec {
public:
	charspec(uint8 type = 0, const char *info = NULL);

	void dump() const;

	uint8 character_set_type() const { return _character_set_type; } 
	const char* character_set_info() const { return _character_set_info; }
	char* character_set_info() { return _character_set_info; }
	
	void set_character_set_type(uint8 type) { _character_set_type = type; }
	void set_character_set_info(const char *info);
private:
	uint8 _character_set_type;	//!< to be set to 0 to indicate CS0
	char _character_set_info[63];	//!< "OSTA Compressed Unicode"
} __attribute__((packed));

extern const charspec kCs0CharacterSet;

/*! \brief Date and time stamp 

	See also: ECMA 167 1/7.3, UDF-2.01 2.1.4
*/
class timestamp {
private:
	union type_and_timezone_accessor {
		uint16 type_and_timezone;
		struct {
			uint16 timezone:12,
			       type:4;
		} bits;
	};

public:
	timestamp() { _clear(); }
	timestamp(time_t time);	

	void dump() const;

	// Get functions
	uint16 type_and_timezone() const { return B_LENDIAN_TO_HOST_INT16(_type_and_timezone); }
	uint8 type() const {
		type_and_timezone_accessor t;
		t.type_and_timezone = type_and_timezone();
		return t.bits.type;
	}
	int16 timezone() const {
		type_and_timezone_accessor t;
		t.type_and_timezone = type_and_timezone();
		int16 result = t.bits.timezone;
		// Fill the lefmost bits with ones if timezone is negative
		result <<= 4;
		result >>= 4;	
		return result;
	}
	uint16 year() const { return B_LENDIAN_TO_HOST_INT16(_year); }
	uint8 month() const { return _month; }
	uint8 day() const { return _day; }
	uint8 hour() const { return _hour; }
	uint8 minute() const { return _minute; }
	uint8 second() const { return _second; }
	uint8 centisecond() const { return _centisecond; }
	uint8 hundred_microsecond() const { return _hundred_microsecond; }
	uint8 microsecond() const { return _microsecond; }
	
	// Set functions
	void set_type_and_timezone(uint16 type_and_timezone) { 
		_type_and_timezone = B_HOST_TO_LENDIAN_INT16(type_and_timezone); }
	void set_type(uint8 type) {
		type_and_timezone_accessor t;
		t.type_and_timezone = type_and_timezone();
		t.bits.type = type;
		set_type_and_timezone(t.type_and_timezone);
	}
	void set_timezone(int16 tz) {
		type_and_timezone_accessor t;
		t.type_and_timezone = type_and_timezone();
		t.bits.timezone = tz;
		set_type_and_timezone(t.type_and_timezone);
	}
	void set_year(uint16 year) { _year = B_HOST_TO_LENDIAN_INT16(year); }
	void set_month(uint8 month) { _month = month; }
	void set_day(uint8 day) { _day = day; }
	void set_hour(uint8 hour) { _hour = hour; }
	void set_minute(uint8 minute) { _minute = minute; }
	void set_second(uint8 second) { _second = second; }
	void set_centisecond(uint8 centisecond) { _centisecond = centisecond; }
	void set_hundred_microsecond(uint8 hundred_microsecond) { 
		_hundred_microsecond = hundred_microsecond; }
	void set_microsecond(uint8 microsecond) { _microsecond = microsecond; }
private:
	void _clear();
	
	uint16 _type_and_timezone;
	uint16 _year;
	uint8 _month;
	uint8 _day;
	uint8 _hour;
	uint8 _minute;
	uint8 _second;
	uint8 _centisecond;
	uint8 _hundred_microsecond;
	uint8 _microsecond;

} __attribute__((packed));


/*! \brief UDF ID Identify Suffix

	See also: UDF 2.50 2.1.5.3
*/
struct udf_id_suffix {
public:
	udf_id_suffix(uint16 udfRevision, uint8 os_class, uint8 os_identifier);

	//! Note that revision 2.50 is denoted by 0x0250.
	uint16 udf_revision() const { return _udf_revision; }
	uint8 os_class() const { return _os_class; }
	uint8 os_identifier() const { return _os_identifier; }

	void set_os_class(uint8 os_class) { _os_class = os_class; }
	void set_os_identifier(uint8 identifier) { _os_identifier = identifier; }
private:
	uint16 _udf_revision;
	uint8 _os_class;
	uint8 _os_identifier;
	array<uint8, 4> _reserved;
};

/*! \brief Implementation ID Identify Suffix

	See also: UDF 2.50 2.1.5.3
*/
struct implementation_id_suffix {
public:
	implementation_id_suffix(uint8 os_class, uint8 os_identifier);

	uint8 os_class() const { return _os_class; }
	uint8 os_identifier() const { return _os_identifier; }

	void set_os_class(uint8 os_class) { _os_class = os_class; }
	void set_os_identifier(uint8 identifier) { _os_identifier = identifier; }
private:
	uint8 _os_class;
	uint8 _os_identifier;
	array<uint8, 6> _implementation_use;
};

/*! \brief Operating system classes for implementation_id_suffixes

	See also: Udf 2.50 6.3
*/
enum {
	OS_UNDEFINED = 0,
	OS_DOS,
	OS_OS2,
	OS_MACOS,
	OS_UNIX,
	OS_WIN9X,
	OS_WINNT,
	OS_OS400,
	OS_BEOS,
	OS_WINCE
};
	
/*! \brief BeOS operating system classes identifiers for implementation_id_suffixes

	See also: Udf 2.50 6.3
*/
enum {
	BEOS_GENERIC = 0,
	BEOS_OPENBEOS = 1	// not part of the standard, but perhaps someday. :-)
};

/*! \brief Domain ID Identify Suffix

	See also: UDF 2.50 2.1.5.3
*/
struct domain_id_suffix {
public:
	domain_id_suffix(uint16 udfRevision, uint8 domainFlags);

	//! Note that revision 2.50 is denoted by 0x0250.
	uint16 udf_revision() const { return _udf_revision; }
	uint8 domain_flags() const { return _domain_flags; }

	void set_udf_revision(uint16 revision) { _udf_revision = B_HOST_TO_LENDIAN_INT16(revision); }
	void set_domain_flags(uint8 flags) { _domain_flags = flags; }
private:
	uint16 _udf_revision;
	uint8 _domain_flags;
	array<uint8, 5> _reserved;
};

/*! \brief Domain flags

	See also: UDF 2.50 2.1.5.3
*/
enum {
	DF_HARD_WRITE_PROTECT = 0x01,
	DF_SOFT_WRITE_PROTECT = 0x02
};
	
/*! \brief Identifier used to designate the implementation responsible
	for writing associated data structures on the medium.
	
	See also: ECMA 167 1/7.4, UDF 2.01 2.1.5
*/
struct entity_id {
public:
	static const int kIdentifierLength = 23;
	static const int kIdentifierSuffixLength = 8;

	entity_id(uint8 flags = 0, const char *identifier = NULL,
	          uint8 *identifier_suffix = NULL);
	entity_id(uint8 flags, const char *identifier,
	          const udf_id_suffix &suffix);
	entity_id(uint8 flags, const char *identifier,
	          const implementation_id_suffix &suffix);
	entity_id(uint8 flags, const char *identifier,
	          const domain_id_suffix &suffix);
	
	void dump() const;
	bool matches(const entity_id &id) const;

	// Get functions
	uint8 flags() const { return _flags; }
	const char* identifier() const { return _identifier; }
	char* identifier() { return _identifier; }
	const array<uint8, kIdentifierSuffixLength>& identifier_suffix() const { return _identifier_suffix; }
	array<uint8, kIdentifierSuffixLength>& identifier_suffix() { return _identifier_suffix; }

	// Set functions
	void set_flags(uint8 flags) { _flags = flags; }
private:
	uint8 _flags;
	char _identifier[kIdentifierLength];
	array<uint8, kIdentifierSuffixLength> _identifier_suffix;
} __attribute__((packed));

extern entity_id kMetadataPartitionMapId;
extern entity_id kSparablePartitionMapId;
extern entity_id kVirtualPartitionMapId;
extern entity_id kImplementationId;
extern entity_id kPartitionContentsId1xx;
extern entity_id kPartitionContentsId2xx;
extern entity_id kUdfId;
extern entity_id kLogicalVolumeInfoId150;
extern entity_id kLogicalVolumeInfoId201;
extern entity_id kDomainId150;
extern entity_id kDomainId201;
extern void init_entities(void);

//----------------------------------------------------------------------
// ECMA-167 Part 2
//----------------------------------------------------------------------


/*! \brief Header for volume structure descriptors

	Each descriptor consumes an entire block. All unused trailing
	bytes in the descriptor should be set to 0.
	
	The following descriptors contain no more information than
	that contained in the header:
	
	- BEA01:
	  - type: 0
	  - id: "BEA01"
	  - version: 1

	- TEA01:
	  - type: 0
	  - id: "TEA01"
	  - version: 1
	
	- NSR03:
	  - type: 0
	  - id: "NSR03"
	  - version: 1

	See also: ECMA 167 2/9.1
*/	
struct volume_structure_descriptor_header {
public:
	volume_structure_descriptor_header(uint8 type, const char *id, uint8 version);
	
	uint8 type;
	char id[5];
	uint8 version;
	
	bool id_matches(const char *id);
} __attribute__((packed));

// Volume structure descriptor ids 
extern const char* kVSDID_BEA;
extern const char* kVSDID_TEA;
extern const char* kVSDID_BOOT;
extern const char* kVSDID_ISO;
extern const char* kVSDID_ECMA167_2;
extern const char* kVSDID_ECMA167_3;
extern const char* kVSDID_ECMA168;

//----------------------------------------------------------------------
// ECMA-167 Part 3
//----------------------------------------------------------------------


/*! \brief Location and length of a contiguous chunk of data on the volume.

	\c _location is an absolute block address.

	See also: ECMA 167 3/7.1
*/
struct extent_address {
public:
	extent_address(uint32 location = 0, uint32 length = 0);

	void dump() const;

	uint32 length() const { return B_LENDIAN_TO_HOST_INT32(_length); }	
	uint32 location() const { return B_LENDIAN_TO_HOST_INT32(_location); }
	
	void set_length(int32 length) { _length = B_HOST_TO_LENDIAN_INT32(length); }
	void set_location(int32 location) { _location = B_HOST_TO_LENDIAN_INT32(location); }
private:
	uint32 _length;
	uint32 _location;
} __attribute__((packed));


/*! \brief Location of a logical block within a logical volume.

	See also: ECMA 167 4/7.1
*/
struct logical_block_address {
public:
	void dump() const;
	logical_block_address(uint16 partition = 0, uint32 block = 0);

	uint32 block() const { return B_LENDIAN_TO_HOST_INT32(_block); }
	uint16 partition() const { return B_LENDIAN_TO_HOST_INT16(_partition); }
	
	void set_block(uint32 block) { _block = B_HOST_TO_LENDIAN_INT32(block); }
	void set_partition(uint16 partition) { _partition = B_HOST_TO_LENDIAN_INT16(partition); }
	
private:
	uint32 _block;	//!< Block location relative to start of corresponding partition
	uint16 _partition;	//!< Numeric partition id within logical volume
} __attribute__((packed));

/*! \brief Extent types used in short_address, long_address,
	and extended_address.
	
	See also: ECMA-167 4/14.14.1.1
*/
enum extent_type {
	EXTENT_TYPE_RECORDED = 0,	//!< Allocated and recorded
	EXTENT_TYPE_ALLOCATED,		//!< Allocated but unrecorded
	EXTENT_TYPE_UNALLOCATED,	//!< Unallocated and unrecorded
	EXTENT_TYPE_CONTINUATION,	//!< Specifies next extent of descriptors
};


/*! \brief Allocation descriptor.

	See also: ECMA 167 4/14.14.1
*/
struct short_address {
private:
	union type_and_length_accessor {
		uint32 type_and_length;
		struct {
			uint32 length:30,
			       type:2;
//			uint32 type:2,
//			       length:30;
		} bits;
	};
	
public:
	void dump() const;

	uint8 type() const {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		return t.bits.type;
	}
	uint32 length() const {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		return t.bits.length;
	}
	uint32 block() const { return B_LENDIAN_TO_HOST_INT32(_block); }

	void set_type(uint8 type) {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		t.bits.type = type;
		set_type_and_length(t.type_and_length);
	}
	void set_length(uint32 length) {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		t.bits.length = length;
		set_type_and_length(t.type_and_length);
	}
	void set_block(uint32 block) { _block = B_HOST_TO_LENDIAN_INT32(block); }
private:
	uint32 type_and_length() const { return B_LENDIAN_TO_HOST_INT32(_type_and_length); }
	void set_type_and_length(uint32 value) { _type_and_length = B_HOST_TO_LENDIAN_INT32(value); }

	uint32 _type_and_length;
	uint32 _block;
} __attribute__((packed));


/*! \brief Allocation descriptor w/ 6 byte implementation use field.

	See also: ECMA 167 4/14.14.2
*/
struct long_address {
private:
	union type_and_length_accessor {
		uint32 type_and_length;
		struct {
			uint32 length:30,
			       type:2;
		} bits;
	};
	
public:
	long_address(uint16 partition = 0, uint32 block = 0, uint32 length = 0,
	             uint8 type = 0);

	void dump() const;

	uint8 type() const {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		return t.bits.type;
	}
	uint32 length() const {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		return t.bits.length;
	}
	
	uint32 block() const { return _location.block(); }
	uint16 partition() const { return _location.partition(); }
	
	const array<uint8, 6>& implementation_use() const { return _implementation_use; }
	array<uint8, 6>& implementation_use() { return _implementation_use; }
	
	uint16 flags() const { return B_LENDIAN_TO_HOST_INT16(_accessor().flags); }
	uint32 unique_id() const { return B_LENDIAN_TO_HOST_INT32(_accessor().unique_id); }

	void set_type(uint8 type) {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		t.bits.type = type;
		set_type_and_length(t.type_and_length);
	}
	void set_length(uint32 length) {
		type_and_length_accessor t;
		t.type_and_length = type_and_length();
		t.bits.length = length;
		set_type_and_length(t.type_and_length);
	}
	void set_block(uint32 block) { _location.set_block(block); }
	void set_partition(uint16 partition) { _location.set_partition(partition); }
	
	void set_flags(uint16 flags) { _accessor().flags = B_HOST_TO_LENDIAN_INT16(flags); }
	void set_unique_id(uint32 id) { _accessor().unique_id = B_HOST_TO_LENDIAN_INT32(id); }

	void set_to(uint32 block, uint16 partition, uint32 length = 1,
	       uint8 type = EXTENT_TYPE_RECORDED, uint16 flags = 0, uint32 unique_id = 0)
	{
		set_block(block);
		set_partition(partition);
		set_length(length);
		set_type(type);
		set_flags(flags);
		set_unique_id(unique_id);
	}

private:
	//! See UDF-2.50 2.3.4.3
	struct _implementation_use_accessor {
		uint16 flags;
		uint32 unique_id;
	} __attribute__((packed));
	
	_implementation_use_accessor& _accessor() { return
		*reinterpret_cast<_implementation_use_accessor*>(implementation_use().data); }
	const _implementation_use_accessor& _accessor() const { return
		*reinterpret_cast<const _implementation_use_accessor*>(implementation_use().data); }
	
	uint32 type_and_length() const { return B_LENDIAN_TO_HOST_INT32(_type_and_length); }
	void set_type_and_length(uint32 value) { _type_and_length = B_HOST_TO_LENDIAN_INT32(value); }

	uint32 _type_and_length;
	logical_block_address _location;
	array<uint8, 6> _implementation_use;
} __attribute__((packed));

/*! \brief Common tag found at the beginning of most udf descriptor structures.

	For error checking, \c descriptor_tag  structures have:
	- The disk location of the tag redundantly stored in the tag itself
	- A checksum value for the tag
	- A CRC value and length

	See also: ECMA 167 1/7.2, UDF 2.01 2.2.1, UDF 2.01 2.3.1
*/
struct descriptor_tag {
public:
	void dump() const;	

	status_t init_check(uint32 block, bool calculateCrc = true);	

	uint16 id() const { return B_LENDIAN_TO_HOST_INT16(_id); }
	uint16 version() const { return B_LENDIAN_TO_HOST_INT16(_version); }
	uint8 checksum() const { return _checksum; }
	uint16 serial_number() const { return B_LENDIAN_TO_HOST_INT16(_serial_number); }
	uint16 crc() const { return B_LENDIAN_TO_HOST_INT16(_crc); }
	uint16 crc_length() const { return B_LENDIAN_TO_HOST_INT16(_crc_length); }
	uint32 location() const { return B_LENDIAN_TO_HOST_INT32(_location); }

	void set_id(uint16 id) { _id = B_HOST_TO_LENDIAN_INT16(id); }
	void set_version(uint16 version) { _version = B_HOST_TO_LENDIAN_INT16(version); }
	void set_checksum(uint8 checksum) { _checksum = checksum; }
	void set_serial_number(uint16 serial_number) { _serial_number = B_HOST_TO_LENDIAN_INT16(serial_number); }
	void set_crc(uint16 crc) { _crc = B_HOST_TO_LENDIAN_INT16(crc); }
	void set_crc_length(uint16 crc_length) { _crc_length = B_HOST_TO_LENDIAN_INT16(crc_length); }
	void set_location(uint32 location) { _location = B_HOST_TO_LENDIAN_INT32(location); }
	
	/*! \brief Calculates and sets the crc length, crc checksumm, and
		checksum for the tag.
		
		This function should not be called until all member variables in
		the descriptor_tag's enclosing descriptor and all member variables
		in the descriptor_tag itself other than crc_length, crc, and checksum
		have been set (since the checksum is based off of values in the
		descriptor_tag, and the crc is based off the values in and the
		size of the	enclosing descriptor).
	
		\param The tag's enclosing descriptor.
		\param The size of the tag's enclosing descriptor (including the
		       tag); only necessary if different from sizeof(Descriptor).
	*/
	template <class Descriptor>
	void
	set_checksums(Descriptor &descriptor, uint16 size = sizeof(Descriptor))
	{

		// check that this tag is actually owned by
		// the given descriptor
		if (this == &descriptor.tag())
		{
			// crc_length, based off provided descriptor
			set_crc_length(size - sizeof(descriptor_tag));
			// crc
			uint16 crc = calculate_crc(reinterpret_cast<uint8*>(this)
			               + sizeof(descriptor_tag), crc_length());
			set_crc(crc);
			// checksum (which depends on the other two values)
			uint32 sum = 0;
			for (int i = 0; i <= 3; i++)
				sum += reinterpret_cast<uint8*>(this)[i];
			for (int i = 5; i <= 15; i++)
				sum += reinterpret_cast<uint8*>(this)[i];
			set_checksum(sum % 256);
		}
	}
private:
	uint16 _id;
	uint16 _version;
	uint8 _checksum;			//!< Sum modulo 256 of bytes 0-3 and 5-15 of this struct.
	uint8 _reserved;			//!< Set to #00.
	uint16 _serial_number;
	uint16 _crc;				//!< May be 0 if \c crc_length field is 0.
	/*! \brief Length of the data chunk used to calculate CRC.
	
		If 0, no CRC was calculated, and the \c crc field must be 0.
		
		According to UDF-2.01 2.3.1.2, the CRC shall be calculated for all descriptors
		unless otherwise noted, and this field shall be set to:
		
		<code>(descriptor length) - (descriptor tag length)</code>
	*/
	uint16 _crc_length;
	/*! \brief Address of this tag within its partition (for error checking).
	
		For virtually addressed structures (i.e. those accessed thru a VAT), this
		shall be the virtual address, not the physical or logical address.
	*/
	uint32 _location;		
	
} __attribute__((packed));


/*! \c descriptor_tag ::id values
*/
enum tag_id {
	TAGID_UNDEFINED	= 0,

	// ECMA 167, PART 3
	TAGID_PRIMARY_VOLUME_DESCRIPTOR,
	TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER,
	TAGID_VOLUME_DESCRIPTOR_POINTER,
	TAGID_IMPLEMENTATION_USE_VOLUME_DESCRIPTOR,
	TAGID_PARTITION_DESCRIPTOR,
	TAGID_LOGICAL_VOLUME_DESCRIPTOR,
	TAGID_UNALLOCATED_SPACE_DESCRIPTOR,
	TAGID_TERMINATING_DESCRIPTOR,
	TAGID_LOGICAL_VOLUME_INTEGRITY_DESCRIPTOR,
	
	TAGID_CUSTOM_START = 65280,
	TAGID_CUSTOM_END = 65535,
	
	// ECMA 167, PART 4
	TAGID_FILE_SET_DESCRIPTOR = 256,
	TAGID_FILE_ID_DESCRIPTOR,
	TAGID_ALLOCATION_EXTENT_DESCRIPTOR,
	TAGID_INDIRECT_ENTRY,
	TAGID_TERMINAL_ENTRY,
	TAGID_FILE_ENTRY,
	TAGID_EXTENDED_ATTRIBUTE_HEADER_DESCRIPTOR,
	TAGID_UNALLOCATED_SPACE_ENTRY,
	TAGID_SPACE_BITMAP_DESCRIPTOR,
	TAGID_PARTITION_INTEGRITY_ENTRY,
	TAGID_EXTENDED_FILE_ENTRY,
};

const char *tag_id_to_string(tag_id id);

extern const uint16 kCrcTable[256];

/*! \brief Primary volume descriptor
*/
struct primary_volume_descriptor {
public:
	void dump() const;	

	// Get functions
	const descriptor_tag & tag() const { return _tag; }
	descriptor_tag & tag() { return _tag; }

	uint32 vds_number() const { return B_LENDIAN_TO_HOST_INT32(_vds_number); }
	uint32 primary_volume_descriptor_number() const { return B_LENDIAN_TO_HOST_INT32(_primary_volume_descriptor_number); }

	const array<char, 32>& volume_identifier() const { return _volume_identifier; }
	array<char, 32>& volume_identifier() { return _volume_identifier; }
	
	uint16 volume_sequence_number() const { return B_LENDIAN_TO_HOST_INT16(_volume_sequence_number); }
	uint16 max_volume_sequence_number() const { return B_LENDIAN_TO_HOST_INT16(_max_volume_sequence_number); }
	uint16 interchange_level() const { return B_LENDIAN_TO_HOST_INT16(_interchange_level); }
	uint16 max_interchange_level() const { return B_LENDIAN_TO_HOST_INT16(_max_interchange_level); }
	uint32 character_set_list() const { return B_LENDIAN_TO_HOST_INT32(_character_set_list); }
	uint32 max_character_set_list() const { return B_LENDIAN_TO_HOST_INT32(_max_character_set_list); }

	const array<char, 128>& volume_set_identifier() const { return _volume_set_identifier; }
	array<char, 128>& volume_set_identifier() { return _volume_set_identifier; }
	
	const charspec& descriptor_character_set() const { return _descriptor_character_set; }
	charspec& descriptor_character_set() { return _descriptor_character_set; }

	const charspec& explanatory_character_set() const { return _explanatory_character_set; }
	charspec& explanatory_character_set() { return _explanatory_character_set; }

	const extent_address& volume_abstract() const { return _volume_abstract; }
	extent_address& volume_abstract() { return _volume_abstract; }
	const extent_address& volume_copyright_notice() const { return _volume_copyright_notice; }
	extent_address& volume_copyright_notice() { return _volume_copyright_notice; }

	const entity_id& application_id() const { return _application_id; }
	entity_id& application_id() { return _application_id; }
	
	const timestamp& recording_date_and_time() const { return _recording_date_and_time; }
	timestamp& recording_date_and_time() { return _recording_date_and_time; }

	const entity_id& implementation_id() const { return _implementation_id; }
	entity_id& implementation_id() { return _implementation_id; }

	const array<uint8, 64>& implementation_use() const { return _implementation_use; }
	array<uint8, 64>& implementation_use() { return _implementation_use; }

	uint32 predecessor_volume_descriptor_sequence_location() const
	  { return B_LENDIAN_TO_HOST_INT32(_predecessor_volume_descriptor_sequence_location); }
	uint16 flags() const { return B_LENDIAN_TO_HOST_INT16(_flags); }
	
	const array<uint8, 22>& reserved() const { return _reserved; }
	array<uint8, 22>& reserved() { return _reserved; }

	// Set functions
	void set_vds_number(uint32 number)
	  { _vds_number = B_HOST_TO_LENDIAN_INT32(number); }
	void set_primary_volume_descriptor_number(uint32 number)
	  { _primary_volume_descriptor_number = B_HOST_TO_LENDIAN_INT32(number); }
	void set_volume_sequence_number(uint16 number)
	  { _volume_sequence_number = B_HOST_TO_LENDIAN_INT16(number); }
	void set_max_volume_sequence_number(uint16 number)
	  { _max_volume_sequence_number = B_HOST_TO_LENDIAN_INT16(number); }
	void set_interchange_level(uint16 level)
	  { _interchange_level = B_HOST_TO_LENDIAN_INT16(level); }
	void set_max_interchange_level(uint16 level)
	  { _max_interchange_level = B_HOST_TO_LENDIAN_INT16(level); }
	void set_character_set_list(uint32 list)
	  { _character_set_list = B_HOST_TO_LENDIAN_INT32(list); }
	void set_max_character_set_list(uint32 list)
	  { _max_character_set_list = B_HOST_TO_LENDIAN_INT32(list); }
	void set_predecessor_volume_descriptor_sequence_location(uint32 location)
	  { _predecessor_volume_descriptor_sequence_location = B_HOST_TO_LENDIAN_INT32(location); }
	void set_flags(uint16 flags)
	  { _flags = B_HOST_TO_LENDIAN_INT16(flags); }

private:
	descriptor_tag  _tag;
	uint32 _vds_number;
	uint32 _primary_volume_descriptor_number;
	array<char, 32> _volume_identifier;
	uint16 _volume_sequence_number;
	uint16 _max_volume_sequence_number;
	uint16 _interchange_level; //!< to be set to 3 if part of multivolume set, 2 otherwise
	uint16 _max_interchange_level; //!< to be set to 3 unless otherwise directed by user
	uint32 _character_set_list;
	uint32 _max_character_set_list;
	array<char, 128> _volume_set_identifier;

	/*! \brief Identifies the character set for the \c volume_identifier
		and \c volume_set_identifier fields.
		
		To be set to CS0.
	*/
	charspec _descriptor_character_set;	

	/*! \brief Identifies the character set used in the \c volume_abstract
		and \c volume_copyright_notice extents.
		
		To be set to CS0.
	*/
	charspec _explanatory_character_set;
	
	extent_address _volume_abstract;
	extent_address _volume_copyright_notice;
	
	entity_id _application_id;
	timestamp _recording_date_and_time;
	entity_id _implementation_id;
	array<uint8, 64> _implementation_use;
	uint32 _predecessor_volume_descriptor_sequence_location;
	uint16 _flags;
	array<uint8, 22> _reserved;

} __attribute__((packed));


/*! \brief Anchor Volume Descriptor Pointer

	vd recorded at preset locations in the partition, used as a reference
	point to the main vd sequences
	
	According to UDF 2.01, an avdp shall be recorded in at least 2 of
	the 3 following locations, where N is the last recordable sector
	of the partition:
	- 256
	- (N - 256)
	- N	
	
	See also: ECMA 167 3/10.2, UDF-2.01 2.2.3
*/
struct anchor_volume_descriptor {
public:
	anchor_volume_descriptor() { memset(_reserved.data, 0, _reserved.size()); }
	void dump() const;
	
	descriptor_tag & tag() { return _tag; }
	const descriptor_tag & tag() const { return _tag; }

	extent_address& main_vds() { return _main_vds; }
	const extent_address& main_vds() const { return _main_vds; }

	extent_address& reserve_vds() { return _reserve_vds; }
	const extent_address& reserve_vds() const { return _reserve_vds; }
private:
	descriptor_tag  _tag;
	extent_address _main_vds;	//!< min length of 16 sectors
	extent_address _reserve_vds;	//!< min length of 16 sectors
	array<uint8, 480> _reserved;	
} __attribute__((packed));


/*! \brief Volume Descriptor Pointer

	Used to chain extents of volume descriptor sequences together.
	
	See also: ECMA 167 3/10.3
*/
struct descriptor_pointer {
	descriptor_tag  tag;
	uint32 vds_number;
	extent_address next;
} __attribute__((packed));


/*! \brief UDF Implementation Use Volume Descriptor struct found in
	implementation_use() field of implementation_use_descriptor when
	said descriptor's implementation_id() field specifies "*UDF LV Info"
	
	See also: UDF 2.50 2.2.7
*/
struct logical_volume_info {
public:
	void dump() const;

	charspec& character_set() { return _character_set; }
	const charspec& character_set() const { return _character_set; }

	array<char, 128>& logical_volume_id() { return _logical_volume_id; }
	const array<char, 128>& logical_volume_id() const { return _logical_volume_id; }

	array<char, 36>& logical_volume_info_1() { return _logical_volume_info.data[0]; }
	const array<char, 36>& logical_volume_info_1() const { return _logical_volume_info.data[0]; }

	array<char, 36>& logical_volume_info_2() { return _logical_volume_info.data[1]; }
	const array<char, 36>& logical_volume_info_2() const { return _logical_volume_info.data[1]; }

	array<char, 36>& logical_volume_info_3() { return _logical_volume_info.data[2]; }
	const array<char, 36>& logical_volume_info_3() const { return _logical_volume_info.data[2]; }

	entity_id& implementation_id() { return _implementation_id; }
	const entity_id& implementation_id() const { return _implementation_id; }

	array<uint8, 128>& implementation_use() { return _implementation_use; }
	const array<uint8, 128>& implementation_use() const { return _implementation_use; }
private:
	charspec _character_set;
	array<char, 128> _logical_volume_id;				// d-string
	array<array<char, 36>, 3> _logical_volume_info;	// d-strings
	entity_id _implementation_id;
	array<uint8, 128> _implementation_use;	
} __attribute__((packed));

/*! \brief Implementation Use Volume Descriptor

	See also: ECMA 167 3/10.4
*/
struct implementation_use_descriptor {
public:
	void dump() const;

	// Get functions
	const descriptor_tag & tag() const { return _tag; }
	descriptor_tag & tag() { return _tag; }
	
	uint32 vds_number() const { return B_LENDIAN_TO_HOST_INT32(_vds_number); }

	const entity_id& implementation_id() const { return _implementation_id; }
	entity_id& implementation_id() { return _implementation_id; }

	const array<uint8, 460>& implementation_use() const { return _implementation_use; }
	array<uint8, 460>& implementation_use() { return _implementation_use; }
	
	// Only valid if implementation_id() returns Udf::kLogicalVolumeInfoId.
	logical_volume_info& info() { return *reinterpret_cast<logical_volume_info*>(_implementation_use.data); }
	const logical_volume_info& info() const { return *reinterpret_cast<const logical_volume_info*>(_implementation_use.data); }
	
	// Set functions
	void set_vds_number(uint32 number) { _vds_number = B_HOST_TO_LENDIAN_INT32(number); }
private:
	descriptor_tag  _tag;
	uint32 _vds_number;
	entity_id _implementation_id;
	array<uint8, 460> _implementation_use;
} __attribute__((packed));


/*! \brief Maximum number of partition descriptors to be found in volume
	descriptor sequence, per UDF-2.50
*/
extern const uint8 kMaxPartitionDescriptors;
#define UDF_MAX_PARTITION_MAPS 2
#define UDF_MAX_PARTITION_MAP_SIZE 64

/*! \brief Partition Descriptor

	See also: ECMA 167 3/10.5
*/
struct partition_descriptor {
private:
	union partition_flags_accessor {
		uint16 partition_flags;
		struct {
			uint16 allocated:1,
			       reserved:15;
		} bits;
	};

public:
	void dump() const;
	
	// Get functions
	const descriptor_tag & tag() const { return _tag; }
	descriptor_tag & tag() { return _tag; }
	
	uint32 vds_number() const { return B_LENDIAN_TO_HOST_INT32(_vds_number); }
	uint16 partition_flags() const { return B_LENDIAN_TO_HOST_INT16(_partition_flags); }
	bool allocated() const {
		partition_flags_accessor f;
		f.partition_flags = partition_flags();
		return f.bits.allocated;
	}
	uint16 partition_number() const { return B_LENDIAN_TO_HOST_INT16(_partition_number); }
		
	const entity_id& partition_contents() const { return _partition_contents; }
	entity_id& partition_contents() { return _partition_contents; }
	
	const array<uint8, 128>& partition_contents_use() const { return _partition_contents_use; }
	array<uint8, 128>& partition_contents_use() { return _partition_contents_use; }
	
	uint32 access_type() const { return B_LENDIAN_TO_HOST_INT32(_access_type); }
	uint32 start() const { return B_LENDIAN_TO_HOST_INT32(_start); }
	uint32 length() const { return B_LENDIAN_TO_HOST_INT32(_length); }

	const entity_id& implementation_id() const { return _implementation_id; }
	entity_id& implementation_id() { return _implementation_id; }

	const array<uint8, 128>& implementation_use() const { return _implementation_use; }
	array<uint8, 128>& implementation_use() { return _implementation_use; }
	
	const array<uint8, 156>& reserved() const { return _reserved; }
	array<uint8, 156>& reserved() { return _reserved; }
	
	// Set functions
	void set_vds_number(uint32 number) { _vds_number = B_HOST_TO_LENDIAN_INT32(number); }
	void set_partition_flags(uint16 flags) { _partition_flags = B_HOST_TO_LENDIAN_INT16(flags); }
	void set_allocated(bool allocated) {
		partition_flags_accessor f;
		f.partition_flags = partition_flags();
		f.bits.allocated = allocated;
		set_partition_flags(f.partition_flags);
	}
	void set_partition_number(uint16 number) { _partition_number = B_HOST_TO_LENDIAN_INT16(number); }
	void set_access_type(uint32 type) { _access_type = B_HOST_TO_LENDIAN_INT32(type); }
	void set_start(uint32 start) { _start = B_HOST_TO_LENDIAN_INT32(start); }
	void set_length(uint32 length) { _length = B_HOST_TO_LENDIAN_INT32(length); }

private:
	descriptor_tag  _tag;
	uint32 _vds_number;
	/*! Bit 0: If 0, shall mean volume space has not been allocated. If 1,
	    shall mean volume space has been allocated.
	*/
	uint16 _partition_flags;
	uint16 _partition_number;
	
	/*! - "+NSR03" Volume recorded according to ECMA-167, i.e. UDF
		- "+CD001" Volume recorded according to ECMA-119, i.e. iso9660
		- "+FDC01" Volume recorded according to ECMA-107
		- "+CDW02" Volume recorded according to ECMA-168
	*/		
	entity_id _partition_contents;
	array<uint8, 128> _partition_contents_use;

	/*! See \c partition_access_type enum
	*/
	uint32 _access_type;
	uint32 _start;
	uint32 _length;
	entity_id _implementation_id;
	array<uint8, 128> _implementation_use;
	array<uint8, 156> _reserved;
} __attribute__((packed));


enum partition_access_type {
	ACCESS_UNSPECIFIED,
	ACCESS_READ_ONLY,
	ACCESS_WRITE_ONCE,
	ACCESS_REWRITABLE,
	ACCESS_OVERWRITABLE,
};


/*! \brief Logical volume descriptor

	See also: ECMA 167 3/10.6, UDF-2.01 2.2.4
*/
struct logical_volume_descriptor {
	void dump() const;
	
	// Get functions
	const descriptor_tag & tag() const { return _tag; }
	descriptor_tag & tag() { return _tag; }

	uint32 vds_number() const { return B_LENDIAN_TO_HOST_INT32(_vds_number); }

	const charspec& character_set() const { return _character_set; }
	charspec& character_set() { return _character_set; }
	
	const array<char, 128>& logical_volume_identifier() const { return _logical_volume_identifier; }
	array<char, 128>& logical_volume_identifier() { return _logical_volume_identifier; }
	
	uint32 logical_block_size() const { return B_LENDIAN_TO_HOST_INT32(_logical_block_size); }

	const entity_id& domain_id() const { return _domain_id; }
	entity_id& domain_id() { return _domain_id; }

	const array<uint8, 16>& logical_volume_contents_use() const { return _logical_volume_contents_use; }
	array<uint8, 16>& logical_volume_contents_use() { return _logical_volume_contents_use; }

	const long_address& file_set_address() const { return *reinterpret_cast<const long_address*>(&_logical_volume_contents_use); }
	long_address& file_set_address() { return *reinterpret_cast<long_address*>(&_logical_volume_contents_use); }

	uint32 map_table_length() const { return B_LENDIAN_TO_HOST_INT32(_map_table_length); }
	uint32 partition_map_count() const { return B_LENDIAN_TO_HOST_INT32(_partition_map_count); }

	const entity_id& implementation_id() const { return _implementation_id; }
	entity_id& implementation_id() { return _implementation_id; }

	const array<uint8, 128>& implementation_use() const { return _implementation_use; }
	array<uint8, 128>& implementation_use() { return _implementation_use; }

	const extent_address& integrity_sequence_extent() const { return _integrity_sequence_extent; }
	extent_address& integrity_sequence_extent() { return _integrity_sequence_extent; }

	const uint8* partition_maps() const { return _partition_maps; }
	uint8* partition_maps() { return _partition_maps; }

	// Set functions
	void set_vds_number(uint32 number) { _vds_number = B_HOST_TO_LENDIAN_INT32(number); }
	void set_logical_block_size(uint32 size) { _logical_block_size = B_HOST_TO_LENDIAN_INT32(size); }
		
	void set_map_table_length(uint32 length) { _map_table_length = B_HOST_TO_LENDIAN_INT32(length); }
	void set_partition_map_count(uint32 count) { _partition_map_count = B_HOST_TO_LENDIAN_INT32(count); }

	// Other functions
	logical_volume_descriptor& operator=(const logical_volume_descriptor &rhs);
	
private:
	descriptor_tag  _tag;
	uint32 _vds_number;
	
	/*! \brief Identifies the character set for the
		\c logical_volume_identifier field.
		
		To be set to CS0.
	*/
	charspec _character_set;
	array<char, 128> _logical_volume_identifier;
	uint32 _logical_block_size;
	
	/*! \brief To be set to 0 or "*OSTA UDF Compliant". See UDF specs.
	*/
	entity_id _domain_id;
	
	/*! \brief For UDF, shall contain a \c long_address which identifies
		the location of the logical volume's first file set.
	*/
	array<uint8, 16> _logical_volume_contents_use;

	uint32 _map_table_length;
	uint32 _partition_map_count;
	entity_id _implementation_id;
	array<uint8, 128> _implementation_use;
	
	/*! \brief Logical volume integrity sequence location.
	
		For re/overwritable media, shall be a min of 8KB in length.
		For WORM media, shall be quite frickin large, as a new volume
		must be added to the set if the extent fills up (since you
		can't chain lvis's I guess).
	*/
	extent_address _integrity_sequence_extent;
	
	/*! \brief Restricted to maps of type 1 for normal maps and
		UDF type 2 for virtual maps or maps on systems not supporting
		defect management.
		
		Note that we actually allocate memory for the partition maps
		here due to the fact that we allocate logical_volume_descriptor
		objects on the stack sometimes.
		
		See UDF-2.01 2.2.8, 2.2.9
	*/
	uint8 _partition_maps[UDF_MAX_PARTITION_MAPS * UDF_MAX_PARTITION_MAP_SIZE];
} __attribute__((packed));

//! Base size (excluding partition maps) of lvd
extern const uint32 kLogicalVolumeDescriptorBaseSize;

/*! \brief (Mostly) common portion of various partition maps

	See also: ECMA-167 3/10.7.1
*/
struct partition_map_header {
public:
	uint8 type() const { return _type; }
	uint8 length() const { return _length; }
	uint8 *map_data() { return _map_data; }
	const uint8 *map_data() const { return _map_data; }
	
	entity_id& partition_type_id()
		{ return *reinterpret_cast<entity_id*>(&_map_data[2]); }
	const entity_id& partition_type_id() const
		{ return *reinterpret_cast<const entity_id*>(&_map_data[2]); }

	void set_type(uint8 type) { _type = type; }
	void set_length(uint8 length) { _length = length; }
private:
	uint8 _type;
	uint8 _length;
	uint8 _map_data[0];
};// __attribute__((packed));


/*! \brief Physical partition map (i.e. ECMA-167 Type 1 partition map)

	See also: ECMA-167 3/10.7.2
*/
struct physical_partition_map {
public:
	void dump();

	uint8 type() const { return _type; }
	uint8 length() const { return _length; }

	uint16 volume_sequence_number() const {
		return B_LENDIAN_TO_HOST_INT16(_volume_sequence_number); }
	uint16 partition_number() const {
		return B_LENDIAN_TO_HOST_INT16(_partition_number); }

	void set_type(uint8 type) { _type = type; }
	void set_length(uint8 length) { _length = length; }
	void set_volume_sequence_number(uint16 number) {
		_volume_sequence_number = B_HOST_TO_LENDIAN_INT16(number); }
	void set_partition_number(uint16 number) {
		_partition_number = B_HOST_TO_LENDIAN_INT16(number); }
private:
	uint8 _type;
	uint8 _length;
	uint16 _volume_sequence_number;
	uint16 _partition_number;	
} __attribute__((packed));


/* ----UDF Specific---- */
/*! \brief Virtual partition map 

	Note that this map is a customization of the ECMA-167
	type 2 partition map.
	
	See also: UDF-2.01 2.2.8
*/
struct virtual_partition_map {
	uint8 type;
	uint8 length;
	uint8 reserved1[2];
	
	/*! - flags: 0
	    - identifier: "*UDF Virtual Partition"
	    - identifier_suffix: per UDF-2.01 2.1.5.3
	*/
	entity_id partition_type_id;
	uint16 volume_sequence_number;
	
	/*! corresponding type 1 partition map in same logical volume
	*/
	uint16 partition_number;	
	uint8 reserved2[24];
} __attribute__((packed));


/*! \brief Maximum number of redundant sparing tables found in
	sparable_partition_map structures.
*/
#define UDF_MAX_SPARING_TABLE_COUNT 4

/* ----UDF Specific---- */
/*! \brief Sparable partition map 

	Note that this map is a customization of the ECMA-167
	type 2 partition map.
	
	See also: UDF-2.01 2.2.9
*/
struct sparable_partition_map {
public:
	void dump();

	uint8 type() const { return _type; }
	uint8 length() const { return _length; }

	entity_id& partition_type_id() { return _partition_type_id; }
	const entity_id& partition_type_id() const { return _partition_type_id; }

	uint16 volume_sequence_number() const {
		return B_LENDIAN_TO_HOST_INT16(_volume_sequence_number); }
	uint16 partition_number() const {
		return B_LENDIAN_TO_HOST_INT16(_partition_number); }
	uint16 packet_length() const {
		return B_LENDIAN_TO_HOST_INT16(_packet_length); }
	uint8 sparing_table_count() const { return _sparing_table_count; }
	uint32 sparing_table_size() const {
		return B_LENDIAN_TO_HOST_INT32(_sparing_table_size); }
	uint32 sparing_table_location(uint8 index) const {
		return B_LENDIAN_TO_HOST_INT32(_sparing_table_locations[index]); }
		

	void set_type(uint8 type) { _type = type; }
	void set_length(uint8 length) { _length = length; }
	void set_volume_sequence_number(uint16 number) {
		_volume_sequence_number = B_HOST_TO_LENDIAN_INT16(number); }
	void set_partition_number(uint16 number) {
		_partition_number = B_HOST_TO_LENDIAN_INT16(number); }
	void set_packet_length(uint16 length) {
		_packet_length = B_HOST_TO_LENDIAN_INT16(length); }
	void set_sparing_table_count(uint8 count) {
		_sparing_table_count = count; }
	void set_sparing_table_size(uint32 size) {
		_sparing_table_size = B_HOST_TO_LENDIAN_INT32(size); }
	void set_sparing_table_location(uint8 index, uint32 location) {
		_sparing_table_locations[index] = B_HOST_TO_LENDIAN_INT32(location); }
private:
	uint8 _type;
	uint8 _length;
	uint8 _reserved1[2];
	
	/*! - flags: 0
	    - identifier: "*UDF Sparable Partition"
	    - identifier_suffix: per UDF-2.01 2.1.5.3
	*/
	entity_id _partition_type_id;
	uint16 _volume_sequence_number;
	
	//! partition number of corresponding partition descriptor
	uint16 _partition_number;
	uint16 _packet_length;
	uint8 _sparing_table_count;
	uint8 _reserved2;
	uint32 _sparing_table_size;
	uint32 _sparing_table_locations[UDF_MAX_SPARING_TABLE_COUNT];
} __attribute__((packed));


/* ----UDF Specific---- */
/*! \brief Metadata partition map 

	Note that this map is a customization of the ECMA-167
	type 2 partition map.
	
	See also: UDF-2.50 2.2.10
*/
struct metadata_partition_map {
public:
	entity_id& partition_type_id() { return _partition_type_id; }
	const entity_id& partition_type_id() const { return _partition_type_id; }

	uint16 volume_sequence_number() const {
		return B_LENDIAN_TO_HOST_INT16(_volume_sequence_number); }
	void set_volume_sequence_number(uint16 number) {
		_volume_sequence_number = B_HOST_TO_LENDIAN_INT16(number); }

	uint16 partition_number() const {
		return B_LENDIAN_TO_HOST_INT16(_partition_number); }
	void set_partition_number(uint16 number) {
		_partition_number = B_HOST_TO_LENDIAN_INT16(number); }

	uint32 metadata_file_location() const {
		return B_LENDIAN_TO_HOST_INT32(_metadata_file_location); }
	void set_metadata_file_location(uint32 location) {
		_metadata_file_location = B_HOST_TO_LENDIAN_INT32(location); }

	uint32 metadata_mirror_file_location() const {
		return B_LENDIAN_TO_HOST_INT32(_metadata_mirror_file_location); }
	void set_metadata_mirror_file_location(uint32 location) {
		_metadata_mirror_file_location = B_HOST_TO_LENDIAN_INT32(location); }

	uint32 metadata_bitmap_file_location() const {
		return B_LENDIAN_TO_HOST_INT32(_metadata_bitmap_file_location); }
	void set_metadata_bitmap_file_location(uint32 location) {
		_metadata_bitmap_file_location = B_HOST_TO_LENDIAN_INT32(location); }

	uint32 allocation_unit_size() const {
		return B_LENDIAN_TO_HOST_INT32(_allocation_unit_size); }
	void set_allocation_unit_size(uint32 size) {
		_allocation_unit_size = B_HOST_TO_LENDIAN_INT32(size); }

	uint32 alignment_unit_size() const {
		return B_LENDIAN_TO_HOST_INT32(_alignment_unit_size); }
	void set_alignment_unit_size(uint32 size) {
		_alignment_unit_size = B_HOST_TO_LENDIAN_INT32(size); }

	uint8 flags() const { return _flags; }
	void set_flags(uint8 flags) { _flags = flags; }
	
private:
	uint8 type;
	uint8 length;
	uint8 reserved1[2];
	
	/*! - flags: 0
	    - identifier: "*UDF Metadata Partition"
	    - identifier_suffix: per UDF-2.50 2.1.5
	*/
	entity_id _partition_type_id;
	uint16 _volume_sequence_number;
	
	/*! corresponding type 1 or type 2 sparable partition
	    map in same logical volume
	*/
	uint16 _partition_number;	
	uint32 _metadata_file_location;
	uint32 _metadata_mirror_file_location;
	uint32 _metadata_bitmap_file_location;
	uint32 _allocation_unit_size;
	uint16 _alignment_unit_size;
	uint8 _flags;
	uint8 reserved2[5];
} __attribute__((packed));


/*! \brief Unallocated space descriptor

	See also: ECMA-167 3/10.8
*/
struct unallocated_space_descriptor {
	void dump() const;

	// Get functions
	const descriptor_tag & tag() const { return _tag; }
	descriptor_tag & tag() { return _tag; }
	uint32 vds_number() const { return B_LENDIAN_TO_HOST_INT32(_vds_number); }
	uint32 allocation_descriptor_count() const { return B_LENDIAN_TO_HOST_INT32(_allocation_descriptor_count); }
	extent_address* allocation_descriptors() { return _allocation_descriptors; }

	// Set functions
	void set_vds_number(uint32 number) { _vds_number = B_HOST_TO_LENDIAN_INT32(number); }
	void set_allocation_descriptor_count(uint32 count) { _allocation_descriptor_count = B_HOST_TO_LENDIAN_INT32(count); }
private:
	descriptor_tag  _tag;
	uint32 _vds_number;
	uint32 _allocation_descriptor_count;
	extent_address _allocation_descriptors[0];
} __attribute__((packed));


/*! \brief Terminating descriptor

	See also: ECMA-167 3/10.9
*/
struct terminating_descriptor {
	terminating_descriptor() { memset(_reserved.data, 0, _reserved.size()); }
	void dump() const;

	// Get functions
	const descriptor_tag & tag() const { return _tag; }
	descriptor_tag & tag() { return _tag; }
private:
	descriptor_tag  _tag;
	array<uint8, 496> _reserved;	
} __attribute__((packed));


/*! \brief Logical volume integrity descriptor

	See also: ECMA-167 3/10.10, UDF-2.50 2.2.6
*/
struct logical_volume_integrity_descriptor {
public:
	static const uint32 minimum_implementation_use_length = 46;

	void dump() const;
	uint32 descriptor_size() const { return sizeof(*this)+implementation_use_length()
	                                 + partition_count()*sizeof(uint32)*2; }

	descriptor_tag& tag() { return _tag; }
	const descriptor_tag& tag() const { return _tag; }

	timestamp& recording_time() { return _recording_time; }
	const timestamp& recording_time() const { return _recording_time; }
	
	uint32 integrity_type() const { return B_LENDIAN_TO_HOST_INT32(_integrity_type); }

	extent_address& next_integrity_extent() { return _next_integrity_extent; }
	const extent_address& next_integrity_extent() const { return _next_integrity_extent; }

	array<uint8, 32>& logical_volume_contents_use() { return _logical_volume_contents_use; }
	const array<uint8, 32>& logical_volume_contents_use() const { return _logical_volume_contents_use; }
	
	// next_unique_id() field is actually stored in the logical_volume_contents_use()
	// field, per UDF-2.50 3.2.1
	uint64 next_unique_id() const { return B_LENDIAN_TO_HOST_INT64(_next_unique_id()); }
	
	uint32 partition_count() const { return B_LENDIAN_TO_HOST_INT32(_partition_count); }
	uint32 implementation_use_length() const { return B_LENDIAN_TO_HOST_INT32(_implementation_use_length); }

	/*! \todo double-check the pointer arithmetic here. */
	uint32* free_space_table() { return reinterpret_cast<uint32*>(reinterpret_cast<uint8*>(this)+80); }
	const uint32* free_space_table() const { return reinterpret_cast<const uint32*>(reinterpret_cast<const uint8*>(this)+80); }
	uint32* size_table() { return reinterpret_cast<uint32*>(reinterpret_cast<uint8*>(free_space_table())+partition_count()*sizeof(uint32)); }
	const uint32* size_table() const { return reinterpret_cast<const uint32*>(reinterpret_cast<const uint8*>(free_space_table())+partition_count()*sizeof(uint32)); }
	uint8* implementation_use() { return reinterpret_cast<uint8*>(reinterpret_cast<uint8*>(size_table())+partition_count()*sizeof(uint32)); }	
	const uint8* implementation_use() const { return reinterpret_cast<const uint8*>(reinterpret_cast<const uint8*>(size_table())+partition_count()*sizeof(uint32)); }	

	// accessors for fields stored in implementation_use() field per UDF-2.50 2.2.6.4
	entity_id& implementation_id() { return _accessor().id; }
	const entity_id& implementation_id() const { return _accessor().id; }
	uint32 file_count() const { return B_LENDIAN_TO_HOST_INT32(_accessor().file_count); }
	uint32 directory_count() const { return B_LENDIAN_TO_HOST_INT32(_accessor().directory_count); }
	uint16 minimum_udf_read_revision() const { return B_LENDIAN_TO_HOST_INT16(_accessor().minimum_udf_read_revision); }
	uint16 minimum_udf_write_revision() const { return B_LENDIAN_TO_HOST_INT16(_accessor().minimum_udf_write_revision); }
	uint16 maximum_udf_write_revision() const { return B_LENDIAN_TO_HOST_INT16(_accessor().maximum_udf_write_revision); }
		
	// set functions
	void set_integrity_type(uint32 type) { _integrity_type = B_HOST_TO_LENDIAN_INT32(type); }
	void set_next_unique_id(uint64 id) { _next_unique_id() = B_HOST_TO_LENDIAN_INT64(id); }
	void set_partition_count(uint32 count) { _partition_count = B_HOST_TO_LENDIAN_INT32(count); }
	void set_implementation_use_length(uint32 length) { _implementation_use_length = B_HOST_TO_LENDIAN_INT32(length); }
	
	// set functions for fields stored in implementation_use() field per UDF-2.50 2.2.6.4
	void set_file_count(uint32 count) { _accessor().file_count = B_HOST_TO_LENDIAN_INT32(count); }
	void set_directory_count(uint32 count) { _accessor().directory_count = B_HOST_TO_LENDIAN_INT32(count); }
	void set_minimum_udf_read_revision(uint16 revision) { _accessor().minimum_udf_read_revision = B_HOST_TO_LENDIAN_INT16(revision); }
	void set_minimum_udf_write_revision(uint16 revision) { _accessor().minimum_udf_write_revision = B_HOST_TO_LENDIAN_INT16(revision); }
	void set_maximum_udf_write_revision(uint16 revision) { _accessor().maximum_udf_write_revision = B_HOST_TO_LENDIAN_INT16(revision); }
		
private:
	struct _lvid_implementation_use_accessor {
		entity_id id;
		uint32 file_count;
		uint32 directory_count;
		uint16 minimum_udf_read_revision;
		uint16 minimum_udf_write_revision;
		uint16 maximum_udf_write_revision;
	};
	
	_lvid_implementation_use_accessor& _accessor() {
		return *reinterpret_cast<_lvid_implementation_use_accessor*>(implementation_use());
	}
	const _lvid_implementation_use_accessor& _accessor() const {
		return *reinterpret_cast<const _lvid_implementation_use_accessor*>(implementation_use());
	}
	
	uint64& _next_unique_id() { return *reinterpret_cast<uint64*>(logical_volume_contents_use().data); }
	const uint64& _next_unique_id() const { return *reinterpret_cast<const uint64*>(logical_volume_contents_use().data); }

	descriptor_tag  _tag;
	timestamp _recording_time;
	uint32 _integrity_type;
	extent_address _next_integrity_extent;
	array<uint8, 32> _logical_volume_contents_use;
	uint32 _partition_count;
	uint32 _implementation_use_length;

} __attribute__((packed));

/*! \brief Logical volume integrity types
*/
enum {
	INTEGRITY_OPEN = 0,
	INTEGRITY_CLOSED = 1,
};

/*! \brief Highest currently supported UDF read revision.
*/
#define UDF_MAX_READ_REVISION 0x0250

//----------------------------------------------------------------------
// ECMA-167 Part 4
//----------------------------------------------------------------------



/*! \brief File set descriptor

	Contains all the pertinent info about a file set (i.e. a hierarchy of files)
	
	According to UDF-2.01, only one file set descriptor shall be recorded,
	except on WORM media, where the following rules apply:
	- Multiple file sets are allowed only on WORM media
	- The default file set shall be the one with highest value \c file_set_number field.
	- Only the default file set may be flagged as writeable. All others shall be
	  flagged as "hard write protect".
	- No writeable file set may reference metadata structures which are referenced
	  (directly or indirectly) by any other file set. Writeable file sets may, however,
	  reference actual file data extents that are also referenced by other file sets.
*/
struct file_set_descriptor {
	void dump() const;

	// Get functions
	const descriptor_tag & tag() const { return _tag; }
	descriptor_tag & tag() { return _tag; }

	const timestamp& recording_date_and_time() const { return _recording_date_and_time; }
	timestamp& recording_date_and_time() { return _recording_date_and_time; }

	uint16 interchange_level() const { return B_LENDIAN_TO_HOST_INT16(_interchange_level); }
	uint16 max_interchange_level() const { return B_LENDIAN_TO_HOST_INT16(_max_interchange_level); }
	uint32 character_set_list() const { return B_LENDIAN_TO_HOST_INT32(_character_set_list); }
	uint32 max_character_set_list() const { return B_LENDIAN_TO_HOST_INT32(_max_character_set_list); }
	uint32 file_set_number() const { return B_LENDIAN_TO_HOST_INT32(_file_set_number); }
	uint32 file_set_descriptor_number() const { return B_LENDIAN_TO_HOST_INT32(_file_set_descriptor_number); }

	const charspec& logical_volume_id_character_set() const { return _logical_volume_id_character_set; }
	charspec& logical_volume_id_character_set() { return _logical_volume_id_character_set; }

	const array<char, 128>& logical_volume_id() const { return _logical_volume_id; }
	array<char, 128>& logical_volume_id() { return _logical_volume_id; }

	const charspec& file_set_id_character_set() const { return _file_set_id_character_set; }
	charspec& file_set_id_character_set() { return _file_set_id_character_set; }

	const array<char, 32>& file_set_id() const { return _file_set_id; }
	array<char, 32>& file_set_id() { return _file_set_id; }

	const array<char, 32>& copyright_file_id() const { return _copyright_file_id; }
	array<char, 32>& copyright_file_id() { return _copyright_file_id; }

	const array<char, 32>& abstract_file_id() const { return _abstract_file_id; }
	array<char, 32>& abstract_file_id() { return _abstract_file_id; }

	const long_address& root_directory_icb() const { return _root_directory_icb; }
	long_address& root_directory_icb() { return _root_directory_icb; }

	const entity_id& domain_id() const { return _domain_id; }
	entity_id& domain_id() { return _domain_id; }

	const long_address& next_extent() const { return _next_extent; }
	long_address& next_extent() { return _next_extent; }

	const long_address& system_stream_directory_icb() const { return _system_stream_directory_icb; }
	long_address& system_stream_directory_icb() { return _system_stream_directory_icb; }

	const array<uint8, 32>& reserved() const { return _reserved; }
	array<uint8, 32>& reserved() { return _reserved; }

	// Set functions
	void set_interchange_level(uint16 level) { _interchange_level = B_HOST_TO_LENDIAN_INT16(level); }
	void set_max_interchange_level(uint16 level) { _max_interchange_level = B_HOST_TO_LENDIAN_INT16(level); }
	void set_character_set_list(uint32 list) { _character_set_list = B_HOST_TO_LENDIAN_INT32(list); }
	void set_max_character_set_list(uint32 list) { _max_character_set_list = B_HOST_TO_LENDIAN_INT32(list); }
	void set_file_set_number(uint32 number) { _file_set_number = B_HOST_TO_LENDIAN_INT32(number); }
	void set_file_set_descriptor_number(uint32 number) { _file_set_descriptor_number = B_HOST_TO_LENDIAN_INT32(number); }
private:
	descriptor_tag  _tag;
	timestamp _recording_date_and_time;
	uint16 _interchange_level;			//!< To be set to 3 (see UDF-2.01 2.3.2.1)
	uint16 _max_interchange_level;		//!< To be set to 3 (see UDF-2.01 2.3.2.2)
	uint32 _character_set_list;
	uint32 _max_character_set_list;
	uint32 _file_set_number;
	uint32 _file_set_descriptor_number;
	charspec _logical_volume_id_character_set;	//!< To be set to kCSOCharspec
	array<char, 128> _logical_volume_id;
	charspec _file_set_id_character_set;
	array<char, 32> _file_set_id;
	array<char, 32> _copyright_file_id;
	array<char, 32> _abstract_file_id;
	long_address _root_directory_icb;
	entity_id _domain_id;	
	long_address _next_extent;
	long_address _system_stream_directory_icb;
	array<uint8, 32> _reserved;
} __attribute__((packed));


/*! \brief Partition header descriptor
	
	Contains references to unallocated and freed space data structures.
	
	Note that unallocated space is space ready to be written with no
	preprocessing. Freed space is space needing preprocessing (i.e.
	a special write pass) before use.
	
	Per UDF-2.01 2.3.3, the use of tables or bitmaps shall be consistent,
	i.e. only one type or the other shall be used, not both.
	
	To indicate disuse of a certain field, the fields of the allocation
	descriptor shall all be set to 0.
	
	See also: ECMA-167 4/14.3, UDF-2.01 2.2.3
*/
struct partition_header_descriptor {
	long_address unallocated_space_table;
	long_address unallocated_space_bitmap;
	/*! Unused, per UDF-2.01 2.2.3 */
	long_address partition_integrity_table;
	long_address freed_space_table;
	long_address freed_space_bitmap;
	uint8 reserved[88];
} __attribute__((packed));

#define kMaxFileIdSize (sizeof(file_id_descriptor)+512+3)

/*! \brief File identifier descriptor

	Identifies the name of a file entry, and the location of its corresponding
	ICB.
	
	See also: ECMA-167 4/14.4, UDF-2.01 2.3.4
	
	\todo Check pointer arithmetic
*/
struct file_id_descriptor {
public:
	uint32 descriptor_size() const { return total_length(); }
	void dump() const;

	descriptor_tag & tag() { return _tag; }
	const descriptor_tag & tag() const { return _tag; }

	uint16 version_number() const { return B_LENDIAN_TO_HOST_INT16(_version_number); }

	uint8 characteristics() const { return _characteristics; }
	
	bool may_be_hidden() const {
		characteristics_accessor c;
		c.all = characteristics();
		return c.bits.may_be_hidden;
	}
	
	bool is_directory() const {
		characteristics_accessor c;
		c.all = characteristics();
		return c.bits.is_directory;
	}
	
	bool is_deleted() const {
		characteristics_accessor c;
		c.all = characteristics();
		return c.bits.is_deleted;
	}
	
	bool is_parent() const {
		characteristics_accessor c;
		c.all = characteristics();
		return c.bits.is_parent;
	}
	
	bool is_metadata_stream() const {
		characteristics_accessor c;
		c.all = characteristics();
		return c.bits.is_metadata_stream;
	}
	
	uint8 id_length() const { return _id_length; }

	long_address& icb() { return _icb; }
	const long_address& icb() const { return _icb; }

	uint16 implementation_use_length() const { return B_LENDIAN_TO_HOST_INT16(_implementation_use_length); }
	
	/*! If implementation_use_length is greater than 0, the first 32
		bytes of implementation_use() shall be an entity_id identifying
		the implementation that generated the rest of the data in the
		implementation_use() field.
	*/
	uint8* implementation_use() { return ((uint8*)this)+(38); }
	char* id() { return ((char*)this)+(38)+implementation_use_length(); }	
	const char* id() const { return ((const char*)this)+(38)+implementation_use_length(); }	
	
	uint16 structure_length() const { return (38) + id_length() + implementation_use_length(); }
	uint16 padding_length() const { return ((structure_length()+3)/4)*4 - structure_length(); }
	uint16 total_length() const { return structure_length() + padding_length(); }
	
	// Set functions
	void set_version_number(uint16 number) { _version_number = B_HOST_TO_LENDIAN_INT16(number); }

	void set_characteristics(uint8 characteristics) { _characteristics = characteristics; }

	void set_may_be_hidden(bool how) {
		characteristics_accessor c;
		c.all = characteristics();
		c.bits.may_be_hidden = how;
		set_characteristics(c.all);
	}
	
	void set_is_directory(bool how) {
		characteristics_accessor c;
		c.all = characteristics();
		c.bits.is_directory = how;
		set_characteristics(c.all);
	}
	
	void set_is_deleted(bool how) {
		characteristics_accessor c;
		c.all = characteristics();
		c.bits.is_deleted = how;
		set_characteristics(c.all);
	}
	
	void set_is_parent(bool how) {
		characteristics_accessor c;
		c.all = characteristics();
		c.bits.is_parent = how;
		set_characteristics(c.all);
	}
	
	void set_is_metadata_stream(bool how) {
		characteristics_accessor c;
		c.all = characteristics();
		c.bits.is_metadata_stream = how;
		set_characteristics(c.all);
	}
	

	void set_id_length(uint8 id_length) { _id_length = id_length; }
	void set_implementation_use_length(uint16 implementation_use_length) { _implementation_use_length = B_HOST_TO_LENDIAN_INT16(implementation_use_length); }
	
	
	
private:
	union characteristics_accessor {
		uint8 all;
		struct { 
			uint8	may_be_hidden:1,
					is_directory:1,
					is_deleted:1,
					is_parent:1,
					is_metadata_stream:1,
					reserved_characteristics:3;
		} bits;
	};	
	
	descriptor_tag  _tag;
	/*! According to ECMA-167: 1 <= valid version_number <= 32767, 32768 <= reserved <= 65535.
		
		However, according to UDF-2.01, there shall be exactly one version of
		a file, and it shall be 1.
	 */
	uint16 _version_number;
	/*! \todo Check UDF-2.01 2.3.4.2 for some more restrictions. */
	uint8 _characteristics;
	uint8 _id_length;
	long_address _icb;
	uint16 _implementation_use_length;	
} __attribute__((packed));


/*! \brief Allocation extent descriptor

	See also: ECMA-167 4/14.5
*/
struct allocation_extent_descriptor {
	descriptor_tag  tag;
	uint32 previous_allocation_extent_location;
	uint32 length_of_allocation_descriptors;
	
	/*! \todo Check that this is really how things work: */
	uint8* allocation_descriptors() { return (uint8*)(reinterpret_cast<uint8*>(this)+sizeof(allocation_extent_descriptor)); }
} __attribute__((packed));


/*! \brief icb_tag::file_type values

	See also ECMA-167 4/14.6.6
*/
enum icb_file_types {
	ICB_TYPE_UNSPECIFIED = 0,
	ICB_TYPE_UNALLOCATED_SPACE_ENTRY,
	ICB_TYPE_PARTITION_INTEGRITY_ENTRY,
	ICB_TYPE_INDIRECT_ENTRY,
	ICB_TYPE_DIRECTORY,
	ICB_TYPE_REGULAR_FILE,
	ICB_TYPE_BLOCK_SPECIAL_DEVICE,
	ICB_TYPE_CHARACTER_SPECIAL_DEVICE,
	ICB_TYPE_EXTENDED_ATTRIBUTES_FILE,
	ICB_TYPE_FIFO,
	ICB_TYPE_ISSOCK,
	ICB_TYPE_TERMINAL,
	ICB_TYPE_SYMLINK,
	ICB_TYPE_STREAM_DIRECTORY,

	ICB_TYPE_RESERVED_START = 14,
	ICB_TYPE_RESERVED_END = 247,
	
	ICB_TYPE_CUSTOM_START = 248,
	ICB_TYPE_CUSTOM_END = 255,
};

/*!	\brief idb_entry_tag::_flags::descriptor_flags() values

	See also ECMA-167 4/14.6.8
*/
enum icb_descriptor_types {
	ICB_DESCRIPTOR_TYPE_SHORT = 0,
	ICB_DESCRIPTOR_TYPE_LONG,
	ICB_DESCRIPTOR_TYPE_EXTENDED,
	ICB_DESCRIPTOR_TYPE_EMBEDDED,
};

/*!	\brief idb_entry_tag::strategy_type() values

	See also UDF-2.50 2.3.5.1
*/
enum icb_strategy_types {
	ICB_STRATEGY_SINGLE = 4,
	ICB_STRATEGY_LINKED_LIST = 4096
};

/*! \brief ICB entry tag

	Common tag found in all ICB entries (in addition to, and immediately following,
	the descriptor tag).

	See also: ECMA-167 4/14.6, UDF-2.01 2.3.5
*/
struct icb_entry_tag {
public:
	union flags_accessor {
		uint16 all_flags;
		struct {
			uint16	descriptor_flags:3,			
					if_directory_then_sort:1,	//!< To be set to 0 per UDF-2.01 2.3.5.4
					non_relocatable:1,
					archive:1,
					setuid:1,
					setgid:1,
					sticky:1,
					contiguous:1,
					system:1,
					transformed:1,
					multi_version:1,			//!< To be set to 0 per UDF-2.01 2.3.5.4
					is_stream:1,
					reserved_icb_entry_flags:2;
		} flags;
	};
	
public:
	void dump() const;
 
	uint32 prior_recorded_number_of_direct_entries() const { return B_LENDIAN_TO_HOST_INT32(_prior_recorded_number_of_direct_entries); }
	uint16 strategy_type() const { return B_LENDIAN_TO_HOST_INT16(_strategy_type); }

	array<uint8, 2>& strategy_parameters() { return _strategy_parameters; }
	const array<uint8, 2>& strategy_parameters() const { return _strategy_parameters; }

	uint16 entry_count() const { return B_LENDIAN_TO_HOST_INT16(_entry_count); }
	uint8& reserved() { return _reserved; }
	uint8 file_type() const { return _file_type; }
	logical_block_address& parent_icb_location() { return _parent_icb_location; }
	const logical_block_address& parent_icb_location() const { return _parent_icb_location; }

	uint16 flags() const { return B_LENDIAN_TO_HOST_INT16(_flags); }
	flags_accessor& flags_access() { return *reinterpret_cast<flags_accessor*>(&_flags); }
	
	// flags accessor functions
	uint8 descriptor_flags() const {
		flags_accessor f;
		f.all_flags = flags();
		return f.flags.descriptor_flags;
	}
/*	void set_descriptor_flags(uint8 value) {
		flags_accessor f;
		f.all_flags = flags();
		f.flags.descriptor_flags = value;
		set_flags
*/		
	
	void set_prior_recorded_number_of_direct_entries(uint32 entries) { _prior_recorded_number_of_direct_entries = B_LENDIAN_TO_HOST_INT32(entries); }
	void set_strategy_type(uint16 type) { _strategy_type = B_HOST_TO_LENDIAN_INT16(type); }

	void set_entry_count(uint16 count) { _entry_count = B_LENDIAN_TO_HOST_INT16(count); }
	void set_file_type(uint8 type) { _file_type = type; }

	void set_flags(uint16 flags) { _flags = B_LENDIAN_TO_HOST_INT16(flags); }
	
private:
	uint32 _prior_recorded_number_of_direct_entries;
	/*! Per UDF-2.01 2.3.5.1, only strategy types 4 and 4096 shall be supported.
	
		\todo Describe strategy types here.
	*/
	uint16 _strategy_type;
	array<uint8, 2> _strategy_parameters;
	uint16 _entry_count;
	uint8 _reserved;
	/*! \brief icb_file_type value identifying the type of this icb entry */
	uint8 _file_type;
	logical_block_address _parent_icb_location;
	uint16 _flags;
} __attribute__((packed));

/*! \brief Header portion of an ICB entry.
*/
struct icb_header {
public:
	void dump() const;
	
	descriptor_tag  &tag() { return _tag; }
	const descriptor_tag  &tag() const { return _tag; }
	
	icb_entry_tag &icb_tag() { return _icb_tag; }
	const icb_entry_tag &icb_tag() const { return _icb_tag; }
private:
	descriptor_tag  _tag;
	icb_entry_tag _icb_tag;
};

/*! \brief Indirect ICB entry
*/
struct indirect_icb_entry {
	descriptor_tag  tag;
	icb_entry_tag icb_tag;
	long_address indirect_icb;
} __attribute__((packed));


/*! \brief Terminal ICB entry
*/
struct terminal_icb_entry {
	descriptor_tag  tag;
	icb_entry_tag icb_tag;
} __attribute__((packed));

enum permissions {
	OTHER_EXECUTE	 	= 0x0001,
	OTHER_WRITE			= 0x0002,
	OTHER_READ			= 0x0004,
	OTHER_ATTRIBUTES	= 0x0008,
	OTHER_DELETE		= 0x0010,
	GROUP_EXECUTE	 	= 0x0020,
	GROUP_WRITE			= 0x0040,
	GROUP_READ			= 0x0080,
	GROUP_ATTRIBUTES	= 0x0100,
	GROUP_DELETE		= 0x0200,
	USER_EXECUTE	 	= 0x0400,
	USER_WRITE			= 0x0800,
	USER_READ			= 0x1000,
	USER_ATTRIBUTES		= 0x2000,
	USER_DELETE			= 0x4000,
};

/*! \brief File ICB entry

	See also: ECMA-167 4/14.9

	\todo Check pointer math.
*/
struct file_icb_entry {
	void dump() const;
	uint32 descriptor_size() const { return sizeof(*this)+extended_attributes_length()
	                                 +allocation_descriptors_length(); }
	const char* descriptor_name() const { return "file_icb_entry"; }

	// get functions
	descriptor_tag & tag() { return _tag; }
	const descriptor_tag & tag() const { return _tag; }
	
	icb_entry_tag& icb_tag() { return _icb_tag; }
	const icb_entry_tag& icb_tag() const { return _icb_tag; }
	
	uint32 uid() const { return B_LENDIAN_TO_HOST_INT32(_uid); }
	uint32 gid() const { return B_LENDIAN_TO_HOST_INT32(_gid); }
	uint32 permissions() const { return B_LENDIAN_TO_HOST_INT32(_permissions); }
	uint16 file_link_count() const { return B_LENDIAN_TO_HOST_INT16(_file_link_count); }
	uint8 record_format() const { return _record_format; }
	uint8 record_display_attributes() const { return _record_display_attributes; }
	uint8 record_length() const { return _record_length; }
	uint64 information_length() const { return B_LENDIAN_TO_HOST_INT64(_information_length); }
	uint64 logical_blocks_recorded() const { return B_LENDIAN_TO_HOST_INT64(_logical_blocks_recorded); }

	timestamp& access_date_and_time() { return _access_date_and_time; }
	const timestamp& access_date_and_time() const { return _access_date_and_time; }

	timestamp& modification_date_and_time() { return _modification_date_and_time; }
	const timestamp& modification_date_and_time() const { return _modification_date_and_time; }

	timestamp& attribute_date_and_time() { return _attribute_date_and_time; }
	const timestamp& attribute_date_and_time() const { return _attribute_date_and_time; }

	uint32 checkpoint() const { return B_LENDIAN_TO_HOST_INT32(_checkpoint); }

	long_address& extended_attribute_icb() { return _extended_attribute_icb; }
	const long_address& extended_attribute_icb() const { return _extended_attribute_icb; }

	entity_id& implementation_id() { return _implementation_id; }
	const entity_id& implementation_id() const { return _implementation_id; }

	uint64 unique_id() const { return B_LENDIAN_TO_HOST_INT64(_unique_id); }
	uint32 extended_attributes_length() const { return B_LENDIAN_TO_HOST_INT32(_extended_attributes_length); }
	uint32 allocation_descriptors_length() const { return B_LENDIAN_TO_HOST_INT32(_allocation_descriptors_length); }

	uint8* extended_attributes() { return _end(); }
	const uint8* extended_attributes() const { return _end(); }
	uint8* allocation_descriptors() { return _end()+extended_attributes_length(); }
	const uint8* allocation_descriptors() const { return _end()+extended_attributes_length(); }
	
	// set functions
	void set_uid(uint32 uid) { _uid = B_HOST_TO_LENDIAN_INT32(uid); }
	void set_gid(uint32 gid) { _gid = B_HOST_TO_LENDIAN_INT32(gid); }
	void set_permissions(uint32 permissions) { _permissions = B_HOST_TO_LENDIAN_INT32(permissions); }

	void set_file_link_count(uint16 count) { _file_link_count = B_HOST_TO_LENDIAN_INT16(count); }
	void set_record_format(uint8 format) { _record_format = format; }
	void set_record_display_attributes(uint8 attributes) { _record_display_attributes = attributes; }
	void set_record_length(uint8 length) { _record_length = length; }

	void set_information_length(uint64 length) { _information_length = B_HOST_TO_LENDIAN_INT64(length); }
	void set_logical_blocks_recorded(uint64 blocks) { _logical_blocks_recorded = B_HOST_TO_LENDIAN_INT64(blocks); }

	void set_checkpoint(uint32 checkpoint) { _checkpoint = B_HOST_TO_LENDIAN_INT32(checkpoint); }

	void set_unique_id(uint64 id) { _unique_id = B_HOST_TO_LENDIAN_INT64(id); }

	void set_extended_attributes_length(uint32 length) { _extended_attributes_length = B_HOST_TO_LENDIAN_INT32(length); }
	void set_allocation_descriptors_length(uint32 length) { _allocation_descriptors_length = B_HOST_TO_LENDIAN_INT32(length); }

	// extended_file_icb_entry compatability functions
	timestamp& creation_date_and_time() { return _attribute_date_and_time; }
	const timestamp& creation_date_and_time() const { return _attribute_date_and_time; }
	
	
	void set_object_size(uint64 size) { }
	void set_reserved(uint32 reserved) { }
	long_address& stream_directory_icb() { return _dummy_stream_directory_icb; }
	const long_address& stream_directory_icb() const { return _dummy_stream_directory_icb; }


private:
	static const uint32 _descriptor_length = 176;
	static long_address _dummy_stream_directory_icb;
	uint8* _end() { return reinterpret_cast<uint8*>(this)+_descriptor_length; }
	const uint8* _end() const { return reinterpret_cast<const uint8*>(this)+_descriptor_length; }

	descriptor_tag  _tag;
	icb_entry_tag _icb_tag;
	uint32 _uid;
	uint32 _gid;
	/*! \todo List perms in comment and add handy union thingy */
	uint32 _permissions;
	/*! Identifies the number of file identifier descriptors referencing
		this icb.
	*/
	uint16 _file_link_count;
	uint8 _record_format;				//!< To be set to 0 per UDF-2.01 2.3.6.1
	uint8 _record_display_attributes;	//!< To be set to 0 per UDF-2.01 2.3.6.2
	uint8 _record_length;				//!< To be set to 0 per UDF-2.01 2.3.6.3
	uint64 _information_length;
	uint64 _logical_blocks_recorded;		//!< To be 0 for files and dirs with embedded data
	timestamp _access_date_and_time;
	timestamp _modification_date_and_time;
	
	// NOTE: data members following this point in the descriptor are in
	// different locations in extended file entries
	
	timestamp _attribute_date_and_time;
	/*! \brief Initially 1, may be incremented upon user request. */
	uint32 _checkpoint;
	long_address _extended_attribute_icb;
	entity_id _implementation_id;
	/*! \brief The unique id identifying this file entry
	
		The id of the root directory of a file set shall be 0.
		
		\todo Detail the system specific requirements for unique ids from UDF-2.01
	*/
	uint64 _unique_id;
	uint32 _extended_attributes_length;
	uint32 _allocation_descriptors_length;
	
};
		

/*! \brief Extended file ICB entry

	See also: ECMA-167 4/14.17
	
	\todo Check pointer math.
*/
struct extended_file_icb_entry {
	void dump() const;
	uint32 descriptor_size() const { return sizeof(*this)+extended_attributes_length()
	                                 +allocation_descriptors_length(); }
	const char* descriptor_name() const { return "extended_file_icb_entry"; }

	// get functions
	descriptor_tag & tag() { return _tag; }
	const descriptor_tag & tag() const { return _tag; }
	
	icb_entry_tag& icb_tag() { return _icb_tag; }
	const icb_entry_tag& icb_tag() const { return _icb_tag; }
	
	uint32 uid() const { return B_LENDIAN_TO_HOST_INT32(_uid); }
	uint32 gid() const { return B_LENDIAN_TO_HOST_INT32(_gid); }
	uint32 permissions() const { return B_LENDIAN_TO_HOST_INT32(_permissions); }
	uint16 file_link_count() const { return B_LENDIAN_TO_HOST_INT16(_file_link_count); }
	uint8 record_format() const { return _record_format; }
	uint8 record_display_attributes() const { return _record_display_attributes; }
	uint32 record_length() const { return _record_length; }
	uint64 information_length() const { return B_LENDIAN_TO_HOST_INT64(_information_length); }
	uint64 object_size() const { return B_LENDIAN_TO_HOST_INT64(_object_size); }
	uint64 logical_blocks_recorded() const { return B_LENDIAN_TO_HOST_INT64(_logical_blocks_recorded); }

	timestamp& access_date_and_time() { return _access_date_and_time; }
	const timestamp& access_date_and_time() const { return _access_date_and_time; }

	timestamp& modification_date_and_time() { return _modification_date_and_time; }
	const timestamp& modification_date_and_time() const { return _modification_date_and_time; }

	timestamp& creation_date_and_time() { return _creation_date_and_time; }
	const timestamp& creation_date_and_time() const { return _creation_date_and_time; }

	timestamp& attribute_date_and_time() { return _attribute_date_and_time; }
	const timestamp& attribute_date_and_time() const { return _attribute_date_and_time; }

	uint32 checkpoint() const { return B_LENDIAN_TO_HOST_INT32(_checkpoint); }

	long_address& extended_attribute_icb() { return _extended_attribute_icb; }
	const long_address& extended_attribute_icb() const { return _extended_attribute_icb; }

	long_address& stream_directory_icb() { return _stream_directory_icb; }
	const long_address& stream_directory_icb() const { return _stream_directory_icb; }

	entity_id& implementation_id() { return _implementation_id; }
	const entity_id& implementation_id() const { return _implementation_id; }

	uint64 unique_id() const { return B_LENDIAN_TO_HOST_INT64(_unique_id); }
	uint32 extended_attributes_length() const { return B_LENDIAN_TO_HOST_INT32(_extended_attributes_length); }
	uint32 allocation_descriptors_length() const { return B_LENDIAN_TO_HOST_INT32(_allocation_descriptors_length); }

	uint8* extended_attributes() { return _end(); }
	const uint8* extended_attributes() const { return _end(); }
	uint8* allocation_descriptors() { return _end()+extended_attributes_length(); }
	const uint8* allocation_descriptors() const { return _end()+extended_attributes_length(); }
	
	// set functions
	void set_uid(uint32 uid) { _uid = B_HOST_TO_LENDIAN_INT32(uid); }
	void set_gid(uint32 gid) { _gid = B_HOST_TO_LENDIAN_INT32(gid); }
	void set_permissions(uint32 permissions) { _permissions = B_HOST_TO_LENDIAN_INT32(permissions); }

	void set_file_link_count(uint16 count) { _file_link_count = B_HOST_TO_LENDIAN_INT16(count); }
	void set_record_format(uint8 format) { _record_format = format; }
	void set_record_display_attributes(uint8 attributes) { _record_display_attributes = attributes; }
	void set_record_length(uint32 length) { _record_length = B_HOST_TO_LENDIAN_INT32(length); }

	void set_information_length(uint64 length) { _information_length = B_HOST_TO_LENDIAN_INT64(length); }
	void set_object_size(uint64 size) { _object_size = B_HOST_TO_LENDIAN_INT64(size); }
	void set_logical_blocks_recorded(uint64 blocks) { _logical_blocks_recorded = B_HOST_TO_LENDIAN_INT64(blocks); }

	void set_checkpoint(uint32 checkpoint) { _checkpoint = B_HOST_TO_LENDIAN_INT32(checkpoint); }
	void set_reserved(uint32 reserved) { _reserved = B_HOST_TO_LENDIAN_INT32(reserved); }

	void set_unique_id(uint64 id) { _unique_id = B_HOST_TO_LENDIAN_INT64(id); }

	void set_extended_attributes_length(uint32 length) { _extended_attributes_length = B_HOST_TO_LENDIAN_INT32(length); }
	void set_allocation_descriptors_length(uint32 length) { _allocation_descriptors_length = B_HOST_TO_LENDIAN_INT32(length); }

private:
	static const uint32 _descriptor_length = 216;
	uint8* _end() { return reinterpret_cast<uint8*>(this)+_descriptor_length; }
	const uint8* _end() const { return reinterpret_cast<const uint8*>(this)+_descriptor_length; }

	descriptor_tag  _tag;
	icb_entry_tag _icb_tag;
	uint32 _uid;
	uint32 _gid;
	/*! \todo List perms in comment and add handy union thingy */
	uint32 _permissions;
	/*! Identifies the number of file identifier descriptors referencing
		this icb.
	*/
	uint16 _file_link_count;
	uint8 _record_format;				//!< To be set to 0 per UDF-2.01 2.3.6.1
	uint8 _record_display_attributes;	//!< To be set to 0 per UDF-2.01 2.3.6.2
	uint32 _record_length;				//!< To be set to 0 per UDF-2.01 2.3.6.3
	uint64 _information_length;
	uint64 _object_size;
	uint64 _logical_blocks_recorded;		//!< To be 0 for files and dirs with embedded data
	timestamp _access_date_and_time;
	timestamp _modification_date_and_time;
	timestamp _creation_date_and_time;	// <== EXTENDED FILE ENTRY ONLY
	timestamp _attribute_date_and_time;
	/*! \brief Initially 1, may be incremented upon user request. */
	uint32 _checkpoint;
	uint32 _reserved;	// <== EXTENDED FILE ENTRY ONLY
	long_address _extended_attribute_icb;
	long_address _stream_directory_icb;	// <== EXTENDED FILE ENTRY ONLY
	entity_id _implementation_id;
	/*! \brief The unique id identifying this file entry
	
		The id of the root directory of a file set shall be 0.
		
		\todo Detail the system specific requirements for unique ids from UDF-2.01 3.2.1.1
	*/
	uint64 _unique_id;
	uint32 _extended_attributes_length;
	uint32 _allocation_descriptors_length;

};
		

#endif	// _UDF_DISK_STRUCTURES_H

