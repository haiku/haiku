/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 * Copyright 2007-2009, Axel Dörfler, axeld@pinc-software.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include "efi_gpt.h"

#include <KernelExport.h>
#include <disk_device_manager/ddm_modules.h>
#include <disk_device_types.h>
#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#else
#	include <DiskDeviceTypes.h>
#	include "PartitionLocker.h"
#	include <utf8_functions.h>
#endif
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>


#define TRACE_EFI_GPT
#ifdef TRACE_EFI_GPT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define EFI_PARTITION_MODULE_NAME "partitioning_systems/efi_gpt/v1"


struct static_guid {
	uint32	data1;
	uint16	data2;
	uint16	data3;
	uint64	data4;

	inline bool operator==(const guid &other) const;
} _PACKED;

const static struct type_map {
	static_guid	guid;
	const char	*type;
} kTypeMap[] = {
	{{0x48465300, 0x0000, 0x11aa, 0xaa1100306543ECACLL}, "HFS+ File System"}
};


namespace EFI {

class Header {
	public:
		Header(int fd, off_t block, uint32 blockSize);
#ifndef _BOOT_MODE
		// constructor for empty header
		Header(off_t block, uint32 blockSize);
#endif
		~Header();

		status_t InitCheck() const;
		bool IsPrimary() const
			{ return fBlock == EFI_HEADER_LOCATION; }

		uint64 FirstUsableBlock() const
			{ return fHeader.FirstUsableBlock(); }
		uint64 LastUsableBlock() const
			{ return fHeader.LastUsableBlock(); }

		uint32 EntryCount() const
			{ return fHeader.EntryCount(); }
		efi_partition_entry &EntryAt(int32 index) const
			{ return *(efi_partition_entry *)
				(fEntries + fHeader.EntrySize() * index); }

#ifndef _BOOT_MODE
		status_t WriteEntry(int fd, uint32 entryIndex);
		status_t Write(int fd);
#endif

	private:
#ifdef TRACE_EFI_GPT
		const char *_PrintGUID(const guid_t &id);
		void _Dump();
		void _DumpPartitions();
#endif

		bool _ValidateCRC(uint8 *data, size_t size) const;
		size_t _EntryArraySize() const
			{ return fHeader.EntrySize() * fHeader.EntryCount(); }

		uint64				fBlock;
		uint32				fBlockSize;
		status_t			fStatus;
		efi_table_header	fHeader;
		uint8				*fEntries;
};

}	// namespace EFI


const static guid_t kEmptyGUID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};


inline bool
static_guid::operator==(const guid_t &other) const
{
	return B_HOST_TO_LENDIAN_INT32(data1) == other.data1
		&& B_HOST_TO_LENDIAN_INT16(data2) == other.data2
		&& B_HOST_TO_LENDIAN_INT16(data3) == other.data3
		&& B_HOST_TO_BENDIAN_INT64(*(uint64 *)&data4) == *(uint64 *)other.data4;
			// the last 8 bytes are in big-endian order
}


static void
put_utf8_byte(char *&to, size_t &left, char c)
{
	if (left <= 1)
		return;

	*(to++) = c;
	left--;
}


static void
to_utf8(const uint16 *from, size_t maxFromLength, char *to, size_t toSize)
{
	for (uint32 i = 0; i < maxFromLength; i++) {
		uint16 c = B_LENDIAN_TO_HOST_INT16(from[i]);
		if (!c)
			break;

		if (c < 0x80)
			put_utf8_byte(to, toSize, c);
		else if (c < 0x800) {
			put_utf8_byte(to, toSize, 0xc0 | (c >> 6));
			put_utf8_byte(to, toSize, 0x80 | (c & 0x3f));
		} else if (c < 0x10000) {
			put_utf8_byte(to, toSize, 0xe0 | (c >> 12));
			put_utf8_byte(to, toSize, 0x80 | ((c >> 6) & 0x3f));
			put_utf8_byte(to, toSize, 0x80 | (c & 0x3f));
		} else if (c <= 0x10ffff) {
			put_utf8_byte(to, toSize, 0xf0 | (c >> 18));
			put_utf8_byte(to, toSize, 0x80 | ((c >> 12) & 0x3f));
			put_utf8_byte(to, toSize, 0x80 | ((c >> 6) & 0x3f));
			put_utf8_byte(to, toSize, 0x80 | (c & 0x3f));
		}
	}

	if (toSize > 0)
		*to = '\0';
}


