/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BasicFunctionDebugInfo.h"

#include "SpecificImageDebugInfo.h"


BasicFunctionDebugInfo::BasicFunctionDebugInfo(
	SpecificImageDebugInfo* debugInfo, target_addr_t address,
	target_size_t size, const BString& name, const BString& prettyName)
	:
	fImageDebugInfo(debugInfo),
	fAddress(address),
	fSize(size),
	fName(name),
	fPrettyName(prettyName)
{
	fImageDebugInfo->AcquireReference();
}


BasicFunctionDebugInfo::~BasicFunctionDebugInfo()
{
	fImageDebugInfo->ReleaseReference();
}


SpecificImageDebugInfo*
BasicFunctionDebugInfo::GetSpecificImageDebugInfo() const
{
	return fImageDebugInfo;
}


target_addr_t
BasicFunctionDebugInfo::Address() const
{
	return fAddress;
}


target_size_t
BasicFunctionDebugInfo::Size() const
{
	return fSize;
}


const BString&
BasicFunctionDebugInfo::Name() const
{
	return fName;
}


const BString&
BasicFunctionDebugInfo::PrettyName() const
{
	return fPrettyName;
}


LocatableFile*
BasicFunctionDebugInfo::SourceFile() const
{
	return NULL;
}


SourceLocation
BasicFunctionDebugInfo::SourceStartLocation() const
{
	return SourceLocation();
}


SourceLocation
BasicFunctionDebugInfo::SourceEndLocation() const
{
	return SourceLocation();
}
