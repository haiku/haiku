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
#include "DwarfTargetInterface.h"
#include "DwarfUtils.h"
#include "ElfFile.h"
#include "FileManager.h"
#include "FileSourceCode.h"
#include "LocatableFile.h"
#include "Register.h"
#include "RegisterMap.h"
#include "SourceFile.h"
#include "StackFrame.h"
#include "Statement.h"
#include "StringUtils.h"
#include "TeamMemory.h"
#include "UnsupportedLanguage.h"


// #pragma mark - UnwindTargetInterface


struct DwarfImageDebugInfo::UnwindTargetInterface : DwarfTargetInterface {
	UnwindTargetInterface(const Register* registers, int32 registerCount,
		RegisterMap* fromDwarfMap, RegisterMap* toDwarfMap, CpuState* cpuState,
		Architecture* architecture, TeamMemory* teamMemory)
		:
		fRegisters(registers),
		fRegisterCount(registerCount),
		fFromDwarfMap(fromDwarfMap),
		fToDwarfMap(toDwarfMap),
		fCpuState(cpuState),
		fArchitecture(architecture),
		fTeamMemory(teamMemory)
	{
		fFromDwarfMap->AcquireReference();
		fToDwarfMap->AcquireReference();
		fCpuState->AcquireReference();
	}

