/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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
#include "DwarfTargetInterface.h"
#include "ElfFile.h"
#include "TagNames.h"
#include "TargetAddressRangeList.h"


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
		fprintf(stderr, "DwarfManager::File::Load(\"%s\"): no "
			".debug_info, .debug_abbrev, or .debug_str section.\n",
			fileName);
		return B_ERROR;
	}

	// non mandatory sections
	fDebugStringSection = fElfFile->GetSection(".debug_str");
	fDebugRangesSection = fElfFile->GetSection(".debug_ranges");
	fDebugLineSection = fElfFile->GetSection(".debug_line");
	fDebugFrameSection = fElfFile->GetSection(".debug_frame");

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
			printf("\"%s\": Invalid compilation unit length.\n", fileName);
			break;
		}

		int version = dataReader.Read<uint16>(0);
		off_t abbrevOffset = dwarf64
			? dataReader.Read<uint64>(0)
			: dataReader.Read<uint32>(0);
		uint8 addressSize = dataReader.Read<uint8>(0);

		if (dataReader.HasOverflow()) {
			printf("\"%s\": Unexpected end of data in compilation unit "
				"header.\n", fileName);
			break;
		}

		printf("DWARF%d compilation unit: version %d, length: %lld, "
			"abbrevOffset: %lld, address size: %d\n", dwarf64 ? 64 : 32,
			version, unitLength, abbrevOffset, addressSize);

		if (version != 2 && version != 3) {
			printf("\"%s\": Unsupported compilation unit version: %d\n",
				fileName, version);
			break;
		}

		if (addressSize != 4 && addressSize != 8) {
			printf("\"%s\": Unsupported address size: %d\n", fileName,
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


status_t
DwarfFile::UnwindCallFrame(CompilationUnit* unit, target_addr_t location,
	const DwarfTargetInterface* inputInterface,
	DwarfTargetInterface* outputInterface, target_addr_t& _framePointer)
{
	if (fDebugFrameSection == NULL)
		return B_ENTRY_NOT_FOUND;

printf("DwarfFile::UnwindCallFrame(%#llx)\n", location);

	DataReader dataReader((uint8*)fDebugFrameSection->Data(),
		fDebugFrameSection->Size(), unit->AddressSize());

	while (dataReader.BytesRemaining() > 0) {
		// length
		bool dwarf64;
		uint64 length = dataReader.ReadInitialLength(dwarf64);
		if (length > (uint64)dataReader.BytesRemaining())
			return B_BAD_DATA;
		off_t lengthOffset = dataReader.Offset();

		// CIE ID/CIE pointer
		uint64 cieID = dwarf64
			? dataReader.Read<uint64>(0) : dataReader.Read<uint32>(0);
		if (dwarf64 ? cieID == 0xffffffffffffffffULL : cieID == 0xffffffff) {
			// this is a CIE -- skip it
		} else {
			// this is a FDE
			target_addr_t initialLocation = dataReader.ReadAddress(0);
			target_size_t addressRange = dataReader.ReadAddress(0);

			if (dataReader.HasOverflow())
				return B_BAD_DATA;

			if (location >= initialLocation
				&& location < initialLocation + addressRange) {
				// This is the FDE we're looking for.
				off_t remaining = (off_t)length
					- (dataReader.Offset() - lengthOffset);
				if (remaining < 0)
					return B_BAD_DATA;
printf("  found fde: length: %llu (%lld), CIE offset: %llu, location: %#llx, range: %#llx\n", length, remaining, cieID,
initialLocation, addressRange);

				CfaContext context(location, initialLocation);
				uint32 registerCount = outputInterface->CountRegisters();
				status_t error = context.Init(registerCount);
				if (error != B_OK)
					return error;

				// Init the initial register rules. The DWARF 3 specs on the
				// matter: "The default rule for all columns before
				// interpretation of the initial instructions is the undefined
				// rule. However, an ABI authoring body or a compilation system
				// authoring body may specify an alternate default value for any
				// or all columns."
				// GCC's assumes the "same value" rule for all callee preserved
				// registers. We set them respectively.
				for (uint32 i = 0; i < registerCount; i++) {
					if (outputInterface->IsCalleePreservedRegister(i))
						context.RegisterRule(i)->SetToSameValue();
				}

				// process the CIE
				error = _ParseCIE(unit, context, cieID);
				if (error != B_OK)
					return error;

				error = context.SaveInitialRuleSet();
				if (error != B_OK)
					return error;

				error = _ParseFrameInfoInstructions(unit, context,
					dataReader.Offset(), remaining);
				if (error != B_OK)
					return error;

printf("  found row!\n");

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
// TODO: Implement!
						return B_UNSUPPORTED;
					case CFA_CFA_RULE_UNDEFINED:
					default:
						return B_BAD_VALUE;
				}
printf("  frame address: %#llx\n", frameAddress);

				// apply the register rules
				for (uint32 i = 0; i < registerCount; i++) {
printf("  reg %lu\n", i);
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
printf("  -> CFA_RULE_SAME_VALUE\n");
							BVariant value;
							if (inputInterface->GetRegisterValue(i, value))
								outputInterface->SetRegisterValue(i, value);
							break;
						}
						case CFA_RULE_LOCATION_OFFSET:
						{
printf("  -> CFA_RULE_LOCATION_OFFSET: %lld\n", rule->Offset());
							BVariant value;
							if (inputInterface->ReadValueFromMemory(
									frameAddress + rule->Offset(), valueType,
									value)) {
								outputInterface->SetRegisterValue(i, value);
							}
							break;
						}
						case CFA_RULE_VALUE_OFFSET:
printf("  -> CFA_RULE_VALUE_OFFSET\n");
							outputInterface->SetRegisterValue(i,
								frameAddress + rule->Offset());
							break;
						case CFA_RULE_REGISTER:
						{
printf("  -> CFA_RULE_REGISTER\n");
							BVariant value;
							if (inputInterface->GetRegisterValue(
									rule->Register(), value)) {
								outputInterface->SetRegisterValue(i, value);
							}
							break;
						}
						case CFA_RULE_LOCATION_EXPRESSION:
printf("  -> CFA_RULE_LOCATION_EXPRESSION\n");
// TODO:...
							break;
						case CFA_RULE_VALUE_EXPRESSION:
printf("  -> CFA_RULE_VALUE_EXPRESSION\n");
// TODO:...
							break;
						case CFA_RULE_UNDEFINED:
printf("  -> CFA_RULE_UNDEFINED\n");
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
		fprintf(stderr, "No compilation unit entry in .debug_info "
			"section.\n");
		return B_BAD_DATA;
	}

	unit->SetUnitEntry(unitEntry);

printf("remaining bytes in unit: %lld\n", dataReader.BytesRemaining());
if (dataReader.HasData()) {
printf("  ");
while (dataReader.HasData()) {
printf("%02x", dataReader.Read<uint8>(0));
}
printf("\n");
}
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
			fprintf(stderr, "Unexpected end of .debug_info section.\n");
			return B_BAD_DATA;
		}
		_entry = NULL;
		_endOfEntryList = true;
		return B_OK;
	}

	// get the corresponding abbreviation entry
	AbbreviationEntry abbreviationEntry;
	if (!abbreviationTable->GetAbbreviationEntry(code, abbreviationEntry)) {
		fprintf(stderr, "No abbreviation entry for code %lu\n", code);
		return B_BAD_DATA;
	}
printf("%*sentry at %lld: %lu, tag: %s (%lu), children: %d\n", level * 2, "",
entryOffset, abbreviationEntry.Code(), get_entry_tag_name(abbreviationEntry.Tag()),
abbreviationEntry.Tag(), abbreviationEntry.HasChildren());

	DebugInfoEntry* entry;
	status_t error = fDebugInfoFactory.CreateDebugInfoEntry(
		abbreviationEntry.Tag(), entry);
	if (error != B_OK)
		return error;
	ObjectDeleter<DebugInfoEntry> entryDeleter(entry);

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
printf("%*s  -> child unhandled\n", level * 2, "");
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
printf("\nfinishing compilation unit %p\n", unit);
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
printf("entry %p at %lld\n", entry, offset);

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
			fprintf(stderr, "Init after hierarchy failed!\n");
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
			fprintf(stderr, "Init after attributes failed!\n");
			return error;
		}
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
						fprintf(stderr, "Invalid DW_FORM_strp offset: %lld\n",
							offset);
						return B_BAD_DATA;
					}
					attributeValue.SetToString(
						(const char*)fDebugStringSection->Data() + offset);
				} else {
					fprintf(stderr, "Invalid DW_FORM_strp: no string "
						"section!\n");
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
				fprintf(stderr, "Unsupported attribute form: %lu\n",
					attributeForm);
				return B_BAD_DATA;
		}

		// get the attribute class -- skip the attribute, if we can't handle
		// it
		uint8 attributeClass = get_attribute_class(attributeName,
			attributeForm);
		if (attributeClass == ATTRIBUTE_CLASS_UNKNOWN) {
			printf("skipping attribute with unrecognized class: %s (%#lx) "
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
			{
				if (entry != NULL) {
					TargetAddressRangeList* rangeList
						= _ResolveRangeList(value);
					Reference<TargetAddressRangeList> reference(rangeList);
					attributeValue.SetToRangeList(rangeList);
				}
				break;
			}
			case ATTRIBUTE_CLASS_REFERENCE:
				if (entry != NULL) {
					attributeValue.SetToReference(_ResolveReference(value,
						localReference));
					if (attributeValue.reference == NULL) {
						// gcc 2 apparently somtimes produces DW_AT_sibling
						// attributes pointing to the end of the sibling list.
						// Just ignore those.
						if (attributeName == DW_AT_sibling)
							continue;

						fprintf(stderr, "Failed to resolve reference: "
							"%s (%#lx) %s (%#lx): value: %llu\n",
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
			fprintf(stderr, "Unexpected end of .debug_info section.\n");
			return B_BAD_DATA;
		}

		// add the attribute
		if (entry != NULL) {
char buffer[1024];
printf("  attr %s %s (%d): %s\n", get_attribute_name_name(attributeName),
get_attribute_form_name(attributeForm), attributeClass, attributeValue.ToString(buffer, sizeof(buffer)));

			DebugInfoEntrySetter attributeSetter
				= get_attribute_name_setter(attributeName);
			if (attributeSetter != 0) {
				status_t error = (entry->*attributeSetter)(attributeName,
					attributeValue);

				if (error == ATTRIBUTE_NOT_HANDLED) {
					error = B_OK;
printf("    -> unhandled\n");
				}

				if (error != B_OK) {
					fprintf(stderr, "Failed to set attribute: name: %s, "
						"form: %s: %s\n",
						get_attribute_name_name(attributeName),
						get_attribute_form_name(attributeForm),
						strerror(error));
				}
			}
else
printf("    -> no attribute setter!\n");
		}
	}

	return B_OK;
}


status_t
DwarfFile::_ParseLineInfo(CompilationUnit* unit)
{
	off_t offset = unit->UnitEntry()->StatementListOffset();
printf("DwarfFile::_ParseLineInfo(%p), offset: %lld\n", unit, offset);

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

	printf("  unitLength:           %llu\n", unitLength);
	printf("  version:              %u\n", version);
	printf("  headerLength:         %llu\n", headerLength);
	printf("  minInstructionLength: %u\n", minInstructionLength);
	printf("  defaultIsStatement:   %d\n", defaultIsStatement);
	printf("  lineBase:             %d\n", lineBase);
	printf("  lineRange:            %u\n", lineRange);
	printf("  opcodeBase:           %u\n", opcodeBase);

	// include directories
	printf("  include directories:\n");
	while (const char* directory = dataReader.ReadString()) {
		if (*directory == '\0')
			break;
		printf("    \"%s\"\n", directory);

		if (!unit->AddDirectory(directory))
			return B_NO_MEMORY;
	}

	// file names
	printf("  files:\n");
	while (const char* file = dataReader.ReadString()) {
		if (*file == '\0')
			break;
		uint64 dirIndex = dataReader.ReadUnsignedLEB128(0);
		uint64 modificationTime = dataReader.ReadUnsignedLEB128(0);
		uint64 fileLength = dataReader.ReadUnsignedLEB128(0);

		if (dataReader.HasOverflow())
			return B_BAD_DATA;

		printf("    \"%s\", dir index: %llu, mtime: %llu, length: %llu\n", file,
			dirIndex, modificationTime, fileLength);

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
DwarfFile::_ParseCIE(CompilationUnit* unit, CfaContext& context,
	off_t cieOffset)
{
	if (cieOffset < 0 || cieOffset >= fDebugFrameSection->Size())
		return B_BAD_DATA;

	DataReader dataReader((uint8*)fDebugFrameSection->Data() + cieOffset,
		fDebugFrameSection->Size() - cieOffset, unit->AddressSize());

	// length
	bool dwarf64;
	uint64 length = dataReader.ReadInitialLength(dwarf64);
	if (length > (uint64)dataReader.BytesRemaining())
		return B_BAD_DATA;
	off_t lengthOffset = dataReader.Offset();

	// CIE ID/CIE pointer
	uint64 cieID = dwarf64
		? dataReader.Read<uint64>(0) : dataReader.Read<uint32>(0);
	if (dwarf64 ? cieID != 0xffffffffffffffffULL : cieID != 0xffffffff)
		return B_BAD_DATA;

	uint8 version = dataReader.Read<uint8>(0);
	const char* augmentation = dataReader.ReadString();
	if (version != 1 || augmentation == NULL || *augmentation != '\0')
		return B_UNSUPPORTED;

	context.SetCodeAlignment(dataReader.ReadUnsignedLEB128(0));
	context.SetDataAlignment(dataReader.ReadSignedLEB128(0));
	context.SetReturnAddressRegister(dataReader.ReadUnsignedLEB128(0));
printf("  cie: length: %llu, version: %u, augmentation: \"%s\", "
"aligment: code: %lu, data: %ld, return address reg: %lu\n",
length, version, augmentation, context.CodeAlignment(), context.DataAlignment(),
context.ReturnAddressRegister());

	if (dataReader.HasOverflow())
		return B_BAD_DATA;
	off_t remaining = (off_t)length
		- (dataReader.Offset() - lengthOffset);
	if (remaining < 0)
		return B_BAD_DATA;

	return _ParseFrameInfoInstructions(unit, context, dataReader.Offset(),
		remaining);
}


status_t
DwarfFile::_ParseFrameInfoInstructions(CompilationUnit* unit,
	CfaContext& context, off_t instructionOffset, off_t instructionSize)
{
	if (instructionSize <= 0)
		return B_OK;

	DataReader dataReader(
		(uint8*)fDebugFrameSection->Data() + instructionOffset,
		instructionSize, unit->AddressSize());

	while (dataReader.BytesRemaining() > 0) {
printf("    [%2lld]", dataReader.BytesRemaining());
		uint8 opcode = dataReader.Read<uint8>(0);
		if ((opcode >> 6) != 0) {
			uint32 operand = opcode & 0x3f;

			switch (opcode >> 6) {
				case DW_CFA_advance_loc:
				{
printf("    DW_CFA_advance_loc: %#lx\n", operand);
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
printf("    DW_CFA_offset: reg: %lu, offset: %llu\n", operand, offset);
					if (CfaRule* rule = context.RegisterRule(operand)) {
						rule->SetToLocationOffset(
							offset * context.DataAlignment());
					}
					break;
				}
				case DW_CFA_restore:
				{
printf("    DW_CFA_restore: %#lx\n", operand);
					context.RestoreRegisterRule(operand);
					break;
				}
			}
		} else {
			switch (opcode) {
				case DW_CFA_nop:
				{
					printf("    DW_CFA_nop\n");
					break;
				}
				case DW_CFA_set_loc:
				{
					target_addr_t location = dataReader.ReadAddress(0);
printf("    DW_CFA_set_loc: %#llx\n", location);
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
printf("    DW_CFA_advance_loc1: %#lx\n", delta);
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
printf("    DW_CFA_advance_loc2: %#lx\n", delta);
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
printf("    DW_CFA_advance_loc4: %#lx\n", delta);
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
printf("    DW_CFA_offset_extended: reg: %lu, offset: %llu\n", reg, offset);
					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToLocationOffset(
							offset * context.DataAlignment());
					}
					break;
				}
				case DW_CFA_restore_extended:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
printf("    DW_CFA_restore_extended: %#lx\n", reg);
					context.RestoreRegisterRule(reg);
					break;
				}
				case DW_CFA_undefined:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
printf("    DW_CFA_undefined: %lu\n", reg);
					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToUndefined();
					break;
				}
				case DW_CFA_same_value:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
printf("    DW_CFA_same_value: %lu\n", reg);
					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToSameValue();
					break;
				}
				case DW_CFA_register:
				{
					uint32 reg1 = dataReader.ReadUnsignedLEB128(0);
					uint32 reg2 = dataReader.ReadUnsignedLEB128(0);
printf("    DW_CFA_register: reg1: %lu, reg2: %lu\n", reg1, reg2);
					if (CfaRule* rule = context.RegisterRule(reg1))
						rule->SetToValueOffset(reg2);
					break;
				}
				case DW_CFA_remember_state:
				{
printf("    DW_CFA_remember_state\n");
					status_t error = context.PushRuleSet();
					if (error != B_OK)
						return error;
					break;
				}
				case DW_CFA_restore_state:
				{
printf("    DW_CFA_restore_state\n");
					status_t error = context.PopRuleSet();
					if (error != B_OK)
						return error;
					break;
				}
				case DW_CFA_def_cfa:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint64 offset = dataReader.ReadUnsignedLEB128(0);
printf("    DW_CFA_def_cfa: reg: %lu, offset: %llu\n", reg, offset);
					context.GetCfaCfaRule()->SetToRegisterOffset(reg, offset);
					break;
				}
				case DW_CFA_def_cfa_register:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
printf("    DW_CFA_def_cfa_register: %lu\n", reg);
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
printf("    DW_CFA_def_cfa_offset: %llu\n", offset);
					if (context.GetCfaCfaRule()->Type()
							!= CFA_CFA_RULE_REGISTER_OFFSET) {
						return B_BAD_DATA;
					}
					context.GetCfaCfaRule()->SetOffset(offset);
					break;
				}
				case DW_CFA_def_cfa_expression:
				{
					uint8* block = (uint8*)dataReader.Data();
					uint64 blockLength = dataReader.ReadUnsignedLEB128(0);
					dataReader.Skip(blockLength);
printf("    DW_CFA_def_cfa_expression: %p, %llu\n", block, blockLength);
					context.GetCfaCfaRule()->SetToExpression(block,
						blockLength);
					break;
				}
				case DW_CFA_expression:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint8* block = (uint8*)dataReader.Data();
					uint64 blockLength = dataReader.ReadUnsignedLEB128(0);
					dataReader.Skip(blockLength);
printf("    DW_CFA_expression: reg: %lu, block: %p, %llu\n", reg, block, blockLength);
					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToLocationExpression(block, blockLength);
					break;
				}
				case DW_CFA_offset_extended_sf:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					int64 offset = dataReader.ReadSignedLEB128(0);
printf("    DW_CFA_offset_extended: reg: %lu, offset: %lld\n", reg, offset);
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
printf("    DW_CFA_def_cfa_sf: reg: %lu, offset: %lld\n", reg, offset);
					context.GetCfaCfaRule()->SetToRegisterOffset(reg,
						offset * (int32)context.DataAlignment());
					break;
				}
				case DW_CFA_def_cfa_offset_sf:
				{
					int64 offset = dataReader.ReadSignedLEB128(0);
printf("    DW_CFA_def_cfa_offset: %lld\n", offset);
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
printf("    DW_CFA_val_offset: reg: %lu, offset: %llu\n", reg, offset);
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
printf("    DW_CFA_val_offset_sf: reg: %lu, offset: %lld\n", reg, offset);
					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToValueOffset(
							offset * (int32)context.DataAlignment());
					}
					break;
				}
				case DW_CFA_val_expression:
				{
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					uint8* block = (uint8*)dataReader.Data();
					uint64 blockLength = dataReader.ReadUnsignedLEB128(0);
					dataReader.Skip(blockLength);
printf("    DW_CFA_val_expression: reg: %lu, block: %p, %llu\n", reg, block, blockLength);
					if (CfaRule* rule = context.RegisterRule(reg))
						rule->SetToValueExpression(block, blockLength);
					break;
				}

				// extensions
				case DW_CFA_MIPS_advance_loc8:
				{
					uint64 delta = dataReader.Read<uint64>(0);
printf("    DW_CFA_MIPS_advance_loc8: %#llx\n", delta);
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
					printf("    DW_CFA_GNU_window_save\n");
					// TODO: Implement once we have SPARC support!
					break;
				}
				case DW_CFA_GNU_args_size:
				{
					// Updates the total size of arguments on the stack.
					uint64 size = dataReader.ReadUnsignedLEB128(0);
					printf("    DW_CFA_GNU_args_size: %llu\n", size);
// TODO: Implement!
					break;
				}
				case DW_CFA_GNU_negative_offset_extended:
				{
					// obsolete
					uint32 reg = dataReader.ReadUnsignedLEB128(0);
					int64 offset = dataReader.ReadSignedLEB128(0);
printf("    DW_CFA_GNU_negative_offset_extended: reg: %lu, offset: %lld\n", reg, offset);
					if (CfaRule* rule = context.RegisterRule(reg)) {
						rule->SetToLocationOffset(
							offset * (int32)context.DataAlignment());
					}
					break;
				}

				default:
					printf("    unknown opcode %u!\n", opcode);
					return B_BAD_DATA;
			}
		}
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
DwarfFile::_ResolveReference(uint64 offset, bool localReference) const
{
	if (localReference)
		return fCurrentCompilationUnit->EntryForOffset(offset);

	// TODO: Implement program-global references!
	return NULL;
}


TargetAddressRangeList*
DwarfFile::_ResolveRangeList(uint64 offset)
{
	if (fDebugRangesSection == NULL)
		return NULL;

	if (offset >= (uint64)fDebugRangesSection->Size())
		return NULL;

	TargetAddressRangeList* ranges = new(std::nothrow) TargetAddressRangeList;
	if (ranges == NULL) {
		fprintf(stderr, "Out of memory.\n");
		return NULL;
	}
	Reference<TargetAddressRangeList> rangesReference(ranges);

	target_addr_t baseAddress
		= fCurrentCompilationUnit->UnitEntry()->AddressRangeBase();
	target_addr_t maxAddress = fCurrentCompilationUnit->MaxAddress();

	DataReader dataReader((uint8*)fDebugRangesSection->Data() + offset,
		fDebugRangesSection->Size() - offset,
		fCurrentCompilationUnit->AddressSize());
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
			fprintf(stderr, "Out of memory.\n");
			return NULL;
		}
	}

	return rangesReference.Detach();
}
