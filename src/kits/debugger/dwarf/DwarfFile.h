/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_FILE_H
#define DWARF_FILE_H


#include <ObjectList.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "DebugInfoEntries.h"
#include "TypeUnit.h"


struct AbbreviationEntry;
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

			status_t			StartLoading(const char* fileName,
									BString& _requiredExternalFile);
			status_t			Load(uint8 addressSize,
									const BString& externalFilePath);
			status_t			FinishLoading();

			const char*			Name() const		{ return fName; }
			ElfFile*			GetElfFile() const	{ return fElfFile; }

			bool				HasFrameInformation() const
									{ return fDebugFrameSection != NULL
										|| fEHFrameSection != NULL; }

			int32				CountCompilationUnits() const;
			CompilationUnit*	CompilationUnitAt(int32 index) const;
			CompilationUnit*	CompilationUnitForDIE(
									const DebugInfoEntry* entry) const;

			TargetAddressRangeList* ResolveRangeList(CompilationUnit* unit,
									uint64 offset) const;

			status_t			UnwindCallFrame(CompilationUnit* unit,
									uint8 addressSize,
									DIESubprogram* subprogramEntry,
									target_addr_t location,
									const DwarfTargetInterface* inputInterface,
									DwarfTargetInterface* outputInterface,
									target_addr_t& _framePointer);

			status_t			EvaluateExpression(CompilationUnit* unit,
									uint8 addressSize,
									DIESubprogram* subprogramEntry,
									const void* expression,
									off_t expressionLength,
									const DwarfTargetInterface* targetInterface,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									target_addr_t valueToPush, bool pushValue,
									target_addr_t& _result);
			status_t			ResolveLocation(CompilationUnit* unit,
									uint8 addressSize,
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
									uint8 addressSize,
									DIESubprogram* subprogramEntry,
									const ConstantAttributeValue* value,
									const DwarfTargetInterface* targetInterface,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									BVariant& _result);
			status_t			EvaluateDynamicValue(CompilationUnit* unit, uint8 addressSize,
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
			struct FDELookupInfo;

			typedef DoublyLinkedList<AbbreviationTable> AbbreviationTableList;
			typedef BObjectList<CompilationUnit> CompilationUnitList;
			typedef BOpenHashTable<TypeUnitTableHashDefinition> TypeUnitTable;
			typedef BObjectList<FDELookupInfo> FDEInfoList;

private:
			status_t			_ParseDebugInfoSection();
			status_t			_ParseTypesSection();
			status_t			_ParseFrameSection(ElfSection* section,
									uint8 addressSize, bool ehFrame,
									FDEInfoList& infos);
			status_t			_ParseCompilationUnit(CompilationUnit* unit);
			status_t			_ParseTypeUnit(TypeUnit* unit);
			status_t			_ParseDebugInfoEntry(DataReader& dataReader,
									BaseUnit* unit,
									AbbreviationTable* abbreviationTable,
									DebugInfoEntry*& _entry,
									bool& _endOfEntryList, int level = 0);
			status_t			_FinishUnit(BaseUnit* unit);
			status_t			_ParseEntryAttributes(DataReader& dataReader,
									BaseUnit* unit,
									DebugInfoEntry* entry,
									AbbreviationEntry& abbreviationEntry);

			status_t			_ParseLineInfo(CompilationUnit* unit);

			status_t			_UnwindCallFrame(CompilationUnit* unit,
									uint8 addressSize,
									DIESubprogram* subprogramEntry,
									target_addr_t location,
									const FDELookupInfo* info,
									const DwarfTargetInterface* inputInterface,
									DwarfTargetInterface* outputInterface,
									target_addr_t& _framePointer);

			status_t			_ParseCIEHeader(ElfSection* debugFrameSection,
									bool usingEHFrameSection,
									CompilationUnit* unit,
									uint8 addressSize,
									CfaContext& context, off_t cieOffset,
									CIEAugmentation& cieAugmentation,
									DataReader& reader,
									off_t& _cieRemaining);
			status_t			_ParseFrameInfoInstructions(
									CompilationUnit* unit, CfaContext& context,
									DataReader& dataReader,
									CIEAugmentation& cieAugmentation);

			status_t			_ParsePublicTypesInfo();
			status_t			_ParsePublicTypesInfo(DataReader& dataReader,
									bool dwarf64);

			status_t			_GetAbbreviationTable(off_t offset,
									AbbreviationTable*& _table);

			DebugInfoEntry*		_ResolveReference(BaseUnit* unit,
									uint64 offset,
									uint8 refType) const;

			status_t			_GetLocationExpression(CompilationUnit* unit,
									const LocationDescription* location,
									target_addr_t instructionPointer,
									const void*& _expression,
									off_t& _length) const;
			status_t			_FindLocationExpression(CompilationUnit* unit,
									uint64 offset, target_addr_t address,
									const void*& _expression,
									off_t& _length) const;

			status_t			_LocateDebugInfo(
									BString& _requiredExternalFileName,
									const char* locatedFilePath = NULL);

			status_t			_GetDebugInfoPath(const char* fileName,
									BString& _infoPath) const;

			TypeUnitTableEntry*	_GetTypeUnit(uint64 signature) const;
			CompilationUnit*	_GetContainingCompilationUnit(
									off_t refAddr) const;

			FDELookupInfo*		_GetContainingFDEInfo(
									target_addr_t offset) const;

			FDELookupInfo*		_GetContainingFDEInfo(
									target_addr_t offset,
									const FDEInfoList& infoList) const;

private:
			friend struct 		DwarfFile::ExpressionEvaluationContext;

private:
			char*				fName;
			char*				fAlternateName;
			ElfFile*			fElfFile;
			ElfFile*			fAlternateElfFile;
			ElfSection*			fDebugInfoSection;
			ElfSection*			fDebugAbbrevSection;
			ElfSection*			fDebugStringSection;
			ElfSection*			fDebugRangesSection;
			ElfSection*			fDebugLineSection;
			ElfSection*			fDebugFrameSection;
			ElfSection*			fEHFrameSection;
			ElfSection*			fDebugLocationSection;
			ElfSection*			fDebugPublicTypesSection;
			ElfSection*			fDebugTypesSection;
			AbbreviationTableList fAbbreviationTables;
			DebugInfoEntryFactory fDebugInfoFactory;
			CompilationUnitList	fCompilationUnits;
			TypeUnitTable		fTypeUnits;
			FDEInfoList			fDebugFrameInfos;
			FDEInfoList			fEHFrameInfos;
			bool				fTypesSectionRequired;
			bool				fFinished;
			bool				fItaniumEHFrameFormat;
			status_t			fFinishError;
};


#endif	// DWARF_FILE_H