#ifndef _BOOT_MODE
static void
to_ucs2(const char *from, size_t fromLength, uint16 *to, size_t maxToLength)
{
	size_t index = 0;
	while (from[0] && index < maxToLength) {
		// TODO: handle characters that are not representable in UCS-2 better
		uint32 code = UTF8ToCharCode(&from);
		if (code < 0x10000)
			to[index++] = code;
	}

	if (index < maxToLength)
		to[index] = '\0';
}
#endif // !_BOOT_MODE


static const char *
get_partition_type(const guid_t &guid)
{
	for (uint32 i = 0; i < sizeof(kTypeMap) / sizeof(kTypeMap[0]); i++) {
		if (kTypeMap[i].guid == guid)
			return kTypeMap[i].type;
	}

	return NULL;
}


#ifndef _BOOT_MODE
static const static_guid *
guid_for_partition_type(const char *type)
{
	for (uint32 i = 0; i < sizeof(kTypeMap) / sizeof(kTypeMap[0]); i++) {
		if (strcmp(kTypeMap[i].type, type) == 0)
			return &kTypeMap[i].guid;
	}

	return NULL;
}


static off_t
block_align(partition_data *partition, off_t offset, bool upwards)
{
	if (upwards) {
		return ((offset + partition->block_size - 1) / partition->block_size)
			* partition->block_size;
	}

	return (offset / partition->block_size) * partition->block_size;
}
#endif // !_BOOT_MODE


//	#pragma mark -


