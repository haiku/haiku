/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfImageDebugInfo.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "Architecture.h"
#include "CLanguage.h"
#include "CompilationUnit.h"
#include "CppLanguage.h"
#include "CpuState.h"
#include "DebugInfoEntries.h"
#include "Dwarf.h"
#include "DwarfFile.h"
#include "DwarfFunctionDebugInfo.h"
#include "DwarfStackFrameDebugInfo.h"
#include "DwarfTargetInterface.h"
#include "DwarfTypeFactory.h"
#include "DwarfTypes.h"
#include "DwarfUtils.h"
#include "ElfFile.h"
#include "FileManager.h"
#include "FileSourceCode.h"
#include "FunctionID.h"
#include "FunctionInstance.h"
#include "GlobalTypeLookup.h"
#include "LocatableFile.h"
#include "Register.h"
#include "RegisterMap.h"
#include "SourceFile.h"
#include "StackFrame.h"
#include "Statement.h"
#include "StringUtils.h"
#include "TargetAddressRangeList.h"
#include "TeamMemory.h"
#include "Tracing.h"
#include "TypeLookupConstraints.h"
#include "UnsupportedLanguage.h"
#include "Variable.h"


namespace {


// #pragma mark - HasTypePredicate


template<typename EntryType>
struct HasTypePredicate {
	inline bool operator()(EntryType* entry) const
	{
		return entry->GetType() != NULL;
	}
};

}


// #pragma mark - BasicTargetInterface


struct DwarfImageDebugInfo::BasicTargetInterface : DwarfTargetInterface {
	BasicTargetInterface(const Register* registers, int32 registerCount,
		RegisterMap* fromDwarfMap, Architecture* architecture,
		TeamMemory* teamMemory)
		:
		fRegisters(registers),
		fRegisterCount(registerCount),
		fFromDwarfMap(fromDwarfMap),
		fArchitecture(architecture),
		fTeamMemory(teamMemory)
	{
		fFromDwarfMap->AcquireReference();
	}

	~BasicTargetInterface()
	{
		fFromDwarfMap->ReleaseReference();
	}

	virtual uint32 CountRegisters() const
	{
		return fRegisterCount;
	}

	virtual uint32 RegisterValueType(uint32 index) const
	{
		const Register* reg = _RegisterAt(index);
		return reg != NULL ? reg->ValueType() : 0;
	}

	virtual bool GetRegisterValue(uint32 index, BVariant& _value) const
	{
		return false;
	}

	virtual bool SetRegisterValue(uint32 index, const BVariant& value)
	{
		return false;
	}

	virtual bool IsCalleePreservedRegister(uint32 index) const
	{
		const Register* reg = _RegisterAt(index);
		return reg != NULL && reg->IsCalleePreserved();
	}

	virtual status_t InitRegisterRules(CfaContext& context) const
	{
		return fArchitecture->InitRegisterRules(context);
	}

	virtual bool ReadMemory(target_addr_t address, void* buffer,
		size_t size) const
	{
		ssize_t bytesRead = fTeamMemory->ReadMemory(address, buffer, size);
		return bytesRead >= 0 && (size_t)bytesRead == size;
	}

	virtual bool ReadValueFromMemory(target_addr_t address,
		uint32 valueType, BVariant& _value) const
	{
		return fArchitecture->ReadValueFromMemory(address, valueType, _value)
			== B_OK;
	}

	virtual bool ReadValueFromMemory(target_addr_t addressSpace,
		target_addr_t address, uint32 valueType, BVariant& _value) const
	{
		return fArchitecture->ReadValueFromMemory(addressSpace, address,
			valueType, _value) == B_OK;
	}

protected:
	const Register* _RegisterAt(uint32 dwarfIndex) const
	{
		int32 index = fFromDwarfMap->MapRegisterIndex(dwarfIndex);
		return index >= 0 && index < fRegisterCount ? fRegisters + index : NULL;
	}

protected:
	const Register*	fRegisters;
	int32			fRegisterCount;
	RegisterMap*	fFromDwarfMap;
	Architecture*	fArchitecture;
	TeamMemory*		fTeamMemory;
};


// #pragma mark - UnwindTargetInterface


