/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "StackFrameDebugInfo.h"

#include "Architecture.h"
#include "ValueLocation.h"


StackFrameDebugInfo::StackFrameDebugInfo(Architecture* architecture)
	:
	fArchitecture(architecture)
{
	fArchitecture->AcquireReference();
}


StackFrameDebugInfo::~StackFrameDebugInfo()
{
	fArchitecture->ReleaseReference();
}


status_t
StackFrameDebugInfo::ResolveObjectDataLocation(StackFrame* stackFrame,
	Type* type, target_addr_t objectAddress, ValueLocation*& _location)
{
	ValuePieceLocation piece;
	piece.SetToMemory(objectAddress);
	piece.SetSize(0);
		// We set the piece size to 0 as an indicator that the size has to be
		// set.
		// TODO: We could set the byte size from type, but that may not be
		// accurate. We may want to add bit offset and size to Type.

	ValueLocation location(fArchitecture->IsBigEndian());
	if (!location.AddPiece(piece))
		return B_NO_MEMORY;

	return ResolveObjectDataLocation(stackFrame, type, location, _location);
}
