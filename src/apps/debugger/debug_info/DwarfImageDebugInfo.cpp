/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfImageDebugInfo.h"

#include <stdio.h>
#include <unistd.h>

#include <new>

#include <AutoDeleter.h>

#include "Architecture.h"
#include "CompilationUnit.h"
#include "DebugInfoEntries.h"
#include "Dwarf.h"
#include "DwarfFile.h"
#include "DwarfFunctionDebugInfo.h"
#include "DwarfUtils.h"
#include "ElfFile.h"


DwarfImageDebugInfo::DwarfImageDebugInfo(const ImageInfo& imageInfo,
	Architecture* architecture, DwarfFile* file)
	:
	fImageInfo(imageInfo),
	fArchitecture(architecture),
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
				|| subprogramEntry->Inline() == DW_INL_declared_inlined
				|| subprogramEntry->AbstractOrigin() != NULL) {
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
			const char* directory = NULL;
			const char* file = NULL;
			uint32 line = 0;
			uint32 column = 0;
			DwarfUtils::GetDeclarationLocation(fFile, subprogramEntry,
				directory, file, line, column);
			BString fileName;
			if (file != NULL) {
				if (directory != NULL)
					fileName << directory << '/' << file;
				else
					fileName << file;
			}
			// TODO: Avoid unnessecary string allocation! The source file name
			// is the same for all contained functions, so they could share the
			// string.

			// create and add the functions
			DwarfFunctionDebugInfo* function
				= new(std::nothrow) DwarfFunctionDebugInfo(this,
					subprogramEntry, rangeList, name, fileName,
					SourceLocation(line, column));
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
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
DwarfImageDebugInfo::LoadSourceCode(FunctionDebugInfo* function,
	SourceCode*& _sourceCode)
{
	// TODO: Load the actual source code!

	static const target_size_t kMaxBufferSize = 64 * 1024;
	target_size_t bufferSize = std::min(function->Size(), kMaxBufferSize);
	void* buffer = malloc(bufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	// read the function code
	target_addr_t functionOffset = function->Address() - fRelocationDelta
		- fTextSegment->LoadAddress() + fTextSegment->FileOffset();
	ssize_t bytesRead = pread(fFile->GetElfFile()->FD(), buffer, bufferSize,
		functionOffset);
	if (bytesRead < 0)
		return bytesRead;

	return fArchitecture->DisassembleCode(function, buffer, bytesRead,
		_sourceCode);
}


status_t
DwarfImageDebugInfo::GetStatement(FunctionDebugInfo* function,
	target_addr_t address, Statement*& _statement)
{
	// TODO:...
	return fArchitecture->GetStatement(function, address, _statement);
}
