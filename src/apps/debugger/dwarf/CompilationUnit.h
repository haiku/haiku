/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef COMPILATION_UNIT_H
#define COMPILATION_UNIT_H

#include <ObjectList.h>
#include <String.h>

#include "BaseUnit.h"
#include "LineNumberProgram.h"
#include "Types.h"


class DIECompileUnitBase;
class TargetAddressRangeList;


class CompilationUnit : public BaseUnit {
public:
								CompilationUnit(off_t headerOffset,
									off_t contentOffset,
									off_t totalSize,
									off_t abbreviationOffset,
									uint8 addressSize, bool isDwarf64);
	virtual						~CompilationUnit();

	inline	target_addr_t		MaxAddress() const;

			DIECompileUnitBase*	UnitEntry() const	{ return fUnitEntry; }
			void				SetUnitEntry(DIECompileUnitBase* entry);

			TargetAddressRangeList* AddressRanges() const
									{ return fAddressRanges; }
			void				SetAddressRanges(
									TargetAddressRangeList* ranges);

			target_addr_t		AddressRangeBase() const;

			LineNumberProgram&	GetLineNumberProgram()
									{ return fLineNumberProgram; }

			bool				AddDirectory(const char* directory);
			int32				CountDirectories() const;
			const char*			DirectoryAt(int32 index) const;

			bool				AddFile(const char* fileName, int32 dirIndex);
			int32				CountFiles() const;
			const char*			FileAt(int32 index,
									const char** _directory = NULL) const;

	virtual	dwarf_unit_kind		Kind() const;

private:
			struct File;
			typedef BObjectList<BString> DirectoryList;
			typedef BObjectList<File> FileList;

private:
			DIECompileUnitBase*	fUnitEntry;
			TargetAddressRangeList* fAddressRanges;
			DirectoryList		fDirectories;
			FileList			fFiles;
			LineNumberProgram	fLineNumberProgram;
};


target_addr_t
CompilationUnit::MaxAddress() const
{
	return AddressSize() == 4 ? 0xffffffffULL : 0xffffffffffffffffULL;
}


#endif	// COMPILATION_UNIT_H
