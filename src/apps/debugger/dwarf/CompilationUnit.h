/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef COMPILATION_UNIT_H
#define COMPILATION_UNIT_H


#include <ObjectList.h>
#include <String.h>

#include <Array.h>

#include "LineNumberProgram.h"
#include "Types.h"


class AbbreviationTable;
class DebugInfoEntry;
class DIECompileUnitBase;
class SourceLanguageInfo;
class TargetAddressRangeList;


class CompilationUnit {
public:
								CompilationUnit(off_t headerOffset,
									off_t contentOffset,
									off_t totalSize,
									off_t abbreviationOffset,
									uint8 addressSize, bool isDwarf64);
								~CompilationUnit();

			off_t				HeaderOffset() const { return fHeaderOffset; }
			off_t				ContentOffset() const { return fContentOffset; }
			off_t 				RelativeContentOffset() const
									{ return fContentOffset - fHeaderOffset; }
			off_t				TotalSize() const	{ return fTotalSize; }
			off_t				ContentSize() const
									{ return fTotalSize
										- RelativeContentOffset(); }
			off_t				AbbreviationOffset() const
									{ return fAbbreviationOffset; }

			uint8				AddressSize() const	{ return fAddressSize; }
	inline	target_addr_t		MaxAddress() const;
			bool				IsDwarf64() const	{ return fIsDwarf64; }

			AbbreviationTable*	GetAbbreviationTable() const
									{ return fAbbreviationTable; }
			void				SetAbbreviationTable(
									AbbreviationTable* abbreviationTable);

			DIECompileUnitBase*	UnitEntry() const	{ return fUnitEntry; }
			void				SetUnitEntry(DIECompileUnitBase* entry);

			const SourceLanguageInfo* SourceLanguage() const
									{ return fSourceLanguage; }
			void				SetSourceLanguage(
									const SourceLanguageInfo* language);

			TargetAddressRangeList* AddressRanges() const
									{ return fAddressRanges; }
			void				SetAddressRanges(
									TargetAddressRangeList* ranges);

			target_addr_t		AddressRangeBase() const;

			LineNumberProgram&	GetLineNumberProgram()
									{ return fLineNumberProgram; }

			status_t			AddDebugInfoEntry(DebugInfoEntry* entry,
									off_t offset);
			int					CountEntries() const;
			void				GetEntryAt(int index, DebugInfoEntry*& entry,
									off_t& offset) const;
			DebugInfoEntry*		EntryForOffset(off_t offset) const;

			bool				AddDirectory(const char* directory);
			int32				CountDirectories() const;
			const char*			DirectoryAt(int32 index) const;

			bool				AddFile(const char* fileName, int32 dirIndex);
			int32				CountFiles() const;
			const char*			FileAt(int32 index,
									const char** _directory = NULL) const;

private:
			struct File;
			typedef BObjectList<BString> DirectoryList;
			typedef BObjectList<File> FileList;

private:
			off_t				fHeaderOffset;
			off_t				fContentOffset;
			off_t				fTotalSize;
			off_t				fAbbreviationOffset;
			AbbreviationTable*	fAbbreviationTable;
			DIECompileUnitBase*	fUnitEntry;
			const SourceLanguageInfo* fSourceLanguage;
			TargetAddressRangeList* fAddressRanges;
			Array<DebugInfoEntry*> fEntries;
			Array<off_t>		fEntryOffsets;
			DirectoryList		fDirectories;
			FileList			fFiles;
			LineNumberProgram	fLineNumberProgram;
			uint8				fAddressSize;
			bool				fIsDwarf64;
};


target_addr_t
CompilationUnit::MaxAddress() const
{
	return fAddressSize == 4 ? 0xffffffffULL : 0xffffffffffffffffULL;
}


#endif	// COMPILATION_UNIT_H