namespace EFI {

Header::Header(int fd, off_t block, uint32 blockSize)
	:
	fBlock(block),
	fBlockSize(blockSize),
	fStatus(B_NO_INIT),
	fEntries(NULL)
{
	// TODO: check the correctness of the protective MBR

	// read and check the partition table header

	ssize_t bytesRead = read_pos(fd, block * blockSize, &fHeader,
		sizeof(fHeader));
	if (bytesRead != (ssize_t)sizeof(fHeader)) {
		if (bytesRead < B_OK)
			fStatus = bytesRead;
		else
			fStatus = B_IO_ERROR;

		return;
	}

	if (memcmp(fHeader.header, EFI_PARTITION_HEADER, sizeof(fHeader.header))
		|| !_ValidateCRC((uint8 *)&fHeader, sizeof(fHeader))
		|| fHeader.AbsoluteBlock() != fBlock) {
		// TODO: check that partition counts are in valid bounds
		fStatus = B_BAD_DATA;
		return;
	}

	// allocate, read, and check partition entry array

	fEntries = new (std::nothrow) uint8[_EntryArraySize()];
	if (fEntries == NULL) {
		// TODO: if there cannot be allocated enough (ie. the boot loader's
		//	heap is limited), try a smaller size before failing
		fStatus = B_NO_MEMORY;
		return;
	}

	bytesRead = read_pos(fd, fHeader.EntriesBlock() * blockSize,
		fEntries, _EntryArraySize());
	if (bytesRead != (ssize_t)_EntryArraySize()) {
		if (bytesRead < B_OK)
			fStatus = bytesRead;
		else
			fStatus = B_IO_ERROR;

		return;
	}

	if (!_ValidateCRC(fEntries, _EntryArraySize())) {
		// TODO: check overlapping or out of range partitions
		fStatus = B_BAD_DATA;
		return;
	}

#ifdef TRACE_EFI_GPT
	_Dump();
	_DumpPartitions();
#endif

	fStatus = B_OK;
}


#ifndef _BOOT_MODE
Header::Header(off_t block, uint32 blockSize)
	:
	fBlock(block),
	fBlockSize(blockSize),
	fStatus(B_NO_INIT),
	fEntries(NULL)
{
	// initialize to an empty header
	memcpy(fHeader.header, EFI_PARTITION_HEADER, sizeof(fHeader.header));
	fHeader.SetRevision(EFI_TABLE_REVISION);
	fHeader.SetHeaderSize(sizeof(fHeader));
	fHeader.SetHeaderCRC(0);
	fHeader.SetAbsoluteBlock(fBlock);
	fHeader.SetAlternateBlock(0); // TODO
	// TODO: set disk guid
	fHeader.SetEntriesBlock(EFI_PARTITION_ENTRIES_BLOCK);
	fHeader.SetEntryCount(EFI_PARTITION_ENTRY_COUNT);
	fHeader.SetEntrySize(EFI_PARTITION_ENTRY_SIZE);
	fHeader.SetEntriesCRC(0);

	size_t arraySize = _EntryArraySize();
	fEntries = new (std::nothrow) uint8[arraySize];
	if (fEntries == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	memset(fEntries, 0, arraySize);
		// TODO: initialize the entry guids

	fHeader.SetFirstUsableBlock(EFI_PARTITION_ENTRIES_BLOCK
		+ (arraySize + fBlockSize - 1) / fBlockSize);
	fHeader.SetLastUsableBlock(0); // TODO

#ifdef TRACE_EFI_GPT
	_Dump();
	_DumpPartitions();
#endif

	fStatus = B_OK;
}
#endif // !_BOOT_MODE


Header::~Header()
{
	delete[] fEntries;
}


status_t
Header::InitCheck() const
{
	return fStatus;
}


#ifndef _BOOT_MODE
status_t
Header::WriteEntry(int fd, uint32 entryIndex)
{
	// TODO: implement
	return B_ERROR;
}


status_t
Header::Write(int fd)
{
	// TODO: implement
	return B_ERROR;
}
#endif // !_BOOT_MODE


bool
Header::_ValidateCRC(uint8 *data, size_t size) const
{
	// TODO: implement!
	return true;
}


#ifdef TRACE_EFI_GPT
const char *
Header::_PrintGUID(const guid_t &id)
{
	static char guid[48];
	snprintf(guid, sizeof(guid),
		"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		B_LENDIAN_TO_HOST_INT32(id.data1), B_LENDIAN_TO_HOST_INT16(id.data2),
		B_LENDIAN_TO_HOST_INT16(id.data3), id.data4[0], id.data4[1],
		id.data4[2], id.data4[3], id.data4[4], id.data4[5], id.data4[6],
		id.data4[7]);
	return guid;
}


void
Header::_Dump()
{
	dprintf("EFI header: %.8s\n", fHeader.header);
	dprintf("EFI revision: %ld\n", fHeader.Revision());
	dprintf("header size: %ld\n", fHeader.HeaderSize());
	dprintf("header CRC: %ld\n", fHeader.HeaderCRC());
	dprintf("absolute block: %Ld\n", fHeader.AbsoluteBlock());
	dprintf("alternate block: %Ld\n", fHeader.AlternateBlock());
	dprintf("first usable block: %Ld\n", fHeader.FirstUsableBlock());
	dprintf("last usable block: %Ld\n", fHeader.LastUsableBlock());
	dprintf("disk GUID: %s\n", _PrintGUID(fHeader.disk_guid));
	dprintf("entries block: %Ld\n", fHeader.EntriesBlock());
	dprintf("entry size:  %ld\n", fHeader.EntrySize());
	dprintf("entry count: %ld\n", fHeader.EntryCount());
	dprintf("entries CRC: %ld\n", fHeader.EntriesCRC());
}


void
Header::_DumpPartitions()
{
	for (uint32 i = 0; i < EntryCount(); i++) {
		const efi_partition_entry &entry = EntryAt(i);

		if (entry.partition_type == kEmptyGUID)
			continue;

		dprintf("[%3ld] partition type: %s\n", i,
			_PrintGUID(entry.partition_type));
		dprintf("      unique id: %s\n", _PrintGUID(entry.unique_guid));
		dprintf("      start block: %Ld\n", entry.StartBlock());
		dprintf("      end block: %Ld\n", entry.EndBlock());
		dprintf("      size: %g MB\n", (entry.EndBlock() - entry.StartBlock())
			* 512 / 1024.0 / 1024.0);
		dprintf("      attributes: %Lx\n", entry.Attributes());

		char name[64];
		to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, name, sizeof(name));
		dprintf("      name: %s\n", name);
	}
}
#endif	// TRACE_EFI_GPT

}	// namespace EFI


//	#pragma mark - public module interface


static status_t
efi_gpt_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


static float
efi_gpt_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	EFI::Header *header = new (std::nothrow) EFI::Header(fd,
		EFI_HEADER_LOCATION, partition->block_size);
	status_t status = header->InitCheck();
	if (status < B_OK) {
		delete header;
		return -1;
	}

	*_cookie = header;
	return 0.96;
		// This must be higher as Intel partitioning, as EFI can contain this
		// partitioning for compatibility
}


