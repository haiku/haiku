/*
 * Copyright 2007-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef GPT_HEADER_H
#define GPT_HEADER_H


#include "efi_gpt.h"


namespace EFI {


class Header {
public:
								Header(int fd, off_t block, uint32 blockSize);
#ifndef _BOOT_MODE
								// constructor for empty header
								Header(off_t block, off_t lastBlock,
									uint32 blockSize);
#endif
								~Header();

			status_t			InitCheck() const;
			bool				IsPrimary() const
									{ return fBlock == EFI_HEADER_LOCATION; }

			uint64				FirstUsableBlock() const
									{ return fHeader.FirstUsableBlock(); }
			uint64				LastUsableBlock() const
									{ return fHeader.LastUsableBlock(); }

			uint32				EntryCount() const
									{ return fHeader.EntryCount(); }
			efi_partition_entry& EntryAt(int32 index) const
									{ return *(efi_partition_entry*)(fEntries
										+ fHeader.EntrySize() * index); }

#ifndef _BOOT_MODE
			status_t			WriteEntry(int fd, uint32 entryIndex);
			status_t			Write(int fd);
#endif

private:
			const char*			_PrintGUID(const guid_t& id);
			void				_Dump();
			void				_DumpPartitions();

			status_t			_Write(int fd, off_t offset, const void* data,
									size_t size) const;
			void				_UpdateCRC();
			bool				_ValidateHeaderCRC();
			bool				_ValidateEntriesCRC() const;
			size_t				_EntryArraySize() const
									{ return fHeader.EntrySize()
										* fHeader.EntryCount(); }

private:
			uint64				fBlock;
			uint32				fBlockSize;
			status_t			fStatus;
			efi_table_header	fHeader;
			uint8*				fEntries;
};


}	// namespace EFI


#endif	// GPT_HEADER_H
