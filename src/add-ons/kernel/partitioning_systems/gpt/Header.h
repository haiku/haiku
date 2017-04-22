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
								Header(int fd, uint64 lastBlock,
									uint32 blockSize);
#ifndef _BOOT_MODE
								// constructor for empty header
								Header(uint64 lastBlock, uint32 blockSize);
#endif
								~Header();

			status_t			InitCheck() const;
			bool				IsDirty() const;

			uint64				FirstUsableBlock() const
									{ return fHeader.FirstUsableBlock(); }
			uint64				LastUsableBlock() const
									{ return fHeader.LastUsableBlock(); }
			const efi_table_header& TableHeader() const
									{ return fHeader; }

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
#ifndef _BOOT_MODE
			status_t			_WriteHeader(int fd);
			status_t			_Write(int fd, off_t offset, const void* data,
									size_t size) const;
			void				_UpdateCRC();
			void				_UpdateCRC(efi_table_header& header);
#endif

			status_t			_Read(int fd, off_t offset, void* data,
									size_t size) const;
			static bool			_IsHeaderValid(efi_table_header& header,
									uint64 block);
			static bool			_ValidateHeaderCRC(efi_table_header& header);
			bool				_ValidateEntriesCRC() const;
			void				_SetBackupHeaderFromPrimary(uint64 lastBlock);
			size_t				_EntryArraySize() const
									{ return fHeader.EntrySize()
										* fHeader.EntryCount(); }

			const char*			_PrintGUID(const guid_t& id);
			void				_Dump(const efi_table_header& header);
			void				_DumpPartitions();

private:
			uint32				fBlockSize;
			status_t			fStatus;
			efi_table_header	fHeader;
			efi_table_header	fBackupHeader;
			uint8*				fEntries;
			bool				fDirty;
};


}	// namespace EFI


#endif	// GPT_HEADER_H