static status_t
efi_gpt_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	TRACE(("efi_gpt_scan_partition(cookie = %p)\n", _cookie));
	EFI::Header *header = (EFI::Header *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM | B_PARTITION_READ_ONLY;
	partition->content_size = partition->size;
	partition->content_cookie = header;

	// scan all children

	uint32 index = 0;

	for (uint32 i = 0; i < header->EntryCount(); i++) {
		const efi_partition_entry &entry = header->EntryAt(i);

		if (entry.partition_type == kEmptyGUID)
			continue;

		if (entry.EndBlock() * partition->block_size
				> (uint64)partition->size) {
			TRACE(("efi_gpt: child partition exceeds existing space (%Ld MB)\n",
				(entry.EndBlock() - entry.StartBlock()) * partition->block_size
				/ 1024 / 1024));
			continue;
		}

		partition_data *child = create_child_partition(partition->id, index++,
			partition->offset + entry.StartBlock() * partition->block_size,
			entry.BlockCount() * partition->block_size, -1);
		if (child == NULL) {
			TRACE(("efi_gpt: Creating child at index %ld failed\n", index - 1));
			return B_ERROR;
		}

		char name[B_OS_NAME_LENGTH];
		to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, name, sizeof(name));
		child->name = strdup(name);
		child->type = strdup(get_partition_type(entry.partition_type));
		child->block_size = partition->block_size;
		child->cookie = (void *)i;
	}

	return B_OK;
}


static void
efi_gpt_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	// Cookie is freed in efi_gpt_free_partition_content_cookie().
}


static void
efi_gpt_free_partition_content_cookie(partition_data *partition)
{
	delete (EFI::Header *)partition->content_cookie;
}


#ifndef _BOOT_MODE
static uint32
efi_gpt_get_supported_operations(partition_data *partition, uint32 mask)
{
	uint32 flags = B_DISK_SYSTEM_SUPPORTS_INITIALIZING
		| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
		| B_DISK_SYSTEM_SUPPORTS_MOVING
		| B_DISK_SYSTEM_SUPPORTS_RESIZING
		| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD;
		// TODO: check for available entries and partitionable space and only
		// add creating child support if both is valid

	return flags;
}


static uint32
efi_gpt_get_supported_child_operations(partition_data *partition,
	partition_data *child, uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
		| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


static bool
efi_gpt_is_sub_system_for(partition_data *partition)
{
	// a GUID Partition Table doesn't usually live inside another partition
	return false;
}


static bool
efi_gpt_validate_resize(partition_data *partition, off_t *size)
{
	off_t newSize = *size;
	if (newSize == partition->size)
		return true;

	if (newSize < 0)
		newSize = 0;
	else
		newSize = block_align(partition, newSize, false);

	// growing
	if (newSize > partition->size) {
		*size = newSize;
		return true;
	}

	// shrinking, only so that no child would be truncated
	off_t newEnd = partition->offset + newSize;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data *child = get_child_partition(partition->id, i);
		if (child == NULL)
			continue;

		if (child->offset + child->size > newEnd)
			newEnd = child->offset + child->size;
	}

	newSize = block_align(partition, newEnd - partition->offset, true);
	*size = newSize;
	return true;
}


static bool
efi_gpt_validate_resize_child(partition_data *partition, partition_data *child,
	off_t *size)
{
	off_t newSize = *size;
	if (newSize == child->size)
		return true;

	// shrinking
	if (newSize < child->size) {
		if (newSize < 0)
			newSize = 0;

		*size = block_align(partition, newSize, false);
		return true;
	}

	// growing, but only so much that the child doesn't get bigger than
	// the parent
	if (child->offset + newSize > partition->offset + partition->size)
		newSize = partition->offset + partition->size - child->offset;

	// make sure that the child doesn't overlap any sibling partitions
	off_t newEnd = child->offset + newSize;
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data *other = get_child_partition(partition->id, i);
		if (other == NULL || other->id == child->id
			|| other->offset < child->offset)
			continue;

		if (newEnd > other->offset)
			newEnd = other->offset;
	}

	*size = block_align(partition, newEnd - child->offset, false);
	return true;
}


static bool
efi_gpt_validate_move(partition_data *partition, off_t *start)
{
	// nothing to do
	return true;
}


