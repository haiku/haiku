/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_DEBUG_INFO_H
#define FUNCTION_DEBUG_INFO_H

#include <Referenceable.h>

#include "SourceLocation.h"
#include "Types.h"


class BString;
class LocatableFile;
class SpecificImageDebugInfo;


class FunctionDebugInfo : public BReferenceable {
public:
								FunctionDebugInfo();
	virtual						~FunctionDebugInfo();

	virtual	SpecificImageDebugInfo*	GetSpecificImageDebugInfo() const = 0;
	virtual	target_addr_t		Address() const = 0;
	virtual	target_size_t		Size() const = 0;
	virtual	const BString&		Name() const = 0;
	virtual	const BString&		PrettyName() const = 0;

	virtual	LocatableFile*		SourceFile() const = 0;
	virtual	SourceLocation		SourceStartLocation() const = 0;
	virtual	SourceLocation		SourceEndLocation() const = 0;
};


#endif	// FUNCTION_DEBUG_INFO_H
