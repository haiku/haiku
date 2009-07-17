/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfFunctionDebugInfo.h"

#include "DebugInfoEntries.h"
#include "DwarfImageDebugInfo.h"
#include "LocatableFile.h"
#include "TargetAddressRangeList.h"


DwarfFunctionDebugInfo::DwarfFunctionDebugInfo(
	DwarfImageDebugInfo* imageDebugInfo, CompilationUnit* compilationUnit,
	DIESubprogram* subprogramEntry, TargetAddressRangeList* addressRanges,
	const BString& name, LocatableFile* sourceFile,
	const SourceLocation& sourceLocation)
	:
	fImageDebugInfo(imageDebugInfo),
	fCompilationUnit(compilationUnit),
	fSubprogramEntry(subprogramEntry),
	fAddressRanges(addressRanges),
	fName(name),
	fSourceFile(sourceFile),
	fSourceLocation(sourceLocation)
{
	fImageDebugInfo->AcquireReference();
	fAddressRanges->AcquireReference();

	if (fSourceFile != NULL)
		fSourceFile->AcquireReference();
}


DwarfFunctionDebugInfo::~DwarfFunctionDebugInfo()
{
	if (fSourceFile != NULL)
		fSourceFile->ReleaseReference();

	fAddressRanges->ReleaseReference();
	fImageDebugInfo->ReleaseReference();
}


SpecificImageDebugInfo*
DwarfFunctionDebugInfo::GetSpecificImageDebugInfo() const
{
	return fImageDebugInfo;
}


target_addr_t
DwarfFunctionDebugInfo::Address() const
{
	return fAddressRanges->LowestAddress() + fImageDebugInfo->RelocationDelta();
}


target_size_t
DwarfFunctionDebugInfo::Size() const
{
	return fAddressRanges->CoveringRange().Size();
}


const BString&
DwarfFunctionDebugInfo::Name() const
{
	return fName;
}


const BString&
DwarfFunctionDebugInfo::PrettyName() const
{
	return fName;
}


LocatableFile*
DwarfFunctionDebugInfo::SourceFile() const
{
	return fSourceFile;
}


SourceLocation
DwarfFunctionDebugInfo::SourceStartLocation() const
{
	return fSourceLocation;
}


SourceLocation
DwarfFunctionDebugInfo::SourceEndLocation() const
{
	return fSourceLocation;
}
