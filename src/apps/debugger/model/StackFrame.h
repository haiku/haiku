/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_H
#define STACK_FRAME_H


#include <OS.h>

#include <ObjectList.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "Types.h"


enum stack_frame_type {
	STACK_FRAME_TYPE_SYSCALL,		// syscall frame
	STACK_FRAME_TYPE_STANDARD,		// standard frame
	STACK_FRAME_TYPE_SIGNAL,		// signal handler frame
	STACK_FRAME_TYPE_FRAMELESS		// dummy frame for a frameless function
};


class CpuState;
class Image;
class FunctionInstance;
class StackFrameDebugInfo;
class StackFrameValueInfos;
class StackFrameValues;
class TypeComponentPath;
class Variable;


class StackFrame : public BReferenceable {
public:
	class Listener;

public:
								StackFrame(stack_frame_type type,
									CpuState* cpuState,
									target_addr_t frameAddress,
									target_addr_t instructionPointer,
									StackFrameDebugInfo* debugInfo);
								~StackFrame();

			status_t			Init();

			stack_frame_type	Type() const			{ return fType; }
			CpuState*			GetCpuState() const		{ return fCpuState; }
			target_addr_t		FrameAddress() const { return fFrameAddress; }
			StackFrameDebugInfo* DebugInfo() const		{ return fDebugInfo; }

			target_addr_t		InstructionPointer() const
									{ return fInstructionPointer; }

			CpuState*			PreviousCpuState() const
									{ return fPreviousCpuState; }
			void				SetPreviousCpuState(CpuState* state);

			target_addr_t		ReturnAddress() const { return fReturnAddress; }
			void				SetReturnAddress(target_addr_t address);

			Image*				GetImage() const		{ return fImage; }
			void				SetImage(Image* image);

			FunctionInstance*	Function() const		{ return fFunction; }
			void				SetFunction(FunctionInstance* function);

			int32				CountParameters() const;
			Variable*			ParameterAt(int32 index) const;
			bool				AddParameter(Variable* parameter);

			int32				CountLocalVariables() const;
			Variable*			LocalVariableAt(int32 index) const;
			bool				AddLocalVariable(Variable* variable);

			StackFrameValues*	Values() const			{ return fValues; }
			StackFrameValueInfos* ValueInfos() const	{ return fValueInfos; }

			// team lock must be held
			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			void				NotifyValueRetrieved(Variable* variable,
									TypeComponentPath* path);

private:
			typedef BObjectList<Variable> VariableList;
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			stack_frame_type	fType;
			CpuState*			fCpuState;
			CpuState*			fPreviousCpuState;
			target_addr_t		fFrameAddress;
			target_addr_t		fInstructionPointer;
			target_addr_t		fReturnAddress;
			StackFrameDebugInfo* fDebugInfo;
			Image*				fImage;
			FunctionInstance*	fFunction;
			VariableList		fParameters;
			VariableList		fLocalVariables;
			StackFrameValues*	fValues;
			StackFrameValueInfos* fValueInfos;
			ListenerList		fListeners;
};


class StackFrame::Listener : public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual						~Listener();

	virtual	void				StackFrameValueRetrieved(StackFrame* stackFrame,
									Variable* variable,
									TypeComponentPath* path);
									// called with lock held
};


#endif	// STACK_FRAME_H
