/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfImageDebugInfo.h"

#include <stdio.h>

#include <new>

#include <AutoDeleter.h>

#include "Architecture.h"
#include "CompilationUnit.h"
#include "DebugInfoEntries.h"
#include "Dwarf.h"
#include "DwarfFile.h"


DwarfImageDebugInfo::DwarfImageDebugInfo(const ImageInfo& imageInfo,
	Architecture* architecture, DwarfFile* file)
	:
	fImageInfo(imageInfo),
	fArchitecture(architecture),
	fFile(file)
{
}


DwarfImageDebugInfo::~DwarfImageDebugInfo()
{
}


status_t
DwarfImageDebugInfo::Init()
{
	return B_OK;
}


status_t
DwarfImageDebugInfo::GetFunctions(BObjectList<FunctionDebugInfo>& functions)
{
printf("DwarfImageDebugInfo::GetFunctions()\n");
printf("  %ld compilation units\n", fFile->CountCompilationUnits());

	for (int32 i = 0; CompilationUnit* unit = fFile->CompilationUnitAt(i);
			i++) {
		printf("  %s:\n", unit->UnitEntry()->Name());

		for (DebugInfoEntryList::ConstIterator it
					= unit->UnitEntry()->OtherChildren().GetIterator();
				DebugInfoEntry* entry = it.Next();) {
			if (entry->Tag() != DW_TAG_subprogram)
				continue;

			DIESubprogram* subprogramEntry = static_cast<DIESubprogram*>(entry);
			printf("    subprogram entry: %p, name: %s, declaration: %d\n",
				subprogramEntry, subprogramEntry->Name(),
				subprogramEntry->IsDeclaration());
		}
	}

	// TODO:...
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
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
DwarfImageDebugInfo::GetStatement(FunctionDebugInfo* function,
	target_addr_t address, Statement*& _statement)
{
	// TODO:...
	return fArchitecture->GetStatement(function, address, _statement);
}
