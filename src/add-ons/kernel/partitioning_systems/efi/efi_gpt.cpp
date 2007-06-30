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
#include <string.h>


#define TRACE_EFI_GPT
#ifdef TRACE_EFI_GPT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define EFI_PARTITION_MODULE_NAME "partitioning_systems/efi_gpt/v1"
#define EFI_PARTITION_NAME "EFI GUID Partition Table"


class Header {
	public:
		Header(int fd, off_t block, uint32 blockSize);
		~Header();

		status_t InitCheck();
		bool IsPrimary() const
			{ return fBlock == EFI_HEADER_LOCATION; }

		uint32 EntryCount() const
			{ return fHeader.EntryCount(); }
		const efi_partition_entry &EntryAt(int32 index) const
			{ return *(const efi_partition_entry*)
				(fEntries + fHeader.EntrySize() * index); }

		void Dump();

	private:
		
		bool _ValidateCRC(uint8 *data, size_t size);
		size_t _EntryArraySize() const
			{ return fHeader.EntrySize() * fHeader.EntryCount(); }

		uint64				fBlock;
		uint32				fBlockSize;
		status_t			fStatus;
		efi_table_header	fHeader;
		uint8				*fEntries;
};


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

	fStatus = B_OK;
	Dump();
}


Header::~Header()
{
	delete[] fEntries;
}


status_t
Header::InitCheck()
{
	return fStatus;
}


bool
Header::_ValidateCRC(uint8 *data, size_t size)
{
	// TODO: implement!
	return true;
}


void
Header::Dump()
{
	dprintf("entry size:  %ld\n", fHeader.EntrySize());
	dprintf("entry count: %ld\n", fHeader.EntryCount());
}


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
	return B_ERROR;
}


static status_t
efi_gpt_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	TRACE(("efi_gpt_scan_partition(cookie = %p)\n", _cookie));

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM
						| B_PARTITION_READ_ONLY;
	partition->content_size = partition->size;

	// scan all children

	status_t status = B_ERROR;
	if (status == B_ENTRY_NOT_FOUND)
		return B_OK;

	return status;
}


static void
efi_gpt_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
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
//	efi_gpt_free_partition_cookie,
//	efi_gpt_free_partition_content_cookie,
};

#ifndef _BOOT_MODE
partition_module_info *modules[] = {
	&sEFIPartitionModule,
	NULL
};
#endif

