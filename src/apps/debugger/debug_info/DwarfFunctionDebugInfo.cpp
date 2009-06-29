/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfFunctionDebugInfo.h"

#include "DebugInfoEntries.h"
#include "DwarfImageDebugInfo.h"
#include "TargetAddressRangeList.h"


DwarfFunctionDebugInfo::DwarfFunctionDebugInfo(
	DwarfImageDebugInfo* imageDebugInfo, DIESubprogram* subprogramEntry,
	TargetAddressRangeList* addressRanges, const BString& name)
	:
	fImageDebugInfo(imageDebugInfo),
	fAddressRanges(addressRanges),
	fName(name)
{
	fImageDebugInfo->AddReference();
	fAddressRanges->AddReference();
}


DwarfFunctionDebugInfo::~DwarfFunctionDebugInfo()
{
	fAddressRanges->RemoveReference();
	fImageDebugInfo->RemoveReference();
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


const char*
DwarfFunctionDebugInfo::Name() const
{
	return fName.String();
}


const char*
DwarfFunctionDebugInfo::PrettyName() const
{
	return fName.String();
}


const char*
DwarfFunctionDebugInfo::SourceFileName() const
{
	return NULL;
}


SourceLocation
DwarfFunctionDebugInfo::SourceStartLocation() const
{
	return SourceLocation();
}


SourceLocation
DwarfFunctionDebugInfo::SourceEndLocation() const
{
	return SourceLocation();
}
