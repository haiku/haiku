/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfFile.h"

#include <algorithm>
#include <new>

#include <AutoDeleter.h>

#include "AttributeClasses.h"
#include "AttributeValue.h"
#include "AbbreviationTable.h"
#include "CfaContext.h"
#include "CompilationUnit.h"
#include "DataReader.h"
#include "DwarfExpressionEvaluator.h"
#include "DwarfTargetInterface.h"
#include "ElfFile.h"
#include "TagNames.h"
#include "TargetAddressRangeList.h"
#include "Tracing.h"
#include "Variant.h"


// #pragma mark - ExpressionEvaluationContext


struct DwarfFile::ExpressionEvaluationContext
	: DwarfExpressionEvaluationContext {
public:
	ExpressionEvaluationContext(DwarfFile* file, CompilationUnit* unit,
		DIESubprogram* subprogramEntry,
		const DwarfTargetInterface* targetInterface,
		target_addr_t instructionPointer, target_addr_t objectPointer,
		bool hasObjectPointer, target_addr_t framePointer,
		target_addr_t relocationDelta)
		:
		DwarfExpressionEvaluationContext(targetInterface, unit->AddressSize(),
			relocationDelta),
		fFile(file),
		fUnit(unit),
		fSubprogramEntry(subprogramEntry),
		fInstructionPointer(instructionPointer),
		fObjectPointer(objectPointer),
		fHasObjectPointer(hasObjectPointer),
		fFramePointer(framePointer),
		fFrameBasePointer(0),
		fFrameBaseEvaluated(false)
	{
	}

	virtual bool GetObjectAddress(target_addr_t& _address)
	{
		if (!fHasObjectPointer)
			return false;

		_address = fObjectPointer;
		return true;
	}

	virtual bool GetFrameAddress(target_addr_t& _address)
	{
		if (fFramePointer == 0)
			return false;

		_address = fFramePointer;
		return true;
	}

	virtual bool GetFrameBaseAddress(target_addr_t& _address)
	{
		if (fFrameBaseEvaluated) {
			if (fFrameBasePointer == 0)
				return false;

			_address = fFrameBasePointer;
			return true;
		}

		// set flag already to prevent recursion for a buggy expression
		fFrameBaseEvaluated = true;

		// get the subprogram's frame base location
		if (fSubprogramEntry == NULL)
			return false;
		const LocationDescription* location = fSubprogramEntry->FrameBase();
		if (!location->IsValid())
			return false;

		// get the expression
		const void* expression;
		off_t expressionLength;
		status_t error = fFile->_GetLocationExpression(fUnit, location,
			fInstructionPointer, expression, expressionLength);
		if (error != B_OK)
			return false;

		// evaluate the expression
		DwarfExpressionEvaluator evaluator(this);
		error = evaluator.Evaluate(expression, expressionLength,
			fFrameBasePointer);
		if (error != B_OK)
			return false;

		TRACE_EXPR("  -> frame base: %llx\n", fFrameBasePointer);

		_address = fFrameBasePointer;
		return true;
	}

	virtual bool GetTLSAddress(target_addr_t localAddress,
		target_addr_t& _address)
	{
		// TODO:...
		return false;
	}

	virtual status_t GetCallTarget(uint64 offset, bool local,
		const void*& _block, off_t& _size)
	{
		// resolve the entry
		DebugInfoEntry* entry = fFile->_ResolveReference(fUnit, offset, local);
		if (entry == NULL)
			return B_ENTRY_NOT_FOUND;

		// get the location description
		LocationDescription* location = entry->GetLocationDescription();
		if (location == NULL || !location->IsValid()) {
			_block = NULL;
			_size = 0;
			return B_OK;
		}

		// get the expression
		return fFile->_GetLocationExpression(fUnit, location,
			fInstructionPointer, _block, _size);
	}

private:
	DwarfFile*			fFile;
	CompilationUnit*	fUnit;
	DIESubprogram*		fSubprogramEntry;
	target_addr_t		fInstructionPointer;
	target_addr_t		fObjectPointer;
	bool				fHasObjectPointer;
	target_addr_t		fFramePointer;
	target_addr_t		fFrameBasePointer;
	bool				fFrameBaseEvaluated;
};


// #pragma mark - FDEAugmentation


struct DwarfFile::FDEAugmentation {
	// Currently we're ignoring all augmentation data.
};


// #pragma mark - CIEAugmentation


enum {
	CFI_AUGMENTATION_DATA					= 0x01,
	CFI_AUGMENTATION_LANGUAGE_SPECIFIC_DATA	= 0x02,
	CFI_AUGMENTATION_PERSONALITY			= 0x04,
	CFI_AUGMENTATION_ADDRESS_POINTER_FORMAT	= 0x08,
};


struct DwarfFile::CIEAugmentation {
	CIEAugmentation()
		:
		fString(NULL),
		fFlags(0)
	{
	}

	void Init(DataReader& dataReader)
	{
		fFlags = 0;
		fString = dataReader.ReadString();
	}

	status_t Read(DataReader& dataReader)
	{
		if (fString == NULL || *fString == '\0')
			return B_OK;

		if (*fString == 'z') {
			// There are augmentation data.
			fFlags |= CFI_AUGMENTATION_DATA;
			const char* string = fString + 1;

			// let's see what data we have to expect
			while (*string != '\0') {
				switch (*string) {
					case 'L':
						fFlags |= CFI_AUGMENTATION_LANGUAGE_SPECIFIC_DATA;
						break;
					case 'P':
						fFlags |= CFI_AUGMENTATION_PERSONALITY;
						break;
					case 'R':
						fFlags |= CFI_AUGMENTATION_ADDRESS_POINTER_FORMAT;
						break;
					default:
						return B_UNSUPPORTED;
				}
				string++;
			}

			// read the augmentation data block -- it is preceeded by an
			// LEB128 indicating the length of the data block
			uint64 length = dataReader.ReadUnsignedLEB128(0);
			dataReader.Skip(length);
				// TODO: Actually read what is interesting for us! The
				// CFI_AUGMENTATION_ADDRESS_POINTER_FORMAT might be. The
				// specs are not saying much about it.

			TRACE_CFI("    %" B_PRIu64 " bytes of augmentation data\n", length);

			if (dataReader.HasOverflow())
				return B_BAD_DATA;

			return B_OK;
		}

		// nothing to do
		if (strcmp(fString, "eh") == 0)
			return B_OK;

		// something we can't handle
		return B_UNSUPPORTED;
	}

	status_t ReadFDEData(DataReader& dataReader,
		FDEAugmentation& fdeAugmentation)
	{
		if (!HasData())
			return B_OK;

		// read the augmentation data block -- it is preceeded by an LEB128
		// indicating the length of the data block
		uint64 length = dataReader.ReadUnsignedLEB128(0);
		dataReader.Skip(length);
			// TODO: Actually read what is interesting for us!

		TRACE_CFI("    %" B_PRIu64 " bytes of augmentation data\n", length);

		if (dataReader.HasOverflow())
			return B_BAD_DATA;

		return B_OK;
	}

	const char* String() const
	{
		return fString;
	}

	bool HasData() const
	{
		return (fFlags & CFI_AUGMENTATION_DATA) != 0;
	}

private:
	const char*	fString;
	uint32		fFlags;
};


// #pragma mark - DwarfFile


DwarfFile::DwarfFile()
	:
	fName(NULL),
	fElfFile(NULL),
	fDebugInfoSection(NULL),
	fDebugAbbrevSection(NULL),
	fDebugStringSection(NULL),
	fDebugRangesSection(NULL),
	fDebugLineSection(NULL),
	fDebugFrameSection(NULL),
	fEHFrameSection(NULL),
	fDebugLocationSection(NULL),
	fDebugPublicTypesSection(NULL),
	fCompilationUnits(20, true),
	fCurrentCompilationUnit(NULL),
	fFinished(false),
	fFinishError(B_OK)
{
}


DwarfFile::~DwarfFile()
{
	while (AbbreviationTable* table = fAbbreviationTables.RemoveHead())
		delete table;

	if (fElfFile != NULL) {
		fElfFile->PutSection(fDebugInfoSection);
		fElfFile->PutSection(fDebugAbbrevSection);
		fElfFile->PutSection(fDebugStringSection);
		fElfFile->PutSection(fDebugRangesSection);
		fElfFile->PutSection(fDebugLineSection);
		fElfFile->PutSection(fDebugFrameSection);
		fElfFile->PutSection(fEHFrameSection);
		fElfFile->PutSection(fDebugLocationSection);
		fElfFile->PutSection(fDebugPublicTypesSection);
		delete fElfFile;
	}

	free(fName);
}


