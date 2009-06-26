/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BasicFunctionDebugInfo.h"

#include "DebugInfo.h"


BasicFunctionDebugInfo::BasicFunctionDebugInfo(DebugInfo* debugInfo,
	target_addr_t address, target_size_t size, const BString& name,
	const BString& prettyName)
	:
	fDebugInfo(debugInfo),
	fAddress(address),
	fSize(size),
	fName(name),
	fPrettyName(prettyName)
{
	fDebugInfo->AddReference();
}


BasicFunctionDebugInfo::~BasicFunctionDebugInfo()
{
	fDebugInfo->RemoveReference();
}


DebugInfo*
BasicFunctionDebugInfo::GetDebugInfo() const
{
	return fDebugInfo;
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


const char*
BasicFunctionDebugInfo::Name() const
{
	return fName.String();
}


const char*
BasicFunctionDebugInfo::PrettyName() const
{
	return fPrettyName.String();
}


const char*
BasicFunctionDebugInfo::SourceFileName() const
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
