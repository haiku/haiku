/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_FILE_H
#define ELF_FILE_H

#include <sys/types.h>

#include <SupportDefs.h>

#include <elf32.h>
#include <elf64.h>
#include <util/DoublyLinkedList.h>

#include "Types.h"


class ElfSection : public DoublyLinkedListLinkImpl<ElfSection> {
public:
								ElfSection(const char* name, int fd,
									off_t offset, off_t size,
									target_addr_t loadAddress, uint32 flags);
								~ElfSection();

			const char*			Name() const	{ return fName; }
			off_t				Offset() const	{ return fOffset; }
			off_t				Size() const	{ return fSize; }
			const void*			Data() const	{ return fData; }
			target_addr_t		LoadAddress() const
									{ return fLoadAddress; }
			bool				IsWritable() const
									{ return (fFlags & SHF_WRITE) != 0; }

			status_t			Load();
			void				Unload();
			bool				IsLoaded() const { return fLoadCount > 0; }

private:
			const char*			fName;
			int					fFD;
			off_t				fOffset;
			off_t				fSize;
			void*				fData;
			target_addr_t		fLoadAddress;
			uint32				fFlags;
			int32				fLoadCount;
};


class ElfSegment : public DoublyLinkedListLinkImpl<ElfSegment> {
public:
								ElfSegment(off_t fileOffset, off_t fileSize,
									target_addr_t loadAddress,
									target_size_t loadSize, bool writable);
								~ElfSegment();

			off_t				FileOffset() const	{ return fFileOffset; }
			off_t				FileSize() const	{ return fFileSize; }
			target_addr_t		LoadAddress() const	{ return fLoadAddress; }
			target_size_t		LoadSize() const	{ return fLoadSize; }
			bool				IsWritable() const	{ return fWritable; }

private:
			off_t				fFileOffset;
			off_t				fFileSize;
			target_addr_t		fLoadAddress;
			target_size_t		fLoadSize;
			bool				fWritable;
};


class ElfFile {
public:
								ElfFile();
								~ElfFile();

			status_t			Init(const char* fileName);

			int					FD() const	{ return fFD; }

			ElfSection*			GetSection(const char* name);
			void				PutSection(ElfSection* section);
			ElfSection*			FindSection(const char* name) const;

			ElfSegment*			TextSegment() const;
			ElfSegment*			DataSegment() const;

private:
			typedef DoublyLinkedList<ElfSection> SectionList;
			typedef DoublyLinkedList<ElfSegment> SegmentList;

private:
			template<typename Ehdr, typename Phdr, typename Shdr>
			status_t			_LoadFile(const char* fileName);

			bool				_CheckRange(off_t offset, off_t size) const;
			static bool			_CheckElfHeader(Elf32_Ehdr& elfHeader);
			static bool			_CheckElfHeader(Elf64_Ehdr& elfHeader);

private:
			off_t				fFileSize;
			int					fFD;
			SectionList			fSections;
			SegmentList			fSegments;
};


#endif	// ELF_FILE_H