status_t
DwarfFile::Load(const char* fileName)
{
	fName = strdup(fileName);
	if (fName == NULL)
		return B_NO_MEMORY;

	// load the ELF file
	fElfFile = new(std::nothrow) ElfFile;
	if (fElfFile == NULL)
		return B_NO_MEMORY;

	status_t error = fElfFile->Init(fileName);
	if (error != B_OK)
		return error;

	// get the interesting sections
	fDebugInfoSection = fElfFile->GetSection(".debug_info");
	fDebugAbbrevSection = fElfFile->GetSection(".debug_abbrev");
	if (fDebugInfoSection == NULL || fDebugAbbrevSection == NULL) {
		WARNING("DwarfManager::File::Load(\"%s\"): no "
			".debug_info or .debug_abbrev.\n", fileName);
		return B_ERROR;
	}

	// non mandatory sections
	fDebugStringSection = fElfFile->GetSection(".debug_str");
	fDebugRangesSection = fElfFile->GetSection(".debug_ranges");
	fDebugLineSection = fElfFile->GetSection(".debug_line");
	fDebugFrameSection = fElfFile->GetSection(".debug_frame");
	fEHFrameSection = fElfFile->GetSection(".eh_frame");
	fDebugLocationSection = fElfFile->GetSection(".debug_loc");
	fDebugPublicTypesSection = fElfFile->GetSection(".debug_pubtypes");

	// iterate through the debug info section
	DataReader dataReader(fDebugInfoSection->Data(),
		fDebugInfoSection->Size(), 4);
			// address size doesn't matter here
	while (dataReader.HasData()) {
		off_t unitHeaderOffset = dataReader.Offset();
		bool dwarf64;
		uint64 unitLength = dataReader.ReadInitialLength(dwarf64);

		off_t unitLengthOffset = dataReader.Offset();
			// the unitLength starts here

		if (unitLengthOffset + unitLength
				> (uint64)fDebugInfoSection->Size()) {
			WARNING("\"%s\": Invalid compilation unit length.\n", fileName);
			break;
		}

		int version = dataReader.Read<uint16>(0);
		off_t abbrevOffset = dwarf64
			? dataReader.Read<uint64>(0)
			: dataReader.Read<uint32>(0);
		uint8 addressSize = dataReader.Read<uint8>(0);

		if (dataReader.HasOverflow()) {
			WARNING("\"%s\": Unexpected end of data in compilation unit "
				"header.\n", fileName);
			break;
		}

		TRACE_DIE("DWARF%d compilation unit: version %d, length: %lld, "
			"abbrevOffset: %lld, address size: %d\n", dwarf64 ? 64 : 32,
			version, unitLength, abbrevOffset, addressSize);

		if (version != 2 && version != 3) {
			WARNING("\"%s\": Unsupported compilation unit version: %d\n",
				fileName, version);
			break;
		}

		if (addressSize != 4 && addressSize != 8) {
			WARNING("\"%s\": Unsupported address size: %d\n", fileName,
				addressSize);
			break;
		}
		dataReader.SetAddressSize(addressSize);

		off_t unitContentOffset = dataReader.Offset();

		// create a compilation unit object
		CompilationUnit* unit = new(std::nothrow) CompilationUnit(
			unitHeaderOffset, unitContentOffset,
			unitLength + (unitLengthOffset - unitHeaderOffset),
			abbrevOffset, addressSize, dwarf64);
		if (unit == NULL || !fCompilationUnits.AddItem(unit)) {
			delete unit;
			return B_NO_MEMORY;
		}

		// parse the debug info for the unit
		fCurrentCompilationUnit = unit;
		error = _ParseCompilationUnit(unit);
		if (error != B_OK)
			return error;

		dataReader.SeekAbsolute(unitLengthOffset + unitLength);
	}

	return B_OK;
}


status_t
DwarfFile::FinishLoading()
{
	if (fFinished)
		return B_OK;
	if (fFinishError != B_OK)
		return fFinishError;

	for (int32 i = 0; CompilationUnit* unit = fCompilationUnits.ItemAt(i);
			i++) {
		fCurrentCompilationUnit = unit;
		status_t error = _FinishCompilationUnit(unit);
		if (error != B_OK)
			return fFinishError = error;
	}

	_ParsePublicTypesInfo();

	fFinished = true;
	return B_OK;
}


int32
DwarfFile::CountCompilationUnits() const
{
	return fCompilationUnits.CountItems();
}


CompilationUnit*
DwarfFile::CompilationUnitAt(int32 index) const
{
	return fCompilationUnits.ItemAt(index);
}


CompilationUnit*
DwarfFile::CompilationUnitForDIE(const DebugInfoEntry* entry) const
{
	// find the root of the tree the entry lives in
	while (entry != NULL && entry->Parent() != NULL)
		entry = entry->Parent();

	// that should be the compilation unit entry
	const DIECompileUnitBase* unitEntry
		= dynamic_cast<const DIECompileUnitBase*>(entry);
	if (unitEntry == NULL)
		return NULL;

	// find the compilation unit
	for (int32 i = 0; CompilationUnit* unit = fCompilationUnits.ItemAt(i);
			i++) {
		if (unit->UnitEntry() == unitEntry)
			return unit;
	}

	return NULL;
}


TargetAddressRangeList*
DwarfFile::ResolveRangeList(CompilationUnit* unit, uint64 offset) const
{
	if (unit == NULL || fDebugRangesSection == NULL)
		return NULL;

	if (offset < 0 || offset >= (uint64)fDebugRangesSection->Size())
		return NULL;

	TargetAddressRangeList* ranges = new(std::nothrow) TargetAddressRangeList;
	if (ranges == NULL) {
		ERROR("Out of memory.\n");
		return NULL;
	}
	BReference<TargetAddressRangeList> rangesReference(ranges, true);

	target_addr_t baseAddress = unit->AddressRangeBase();
	target_addr_t maxAddress = unit->MaxAddress();

	DataReader dataReader((uint8*)fDebugRangesSection->Data() + offset,
		fDebugRangesSection->Size() - offset, unit->AddressSize());
	while (true) {
		target_addr_t start = dataReader.ReadAddress(0);
		target_addr_t end = dataReader.ReadAddress(0);
		if (dataReader.HasOverflow())
			return NULL;

		if (start == 0 && end == 0)
			break;
		if (start == maxAddress) {
			baseAddress = end;
			continue;
		}
		if (start == end)
			continue;

		if (!ranges->AddRange(baseAddress + start, end - start)) {
			ERROR("Out of memory.\n");
			return NULL;
		}
	}

	return rangesReference.Detach();
}


