/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_DEBUG_INFO_H
#define FUNCTION_DEBUG_INFO_H

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "SourceLocation.h"
#include "Types.h"


enum function_source_state {
	FUNCTION_SOURCE_NOT_LOADED,
	FUNCTION_SOURCE_LOADING,
	FUNCTION_SOURCE_LOADED,
	FUNCTION_SOURCE_UNAVAILABLE
};


class LocatableFile;
class SourceCode;
class SpecificImageDebugInfo;


class FunctionDebugInfo : public Referenceable {
public:
	class Listener;

public:
								FunctionDebugInfo();
	virtual						~FunctionDebugInfo();

	virtual	SpecificImageDebugInfo*	GetSpecificImageDebugInfo() const = 0;
	virtual	target_addr_t		Address() const = 0;
	virtual	target_size_t		Size() const = 0;
	virtual	const char*			Name() const = 0;
	virtual	const char*			PrettyName() const = 0;

	virtual	LocatableFile*		SourceFile() const = 0;
	virtual	SourceLocation		SourceStartLocation() const = 0;
	virtual	SourceLocation		SourceEndLocation() const = 0;

			// mutable attributes follow (locking required)
			SourceCode*			GetSourceCode() const	{ return fSourceCode; }
			function_source_state SourceCodeState() const
									{ return fSourceCodeState; }
			void				SetSourceCode(SourceCode* source,
									function_source_state state);

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			// mutable
			SourceCode*			fSourceCode;
			function_source_state fSourceCodeState;
			ListenerList		fListeners;
};


class FunctionDebugInfo::Listener : public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual						~Listener();

	virtual	void				FunctionSourceCodeChanged(
									FunctionDebugInfo* function);
									// called with lock held
};


#endif	// FUNCTION_DEBUG_INFO_H
