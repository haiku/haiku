/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "NoOpStackFrameDebugInfo.h"


NoOpStackFrameDebugInfo::NoOpStackFrameDebugInfo(Architecture* architecture)
	:
	StackFrameDebugInfo(architecture)
{
}


NoOpStackFrameDebugInfo::~NoOpStackFrameDebugInfo()
{
}


status_t
NoOpStackFrameDebugInfo::ResolveObjectDataLocation(StackFrame* stackFrame,
	Type* type, const ValueLocation& objectLocation, ValueLocation*& _location)
{
	return B_UNSUPPORTED;
}


status_t
NoOpStackFrameDebugInfo::ResolveBaseTypeLocation(StackFrame* stackFrame,
	Type* type, BaseType* baseType, const ValueLocation& parentLocation,
	ValueLocation*& _location)
{
	return B_UNSUPPORTED;
}


status_t
NoOpStackFrameDebugInfo::ResolveDataMemberLocation(StackFrame* stackFrame,
	Type* type, DataMember* member, const ValueLocation& parentLocation,
	ValueLocation*& _location)
{
	return B_UNSUPPORTED;
}