status_t
DwarfFile::UnwindCallFrame(bool usingEHFrameSection, CompilationUnit* unit,
	DIESubprogram* subprogramEntry, target_addr_t location,
	const DwarfTargetInterface* inputInterface,
	DwarfTargetInterface* outputInterface, target_addr_t& _framePointer)
{
	ElfSection* currentFrameSection = (usingEHFrameSection)
		? fEHFrameSection : fDebugFrameSection;

	if (currentFrameSection == NULL)
		return B_ENTRY_NOT_FOUND;

	bool gcc4EHFrameSection = false;
	if (usingEHFrameSection) {
		gcc4EHFrameSection = !currentFrameSection->IsWritable();
			// Crude heuristic for recognizing GCC 4 (Itanium ABI) style
			// .eh_frame sections. The ones generated by GCC 2 are writable,
			// the ones generated by GCC 4 aren't.
	}



	TRACE_CFI("DwarfFile::UnwindCallFrame(%#llx)\n", location);

	DataReader dataReader((uint8*)currentFrameSection->Data(),
		currentFrameSection->Size(), unit->AddressSize());

	while (dataReader.BytesRemaining() > 0) {
		// length
		bool dwarf64;
		TRACE_CFI_ONLY(off_t entryOffset = dataReader.Offset();)
		uint64 length = dataReader.ReadInitialLength(dwarf64);

		TRACE_CFI("DwarfFile::UnwindCallFrame(): offset: %" B_PRIdOFF
			", length: %" B_PRId64 "\n", entryOffset, length);

		if (length > (uint64)dataReader.BytesRemaining())
			return B_BAD_DATA;
		off_t lengthOffset = dataReader.Offset();

		// CIE ID/CIE pointer
		uint64 cieID = dwarf64
			? dataReader.Read<uint64>(0) : dataReader.Read<uint32>(0);

		// In .debug_frame ~0 indicates a CIE, in .eh_frame 0 does.
		if (usingEHFrameSection
			? cieID == 0
			: (dwarf64
				? cieID == 0xffffffffffffffffULL
				: cieID == 0xffffffff)) {
			// this is a CIE -- skip it
		} else {
			// this is a FDE
			uint64 initialLocationOffset = dataReader.Offset();
			target_addr_t initialLocation = dataReader.ReadAddress(0);
			target_size_t addressRange = dataReader.ReadAddress(0);

			if (dataReader.HasOverflow())
				return B_BAD_DATA;

			// In the GCC 4 .eh_frame initialLocation is relative to the offset
			// of the address.
			if (usingEHFrameSection && gcc4EHFrameSection) {
				// Note: We need to cast to the exact address width, since the
				// initialLocation value can be (and likely is) negative.
				if (dwarf64) {
					initialLocation = (uint64)currentFrameSection
						->LoadAddress()	+ (uint64)initialLocationOffset
						+ (uint64)initialLocation;
				} else {
					initialLocation = (uint32)currentFrameSection
						->LoadAddress()	+ (uint32)initialLocationOffset
						+ (uint32)initialLocation;
				}
			}
			// TODO: For GCC 2 .eh_frame sections things work differently: The
			// initial locations are relocated by the runtime loader and
			// afterwards point to the absolute addresses. Fortunately the
			// relocations that always seem to be used are R_386_RELATIVE, so
			// that the value we read from the file is already absolute
			// (assuming an unchanged segment load address).

			TRACE_CFI("location: %" B_PRIx64 ", initial location: %" B_PRIx64
				", address range: %" B_PRIx64 "\n", location, initialLocation,
				addressRange);

			if (location >= initialLocation
				&& location < initialLocation + addressRange) {
				// This is the FDE we're looking for.
				off_t remaining = lengthOffset + length
					- dataReader.Offset();
				if (remaining < 0)
					return B_BAD_DATA;

				// In .eh_frame the CIE offset is a relative back offset.
				if (usingEHFrameSection) {
					if (cieID > (uint64)lengthOffset) {
						TRACE_CFI("Invalid CIE offset: %" B_PRIu64 ", max "
							"possible: %" B_PRIu64 "\n", cieID, lengthOffset);
						break;
					}
					// convert to a section relative offset
					cieID = lengthOffset - cieID;
				}

				TRACE_CFI("  found fde: length: %llu (%lld), CIE offset: %#llx, "
					"location: %#llx, range: %#llx\n", length, remaining, cieID,
					initialLocation, addressRange);

				CfaContext context(location, initialLocation);
				uint32 registerCount = outputInterface->CountRegisters();
				status_t error = context.Init(registerCount);
				if (error != B_OK)
					return error;

				error = outputInterface->InitRegisterRules(context);
				if (error != B_OK)
					return error;

				// process the CIE
				CIEAugmentation cieAugmentation;
				error = _ParseCIE(currentFrameSection, usingEHFrameSection,
					unit, context, cieID, cieAugmentation);
				if (error != B_OK)
					return error;

				// read the FDE augmentation data (if any)
				FDEAugmentation fdeAugmentation;
				error = cieAugmentation.ReadFDEData(dataReader,
					fdeAugmentation);
				if (error != B_OK) {
					TRACE_CFI("  failed to read FDE augmentation data!\n");
					return error;
				}
				// adjust remaining byte count to take augmentation bytes
				// (if any) into account.
				remaining = lengthOffset + length
					- dataReader.Offset();

				error = context.SaveInitialRuleSet();
				if (error != B_OK)
					return error;

				DataReader restrictedReader =
					dataReader.RestrictedReader(remaining);
				error = _ParseFrameInfoInstructions(unit, context,
					restrictedReader);
				if (error != B_OK)
					return error;

				TRACE_CFI("  found row!\n");

				// apply the rules of the final row
				// get the frameAddress first
				target_addr_t frameAddress;
				CfaCfaRule* cfaCfaRule = context.GetCfaCfaRule();
				switch (cfaCfaRule->Type()) {
					case CFA_CFA_RULE_REGISTER_OFFSET:
					{
						BVariant value;
						if (!inputInterface->GetRegisterValue(
								cfaCfaRule->Register(), value)
							|| !value.IsNumber()) {
							return B_UNSUPPORTED;
						}
						frameAddress = value.ToUInt64() + cfaCfaRule->Offset();
						break;
					}
					case CFA_CFA_RULE_EXPRESSION:
					{
						error = EvaluateExpression(unit, subprogramEntry,
							cfaCfaRule->Expression().block,
							cfaCfaRule->Expression().size,
							inputInterface, location, 0, 0, false,
							frameAddress);
						if (error != B_OK)
							return error;
						break;
					}
					case CFA_CFA_RULE_UNDEFINED:
					default:
						return B_BAD_VALUE;
				}

				TRACE_CFI("  frame address: %#llx\n", frameAddress);

				// apply the register rules
				for (uint32 i = 0; i < registerCount; i++) {
					TRACE_CFI("  reg %lu\n", i);

					uint32 valueType = outputInterface->RegisterValueType(i);
					if (valueType == 0)
						continue;

					CfaRule* rule = context.RegisterRule(i);
					if (rule == NULL)
						continue;

					// apply the rule
					switch (rule->Type()) {
						case CFA_RULE_SAME_VALUE:
						{
							TRACE_CFI("  -> CFA_RULE_SAME_VALUE\n");

							BVariant value;
							if (inputInterface->GetRegisterValue(i, value))
								outputInterface->SetRegisterValue(i, value);
							break;
						}
						case CFA_RULE_LOCATION_OFFSET:
						{
							TRACE_CFI("  -> CFA_RULE_LOCATION_OFFSET: %lld\n",
								rule->Offset());

							BVariant value;
							if (inputInterface->ReadValueFromMemory(
									frameAddress + rule->Offset(), valueType,
									value)) {
								outputInterface->SetRegisterValue(i, value);
							}
							break;
						}
						case CFA_RULE_VALUE_OFFSET:
							TRACE_CFI("  -> CFA_RULE_VALUE_OFFSET\n");

							outputInterface->SetRegisterValue(i,
								frameAddress + rule->Offset());
							break;
						case CFA_RULE_REGISTER:
						{
							TRACE_CFI("  -> CFA_RULE_REGISTER\n");

							BVariant value;
							if (inputInterface->GetRegisterValue(
									rule->Register(), value)) {
								outputInterface->SetRegisterValue(i, value);
							}
							break;
						}
						case CFA_RULE_LOCATION_EXPRESSION:
						{
							TRACE_CFI("  -> CFA_RULE_LOCATION_EXPRESSION\n");

							target_addr_t address;
							error = EvaluateExpression(unit, subprogramEntry,
								rule->Expression().block,
								rule->Expression().size,
								inputInterface, location, frameAddress,
								frameAddress, true, address);
							BVariant value;
							if (error == B_OK
								&& inputInterface->ReadValueFromMemory(address,
									valueType, value)) {
								outputInterface->SetRegisterValue(i, value);
							}
							break;
						}
						case CFA_RULE_VALUE_EXPRESSION:
						{
							TRACE_CFI("  -> CFA_RULE_VALUE_EXPRESSION\n");

							target_addr_t value;
							error = EvaluateExpression(unit, subprogramEntry,
								rule->Expression().block,
								rule->Expression().size,
								inputInterface, location, frameAddress,
								frameAddress, true, value);
							if (error == B_OK)
								outputInterface->SetRegisterValue(i, value);
							break;
						}
						case CFA_RULE_UNDEFINED:
							TRACE_CFI("  -> CFA_RULE_UNDEFINED\n");
						default:
							break;
					}
				}

				_framePointer = frameAddress;
				return B_OK;
			}
		}

		dataReader.SeekAbsolute(lengthOffset + length);
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DwarfFile::EvaluateExpression(CompilationUnit* unit,
	DIESubprogram* subprogramEntry, const void* expression,
	off_t expressionLength, const DwarfTargetInterface* targetInterface,
	target_addr_t instructionPointer, target_addr_t framePointer,
	target_addr_t valueToPush, bool pushValue, target_addr_t& _result)
{
	ExpressionEvaluationContext context(this, unit, subprogramEntry,
		targetInterface, instructionPointer, 0, false, framePointer, 0);
	DwarfExpressionEvaluator evaluator(&context);

	if (pushValue && evaluator.Push(valueToPush) != B_OK)
		return B_NO_MEMORY;

	return evaluator.Evaluate(expression, expressionLength, _result);
}


status_t
DwarfFile::ResolveLocation(CompilationUnit* unit,
	DIESubprogram* subprogramEntry, const LocationDescription* location,
	const DwarfTargetInterface* targetInterface,
	target_addr_t instructionPointer, target_addr_t objectPointer,
	bool hasObjectPointer, target_addr_t framePointer,
	target_addr_t relocationDelta, ValueLocation& _result)
{
	// get the expression
	const void* expression;
	off_t expressionLength;
	status_t error = _GetLocationExpression(unit, location, instructionPointer,
		expression, expressionLength);
	if (error != B_OK)
		return error;

	// evaluate it
	ExpressionEvaluationContext context(this, unit, subprogramEntry,
		targetInterface, instructionPointer, objectPointer, hasObjectPointer,
		framePointer, relocationDelta);
	DwarfExpressionEvaluator evaluator(&context);
	return evaluator.EvaluateLocation(expression, expressionLength,
		_result);
}


status_t
DwarfFile::EvaluateConstantValue(CompilationUnit* unit,
	DIESubprogram* subprogramEntry, const ConstantAttributeValue* value,
	const DwarfTargetInterface* targetInterface,
	target_addr_t instructionPointer, target_addr_t framePointer,
	BVariant& _result)
{
	if (!value->IsValid())
		return B_BAD_VALUE;

	switch (value->attributeClass) {
		case ATTRIBUTE_CLASS_CONSTANT:
			_result.SetTo(value->constant);
			return B_OK;
		case ATTRIBUTE_CLASS_STRING:
			_result.SetTo(value->string);
			return B_OK;
		case ATTRIBUTE_CLASS_BLOCK:
		{
			target_addr_t result;
			status_t error = EvaluateExpression(unit, subprogramEntry,
				value->block.data, value->block.length, targetInterface,
				instructionPointer, framePointer, 0, false, result);
			if (error != B_OK)
				return error;

			_result.SetTo(result);
			return B_OK;
		}
		default:
			return B_BAD_VALUE;
	}
}


status_t
DwarfFile::EvaluateDynamicValue(CompilationUnit* unit,
	DIESubprogram* subprogramEntry, const DynamicAttributeValue* value,
	const DwarfTargetInterface* targetInterface,
	target_addr_t instructionPointer, target_addr_t framePointer,
	BVariant& _result, DIEType** _type)
{
	if (!value->IsValid())
		return B_BAD_VALUE;

	DIEType* dummyType;
	if (_type == NULL)
		_type = &dummyType;

	switch (value->attributeClass) {
		case ATTRIBUTE_CLASS_CONSTANT:
			_result.SetTo(value->constant);
			*_type = NULL;
			return B_OK;

		case ATTRIBUTE_CLASS_REFERENCE:
		{
			// TODO: The specs are a bit fuzzy on this one: "the value is a
			// reference to another entity whose value is the value of the
			// attribute". Supposedly that also means e.g. if the referenced
			// entity is a variable, we should read the value of that variable.
			// ATM we only check for the types that can have a DW_AT_const_value
			// attribute and evaluate it, if present.
			DebugInfoEntry* entry = value->reference;
			if (entry == NULL)
				return B_BAD_VALUE;

			const ConstantAttributeValue* constantValue = NULL;
			DIEType* type = NULL;

			switch (entry->Tag()) {
				case DW_TAG_constant:
				{
					DIEConstant* constantEntry
						= dynamic_cast<DIEConstant*>(entry);
					constantValue = constantEntry->ConstValue();
					type = constantEntry->GetType();
					break;
				}
				case DW_TAG_enumerator:
					constantValue = dynamic_cast<DIEEnumerator*>(entry)
						->ConstValue();
					if (DIEEnumerationType* enumerationType
							= dynamic_cast<DIEEnumerationType*>(
								entry->Parent())) {
						type = enumerationType->GetType();
					}
					break;
				case DW_TAG_formal_parameter:
				{
					DIEFormalParameter* parameterEntry
						= dynamic_cast<DIEFormalParameter*>(entry);
					constantValue = parameterEntry->ConstValue();
					type = parameterEntry->GetType();
					break;
				}
				case DW_TAG_template_value_parameter:
				{
					DIETemplateValueParameter* parameterEntry
						= dynamic_cast<DIETemplateValueParameter*>(entry);
					constantValue = parameterEntry->ConstValue();
					type = parameterEntry->GetType();
					break;
				}
				case DW_TAG_variable:
				{
					DIEVariable* variableEntry
						= dynamic_cast<DIEVariable*>(entry);
					constantValue = variableEntry->ConstValue();
					type = variableEntry->GetType();
					break;
				}
				default:
					return B_BAD_VALUE;
			}

			if (constantValue == NULL || !constantValue->IsValid())
				return B_BAD_VALUE;

			status_t error = EvaluateConstantValue(unit, subprogramEntry,
				constantValue, targetInterface, instructionPointer,
				framePointer, _result);
			if (error != B_OK)
				return error;

			*_type = type;
			return B_OK;
		}

		case ATTRIBUTE_CLASS_BLOCK:
		{
			target_addr_t result;
			status_t error = EvaluateExpression(unit, subprogramEntry,
				value->block.data, value->block.length, targetInterface,
				instructionPointer, framePointer, 0, false, result);
			if (error != B_OK)
				return error;

			_result.SetTo(result);
			*_type = NULL;
			return B_OK;
		}

		default:
			return B_BAD_VALUE;
	}
}


status_t
DwarfFile::_ParseCompilationUnit(CompilationUnit* unit)
{
	AbbreviationTable* abbreviationTable;
	status_t error = _GetAbbreviationTable(unit->AbbreviationOffset(),
		abbreviationTable);
	if (error != B_OK)
		return error;

	unit->SetAbbreviationTable(abbreviationTable);

	DataReader dataReader(
		(const uint8*)fDebugInfoSection->Data() + unit->ContentOffset(),
		unit->ContentSize(), unit->AddressSize());

	DebugInfoEntry* entry;
	bool endOfEntryList;
	error = _ParseDebugInfoEntry(dataReader, abbreviationTable, entry,
		endOfEntryList);
	if (error != B_OK)
		return error;

	DIECompileUnitBase* unitEntry = dynamic_cast<DIECompileUnitBase*>(entry);
	if (unitEntry == NULL) {
		WARNING("No compilation unit entry in .debug_info section.\n");
		return B_BAD_DATA;
	}

	unit->SetUnitEntry(unitEntry);

	TRACE_DIE_ONLY(
		TRACE_DIE("remaining bytes in unit: %lld\n",
			dataReader.BytesRemaining());
		if (dataReader.HasData()) {
			TRACE_DIE("  ");
			while (dataReader.HasData())
				TRACE_DIE("%02x", dataReader.Read<uint8>(0));
			TRACE_DIE("\n");
		}
	)
	return B_OK;
}


status_t
DwarfFile::_ParseDebugInfoEntry(DataReader& dataReader,
	AbbreviationTable* abbreviationTable, DebugInfoEntry*& _entry,
	bool& _endOfEntryList, int level)
{
	off_t entryOffset = dataReader.Offset()
		+ fCurrentCompilationUnit->RelativeContentOffset();

	uint32 code = dataReader.ReadUnsignedLEB128(0);
	if (code == 0) {
		if (dataReader.HasOverflow()) {
			WARNING("Unexpected end of .debug_info section.\n");
			return B_BAD_DATA;
		}
		_entry = NULL;
		_endOfEntryList = true;
		return B_OK;
	}

	// get the corresponding abbreviation entry
	AbbreviationEntry abbreviationEntry;
	if (!abbreviationTable->GetAbbreviationEntry(code, abbreviationEntry)) {
		WARNING("No abbreviation entry for code %lu\n", code);
		return B_BAD_DATA;
	}

	DebugInfoEntry* entry;
	status_t error = fDebugInfoFactory.CreateDebugInfoEntry(
		abbreviationEntry.Tag(), entry);
	if (error != B_OK)
		return error;
	ObjectDeleter<DebugInfoEntry> entryDeleter(entry);

	TRACE_DIE("%*sentry %p at %lld: %lu, tag: %s (%lu), children: %d\n",
		level * 2, "", entry, entryOffset, abbreviationEntry.Code(),
		get_entry_tag_name(abbreviationEntry.Tag()), abbreviationEntry.Tag(),
		abbreviationEntry.HasChildren());

	error = fCurrentCompilationUnit->AddDebugInfoEntry(entry, entryOffset);
	if (error != B_OK)
		return error;

	// parse the attributes (supply NULL entry to avoid adding them yet)
	error = _ParseEntryAttributes(dataReader, NULL, abbreviationEntry);
	if (error != B_OK)
		return error;

	// parse children, if the entry has any
	if (abbreviationEntry.HasChildren()) {
		while (true) {
			DebugInfoEntry* childEntry;
			bool endOfEntryList;
			status_t error = _ParseDebugInfoEntry(dataReader,
				abbreviationTable, childEntry, endOfEntryList, level + 1);
			if (error != B_OK)
				return error;

			// add the child to our entry
			if (childEntry != NULL) {
				if (entry != NULL) {
					error = entry->AddChild(childEntry);
					if (error == B_OK) {
						childEntry->SetParent(entry);
					} else if (error == ENTRY_NOT_HANDLED) {
						error = B_OK;
						TRACE_DIE("%*s  -> child unhandled\n", level * 2, "");
					}

					if (error != B_OK) {
						delete childEntry;
						return error;
					}
				} else
					delete childEntry;
			}

			if (endOfEntryList)
				break;
		}
	}

	entryDeleter.Detach();
	_entry = entry;
	_endOfEntryList = false;
	return B_OK;
}


status_t
DwarfFile::_FinishCompilationUnit(CompilationUnit* unit)
{
	TRACE_DIE("\nfinishing compilation unit %p\n", unit);

	AbbreviationTable* abbreviationTable = unit->GetAbbreviationTable();

	DataReader dataReader(
		(const uint8*)fDebugInfoSection->Data() + unit->HeaderOffset(),
		unit->TotalSize(), unit->AddressSize());

	DebugInfoEntryInitInfo entryInitInfo;

	int entryCount = unit->CountEntries();
	for (int i = 0; i < entryCount; i++) {
		// get the entry
		DebugInfoEntry* entry;
		off_t offset;
		unit->GetEntryAt(i, entry, offset);

		TRACE_DIE("entry %p at %lld\n", entry, offset);

		// seek the reader to the entry
		dataReader.SeekAbsolute(offset);

		// read the entry code
		uint32 code = dataReader.ReadUnsignedLEB128(0);

		// get the respective abbreviation entry
		AbbreviationEntry abbreviationEntry;
		abbreviationTable->GetAbbreviationEntry(code, abbreviationEntry);

		// initialization before setting the attributes
		status_t error = entry->InitAfterHierarchy(entryInitInfo);
		if (error != B_OK) {
			WARNING("Init after hierarchy failed!\n");
			return error;
		}

		// parse the attributes -- this time pass the entry, so that the
		// attribute get set on it
		error = _ParseEntryAttributes(dataReader, entry, abbreviationEntry);
		if (error != B_OK)
			return error;

		// initialization after setting the attributes
		error = entry->InitAfterAttributes(entryInitInfo);
		if (error != B_OK) {
			WARNING("Init after attributes failed!\n");
			return error;
		}
	}

	// set the compilation unit's source language
	unit->SetSourceLanguage(entryInitInfo.languageInfo);

	// resolve the compilation unit's address range list
	if (TargetAddressRangeList* ranges = ResolveRangeList(unit,
			unit->UnitEntry()->AddressRangesOffset())) {
		unit->SetAddressRanges(ranges);
		ranges->ReleaseReference();
	}

	// add compilation dir to directory list
	const char* compilationDir = unit->UnitEntry()->CompilationDir();
	if (!unit->AddDirectory(compilationDir != NULL ? compilationDir : "."))
		return B_NO_MEMORY;

	// parse line info header
	if (fDebugLineSection != NULL)
		_ParseLineInfo(unit);

	return B_OK;
}


status_t
DwarfFile::_ParseEntryAttributes(DataReader& dataReader,
	DebugInfoEntry* entry, AbbreviationEntry& abbreviationEntry)
{
	uint32 attributeName;
	uint32 attributeForm;
	while (abbreviationEntry.GetNextAttribute(attributeName,
			attributeForm)) {
		// resolve attribute form indirection
		if (attributeForm == DW_FORM_indirect)
			attributeForm = dataReader.ReadUnsignedLEB128(0);

		// prepare an AttributeValue
		AttributeValue attributeValue;
		attributeValue.attributeForm = attributeForm;
		bool isSigned = false;

		// Read the attribute value according to the attribute's form. For
		// the forms that don't map to a single attribute class only or
		// those that need additional processing, we read a temporary value
		// first.
		uint64 value = 0;
		off_t blockLength = 0;
		bool localReference = true;

		switch (attributeForm) {
			case DW_FORM_addr:
				value = dataReader.ReadAddress(0);
				break;
			case DW_FORM_block2:
				blockLength = dataReader.Read<uint16>(0);
				break;
			case DW_FORM_block4:
				blockLength = dataReader.Read<uint32>(0);
				break;
			case DW_FORM_data2:
				value = dataReader.Read<uint16>(0);
				break;
			case DW_FORM_data4:
				value = dataReader.Read<uint32>(0);
				break;
			case DW_FORM_data8:
				value = dataReader.Read<uint64>(0);
				break;
			case DW_FORM_string:
				attributeValue.SetToString(dataReader.ReadString());
				break;
			case DW_FORM_block:
				blockLength = dataReader.ReadUnsignedLEB128(0);
				break;
			case DW_FORM_block1:
				blockLength = dataReader.Read<uint8>(0);
				break;
			case DW_FORM_data1:
				value = dataReader.Read<uint8>(0);
				break;
			case DW_FORM_flag:
				attributeValue.SetToFlag(dataReader.Read<uint8>(0) != 0);
				break;
			case DW_FORM_sdata:
				value = dataReader.ReadSignedLEB128(0);
				isSigned = true;
				break;
			case DW_FORM_strp:
			{
				if (fDebugStringSection != NULL) {
					off_t offset = fCurrentCompilationUnit->IsDwarf64()
						? (off_t)dataReader.Read<uint64>(0)
						: (off_t)dataReader.Read<uint32>(0);
					if (offset >= fDebugStringSection->Size()) {
						WARNING("Invalid DW_FORM_strp offset: %lld\n", offset);
						return B_BAD_DATA;
					}
					attributeValue.SetToString(
						(const char*)fDebugStringSection->Data() + offset);
				} else {
					WARNING("Invalid DW_FORM_strp: no string section!\n");
					return B_BAD_DATA;
				}
				break;
			}
			case DW_FORM_udata:
				value = dataReader.ReadUnsignedLEB128(0);
				break;
			case DW_FORM_ref_addr:
				value = fCurrentCompilationUnit->IsDwarf64()
					? dataReader.Read<uint64>(0)
					: (uint64)dataReader.Read<uint32>(0);
				localReference = false;
				break;
			case DW_FORM_ref1:
				value = dataReader.Read<uint8>(0);
				break;
			case DW_FORM_ref2:
				value = dataReader.Read<uint16>(0);
				break;
			case DW_FORM_ref4:
				value = dataReader.Read<uint32>(0);
				break;
			case DW_FORM_ref8:
				value = dataReader.Read<uint64>(0);
				break;
			case DW_FORM_ref_udata:
				value = dataReader.ReadUnsignedLEB128(0);
				break;
			case DW_FORM_indirect:
			default:
				WARNING("Unsupported attribute form: %lu\n", attributeForm);
				return B_BAD_DATA;
		}

		// get the attribute class -- skip the attribute, if we can't handle
		// it
		uint8 attributeClass = get_attribute_class(attributeName,
			attributeForm);
		if (attributeClass == ATTRIBUTE_CLASS_UNKNOWN) {
			TRACE_DIE("skipping attribute with unrecognized class: %s (%#lx) "
				"%s (%#lx)\n", get_attribute_name_name(attributeName),
				attributeName, get_attribute_form_name(attributeForm),
				attributeForm);
			continue;
		}
//		attributeValue.attributeClass = attributeClass;

		// set the attribute value according to the attribute's class
		switch (attributeClass) {
			case ATTRIBUTE_CLASS_ADDRESS:
				attributeValue.SetToAddress(value);
				break;
			case ATTRIBUTE_CLASS_BLOCK:
				attributeValue.SetToBlock(dataReader.Data(), blockLength);
				dataReader.Skip(blockLength);
				break;
			case ATTRIBUTE_CLASS_CONSTANT:
				attributeValue.SetToConstant(value, isSigned);
				break;
			case ATTRIBUTE_CLASS_LINEPTR:
				attributeValue.SetToLinePointer(value);
				break;
			case ATTRIBUTE_CLASS_LOCLISTPTR:
				attributeValue.SetToLocationListPointer(value);
				break;
			case ATTRIBUTE_CLASS_MACPTR:
				attributeValue.SetToMacroPointer(value);
				break;
			case ATTRIBUTE_CLASS_RANGELISTPTR:
				attributeValue.SetToRangeListPointer(value);
				break;
			case ATTRIBUTE_CLASS_REFERENCE:
				if (entry != NULL) {
					attributeValue.SetToReference(_ResolveReference(
						fCurrentCompilationUnit, value, localReference));
					if (attributeValue.reference == NULL) {
						// gcc 2 apparently somtimes produces DW_AT_sibling
						// attributes pointing to the end of the sibling list.
						// Just ignore those.
						if (attributeName == DW_AT_sibling)
							continue;

						WARNING("Failed to resolve reference: %s (%#lx) "
							"%s (%#lx): value: %llu\n",
							get_attribute_name_name(attributeName),
							attributeName,
							get_attribute_form_name(attributeForm),
							attributeForm, value);
						return B_ENTRY_NOT_FOUND;
					}
				}
				break;
			case ATTRIBUTE_CLASS_FLAG:
			case ATTRIBUTE_CLASS_STRING:
				// already set
				break;
		}

		if (dataReader.HasOverflow()) {
			WARNING("Unexpected end of .debug_info section.\n");
			return B_BAD_DATA;
		}

		// add the attribute
		if (entry != NULL) {
			TRACE_DIE_ONLY(
				char buffer[1024];
				TRACE_DIE("  attr %s %s (%d): %s\n",
					get_attribute_name_name(attributeName),
					get_attribute_form_name(attributeForm), attributeClass,
					attributeValue.ToString(buffer, sizeof(buffer)));
			)

			DebugInfoEntrySetter attributeSetter
				= get_attribute_name_setter(attributeName);
			if (attributeSetter != 0) {
				status_t error = (entry->*attributeSetter)(attributeName,
					attributeValue);

				if (error == ATTRIBUTE_NOT_HANDLED) {
					error = B_OK;
					TRACE_DIE("    -> unhandled\n");
				}

				if (error != B_OK) {
					WARNING("Failed to set attribute: name: %s, form: %s: %s\n",
						get_attribute_name_name(attributeName),
						get_attribute_form_name(attributeForm),
						strerror(error));
				}
			} else
				TRACE_DIE("    -> no attribute setter!\n");
		}
	}

	return B_OK;
}


status_t
DwarfFile::_ParseLineInfo(CompilationUnit* unit)
{
	off_t offset = unit->UnitEntry()->StatementListOffset();

	TRACE_LINES("DwarfFile::_ParseLineInfo(%p), offset: %lld\n", unit, offset);

	DataReader dataReader((uint8*)fDebugLineSection->Data() + offset,
		fDebugLineSection->Size() - offset, unit->AddressSize());

	// unit length
	bool dwarf64;
	uint64 unitLength = dataReader.ReadInitialLength(dwarf64);
	if (unitLength > (uint64)dataReader.BytesRemaining())
		return B_BAD_DATA;
	off_t unitOffset = dataReader.Offset();

	// version (uhalf)
	uint16 version = dataReader.Read<uint16>(0);

	// header_length (4/8)
	uint64 headerLength = dwarf64
		? dataReader.Read<uint64>(0) : (uint64)dataReader.Read<uint32>(0);
	off_t headerOffset = dataReader.Offset();

	if ((uint64)dataReader.BytesRemaining() < headerLength)
		return B_BAD_DATA;

	// minimum instruction length
	uint8 minInstructionLength = dataReader.Read<uint8>(0);

	// default is statement
	bool defaultIsStatement = dataReader.Read<uint8>(0) != 0;

	// line_base (sbyte)
	int8 lineBase = (int8)dataReader.Read<uint8>(0);

	// line_range (ubyte)
	uint8 lineRange = dataReader.Read<uint8>(0);

	// opcode_base (ubyte)
	uint8 opcodeBase = dataReader.Read<uint8>(0);

	// standard_opcode_lengths (ubyte[])
	const uint8* standardOpcodeLengths = (const uint8*)dataReader.Data();
	dataReader.Skip(opcodeBase - 1);

	if (dataReader.HasOverflow())
		return B_BAD_DATA;

	if (version != 2 && version != 3)
		return B_UNSUPPORTED;

	TRACE_LINES("  unitLength:           %llu\n", unitLength);
	TRACE_LINES("  version:              %u\n", version);
	TRACE_LINES("  headerLength:         %llu\n", headerLength);
	TRACE_LINES("  minInstructionLength: %u\n", minInstructionLength);
	TRACE_LINES("  defaultIsStatement:   %d\n", defaultIsStatement);
	TRACE_LINES("  lineBase:             %d\n", lineBase);
	TRACE_LINES("  lineRange:            %u\n", lineRange);
	TRACE_LINES("  opcodeBase:           %u\n", opcodeBase);

	// include directories
	TRACE_LINES("  include directories:\n");
	while (const char* directory = dataReader.ReadString()) {
		if (*directory == '\0')
			break;
		TRACE_LINES("    \"%s\"\n", directory);

		if (!unit->AddDirectory(directory))
			return B_NO_MEMORY;
	}

	// file names
	TRACE_LINES("  files:\n");
	while (const char* file = dataReader.ReadString()) {
		if (*file == '\0')
			break;
		uint64 dirIndex = dataReader.ReadUnsignedLEB128(0);
		TRACE_LINES_ONLY(uint64 modificationTime =)
			dataReader.ReadUnsignedLEB128(0);
		TRACE_LINES_ONLY(uint64 fileLength =)
			dataReader.ReadUnsignedLEB128(0);

		if (dataReader.HasOverflow())
			return B_BAD_DATA;

		TRACE_LINES("    \"%s\", dir index: %llu, mtime: %llu, length: %llu\n",
			file, dirIndex, modificationTime, fileLength);

		if (!unit->AddFile(file, dirIndex))
			return B_NO_MEMORY;
	}

	off_t readerOffset = dataReader.Offset();
	if ((uint64)readerOffset > readerOffset + headerLength)
		return B_BAD_DATA;
	off_t offsetToProgram = headerOffset + headerLength - readerOffset;

	const uint8* program = (uint8*)dataReader.Data() + offsetToProgram;
	size_t programSize = unitLength - (readerOffset - unitOffset);

	return unit->GetLineNumberProgram().Init(program, programSize,
		minInstructionLength, defaultIsStatement, lineBase, lineRange,
			opcodeBase, standardOpcodeLengths);
}


status_t
DwarfFile::_ParseCIE(ElfSection* debugFrameSection, bool usingEHFrameSection,
	CompilationUnit* unit, CfaContext& context,	off_t cieOffset,
	CIEAugmentation& cieAugmentation)
{
	if (cieOffset < 0 || cieOffset >= debugFrameSection->Size())
		return B_BAD_DATA;

	DataReader dataReader((uint8*)debugFrameSection->Data() + cieOffset,
		debugFrameSection->Size() - cieOffset, unit->AddressSize());

	// length
	bool dwarf64;
	uint64 length = dataReader.ReadInitialLength(dwarf64);
	if (length > (uint64)dataReader.BytesRemaining())
		return B_BAD_DATA;
	off_t lengthOffset = dataReader.Offset();

	// CIE ID/CIE pointer
	uint64 cieID = dwarf64
		? dataReader.Read<uint64>(0) : dataReader.Read<uint32>(0);
	if (usingEHFrameSection) {
		if (cieID != 0)
			return B_BAD_DATA;
	} else {
		if (dwarf64 ? cieID != 0xffffffffffffffffULL : cieID != 0xffffffff)
			return B_BAD_DATA;
	}

	uint8 version = dataReader.Read<uint8>(0);
	if (version != 1) {
		TRACE_CFI("  cie: length: %llu, offset: %#llx, version: %u "
			"-- unsupported\n",	length, cieOffset, version);
		return B_UNSUPPORTED;
	}

	// read the augmentation string
	cieAugmentation.Init(dataReader);

	// in the cause of augmentation string "eh",
	// the exception table pointer is located immediately before the
	// code/data alignment values. We have no use for it so simply skip.
	if (strcmp(cieAugmentation.String(), "eh") == 0)
		dataReader.Skip(dwarf64 ? sizeof(uint64) : sizeof(uint32));

	context.SetCodeAlignment(dataReader.ReadUnsignedLEB128(0));
	context.SetDataAlignment(dataReader.ReadSignedLEB128(0));
	context.SetReturnAddressRegister(dataReader.ReadUnsignedLEB128(0));

	TRACE_CFI("  cie: length: %llu, offset: %#llx, version: %u, augmentation: "
		"\"%s\", aligment: code: %lu, data: %ld, return address reg: %lu\n",
		length, cieOffset, version, cieAugmentation.String(),
		context.CodeAlignment(), context.DataAlignment(),
		context.ReturnAddressRegister());

	status_t error = cieAugmentation.Read(dataReader);
	if (error != B_OK) {
		TRACE_CFI("  cie: length: %llu, version: %u, augmentation: \"%s\" "
			"-- unsupported\n", length, version, cieAugmentation.String());
		return error;
	}

	if (dataReader.HasOverflow())
		return B_BAD_DATA;
	off_t remaining = (off_t)length
		- (dataReader.Offset() - lengthOffset);
	if (remaining < 0)
		return B_BAD_DATA;

	DataReader restrictedReader = dataReader.RestrictedReader(remaining);
	return _ParseFrameInfoInstructions(unit, context, restrictedReader);
}


status_t
DwarfFile::_ParseFrameInfoInstructions(CompilationUnit* unit,
	CfaContext& context, DataReader& dataReader)
{
	while (dataReader.BytesRemaining() > 0) {
		TRACE_CFI("    [%2lld]", dataReader.BytesRemaining());

		uint8 opcode = dataReader.Read<uint8>(0);
		if ((opcode >> 6) != 0) {
			uint32 operand = opcode & 0x3f;

			switch (opcode >> 6) {
				case DW_CFA_advance_loc:
				{
					TRACE_CFI("    DW_CFA_advance_loc: %#lx\n", operand);

					target_addr_t location = context.Location()
						+ operand * context.CodeAlignment();
					if (location > context.TargetLocation())
						return B_OK;
					context.SetLocation(location);
					break;
				}
				case DW_CFA_offset:
				{
					uint64 offset = dataReader.ReadUnsignedLEB128(0);
					TRACE_CFI("    DW_CFA_offset: reg: %lu, offset: %llu\n",
						operand, offset);

					if (CfaRule* rule = context.RegisterRule(operand)) {
						rule->SetToLocationOffset(
							offset * context.DataAlignment());
					}
					break;
				}
				case DW_CFA_restore:
				{
					TRACE_CFI("    DW_CFA_restore: %#lx\n", operand);

					context.RestoreRegisterRule(operand);
					break;
				}
			}
		} else {
			switch (opcode) {
				case DW_CFA_nop:
				{
					TRACE_CFI("    DW_CFA_nop\n");
					break;
				}
				case DW_CFA_set_loc:
				{
					target_addr_t location = dataReader.ReadAddress(0);

					TRACE_CFI("    DW_CFA_set_loc: %#llx\n", location);

					if (location < context.Location())
						return B_BAD_VALUE;
					if (location > context.TargetLocation())
						return B_OK;
					context.SetLocation(location);
					break;
				}
				case DW_CFA_advance_loc1:
				{
					uint32 delta = dataReader.Read<uint8>(0);

					TRACE_CFI("    DW_CFA_advance_loc1: %#lx\n", delta);

					target_addr_t location = context.Location()
						+ delta * context.CodeAlignment();
					if (location > context.TargetLocation())
						return B_OK;
					context.SetLocation(location);
					break;
				}
				case DW_CFA_advance_loc2:
				{
					uint32 delta = dataReader.Read<uint16>(0);

					TRACE_CFI("    DW_CFA_advance_loc2: %#lx\n", delta);

					target_addr_t location = context.Location()
						+ delta * context.CodeAlignment();
					if (location > context.TargetLocation())
						return B_OK;
					context.SetLocation(location);
					break;
				}
				case DW_CFA_advance_loc4:
				{
					uint32 delta = dataReader.Read<uint32>(0);

					TRACE_CFI("    DW_CFA_advance_loc4: %#lx\n", delta);

					target_addr_t location = context.Location()
						+ delta * context.CodeAlignment();
					if (location > context.TargetLocation())
						return B_OK;
					context.SetLocation(location);
					break;
				}
				case DW_CFA_offset_extended:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint64 offset = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_offset_extended: reg: %lu, "
						"offset: %llu\n", reg, offset);

					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToLocationOffset(
							offset * context.DataAlignment());
					}
					break;
				}
				case DW_CFA_restore_extended:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_restore_extended: %#lx\n", reg);

					context.RestoreRegisterRule(reg);
					break;
				}
				case DW_CFA_undefined:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_undefined: %lu\n", reg);

					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToUndefined();
					break;
				}
				case DW_CFA_same_value:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_same_value: %lu\n", reg);

					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToSameValue();
					break;
				}
				case DW_CFA_register:
				{
					uint32 reg1 = dataReader.ReadUnsignedLEB128(0);
					uint32 reg2 = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_register: reg1: %lu, reg2: %lu\n", reg1, reg2);

					if (CfaRule* rule = context.RegisterRule(reg1))
						rule->SetToValueOffset(reg2);
					break;
				}
				case DW_CFA_remember_state:
				{
					TRACE_CFI("    DW_CFA_remember_state\n");

					status_t error = context.PushRuleSet();
					if (error != B_OK)
						return error;
					break;
				}
				case DW_CFA_restore_state:
				{
					TRACE_CFI("    DW_CFA_restore_state\n");

					status_t error = context.PopRuleSet();
					if (error != B_OK)
						return error;
					break;
				}
				case DW_CFA_def_cfa:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint64 offset = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_def_cfa: reg: %lu, offset: %llu\n",
						reg, offset);

					context.GetCfaCfaRule()->SetToRegisterOffset(reg, offset);
					break;
				}
				case DW_CFA_def_cfa_register:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_def_cfa_register: %lu\n", reg);

					if (context.GetCfaCfaRule()->Type()
							!= CFA_CFA_RULE_REGISTER_OFFSET) {
						return B_BAD_DATA;
					}
					context.GetCfaCfaRule()->SetRegister(reg);
					break;
				}
				case DW_CFA_def_cfa_offset:
				{
					uint64 offset = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_def_cfa_offset: %llu\n", offset);

					if (context.GetCfaCfaRule()->Type()
							!= CFA_CFA_RULE_REGISTER_OFFSET) {
						return B_BAD_DATA;
					}
					context.GetCfaCfaRule()->SetOffset(offset);
					break;
				}
				case DW_CFA_def_cfa_expression:
				{
					uint64 blockLength = dataReader.ReadUnsignedLEB128(0);
					uint8* block = (uint8*)dataReader.Data();
					dataReader.Skip(blockLength);

					TRACE_CFI("    DW_CFA_def_cfa_expression: %p, %llu\n",
						block, blockLength);

					context.GetCfaCfaRule()->SetToExpression(block,
						blockLength);
					break;
				}
				case DW_CFA_expression:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint64 blockLength = dataReader.ReadUnsignedLEB128(0);
					uint8* block = (uint8*)dataReader.Data();
					dataReader.Skip(blockLength);

					TRACE_CFI("    DW_CFA_expression: reg: %lu, block: %p, "
						"%llu\n", reg, block, blockLength);

					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToLocationExpression(block, blockLength);
					break;
				}
				case DW_CFA_offset_extended_sf:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					int64 offset = dataReader.ReadSignedLEB128(0);

					TRACE_CFI("    DW_CFA_offset_extended: reg: %lu, "
						"offset: %lld\n", reg, offset);

					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToLocationOffset(
							offset * (int32)context.DataAlignment());
					}
					break;
				}
				case DW_CFA_def_cfa_sf:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					int64 offset = dataReader.ReadSignedLEB128(0);

					TRACE_CFI("    DW_CFA_def_cfa_sf: reg: %lu, offset: %lld\n",
						reg, offset);

					context.GetCfaCfaRule()->SetToRegisterOffset(reg,
						offset * (int32)context.DataAlignment());
					break;
				}
				case DW_CFA_def_cfa_offset_sf:
				{
					int64 offset = dataReader.ReadSignedLEB128(0);

					TRACE_CFI("    DW_CFA_def_cfa_offset: %lld\n", offset);

					if (context.GetCfaCfaRule()->Type()
							!= CFA_CFA_RULE_REGISTER_OFFSET) {
						return B_BAD_DATA;
					}
					context.GetCfaCfaRule()->SetOffset(
						offset * (int32)context.DataAlignment());
					break;
				}
				case DW_CFA_val_offset:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint64 offset = dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_val_offset: reg: %lu, offset: %llu\n",
						reg, offset);

					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToValueOffset(
							offset * context.DataAlignment());
					}
					break;
				}
				case DW_CFA_val_offset_sf:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					int64 offset = dataReader.ReadSignedLEB128(0);

					TRACE_CFI("    DW_CFA_val_offset_sf: reg: %lu, "
						"offset: %lld\n", reg, offset);

					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToValueOffset(
							offset * (int32)context.DataAlignment());
					}
					break;
				}
				case DW_CFA_val_expression:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint64 blockLength = dataReader.ReadUnsignedLEB128(0);
					uint8* block = (uint8*)dataReader.Data();
					dataReader.Skip(blockLength);

					TRACE_CFI("    DW_CFA_val_expression: reg: %lu, block: %p, "
						"%llu\n", reg, block, blockLength);

					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToValueExpression(block, blockLength);
					break;
				}

				// extensions
				case DW_CFA_MIPS_advance_loc8:
				{
					uint64 delta = dataReader.Read<uint64>(0);

					TRACE_CFI("    DW_CFA_MIPS_advance_loc8: %#llx\n", delta);

					target_addr_t location = context.Location()
						+ delta * context.CodeAlignment();
					if (location > context.TargetLocation())
						return B_OK;
					context.SetLocation(location);
					break;
				}
				case DW_CFA_GNU_window_save:
				{
					// SPARC specific, no args
					TRACE_CFI("    DW_CFA_GNU_window_save\n");

					// TODO: Implement once we have SPARC support!
					break;
				}
				case DW_CFA_GNU_args_size:
				{
					// Updates the total size of arguments on the stack.
					TRACE_CFI_ONLY(uint64 size =)
						dataReader.ReadUnsignedLEB128(0);

					TRACE_CFI("    DW_CFA_GNU_args_size: %llu\n", size);
// TODO: Implement!
					break;
				}
				case DW_CFA_GNU_negative_offset_extended:
				{
					// obsolete
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					int64 offset = dataReader.ReadSignedLEB128(0);

					TRACE_CFI("    DW_CFA_GNU_negative_offset_extended: "
						"reg: %lu, offset: %lld\n", reg, offset);

					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToLocationOffset(
							offset * (int32)context.DataAlignment());
					}
					break;
				}

				default:
					WARNING("    unknown opcode %u!\n", opcode);
					return B_BAD_DATA;
			}
		}
	}

	return B_OK;
}


