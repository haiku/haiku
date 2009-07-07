/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_INSTANCE_H
#define FUNCTION_INSTANCE_H

#include <util/DoublyLinkedList.h>

#include "FunctionDebugInfo.h"


class Function;
class FunctionDebugInfo;
class ImageDebugInfo;


class FunctionInstance : public Referenceable,
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

			void				SetFunction(Function* function);
									// package private
private:
			ImageDebugInfo*		fImageDebugInfo;
			Function*			fFunction;
			FunctionDebugInfo*	fFunctionDebugInfo;
};


typedef DoublyLinkedList<FunctionInstance> FunctionInstanceList;


#endif	// FUNCTION_INSTANCE_H
