/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StackTrace.h"


StackTrace::StackTrace()
{
}


StackTrace::~StackTrace()
{
	for (int32 i = 0; StackFrame* frame = FrameAt(i); i++)
		frame->ReleaseReference();
}


bool
StackTrace::AddFrame(StackFrame* frame)
{
	if (fStackFrames.AddItem(frame))
		return true;

	frame->ReleaseReference();
	return false;
}


int32
StackTrace::CountFrames() const
{
	return fStackFrames.CountItems();
}


StackFrame*
StackTrace::FrameAt(int32 index) const
{
	return fStackFrames.ItemAt(index);
}