status_t
DwarfFile::_ParsePublicTypesInfo()
{
	TRACE_PUBTYPES("DwarfFile::_ParsePublicTypesInfo()\n");
	if (fDebugPublicTypesSection == NULL) {
		TRACE_PUBTYPES("  -> no public types section\n");
		return B_ENTRY_NOT_FOUND;
	}

	DataReader dataReader((uint8*)fDebugPublicTypesSection->Data(),
		fDebugPublicTypesSection->Size(), 4);
		// address size doesn't matter at this point

	while (dataReader.BytesRemaining() > 0) {
		bool dwarf64;
		uint64 unitLength = dataReader.ReadInitialLength(dwarf64);

		off_t unitLengthOffset = dataReader.Offset();
			// the unitLength starts here

		if (dataReader.HasOverflow())
			return B_BAD_DATA;

		if (unitLengthOffset + unitLength
				> (uint64)fDebugPublicTypesSection->Size()) {
			WARNING("Invalid public types set unit length.\n");
			break;
		}

		DataReader unitDataReader(dataReader.Data(), unitLength, 4);
			// address size doesn't matter
		_ParsePublicTypesInfo(unitDataReader, dwarf64);

		dataReader.SeekAbsolute(unitLengthOffset + unitLength);
	}

	return B_OK;
}


