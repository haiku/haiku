/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_TRACE_H
#define STACK_TRACE_H

#include "StackFrame.h"


class StackTrace : public Referenceable {
public:
								StackTrace();
	virtual						~StackTrace();

			void				AddFrame(StackFrame* frame);
									// takes over reference

			const StackFrameList& Frames() const	{ return fStackFrames; }

			StackFrame*			TopFrame() const
									{ return fStackFrames.Head(); }
			StackFrame*			BottomFrame() const
									{ return fStackFrames.Tail(); }

private:
			StackFrameList		fStackFrames;
};


#endif	// STACK_TRACE_H
