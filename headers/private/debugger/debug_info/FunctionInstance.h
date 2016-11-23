/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_INSTANCE_H
#define FUNCTION_INSTANCE_H

#include <util/DoublyLinkedList.h>

#include "FunctionDebugInfo.h"


enum function_source_state {
	FUNCTION_SOURCE_NOT_LOADED,
	FUNCTION_SOURCE_LOADING,
	FUNCTION_SOURCE_LOADED,
	FUNCTION_SOURCE_UNAVAILABLE,
	FUNCTION_SOURCE_SUPPRESSED
};


class DisassembledCode;
class Function;
class FunctionDebugInfo;
class FunctionID;
class ImageDebugInfo;


class FunctionInstance : public BReferenceable,
	public DoublyLinkedListLinkImpl<FunctionInstance> {
public:
								FunctionInstance(ImageDebugInfo* imageDebugInfo,
									FunctionDebugInfo* functionDebugInfo);
								~FunctionInstance();

			ImageDebugInfo*		GetImageDebugInfo() const
									{ return fImageDebugInfo; }
			Function*			GetFunction() const
									{ return fFunction; }
			FunctionDebugInfo*	GetFunctionDebugInfo() const
									{ return fFunctionDebugInfo; }

			target_addr_t		Address() const
									{ return fFunctionDebugInfo->Address(); }
			target_size_t		Size() const
									{ return fFunctionDebugInfo->Size(); }
			const BString&		Name() const
									{ return fFunctionDebugInfo->Name(); }
			const BString&		PrettyName() const
									{ return fFunctionDebugInfo->PrettyName(); }
			LocatableFile*		SourceFile() const
									{ return fFunctionDebugInfo->SourceFile(); }
			SourceLocation		GetSourceLocation() const
									{ return fFunctionDebugInfo
										->SourceStartLocation(); }

			FunctionID*			GetFunctionID() const;
									// returns a reference

			void				SetFunction(Function* function);
									// package private

			// mutable attributes follow (locking required)
			DisassembledCode*	GetSourceCode() const
									{ return fSourceCode; }
			function_source_state SourceCodeState() const
									{ return fSourceCodeState; }
			void				SetSourceCode(DisassembledCode* source,
									function_source_state state);

private:
			ImageDebugInfo*		fImageDebugInfo;
			Function*			fFunction;
			FunctionDebugInfo*	fFunctionDebugInfo;
			DisassembledCode*	fSourceCode;
			function_source_state fSourceCodeState;
};


typedef DoublyLinkedList<FunctionInstance> FunctionInstanceList;


#endif	// FUNCTION_INSTANCE_H