status_t
DwarfFile::_ParsePublicTypesInfo(DataReader& dataReader, bool dwarf64)
{
	int version = dataReader.Read<uint16>(0);
	if (version != 2) {
		TRACE_PUBTYPES("  pubtypes version %d unsupported\n", version);
		return B_UNSUPPORTED;
	}

	TRACE_PUBTYPES_ONLY(off_t debugInfoOffset =) dwarf64
		? dataReader.Read<uint64>(0)
		: (uint64)dataReader.Read<uint32>(0);
	TRACE_PUBTYPES_ONLY(off_t debugInfoSize =) dwarf64
		? dataReader.Read<uint64>(0)
		: (uint64)dataReader.Read<uint32>(0);

	if (dataReader.HasOverflow())
		return B_BAD_DATA;

	TRACE_PUBTYPES("DwarfFile::_ParsePublicTypesInfo(): compilation unit debug "
		"info: (%lld, %lld)\n", debugInfoOffset, debugInfoSize);

	while (dataReader.BytesRemaining() > 0) {
		off_t entryOffset = dwarf64
			? dataReader.Read<uint64>(0)
			: (uint64)dataReader.Read<uint32>(0);
		if (entryOffset == 0)
			return B_OK;

		TRACE_PUBTYPES_ONLY(const char* name =) dataReader.ReadString();

		TRACE_PUBTYPES("  \"%s\" -> %lld\n", name, entryOffset);
	}

	return B_OK;
}


