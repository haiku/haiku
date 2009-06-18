/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_H
#define STACK_FRAME_H

#include <OS.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "ArchitectureTypes.h"


class CpuState;


class StackFrame : public Referenceable,
	public DoublyLinkedListLinkImpl<StackFrame> {
public:
	virtual						~StackFrame();

	virtual	CpuState*			GetCpuState() const = 0;

	virtual	target_addr_t		FrameAddress() const = 0;
	virtual	target_addr_t		ReturnAddress() const = 0;
	virtual	target_addr_t		PreviousFrameAddress() const = 0;
};


typedef DoublyLinkedList<StackFrame> StackFrameList;


#endif	// STACK_FRAME_H
