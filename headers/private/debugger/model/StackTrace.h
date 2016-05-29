/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_TRACE_H
#define STACK_TRACE_H

#include <ObjectList.h>

#include "StackFrame.h"


class StackTrace : public BReferenceable {
public:
								StackTrace();
	virtual						~StackTrace();

			bool				AddFrame(StackFrame* frame);
									// takes over reference (also on error)

			int32				CountFrames() const;
			StackFrame*			FrameAt(int32 index) const;

private:
			typedef BObjectList<StackFrame> StackFrameList;

private:
			StackFrameList		fStackFrames;
};


#endif	// STACK_TRACE_H