status_t
DwarfFile::_GetAbbreviationTable(off_t offset, AbbreviationTable*& _table)
{
	// check, whether we've already loaded it
	for (AbbreviationTableList::Iterator it
				= fAbbreviationTables.GetIterator();
			AbbreviationTable* table = it.Next();) {
		if (offset == table->Offset()) {
			_table = table;
			return B_OK;
		}
	}

	// create a new table
	AbbreviationTable* table = new(std::nothrow) AbbreviationTable(offset);
	if (table == NULL)
		return B_NO_MEMORY;

	status_t error = table->Init(fDebugAbbrevSection->Data(),
		fDebugAbbrevSection->Size());
	if (error != B_OK) {
		delete table;
		return error;
	}

	fAbbreviationTables.Add(table);
	_table = table;
	return B_OK;
}


DebugInfoEntry*
DwarfFile::_ResolveReference(CompilationUnit* unit, uint64 offset,
	bool localReference) const
{
	if (localReference)
		return unit->EntryForOffset(offset);

	// TODO: Implement program-global references!
	return NULL;
}


status_t
DwarfFile::_GetLocationExpression(CompilationUnit* unit,
	const LocationDescription* location, target_addr_t instructionPointer,
	const void*& _expression, off_t& _length) const
{
	if (!location->IsValid())
		return B_BAD_VALUE;

	if (location->IsExpression()) {
		_expression = location->expression.data;
		_length = location->expression.length;
		return B_OK;
	}

	if (location->IsLocationList() && instructionPointer != 0) {
		return _FindLocationExpression(unit, location->listOffset,
			instructionPointer, _expression, _length);
	}

	return B_BAD_VALUE;
}


