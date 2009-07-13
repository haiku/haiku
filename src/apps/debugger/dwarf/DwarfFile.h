/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_FILE_H
#define DWARF_FILE_H


#include <ObjectList.h>
#include <util/DoublyLinkedList.h>

#include "DebugInfoEntries.h"


class AbbreviationEntry;
class AbbreviationTable;
class CfaContext;
class CompilationUnit;
class DataReader;
class DwarfTargetInterface;
class ElfFile;
class ElfSection;
class TargetAddressRangeList;


class DwarfFile : public DoublyLinkedListLinkImpl<DwarfFile> {
public:
								DwarfFile();
								~DwarfFile();

			status_t			Load(const char* fileName);
			status_t			FinishLoading();

			const char*			Name() const		{ return fName; }
			ElfFile*			GetElfFile() const	{ return fElfFile; }

			int32				CountCompilationUnits() const;
			CompilationUnit*	CompilationUnitAt(int32 index) const;
			CompilationUnit*	CompilationUnitForDIE(
									const DebugInfoEntry* entry) const;

			status_t			UnwindCallFrame(CompilationUnit* unit,
									target_addr_t location,
									const DwarfTargetInterface* inputInterface,
									DwarfTargetInterface* outputInterface,
									target_addr_t& _framePointer);

private:
			typedef DoublyLinkedList<AbbreviationTable> AbbreviationTableList;
			typedef BObjectList<CompilationUnit> CompilationUnitList;

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

			status_t			_ParseLineInfo(CompilationUnit* unit);

			status_t			_ParseCIE(CompilationUnit* unit,
									CfaContext& context, off_t cieOffset);
			status_t			_ParseFrameInfoInstructions(
									CompilationUnit* unit, CfaContext& context,
									off_t instructionOffset,
									off_t instructionSize);

			status_t			_GetAbbreviationTable(off_t offset,
									AbbreviationTable*& _table);

			DebugInfoEntry*		_ResolveReference(uint64 offset,
									bool localReference) const;
			TargetAddressRangeList* _ResolveRangeList(uint64 offset);
									// returns reference

private:
			char*				fName;
			ElfFile*			fElfFile;
			ElfSection*			fDebugInfoSection;
			ElfSection*			fDebugAbbrevSection;
			ElfSection*			fDebugStringSection;
			ElfSection*			fDebugRangesSection;
			ElfSection*			fDebugLineSection;
			ElfSection*			fDebugFrameSection;
			AbbreviationTableList fAbbreviationTables;
			DebugInfoEntryFactory fDebugInfoFactory;
			CompilationUnitList	fCompilationUnits;
			CompilationUnit*	fCurrentCompilationUnit;
			bool				fFinished;
			status_t			fFinishError;
};


#endif	// DWARF_FILE_H