static bool
efi_gpt_validate_move_child(partition_data *partition, partition_data *child,
	off_t *start)
{
	off_t newStart = *start;
	if (newStart < 0)
		newStart = 0;

	if (newStart + child->size > partition->size)
		newStart = partition->size - child->size;

	newStart = block_align(partition, newStart, false);
	if (newStart > child->offset) {
		for (int32 i = 0; i < partition->child_count; i++) {
			partition_data *other = get_child_partition(partition->id, i);
			if (other == NULL || other->id == child->id
				|| other->offset < child->offset)
				continue;

			if (other->offset < newStart + child->size)
				newStart = other->offset - child->size;
		}

		newStart = block_align(partition, newStart, false);
	} else {
		for (int32 i = 0; i < partition->child_count; i++) {
			partition_data *other = get_child_partition(partition->id, i);
			if (other == NULL || other->id == child->id
				|| other->offset > child->offset)
				continue;

			if (other->offset + other->size > newStart)
				newStart = other->offset + other->size;
		}

		newStart = block_align(partition, newStart, true);
	}

	*start = newStart;
	return true;
}


static bool
efi_gpt_validate_set_content_name(partition_data *partition, char *name)
{
	// TODO: should validate that the utf-8 -> ucs-2 is valid
	// TODO: should count actual utf-8 chars
	if (strlen(name) > EFI_PARTITION_NAME_LENGTH)
		name[EFI_PARTITION_NAME_LENGTH - 1] = 0;
	return true;
}


static bool
efi_gpt_validate_set_type(partition_data *partition, const char *type)
{
	return guid_for_partition_type(type) != NULL;
}


static bool
efi_gpt_validate_initialize(partition_data *partition, char *name,
	const char *parameters)
{
	if ((efi_gpt_get_supported_operations(partition, ~0)
		& B_DISK_SYSTEM_SUPPORTS_INITIALIZING) == 0)
		return false;

	// name and parameters are ignored
	if (name != NULL)
		name[0] = 0;

	return true;
}


static bool
efi_gpt_validate_create_child(partition_data *partition, off_t *start,
	off_t *size, const char *type, const char *name, const char *parameters,
	int32 *index)
{
	if ((efi_gpt_get_supported_operations(partition, ~0)
			& B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD) == 0)
		return false;

	if (guid_for_partition_type(type) == NULL)
		return false;

	EFI::Header *header = (EFI::Header *)partition->content_cookie;
	int32 entryIndex = -1;
	for (uint32 i = 0; i < header->EntryCount(); i++) {
		const efi_partition_entry &entry = header->EntryAt(i);
		if (entry.partition_type == kEmptyGUID) {
			entryIndex = i;
			break;
		}
	}

	if (entryIndex < 0)
		return false;

	*index = entryIndex;

	// ensure that child lies between first and last usable block
	off_t firstUsable = header->FirstUsableBlock() * partition->block_size;
	if (*start < firstUsable)
		*start = firstUsable;

	off_t lastUsable = header->LastUsableBlock() * partition->block_size;
	if (*start + *size > lastUsable) {
		if (*start > lastUsable)
			return false;

		*size = lastUsable - *start;
	}

	// ensure that we don't overlap any siblings
	for (int32 i = 0; i < partition->child_count; i++) {
		partition_data *other = get_child_partition(partition->id, i);
		if (other == NULL)
			continue;

		if (other->offset < *start && other->offset + other->size > *start)
			*start = other->offset + other->size;

		if (other->offset > *start && other->offset < *start + *size)
			*size = other->offset - *start;
	}

	*start = block_align(partition, *size, true);
	*size = block_align(partition, *size, false);

	// TODO: support parameters
	return true;
}


static status_t
efi_gpt_get_partitionable_spaces(partition_data *partition,
	partitionable_space_data *buffer, int32 count, int32 *actualCount)
{
	// TODO: implement
	return B_ERROR;
}


static status_t
efi_gpt_get_next_supported_type(partition_data *partition, int32 *cookie,
	char *type)
{
	// TODO: implement
	return B_ERROR;
}


static status_t
efi_gpt_shadow_changed(partition_data *partition, partition_data *child,
	uint32 operation)
{
	// TODO: implement
	return B_ERROR;
}


static status_t
efi_gpt_repair(int fd, partition_id partition, bool checkOnly, disk_job_id job)
{
	// TODO: implement, validate CRCs and restore from backup area if corrupt
	return B_ERROR;
}