status_t
DwarfFile::_FindLocationExpression(CompilationUnit* unit, uint64 offset,
	target_addr_t address, const void*& _expression, off_t& _length) const
{
	if (unit == NULL)
		return B_BAD_VALUE;

	if (fDebugLocationSection == NULL)
		return B_ENTRY_NOT_FOUND;

	if (offset < 0 || offset >= (uint64)fDebugLocationSection->Size())
		return B_BAD_DATA;

	target_addr_t baseAddress = unit->AddressRangeBase();
	target_addr_t maxAddress = unit->MaxAddress();

	DataReader dataReader((uint8*)fDebugLocationSection->Data() + offset,
		fDebugLocationSection->Size() - offset, unit->AddressSize());
	while (true) {
		target_addr_t start = dataReader.ReadAddress(0);
		target_addr_t end = dataReader.ReadAddress(0);
		if (dataReader.HasOverflow())
			return B_BAD_DATA;

		if (start == 0 && end == 0)
			return B_ENTRY_NOT_FOUND;

		if (start == maxAddress) {
			baseAddress = end;
			continue;
		}

		uint16 expressionLength = dataReader.Read<uint16>(0);
		const void* expression = dataReader.Data();
		if (!dataReader.Skip(expressionLength))
			return B_BAD_DATA;

		if (start == end)
			continue;

		start += baseAddress;
		end += baseAddress;

		if (address >= start && address < end) {
			_expression = expression;
			_length = expressionLength;
			return B_OK;
		}
	}
}
