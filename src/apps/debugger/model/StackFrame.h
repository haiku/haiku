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


enum stack_frame_type {
	STACK_FRAME_TYPE_TOP,			// top-most frame
	STACK_FRAME_TYPE_STANDARD,		// non-top-most standard frame
	STACK_FRAME_TYPE_SIGNAL,		// signal handler frame
	STACK_FRAME_TYPE_FRAMELESS		// dummy frame for a frameless function
};


enum stack_frame_source_state {
	STACK_SOURCE_NOT_LOADED,
	STACK_SOURCE_LOADING,
	STACK_SOURCE_LOADED,
	STACK_SOURCE_UNAVAILABLE
};


class CpuState;
class Image;
class FunctionDebugInfo;
class SourceCode;


class StackFrame : public Referenceable {
public:
	class Listener;

public:
								StackFrame(stack_frame_type type,
									CpuState* cpuState,
									target_addr_t frameAddress);
								~StackFrame();

			stack_frame_type	Type() const			{ return fType; }
			CpuState*			GetCpuState() const		{ return fCpuState; }
			target_addr_t		InstructionPointer() const;
			target_addr_t		FrameAddress() const { return fFrameAddress; }

			target_addr_t		ReturnAddress() const { return fReturnAddress; }
			void				SetReturnAddress(target_addr_t address);

			Image*				GetImage() const		{ return fImage; }
			void				SetImage(Image* image);

			FunctionDebugInfo*	Function() const		{ return fFunction; }
			void				SetFunction(FunctionDebugInfo* function);

			// mutable attributes follow (locking required)
			SourceCode*			GetSourceCode() const	{ return fSourceCode; }
			stack_frame_source_state SourceCodeState() const
										{ return fSourceCodeState; }
			void				SetSourceCode(SourceCode* source,
									stack_frame_source_state state);

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			stack_frame_type	fType;
			CpuState*			fCpuState;
			target_addr_t		fFrameAddress;
			target_addr_t		fReturnAddress;
			Image*				fImage;
			FunctionDebugInfo*	fFunction;
			// mutable
			SourceCode*			fSourceCode;
			stack_frame_source_state fSourceCodeState;
			ListenerList		fListeners;
};


class StackFrame::Listener : public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual						~Listener();

	virtual	void				StackFrameSourceCodeChanged(StackFrame* frame);
									// called with lock held
};


#endif	// STACK_FRAME_H
