/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "efi_gpt.h"

#include <KernelExport.h>
#include <ddm_modules.h>
#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#else
#	include <DiskDeviceTypes.h>
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
#define EFI_PARTITION_NAME "EFI GUID Partition Table"


struct static_guid {
	uint32	data1;
	uint16	data2;
	uint16	data3;
	uint64	data4;

	inline bool operator==(const guid &other) const;
};

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
		~Header();

		status_t InitCheck() const;
		bool IsPrimary() const
			{ return fBlock == EFI_HEADER_LOCATION; }

		uint32 EntryCount() const
			{ return fHeader.EntryCount(); }
		const efi_partition_entry &EntryAt(int32 index) const
			{ return *(const efi_partition_entry*)
				(fEntries + fHeader.EntrySize() * index); }

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


const char *
get_partition_type(const guid_t &guid)
{
	for (uint32 i = 0; i < sizeof(kTypeMap) / sizeof(kTypeMap[0]); i++) {
		if (kTypeMap[i].guid == guid)
			return kTypeMap[i].type;
	}

	return NULL;
}


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

	ssize_t bytesRead = read_pos(fd, block * blockSize, &fHeader, sizeof(fHeader));
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


Header::~Header()
{
	delete[] fEntries;
}


status_t
Header::InitCheck() const
{
	return fStatus;
}


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
	snprintf(guid, sizeof(guid), "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		B_LENDIAN_TO_HOST_INT32(id.data1), B_LENDIAN_TO_HOST_INT16(id.data2),
		B_LENDIAN_TO_HOST_INT16(id.data3), id.data4[0], id.data4[1], id.data4[2],
		id.data4[3], id.data4[4], id.data4[5], id.data4[6], id.data4[7]);
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

		dprintf("[%3ld] partition type: %s\n", i, _PrintGUID(entry.partition_type));
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
	EFI::Header *header = new (std::nothrow) EFI::Header(fd, EFI_HEADER_LOCATION,
		partition->block_size);
	status_t status = header->InitCheck();
	if (status < B_OK) {
		delete header;
		return status;
	}

	*_cookie = header;
	return 0.7;
}


static status_t
efi_gpt_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	TRACE(("efi_gpt_scan_partition(cookie = %p)\n", _cookie));
	EFI::Header *header = (EFI::Header *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM | B_PARTITION_READ_ONLY;
	partition->content_size = partition->size;

	// scan all children

	uint32 index = 0;

	for (uint32 i = 0; i < header->EntryCount(); i++) {
		const efi_partition_entry &entry = header->EntryAt(i);

		if (entry.partition_type == kEmptyGUID)
			continue;

		if (entry.EndBlock() * partition->block_size > (uint64)partition->size) {
			TRACE(("efi_gpt: child partition exceeds existing space (%Ld MB)\n",
				(entry.EndBlock() - entry.StartBlock()) * partition->block_size / 1024 / 1024));
			continue;
		}

		partition_data *child = create_child_partition(partition->id, index++, -1);
		if (child == NULL) {
			TRACE(("efi_gpt: Creating child at index %ld failed\n", index - 1));
			return B_ERROR;
		}

		char name[B_OS_NAME_LENGTH];
		to_utf8(entry.name, EFI_PARTITION_NAME_LENGTH, name, sizeof(name));
		child->name = strdup(name);
		child->type = strdup(get_partition_type(entry.partition_type));

		child->offset = partition->offset + entry.StartBlock() * partition->block_size;
		child->size = (entry.EndBlock() - entry.StartBlock()) * partition->block_size;			
		child->block_size = partition->block_size;
	}

	return B_OK;
}


static void
efi_gpt_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	delete (EFI::Header *)_cookie;
}


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
	EFI_PARTITION_NAME,						// pretty_name
	0,										// flags

	// scanning
	efi_gpt_identify_partition,
	efi_gpt_scan_partition,
	efi_gpt_free_identify_partition_cookie,
	NULL,
};

#ifndef _BOOT_MODE
partition_module_info *modules[] = {
	&sEFIPartitionModule,
	NULL
};
#endif