	~UnwindTargetInterface()
	{
		fFromDwarfMap->ReleaseReference();
		fToDwarfMap->ReleaseReference();
		fCpuState->ReleaseReference();
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

	virtual bool IsCalleePreservedRegister(uint32 index) const
	{
		const Register* reg = _RegisterAt(index);
		return reg != NULL && reg->IsCalleePreserved();
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

private:
	const Register* _RegisterAt(uint32 dwarfIndex) const
	{
		int32 index = fFromDwarfMap->MapRegisterIndex(dwarfIndex);
		return index >= 0 && index < fRegisterCount ? fRegisters + index : NULL;
	}

private:
	const Register*	fRegisters;
	int32			fRegisterCount;
	RegisterMap*	fFromDwarfMap;
	RegisterMap*	fToDwarfMap;
	CpuState*		fCpuState;
	Architecture*	fArchitecture;
	TeamMemory*		fTeamMemory;
};



// #pragma mark - DwarfImageDebugInfo


DwarfImageDebugInfo::DwarfImageDebugInfo(const ImageInfo& imageInfo,
	Architecture* architecture, TeamMemory* teamMemory,
	FileManager* fileManager, DwarfFile* file)
	:
	fLock("dwarf image debug info"),
	fImageInfo(imageInfo),
	fArchitecture(architecture),
	fTeamMemory(teamMemory),
	fFileManager(fileManager),
	fFile(file),
	fTextSegment(NULL),
	fRelocationDelta(0)
{
}


DwarfImageDebugInfo::~DwarfImageDebugInfo()
{
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

	return B_OK;
}


status_t
DwarfImageDebugInfo::GetFunctions(BObjectList<FunctionDebugInfo>& functions)
{
printf("DwarfImageDebugInfo::GetFunctions()\n");
printf("  %ld compilation units\n", fFile->CountCompilationUnits());

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

			// ignore declarations, prototypes, and inlined functions
			if (subprogramEntry->IsDeclaration()
				|| subprogramEntry->IsPrototyped()
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
			TargetAddressRangeList* rangeList
				= subprogramEntry->AddressRanges();
			Reference<TargetAddressRangeList> rangeListReference(rangeList);
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
				rangeListReference.SetTo(rangeList, true);
			}

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
			Reference<LocatableFile> fileReference(file, true);

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
DwarfImageDebugInfo::CreateFrame(Image* image, FunctionDebugInfo* function,
	CpuState* cpuState, StackFrame*& _previousFrame,
	CpuState*& _previousCpuState)
{
	int32 registerCount = fArchitecture->CountRegisters();
	const Register* registers = fArchitecture->Registers();

	// get the DWARF <-> architecture register maps
	RegisterMap* toDwarfMap;
	RegisterMap* fromDwarfMap;
	status_t error = fArchitecture->GetDwarfRegisterMaps(&toDwarfMap,
		&fromDwarfMap);
	if (error != B_OK)
		return error;
	Reference<RegisterMap> toDwarfMapReference(toDwarfMap, true);
	Reference<RegisterMap> fromDwarfMapReference(fromDwarfMap, true);

	// create a clean CPU state for the previous frame
	CpuState* previousCpuState;
	error = fArchitecture->CreateCpuState(previousCpuState);
	if (error != B_OK)
		return error;
	Reference<CpuState> previousCpuStateReference(previousCpuState, true);

	// create the target interfaces
	UnwindTargetInterface inputInterface(registers, registerCount,
		fromDwarfMap, toDwarfMap, cpuState, fArchitecture, fTeamMemory);
	UnwindTargetInterface outputInterface(registers, registerCount,
		fromDwarfMap, toDwarfMap, previousCpuState, fArchitecture, fTeamMemory);

	// do the unwinding
	target_addr_t framePointer;
	error = fFile->UnwindCallFrame(
		cpuState->InstructionPointer() - fRelocationDelta,
		&inputInterface, &outputInterface, framePointer);
	if (error != B_OK)
		return B_UNSUPPORTED;

printf("unwound registers:\n");
for (int32 i = 0; i < registerCount; i++) {
const Register* reg = registers + i;
BVariant value;
if (previousCpuState->GetRegisterValue(reg, value)) {
	printf("  %3s: %#lx\n", reg->Name(), value.ToUInt32());
} else
	printf("  %3s: undefined\n", reg->Name());
}

	// create the stack frame
	StackFrame* frame = new(std::nothrow) StackFrame(STACK_FRAME_TYPE_STANDARD,
		cpuState, framePointer, cpuState->InstructionPointer());
	if (frame == NULL)
		return B_NO_MEMORY;

	frame->SetReturnAddress(previousCpuState->InstructionPointer());
		// Note, this is correct, since we actually retrieved the return
		// address. Our caller will fix the IP for us.

	_previousFrame = frame;
	_previousCpuState = previousCpuStateReference.Detach();

	return B_OK;
}


status_t
DwarfImageDebugInfo::GetStatement(FunctionDebugInfo* _function,
	target_addr_t address, Statement*& _statement)
{
printf("DwarfImageDebugInfo::GetStatement(function: %p, address: %#llx)\n",
_function, address);
	DwarfFunctionDebugInfo* function
		= dynamic_cast<DwarfFunctionDebugInfo*>(_function);
	if (function == NULL)
{
printf("  -> no dwarf function\n");
		return B_BAD_VALUE;
}

	AutoLocker<BLocker> locker(fLock);

	// check whether we have the source code
	CompilationUnit* unit = function->GetCompilationUnit();
	LocatableFile* file = function->SourceFile();
	if (file == NULL) {
printf("  -> no source file\n");
		// no source code -- rather return the assembly statement
		return fArchitecture->GetStatement(function, address, _statement);
	}

	// get the index of the source file in the compilation unit for cheaper
	// comparison below
	int32 fileIndex = _GetSourceFileIndex(unit, file);

	// Get the statement by executing the line number program for the
	// compilation unit.
	LineNumberProgram& program = unit->GetLineNumberProgram();
	if (!program.IsValid())
{
printf("  -> no line number program\n");
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

printf("  -> no line number program match\n");
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
printf("DwarfImageDebugInfo::GetStatementAtSourceLocation(%p, (%ld, %ld)): function range: %#llx - %#llx\n",
function, sourceLocation.Line(), sourceLocation.Column(), functionStartAddress, functionEndAddress);

	AutoLocker<BLocker> locker(fLock);

	// get the source file
	LocatableFile* file = function->SourceFile();
	if (file == NULL)
		return B_ENTRY_NOT_FOUND;

	CompilationUnit* unit = function->GetCompilationUnit();

	// get the index of the source file in the compilation unit for cheaper
	// comparison below
	int32 fileIndex = _GetSourceFileIndex(unit, file);

//	target_addr_t functionStartAddress = function->Address() - fRelocationDelta;
//	target_addr_t functionEndAddress = functionStartAddress + function->Size();

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
printf("  statement: %#llx - %#llx, location: (%ld, %ld)\n", statementAddress, endAddress, statementLine, statementColumn);
}
			if (statementAddress < endAddress
				&& statementAddress >= functionStartAddress
				&& statementAddress < functionEndAddress
				&& statementLine == (int32)sourceLocation.Line()
				&& statementColumn == (int32)sourceLocation.Column()) {
printf("  -> found statement!\n");
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
printf("  %#lx  (%ld, %ld, %ld)  %d\n", state.address, state.file, state.line, state.column, state.isStatement);
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
printf("  -> statement: %#llx - %#llx, source location: (%ld, %ld)\n", statementAddress, endAddress, statementLine, statementColumn);
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