static status_t
efi_gpt_resize(int fd, partition_id partitionID, off_t size, disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	off_t validatedSize = size;
	if (!efi_gpt_validate_resize(partition, &validatedSize))
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	partition->size = validatedSize;
	partition->content_size = validatedSize;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_resize_child(int fd, partition_id partitionID, off_t size,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *child = get_partition(partitionID);
	if (child == NULL)
		return B_BAD_VALUE;

	partition_data *partition = get_parent_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header *header = (EFI::Header *)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	off_t validatedSize = size;
	if (!efi_gpt_validate_resize_child(partition, child, &validatedSize))
		return B_BAD_VALUE;

	if (child->size == validatedSize)
		return B_OK;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry &entry = header->EntryAt(entryIndex);
	entry.SetBlockCount(validatedSize / partition->block_size);

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK) {
		entry.SetBlockCount(child->size / partition->block_size);
		return result;
	}

	child->size = validatedSize;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_move(int fd, partition_id partition, off_t offset, disk_job_id job)
{
	// nothing to do here
	return B_OK;
}


static status_t
efi_gpt_move_child(int fd, partition_id partitionID, partition_id childID,
	off_t offset, disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	partition_data *child = get_partition(childID);
	if (child == NULL)
		return B_BAD_VALUE;

	EFI::Header *header = (EFI::Header *)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	off_t validatedOffset = offset;
	if (!efi_gpt_validate_move_child(partition, child, &validatedOffset))
		return B_BAD_VALUE;

	if (child->offset == validatedOffset)
		return B_OK;

	// TODO: implement actual moving, need to move the partition content
	// (the raw data) here and need to take overlap into account
	return B_ERROR;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry &entry = header->EntryAt(entryIndex);
	uint64 blockCount = entry.BlockCount();
	entry.SetStartBlock((validatedOffset - partition->offset)
		/ partition->block_size);
	entry.SetBlockCount(blockCount);

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK) {
		// fatal error: the data has been moved but the partition table could
		// not be updated to reflect that change!
		return result;
	}

	child->offset = validatedOffset;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(childID);
	return B_OK;
}


static status_t
efi_gpt_set_content_name(int fd, partition_id partitionID, const char *name,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *child = get_partition(partitionID);
	if (child == NULL)
		return B_BAD_VALUE;

	partition_data *partition = get_parent_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header *header = (EFI::Header *)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry &entry = header->EntryAt(entryIndex);
	to_ucs2(name, strlen(name), entry.name, EFI_PARTITION_NAME_LENGTH);

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK)
		return result;

	char newName[B_OS_NAME_LENGTH];
	to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, newName, sizeof(newName));
	child->name = strdup(newName);

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_set_type(int fd, partition_id partitionID, const char *type,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *child = get_partition(partitionID);
	if (child == NULL)
		return B_BAD_VALUE;

	partition_data *partition = get_parent_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header *header = (EFI::Header *)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	const static_guid *newType = guid_for_partition_type(type);
	if (newType == NULL)
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	efi_partition_entry &entry = header->EntryAt(entryIndex);
	memcpy(&entry.partition_type, newType, sizeof(entry.partition_type));

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK)
		return result;

	child->type = strdup(type);

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_initialize(int fd, partition_id partitionID, const char *name,
	const char *parameters, off_t partitionSize, disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	partition_data *partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	EFI::Header header(EFI_HEADER_LOCATION, partition->block_size);
	status_t result = header.InitCheck();
	if (result != B_OK)
		return result;

	result = header.Write(fd);
	if (result != B_OK)
		return result;

	result = scan_partition(partitionID);
	if (result != B_OK)
		return result;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_create_child(int fd, partition_id partitionID, off_t offset,
	off_t size, const char *type, const char *name, const char *parameters,
	disk_job_id job, partition_id *childID)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	EFI::Header *header = (EFI::Header *)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	off_t validatedOffset = offset;
	off_t validatedSize = size;
	uint32 entryIndex = 0;

	if (!efi_gpt_validate_create_child(partition, &validatedOffset,
			&validatedSize, type, name, parameters, (int32 *)&entryIndex))
		return B_BAD_VALUE;

	const static_guid *newType = guid_for_partition_type(type);
	if (newType == NULL)
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	partition_data *child = create_child_partition(partition->id, entryIndex,
		validatedOffset, validatedSize, *childID);
	if (child == NULL)
		return B_ERROR;

	efi_partition_entry &entry = header->EntryAt(entryIndex);
	memcpy(&entry.partition_type, newType, sizeof(entry.partition_type));
	to_ucs2(name, strlen(name), entry.name, EFI_PARTITION_NAME_LENGTH);
	entry.SetStartBlock((validatedOffset - partition->offset)
		/ partition->block_size);
	entry.SetBlockCount(validatedSize / partition->block_size);
	entry.SetAttributes(0); // TODO

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK) {
		delete_partition(child->id);
		return result;
	}

	*childID = child->id;
	child->block_size = partition->block_size;
	child->name = strdup(name);
	child->type = strdup(type);
	child->parameters = strdup(parameters);
	child->cookie = (void *)entryIndex;

	if (child->type == NULL || child->parameters == NULL) {
		delete_partition(child->id);
		return B_NO_MEMORY;
	}

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}