struct DwarfImageDebugInfo::UnwindTargetInterface : BasicTargetInterface {
	UnwindTargetInterface(const Register* registers, int32 registerCount,
		RegisterMap* fromDwarfMap, RegisterMap* toDwarfMap, CpuState* cpuState,
		Architecture* architecture, TeamMemory* teamMemory)
		:
		BasicTargetInterface(registers, registerCount, fromDwarfMap,
			architecture, teamMemory),
		fToDwarfMap(toDwarfMap),
		fCpuState(cpuState)
	{
		fToDwarfMap->AcquireReference();
		fCpuState->AcquireReference();
	}

	~UnwindTargetInterface()
	{
		fToDwarfMap->ReleaseReference();
		fCpuState->ReleaseReference();
	}

	virtual bool GetRegisterValue(uint32 index, BVariant& _value) const
	{
		const Register* reg = _RegisterAt(index);
		if (reg == NULL)
			return false;
		return fCpuState->GetRegisterValue(reg, _value);
	}

	virtual bool SetRegisterValue(uint32 index, const BVariant& value)
	{
		const Register* reg = _RegisterAt(index);
		if (reg == NULL)
			return false;
		return fCpuState->SetRegisterValue(reg, value);
	}

private:
	RegisterMap*	fToDwarfMap;
	CpuState*		fCpuState;
};


// #pragma mark - EntryListWrapper


/*!	Wraps a DebugInfoEntryList, which is a typedef and thus cannot appear in
	the header, since our policy disallows us to include DWARF headers there.
*/
struct DwarfImageDebugInfo::EntryListWrapper {
	const DebugInfoEntryList&	list;

	EntryListWrapper(const DebugInfoEntryList& list)
		:
		list(list)
	{
	}
};


// #pragma mark - DwarfImageDebugInfo


DwarfImageDebugInfo::DwarfImageDebugInfo(const ImageInfo& imageInfo,
	Architecture* architecture, TeamMemory* teamMemory,
	FileManager* fileManager, GlobalTypeLookup* typeLookup,
	GlobalTypeCache* typeCache, DwarfFile* file)
	:
	fLock("dwarf image debug info"),
	fImageInfo(imageInfo),
	fArchitecture(architecture),
	fTeamMemory(teamMemory),
	fFileManager(fileManager),
	fTypeLookup(typeLookup),
	fTypeCache(typeCache),
	fFile(file),
	fTextSegment(NULL),
	fRelocationDelta(0),
	fTextSectionStart(0),
	fTextSectionEnd(0),
	fPLTSectionStart(0),
	fPLTSectionEnd(0)
{
	fFile->AcquireReference();
	fTypeCache->AcquireReference();
}


DwarfImageDebugInfo::~DwarfImageDebugInfo()
{
	fFile->ReleaseReference();
	fTypeCache->ReleaseReference();
}


status_t
DwarfImageDebugInfo::Init()
{
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	fTextSegment = fFile->GetElfFile()->TextSegment();
	if (fTextSegment == NULL)
		return B_ENTRY_NOT_FOUND;

	fRelocationDelta = fImageInfo.TextBase() - fTextSegment->LoadAddress();

	ElfSection* section = fFile->GetElfFile()->FindSection(".text");
	if (section != NULL) {
		fTextSectionStart = section->LoadAddress() + fRelocationDelta;
		fTextSectionEnd = fTextSectionStart + section->Size();
	}

	section = fFile->GetElfFile()->FindSection(".plt");
	if (section != NULL) {
		fPLTSectionStart = section->LoadAddress() + fRelocationDelta;
		fPLTSectionEnd = fPLTSectionStart + section->Size();
	}

	return B_OK;
}


