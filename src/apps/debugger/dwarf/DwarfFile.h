/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_FILE_H
#define DWARF_FILE_H

#include <util/DoublyLinkedList.h>

#include "DebugInfoEntries.h"


class AbbreviationEntry;
class AbbreviationTable;
class CompilationUnit;
class DataReader;
class ElfFile;
class ElfSection;


class DwarfFile : public DoublyLinkedListLinkImpl<DwarfFile> {
public:
								DwarfFile();
								~DwarfFile();

			status_t			Load(const char* fileName);
			status_t			FinishLoading();

			const char*			Name() const	{ return fName; }

private:
			typedef DoublyLinkedList<AbbreviationTable> AbbreviationTableList;
			typedef DoublyLinkedList<CompilationUnit> CompilationUnitList;

private:
			status_t			_ParseCompilationUnit(CompilationUnit* unit);
			status_t			 _ParseDebugInfoEntry(DataReader& dataReader,
									AbbreviationTable* abbreviationTable,
									DebugInfoEntry*& _entry,
									bool& _endOfEntryList, int level = 0);
			status_t			_FinishCompilationUnit(CompilationUnit* unit);
			status_t			_ParseEntryAttributes(DataReader& dataReader,
									DebugInfoEntry* entry,
									AbbreviationEntry& abbreviationEntry);

			status_t			_GetAbbreviationTable(off_t offset,
									AbbreviationTable*& _table);

			DebugInfoEntry*		_ResolveReference(uint64 offset,
									bool localReference) const;

private:
			char*				fName;
			ElfFile*			fElfFile;
			ElfSection*			fDebugInfoSection;
			ElfSection*			fDebugAbbrevSection;
			ElfSection*			fDebugStringSection;
			AbbreviationTableList fAbbreviationTables;
			DebugInfoEntryFactory fDebugInfoFactory;
			CompilationUnitList	fCompilationUnits;
			CompilationUnit*	fCurrentCompilationUnit;
			bool				fFinished;
			status_t			fFinishError;
};


#endif	// DWARF_FILE_H