static status_t
efi_gpt_delete_child(int fd, partition_id partitionID, partition_id childID,
	disk_job_id job)
{
	if (fd < 0)
		return B_ERROR;

	PartitionWriteLocker locker(partitionID);
	if (!locker.IsLocked())
		return B_ERROR;

	partition_data *partition = get_partition(partitionID);
	if (partition == NULL)
		return B_BAD_VALUE;

	partition_data *child = get_partition(childID);
	if (child == NULL)
		return B_BAD_VALUE;

	EFI::Header *header = (EFI::Header *)partition->content_cookie;
	if (header == NULL)
		return B_BAD_VALUE;

	uint32 entryIndex = (uint32)child->cookie;
	if (entryIndex >= header->EntryCount())
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	if (!delete_partition(childID))
		return B_ERROR;

	efi_partition_entry &entry = header->EntryAt(entryIndex);
	entry.partition_type = kEmptyGUID;

	status_t result = header->WriteEntry(fd, entryIndex);
	if (result != B_OK)
		return result;

	update_disk_device_job_progress(job, 1.0);
	partition_modified(partitionID);
	return B_OK;
}
#endif // !_BOOT_MODE


#ifndef _BOOT_MODE
static partition_module_info sEFIPartitionModule = {
#else
partition_module_info gEFIPartitionModule = {
#endif
	{
		EFI_PARTITION_MODULE_NAME,
		0,
		efi_gpt_std_ops
	},
	"efi",									// short_name
	EFI_PARTITION_NAME,						// pretty_name
	0										// flags
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_MOVING
	| B_DISK_SYSTEM_SUPPORTS_RESIZING
	| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
	| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD
	,

	// scanning
	efi_gpt_identify_partition,
	efi_gpt_scan_partition,
	efi_gpt_free_identify_partition_cookie,
	NULL, // free_partition_cookie
	efi_gpt_free_partition_content_cookie,

#ifndef _BOOT_MODE
	// querying
	efi_gpt_get_supported_operations,
	efi_gpt_get_supported_child_operations,
	NULL, // supports_initializing_child
	efi_gpt_is_sub_system_for,

	efi_gpt_validate_resize,
	efi_gpt_validate_resize_child,
	efi_gpt_validate_move,
	efi_gpt_validate_move_child,
	NULL, // validate_set_name
	efi_gpt_validate_set_content_name,
	efi_gpt_validate_set_type,
	NULL, // validate_set_parameters
	NULL, // validate_set_content_parameters
	efi_gpt_validate_initialize,
	efi_gpt_validate_create_child,
	efi_gpt_get_partitionable_spaces,
	efi_gpt_get_next_supported_type,
	NULL, // get_type_for_content_type

	// shadow partition modification
	efi_gpt_shadow_changed,

	// writing
	efi_gpt_repair,
	efi_gpt_resize,
	efi_gpt_resize_child,
	efi_gpt_move,
	efi_gpt_move_child,
	NULL, // set_name
	efi_gpt_set_content_name,
	efi_gpt_set_type,
	NULL, // set_parameters
	NULL, // set_content_parameters
	efi_gpt_initialize,
	NULL, // uninitialize
	efi_gpt_create_child,
	efi_gpt_delete_child
#else
	NULL
#endif // _BOOT_MODE
};

#ifndef _BOOT_MODE
partition_module_info *modules[] = {
	&sEFIPartitionModule,
	NULL
};
#endif
