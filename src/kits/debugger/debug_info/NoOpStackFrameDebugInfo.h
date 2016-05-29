/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NO_OP_STACK_FRAME_DEBUG_INFO_H
#define NO_OP_STACK_FRAME_DEBUG_INFO_H


#include "StackFrameDebugInfo.h"


class NoOpStackFrameDebugInfo : public StackFrameDebugInfo {
public:
								NoOpStackFrameDebugInfo();
	virtual						~NoOpStackFrameDebugInfo();

};


#endif	// NO_OP_STACK_FRAME_DEBUG_INFO_H
