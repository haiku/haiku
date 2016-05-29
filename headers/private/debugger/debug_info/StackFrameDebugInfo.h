/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_DEBUG_INFO_H
#define STACK_FRAME_DEBUG_INFO_H


#include <Referenceable.h>

#include "Types.h"


class ArrayIndexPath;
class ArrayType;
class Architecture;
class BaseType;
class DataMember;
class StackFrame;
class Type;
class ValueLocation;


class StackFrameDebugInfo : public BReferenceable {
public:
								StackFrameDebugInfo();
	virtual						~StackFrameDebugInfo();
};


#endif	// STACK_FRAME_DEBUG_INFO_H