status_t
DwarfImageDebugInfo::GetFunctions(BObjectList<FunctionDebugInfo>& functions)
{
	TRACE_IMAGES("DwarfImageDebugInfo::GetFunctions()\n");
	TRACE_IMAGES("  %ld compilation units\n", fFile->CountCompilationUnits());

	for (int32 i = 0; CompilationUnit* unit = fFile->CompilationUnitAt(i);
			i++) {
		DIECompileUnitBase* unitEntry = unit->UnitEntry();
//		printf("  %s:\n", unitEntry->Name());
//		printf("    address ranges:\n");
//		TargetAddressRangeList* rangeList = unitEntry->AddressRanges();
//		if (rangeList != NULL) {
//			int32 count = rangeList->CountRanges();
//			for (int32 i = 0; i < count; i++) {
//				TargetAddressRange range = rangeList->RangeAt(i);
//				printf("      %#llx - %#llx\n", range.Start(), range.End());
//			}
//		} else {
//			printf("      %#llx - %#llx\n", (target_addr_t)unitEntry->LowPC(),
//				(target_addr_t)unitEntry->HighPC());
//		}

//		printf("    functions:\n");
		for (DebugInfoEntryList::ConstIterator it
					= unitEntry->OtherChildren().GetIterator();
				DebugInfoEntry* entry = it.Next();) {
			if (entry->Tag() != DW_TAG_subprogram)
				continue;

			DIESubprogram* subprogramEntry = static_cast<DIESubprogram*>(entry);

			// ignore declarations and inlined functions
			if (subprogramEntry->IsDeclaration()
				|| subprogramEntry->Inline() == DW_INL_inlined
				|| subprogramEntry->Inline() == DW_INL_declared_inlined) {
				continue;
			}

			// get the name
			BString name;
			DwarfUtils::GetFullyQualifiedDIEName(subprogramEntry, name);
			if (name.Length() == 0)
				continue;

			// get the address ranges
			TargetAddressRangeList* rangeList = fFile->ResolveRangeList(unit,
				subprogramEntry->AddressRangesOffset());
			if (rangeList == NULL) {
				target_addr_t lowPC = subprogramEntry->LowPC();
				target_addr_t highPC = subprogramEntry->HighPC();
				if (lowPC >= highPC)
					continue;

				rangeList = new(std::nothrow) TargetAddressRangeList(
					TargetAddressRange(lowPC, highPC - lowPC));
				if (rangeList == NULL)
					return B_NO_MEMORY;
						// TODO: Clean up already added functions!
			}
			BReference<TargetAddressRangeList> rangeListReference(rangeList,
				true);

			// get the source location
			const char* directoryPath = NULL;
			const char* fileName = NULL;
			int32 line = -1;
			int32 column = -1;
			DwarfUtils::GetDeclarationLocation(fFile, subprogramEntry,
				directoryPath, fileName, line, column);

			LocatableFile* file = NULL;
			if (fileName != NULL) {
				file = fFileManager->GetSourceFile(directoryPath,
					fileName);
			}
			BReference<LocatableFile> fileReference(file, true);

			// create and add the functions
			DwarfFunctionDebugInfo* function
				= new(std::nothrow) DwarfFunctionDebugInfo(this, unit,
					subprogramEntry, rangeList, name, file,
					SourceLocation(line, std::max(column, 0L)));
			if (function == NULL || !functions.AddItem(function)) {
				delete function;
				return B_NO_MEMORY;
					// TODO: Clean up already added functions!
			}

//			BString name;
//			DwarfUtils::GetFullyQualifiedDIEName(subprogramEntry, name);
//			printf("      subprogram entry: %p, name: %s, declaration: %d\n",
//				subprogramEntry, name.String(),
//				subprogramEntry->IsDeclaration());
//
//			rangeList = subprogramEntry->AddressRanges();
//			if (rangeList != NULL) {
//				int32 count = rangeList->CountRanges();
//				for (int32 i = 0; i < count; i++) {
//					TargetAddressRange range = rangeList->RangeAt(i);
//					printf("        %#llx - %#llx\n", range.Start(), range.End());
//				}
//			} else {
//				printf("        %#llx - %#llx\n",
//					(target_addr_t)subprogramEntry->LowPC(),
//					(target_addr_t)subprogramEntry->HighPC());
//			}
		}
	}

	return B_OK;
}


