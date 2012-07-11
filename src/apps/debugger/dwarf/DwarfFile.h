/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_FILE_H
#define DWARF_FILE_H


#include <ObjectList.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "DebugInfoEntries.h"


class AbbreviationEntry;
class AbbreviationTable;
class BVariant;
class CfaContext;
class CompilationUnit;
class DataReader;
class DwarfTargetInterface;
class ElfFile;
class ElfSection;
class TargetAddressRangeList;
class ValueLocation;


class DwarfFile : public BReferenceable,
	public DoublyLinkedListLinkImpl<DwarfFile> {
public:
								DwarfFile();
								~DwarfFile();

			status_t			Load(const char* fileName);
			status_t			FinishLoading();

			const char*			Name() const		{ return fName; }
			ElfFile*			GetElfFile() const	{ return fElfFile; }

			bool				HasDebugFrameSection() const {
									return fDebugFrameSection != NULL;
								}
			bool				HasEHFrameSection() const {
									return fEHFrameSection != NULL;
								}

			int32				CountCompilationUnits() const;
			CompilationUnit*	CompilationUnitAt(int32 index) const;
			CompilationUnit*	CompilationUnitForDIE(
									const DebugInfoEntry* entry) const;

			TargetAddressRangeList* ResolveRangeList(CompilationUnit* unit,
									uint64 offset) const;

			status_t			UnwindCallFrame(bool usingEHFrameSection,
									CompilationUnit* unit,
									DIESubprogram* subprogramEntry,
									target_addr_t location,
									const DwarfTargetInterface* inputInterface,
									DwarfTargetInterface* outputInterface,
									target_addr_t& _framePointer);

			status_t			EvaluateExpression(CompilationUnit* unit,
									DIESubprogram* subprogramEntry,
									const void* expression,
									off_t expressionLength,
									const DwarfTargetInterface* targetInterface,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									target_addr_t valueToPush, bool pushValue,
									target_addr_t& _result);
			status_t			ResolveLocation(CompilationUnit* unit,
									DIESubprogram* subprogramEntry,
									const LocationDescription* location,
									const DwarfTargetInterface* targetInterface,
									target_addr_t instructionPointer,
									target_addr_t objectPointer,
									bool hasObjectPointer,
									target_addr_t framePointer,
									target_addr_t relocationDelta,
									ValueLocation& _result);
									// The returned location will have DWARF
									// semantics regarding register numbers and
									// bit offsets/sizes (cf. bit pieces).

			status_t			EvaluateConstantValue(CompilationUnit* unit,
									DIESubprogram* subprogramEntry,
									const ConstantAttributeValue* value,
									const DwarfTargetInterface* targetInterface,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									BVariant& _result);
			status_t			EvaluateDynamicValue(CompilationUnit* unit,
									DIESubprogram* subprogramEntry,
									const DynamicAttributeValue* value,
									const DwarfTargetInterface* targetInterface,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									BVariant& _result, DIEType** _type = NULL);

private:
			struct ExpressionEvaluationContext;
			struct FDEAugmentation;
			struct CIEAugmentation;

			typedef DoublyLinkedList<AbbreviationTable> AbbreviationTableList;
			typedef BObjectList<CompilationUnit> CompilationUnitList;

private:
			status_t			_ParseCompilationUnit(CompilationUnit* unit);
			status_t			_ParseDebugInfoEntry(DataReader& dataReader,
									AbbreviationTable* abbreviationTable,
									DebugInfoEntry*& _entry,
									bool& _endOfEntryList, int level = 0);
			status_t			_FinishCompilationUnit(CompilationUnit* unit);
			status_t			_ParseEntryAttributes(DataReader& dataReader,
									DebugInfoEntry* entry,
									AbbreviationEntry& abbreviationEntry);

			status_t			_ParseLineInfo(CompilationUnit* unit);

			status_t			_ParseCIE(ElfSection* debugFrameSection,
									bool usingEHFrameSection,
									CompilationUnit* unit,
									CfaContext& context, off_t cieOffset,
									CIEAugmentation& cieAugmentation);
			status_t			_ParseFrameInfoInstructions(
									CompilationUnit* unit, CfaContext& context,
									DataReader& dataReader);

			status_t			_ParsePublicTypesInfo();
			status_t			_ParsePublicTypesInfo(DataReader& dataReader,
									bool dwarf64);

			status_t			_GetAbbreviationTable(off_t offset,
									AbbreviationTable*& _table);

			DebugInfoEntry*		_ResolveReference(CompilationUnit* unit,
									uint64 offset, bool localReference) const;

			status_t			_GetLocationExpression(CompilationUnit* unit,
									const LocationDescription* location,
									target_addr_t instructionPointer,
									const void*& _expression,
									off_t& _length) const;
			status_t			_FindLocationExpression(CompilationUnit* unit,
									uint64 offset, target_addr_t address,
									const void*& _expression,
									off_t& _length) const;

private:
			friend class 		DwarfFile::ExpressionEvaluationContext;

private:
			char*				fName;
			ElfFile*			fElfFile;
			ElfSection*			fDebugInfoSection;
			ElfSection*			fDebugAbbrevSection;
			ElfSection*			fDebugStringSection;
			ElfSection*			fDebugRangesSection;
			ElfSection*			fDebugLineSection;
			ElfSection*			fDebugFrameSection;
			ElfSection*			fEHFrameSection;
			ElfSection*			fDebugLocationSection;
			ElfSection*			fDebugPublicTypesSection;
			AbbreviationTableList fAbbreviationTables;
			DebugInfoEntryFactory fDebugInfoFactory;
			CompilationUnitList	fCompilationUnits;
			CompilationUnit*	fCurrentCompilationUnit;
			bool				fFinished;
			status_t			fFinishError;
};


#endif	// DWARF_FILE_H