status_t
DwarfImageDebugInfo::GetType(GlobalTypeCache* cache,
	const BString& name, const TypeLookupConstraints& constraints,
	Type*& _type)
{
	int32 registerCount = fArchitecture->CountRegisters();
	const Register* registers = fArchitecture->Registers();

	// get the DWARF -> architecture register map
	RegisterMap* fromDwarfMap;
	status_t error = fArchitecture->GetDwarfRegisterMaps(NULL, &fromDwarfMap);
	if (error != B_OK)
		return error;
	BReference<RegisterMap> fromDwarfMapReference(fromDwarfMap, true);

	// create the target interface
	BasicTargetInterface inputInterface(registers, registerCount, fromDwarfMap,
		fArchitecture, fTeamMemory);

	// iterate through all compilation units
	for (int32 i = 0; CompilationUnit* unit = fFile->CompilationUnitAt(i);
		i++) {
		DwarfTypeContext* typeContext = NULL;
		BReference<DwarfTypeContext> typeContextReference;

		// iterate through all types of the compilation unit
		for (DebugInfoEntryList::ConstIterator it
				= unit->UnitEntry()->Types().GetIterator();
			DIEType* typeEntry = dynamic_cast<DIEType*>(it.Next());) {
			if (typeEntry->IsDeclaration())
				continue;

			if (constraints.HasTypeKind()) {
				if (dwarf_tag_to_type_kind(typeEntry->Tag())
					!= constraints.TypeKind())
				continue;

				if (!_EvaluateBaseTypeConstraints(typeEntry,
					constraints))
					continue;
			}

			if (constraints.HasSubtypeKind()
				&& dwarf_tag_to_subtype_kind(typeEntry->Tag())
					!= constraints.SubtypeKind())
				continue;

			BString typeEntryName;
			DwarfUtils::GetFullyQualifiedDIEName(typeEntry, typeEntryName);
			if (typeEntryName != name)
				continue;

			// The name matches and the entry is not just a declaration --
			// create the type. First create the type context lazily.
			if (typeContext == NULL) {
				typeContext = new(std::nothrow)
					DwarfTypeContext(fArchitecture, fImageInfo.ImageID(), fFile,
					unit, NULL, 0, 0, fRelocationDelta, &inputInterface,
					fromDwarfMap);
				if (typeContext == NULL)
					return B_NO_MEMORY;
				typeContextReference.SetTo(typeContext, true);
			}

			// create the type
			DwarfType* type;
			DwarfTypeFactory typeFactory(typeContext, fTypeLookup, cache);
			error = typeFactory.CreateType(typeEntry, type);
			if (error != B_OK)
				continue;

			_type = type;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


AddressSectionType
DwarfImageDebugInfo::GetAddressSectionType(target_addr_t address)
{
	if (address >= fTextSectionStart && address < fTextSectionEnd)
		return ADDRESS_SECTION_TYPE_FUNCTION;

 	if (address >= fPLTSectionStart && address < fPLTSectionEnd)
		return ADDRESS_SECTION_TYPE_PLT;

	return ADDRESS_SECTION_TYPE_UNKNOWN;
}


status_t
DwarfImageDebugInfo::CreateFrame(Image* image,
	FunctionInstance* functionInstance, CpuState* cpuState,
	StackFrame*& _previousFrame, CpuState*& _previousCpuState)
{
	DwarfFunctionDebugInfo* function = dynamic_cast<DwarfFunctionDebugInfo*>(
		functionInstance->GetFunctionDebugInfo());
	if (function == NULL)
		return B_BAD_VALUE;

	FunctionID* functionID = functionInstance->GetFunctionID();
	if (functionID == NULL)
		return B_NO_MEMORY;
	BReference<FunctionID> functionIDReference(functionID, true);

	TRACE_CFI("DwarfImageDebugInfo::CreateFrame(): subprogram DIE: %p, "
		"function: %s\n", function->SubprogramEntry(),
		functionID->FunctionName().String());

	int32 registerCount = fArchitecture->CountRegisters();
	const Register* registers = fArchitecture->Registers();

	// get the DWARF <-> architecture register maps
	RegisterMap* toDwarfMap;
	RegisterMap* fromDwarfMap;
	status_t error = fArchitecture->GetDwarfRegisterMaps(&toDwarfMap,
		&fromDwarfMap);
	if (error != B_OK)
		return error;
	BReference<RegisterMap> toDwarfMapReference(toDwarfMap, true);
	BReference<RegisterMap> fromDwarfMapReference(fromDwarfMap, true);

	// create a clean CPU state for the previous frame
	CpuState* previousCpuState;
	error = fArchitecture->CreateCpuState(previousCpuState);
	if (error != B_OK)
		return error;
	BReference<CpuState> previousCpuStateReference(previousCpuState, true);

	// create the target interfaces
	UnwindTargetInterface* inputInterface
		= new(std::nothrow) UnwindTargetInterface(registers, registerCount,
			fromDwarfMap, toDwarfMap, cpuState, fArchitecture, fTeamMemory);
	if (inputInterface == NULL)
		return B_NO_MEMORY;
	BReference<UnwindTargetInterface> inputInterfaceReference(inputInterface,
		true);

	UnwindTargetInterface* outputInterface
		= new(std::nothrow) UnwindTargetInterface(registers, registerCount,
			fromDwarfMap, toDwarfMap, previousCpuState, fArchitecture,
			fTeamMemory);
	if (outputInterface == NULL)
		return B_NO_MEMORY;
	BReference<UnwindTargetInterface> outputInterfaceReference(outputInterface,
		true);

	// do the unwinding
	target_addr_t instructionPointer
		= cpuState->InstructionPointer() - fRelocationDelta;
	target_addr_t framePointer;
	CompilationUnit* unit = function->GetCompilationUnit();
	error = fFile->UnwindCallFrame(unit, function->SubprogramEntry(),
		instructionPointer, inputInterface, outputInterface, framePointer);
	if (error != B_OK) {
		TRACE_CFI("Failed to unwind call frame: %s\n", strerror(error));
		return B_UNSUPPORTED;
	}

	TRACE_CFI_ONLY(
		TRACE_CFI("unwound registers:\n");
		for (int32 i = 0; i < registerCount; i++) {
			const Register* reg = registers + i;
			BVariant value;
			if (previousCpuState->GetRegisterValue(reg, value))
				TRACE_CFI("  %3s: %#lx\n", reg->Name(), value.ToUInt32());
			else
				TRACE_CFI("  %3s: undefined\n", reg->Name());
		}
	)

	// create the stack frame debug info
	DIESubprogram* subprogramEntry = function->SubprogramEntry();
	DwarfStackFrameDebugInfo* stackFrameDebugInfo
		= new(std::nothrow) DwarfStackFrameDebugInfo(fArchitecture,
			fImageInfo.ImageID(), fFile, unit, subprogramEntry, fTypeLookup,
			fTypeCache, instructionPointer, framePointer, fRelocationDelta,
			inputInterface, fromDwarfMap);
	if (stackFrameDebugInfo == NULL)
		return B_NO_MEMORY;
	BReference<DwarfStackFrameDebugInfo> stackFrameDebugInfoReference(
		stackFrameDebugInfo, true);

	error = stackFrameDebugInfo->Init();
	if (error != B_OK)
		return error;

	// create the stack frame
	StackFrame* frame = new(std::nothrow) StackFrame(STACK_FRAME_TYPE_STANDARD,
		cpuState, framePointer, cpuState->InstructionPointer(),
		stackFrameDebugInfo);
	if (frame == NULL)
		return B_NO_MEMORY;
	BReference<StackFrame> frameReference(frame, true);

	error = frame->Init();
	if (error != B_OK)
		return error;

	frame->SetReturnAddress(previousCpuState->InstructionPointer());
		// Note, this is correct, since we actually retrieved the return
		// address. Our caller will fix the IP for us.

	// create function parameter objects
	for (DebugInfoEntryList::ConstIterator it = subprogramEntry->Parameters()
			.GetIterator(); DebugInfoEntry* entry = it.Next();) {
		if (entry->Tag() != DW_TAG_formal_parameter)
			continue;

		BString parameterName;
		DwarfUtils::GetDIEName(entry, parameterName);
		if (parameterName.Length() == 0)
			continue;

		DIEFormalParameter* parameterEntry
			= dynamic_cast<DIEFormalParameter*>(entry);
		Variable* parameter;
		if (stackFrameDebugInfo->CreateParameter(functionID, parameterEntry,
				parameter) != B_OK) {
			continue;
		}
		BReference<Variable> parameterReference(parameter, true);

		if (!frame->AddParameter(parameter))
			return B_NO_MEMORY;
	}

	// create objects for the local variables
	_CreateLocalVariables(unit, frame, functionID, *stackFrameDebugInfo,
		instructionPointer, functionInstance->Address() - fRelocationDelta,
		subprogramEntry->Variables(), subprogramEntry->Blocks());

	_previousFrame = frameReference.Detach();
	_previousCpuState = previousCpuStateReference.Detach();

	return B_OK;
}


status_t
DwarfImageDebugInfo::GetStatement(FunctionDebugInfo* _function,
	target_addr_t address, Statement*& _statement)
{
	TRACE_CODE("DwarfImageDebugInfo::GetStatement(function: %p, address: %#llx)\n",
		_function, address);

	DwarfFunctionDebugInfo* function
		= dynamic_cast<DwarfFunctionDebugInfo*>(_function);
	if (function == NULL) {
		TRACE_LINES("  -> no dwarf function\n");
		return B_BAD_VALUE;
	}

	AutoLocker<BLocker> locker(fLock);

	// check whether we have the source code
	CompilationUnit* unit = function->GetCompilationUnit();
	LocatableFile* file = function->SourceFile();
	if (file == NULL) {
		TRACE_CODE("  -> no source file\n");

		// no source code -- rather return the assembly statement
		return fArchitecture->GetStatement(function, address, _statement);
	}

	// get the index of the source file in the compilation unit for cheaper
	// comparison below
	int32 fileIndex = _GetSourceFileIndex(unit, file);

	// Get the statement by executing the line number program for the
	// compilation unit.
	LineNumberProgram& program = unit->GetLineNumberProgram();
	if (!program.IsValid()) {
		TRACE_CODE("  -> no line number program\n");
		return B_BAD_DATA;
	}

	// adjust address
	address -= fRelocationDelta;

	LineNumberProgram::State state;
	program.GetInitialState(state);

	target_addr_t statementAddress = 0;
	int32 statementLine = -1;
	int32 statementColumn = -1;
	while (program.GetNextRow(state)) {
		bool isOurFile = state.file == fileIndex;

		if (statementAddress != 0
			&& (!isOurFile || state.isStatement || state.isSequenceEnd)) {
			target_addr_t endAddress = state.address;
			if (address >= statementAddress && address < endAddress) {
				ContiguousStatement* statement = new(std::nothrow)
					ContiguousStatement(
						SourceLocation(statementLine, statementColumn),
						TargetAddressRange(fRelocationDelta + statementAddress,
							endAddress - statementAddress));
				if (statement == NULL)
					return B_NO_MEMORY;

				_statement = statement;
				return B_OK;
			}

			statementAddress = 0;
		}

		// skip statements of other files
		if (!isOurFile)
			continue;

		if (state.isStatement) {
			statementAddress = state.address;
			statementLine = state.line - 1;
			statementColumn = std::max(state.column - 1, 0L);
		}
	}

	TRACE_CODE("  -> no line number program match\n");
	return B_ENTRY_NOT_FOUND;
}


status_t
DwarfImageDebugInfo::GetStatementAtSourceLocation(FunctionDebugInfo* _function,
	const SourceLocation& sourceLocation, Statement*& _statement)
{
	DwarfFunctionDebugInfo* function
		= dynamic_cast<DwarfFunctionDebugInfo*>(_function);
	if (function == NULL)
		return B_BAD_VALUE;

	target_addr_t functionStartAddress = function->Address() - fRelocationDelta;
	target_addr_t functionEndAddress = functionStartAddress + function->Size();

	TRACE_LINES2("DwarfImageDebugInfo::GetStatementAtSourceLocation(%p, "
		"(%ld, %ld)): function range: %#llx - %#llx\n", function,
		sourceLocation.Line(), sourceLocation.Column(),
		functionStartAddress, functionEndAddress);

	AutoLocker<BLocker> locker(fLock);

	// get the source file
	LocatableFile* file = function->SourceFile();
	if (file == NULL)
		return B_ENTRY_NOT_FOUND;

	CompilationUnit* unit = function->GetCompilationUnit();

	// get the index of the source file in the compilation unit for cheaper
	// comparison below
	int32 fileIndex = _GetSourceFileIndex(unit, file);

	// Get the statement by executing the line number program for the
	// compilation unit.
	LineNumberProgram& program = unit->GetLineNumberProgram();
	if (!program.IsValid())
		return B_BAD_DATA;

	LineNumberProgram::State state;
	program.GetInitialState(state);

	target_addr_t statementAddress = 0;
	int32 statementLine = -1;
	int32 statementColumn = -1;
	while (program.GetNextRow(state)) {
		bool isOurFile = state.file == fileIndex;

		if (statementAddress != 0
			&& (!isOurFile || state.isStatement || state.isSequenceEnd)) {
			target_addr_t endAddress = state.address;

			if (statementAddress < endAddress) {
				TRACE_LINES2("  statement: %#llx - %#llx, location: "
					"(%ld, %ld)\n", statementAddress, endAddress, statementLine,
					statementColumn);
			}

			if (statementAddress < endAddress
				&& statementAddress >= functionStartAddress
				&& statementAddress < functionEndAddress
				&& statementLine == (int32)sourceLocation.Line()
				&& statementColumn == (int32)sourceLocation.Column()) {
				TRACE_LINES2("  -> found statement!\n");

				ContiguousStatement* statement = new(std::nothrow)
					ContiguousStatement(
						SourceLocation(statementLine, statementColumn),
						TargetAddressRange(fRelocationDelta + statementAddress,
							endAddress - statementAddress));
				if (statement == NULL)
					return B_NO_MEMORY;

				_statement = statement;
				return B_OK;
			}

			statementAddress = 0;
		}

		// skip statements of other files
		if (!isOurFile)
			continue;

		if (state.isStatement) {
			statementAddress = state.address;
			statementLine = state.line - 1;
			statementColumn = std::max(state.column - 1, 0L);
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DwarfImageDebugInfo::GetSourceLanguage(FunctionDebugInfo* _function,
	SourceLanguage*& _language)
{
	DwarfFunctionDebugInfo* function
		= dynamic_cast<DwarfFunctionDebugInfo*>(_function);
	if (function == NULL)
		return B_BAD_VALUE;

	SourceLanguage* language;
	CompilationUnit* unit = function->GetCompilationUnit();
	switch (unit->UnitEntry()->Language()) {
		case DW_LANG_C89:
		case DW_LANG_C:
		case DW_LANG_C99:
			language = new(std::nothrow) CLanguage;
			break;
		case DW_LANG_C_plus_plus:
			language = new(std::nothrow) CppLanguage;
			break;
		case 0:
		default:
			language = new(std::nothrow) UnsupportedLanguage;
			break;
	}

	if (language == NULL)
		return B_NO_MEMORY;

	_language = language;
	return B_OK;
}


ssize_t
DwarfImageDebugInfo::ReadCode(target_addr_t address, void* buffer, size_t size)
{
	target_addr_t offset = address - fRelocationDelta
		- fTextSegment->LoadAddress() + fTextSegment->FileOffset();
	ssize_t bytesRead = pread(fFile->GetElfFile()->FD(), buffer, size, offset);
	return bytesRead >= 0 ? bytesRead : errno;
}


status_t
DwarfImageDebugInfo::AddSourceCodeInfo(LocatableFile* file,
	FileSourceCode* sourceCode)
{
	bool addedAny = false;
	for (int32 i = 0; CompilationUnit* unit = fFile->CompilationUnitAt(i);
			i++) {
		int32 fileIndex = _GetSourceFileIndex(unit, file);
		if (fileIndex < 0)
			continue;

		status_t error = _AddSourceCodeInfo(unit, sourceCode, fileIndex);
		if (error == B_NO_MEMORY)
			return error;
		addedAny |= error == B_OK;
	}

	return addedAny ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
DwarfImageDebugInfo::_AddSourceCodeInfo(CompilationUnit* unit,
	FileSourceCode* sourceCode, int32 fileIndex)
{
	// Get the statements by executing the line number program for the
	// compilation unit and filtering the rows for our source file.
	LineNumberProgram& program = unit->GetLineNumberProgram();
	if (!program.IsValid())
		return B_BAD_DATA;

	LineNumberProgram::State state;
	program.GetInitialState(state);

	target_addr_t statementAddress = 0;
	int32 statementLine = -1;
	int32 statementColumn = -1;
	while (program.GetNextRow(state)) {
		TRACE_LINES2("  %#llx  (%ld, %ld, %ld)  %d\n", state.address,
			state.file, state.line, state.column, state.isStatement);

		bool isOurFile = state.file == fileIndex;

		if (statementAddress != 0
			&& (!isOurFile || state.isStatement || state.isSequenceEnd)) {
			target_addr_t endAddress = state.address;
			if (endAddress > statementAddress) {
				// add the statement
				status_t error = sourceCode->AddSourceLocation(
					SourceLocation(statementLine, statementColumn));
				if (error != B_OK)
					return error;

				TRACE_LINES2("  -> statement: %#llx - %#llx, source location: "
					"(%ld, %ld)\n", statementAddress, endAddress, statementLine,
					statementColumn);
			}

			statementAddress = 0;
		}

		// skip statements of other files
		if (!isOurFile)
			continue;

		if (state.isStatement) {
			statementAddress = state.address;
			statementLine = state.line - 1;
			statementColumn = std::max(state.column - 1, 0L);
		}
	}

	return B_OK;
}


int32
DwarfImageDebugInfo::_GetSourceFileIndex(CompilationUnit* unit,
	LocatableFile* sourceFile) const
{
	// get the index of the source file in the compilation unit for cheaper
	// comparison below
	const char* directory;
	for (int32 i = 0; const char* fileName = unit->FileAt(i, &directory); i++) {
		LocatableFile* file = fFileManager->GetSourceFile(directory, fileName);
		if (file != NULL) {
			file->ReleaseReference();
			if (file == sourceFile) {
				return i + 1;
					// indices are one-based
			}
		}
	}

	return -1;
}


status_t
DwarfImageDebugInfo::_CreateLocalVariables(CompilationUnit* unit,
	StackFrame* frame, FunctionID* functionID,
	DwarfStackFrameDebugInfo& factory, target_addr_t instructionPointer,
	target_addr_t lowPC, const EntryListWrapper& variableEntries,
	const EntryListWrapper& blockEntries)
{
	TRACE_LOCALS("DwarfImageDebugInfo::_CreateLocalVariables(): ip: %#llx, "
		"low PC: %#llx\n", instructionPointer, lowPC);

	// iterate through the variables and add the ones in scope
	for (DebugInfoEntryList::ConstIterator it
			= variableEntries.list.GetIterator();
		DIEVariable* variableEntry = dynamic_cast<DIEVariable*>(it.Next());) {

		TRACE_LOCALS("  variableEntry %p, scope start: %llu\n", variableEntry,
			variableEntry->StartScope());

		// check the variable's scope
		if (instructionPointer < lowPC + variableEntry->StartScope())
			continue;

		// add the variable
		Variable* variable;
		if (factory.CreateLocalVariable(functionID, variableEntry, variable)
				!= B_OK) {
			continue;
		}
		BReference<Variable> variableReference(variable, true);

		if (!frame->AddLocalVariable(variable))
			return B_NO_MEMORY;
	}

	// iterate through the blocks and find the one we're currently in (if any)
	for (DebugInfoEntryList::ConstIterator it = blockEntries.list.GetIterator();
		DIELexicalBlock* block = dynamic_cast<DIELexicalBlock*>(it.Next());) {

		TRACE_LOCALS("  lexical block: %p\n", block);

		// check whether the block has low/high PC attributes
		if (block->LowPC() != 0) {
			TRACE_LOCALS("    has lowPC\n");

			// yep, compare with the instruction pointer
			if (instructionPointer < block->LowPC()
				|| instructionPointer >= block->HighPC()) {
				continue;
			}
		} else {
			TRACE_LOCALS("    no lowPC\n");

			// check the address ranges instead
			TargetAddressRangeList* rangeList = fFile->ResolveRangeList(unit,
				block->AddressRangesOffset());
			if (rangeList == NULL) {
				TRACE_LOCALS("    failed to get ranges\n");
				continue;
			}
			BReference<TargetAddressRangeList> rangeListReference(rangeList,
				true);

			if (!rangeList->Contains(instructionPointer)) {
				TRACE_LOCALS("    ranges don't contain IP\n");
				continue;
			}
		}

		// found a block -- recurse
		return _CreateLocalVariables(unit, frame, functionID, factory,
			instructionPointer, lowPC, block->Variables(), block->Blocks());
	}

	return B_OK;
}


bool
DwarfImageDebugInfo::_EvaluateBaseTypeConstraints(DIEType* type,
	const TypeLookupConstraints& constraints)
{
	if (constraints.HasBaseTypeName()) {
		BString baseEntryName;
		DIEType* baseTypeOwnerEntry = NULL;

		switch (constraints.TypeKind()) {
			case TYPE_ADDRESS:
			{
				DIEAddressingType* addressType =
					dynamic_cast<DIEAddressingType*>(type);
				if (addressType != NULL) {
					baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
						addressType, HasTypePredicate<DIEAddressingType>());
				}
				break;
			}
			case TYPE_ARRAY:
			{
				DIEArrayType* arrayType =
					dynamic_cast<DIEArrayType*>(type);
				if (arrayType != NULL) {
					baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
						arrayType, HasTypePredicate<DIEArrayType>());
				}
				break;
			}
			default:
				break;
		}

		if (baseTypeOwnerEntry != NULL) {
			DwarfUtils::GetFullyQualifiedDIEName(baseTypeOwnerEntry,
				baseEntryName);
			if (!baseEntryName.IsEmpty() && baseEntryName
				!= constraints.BaseTypeName())
				return false;
		}
	}

	return true;
}
