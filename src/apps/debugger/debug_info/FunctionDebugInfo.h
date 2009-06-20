/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_DEBUG_INFO_H
#define FUNCTION_DEBUG_INFO_H

#include <Referenceable.h>

#include "ArchitectureTypes.h"


class DebugInfo;


class FunctionDebugInfo : public Referenceable {
public:
	virtual						~FunctionDebugInfo();

	virtual	DebugInfo*			GetDebugInfo() const = 0;
	virtual	target_addr_t		Address() const = 0;
	virtual	target_size_t		Size() const = 0;
	virtual	const char*			Name() const = 0;
	virtual	const char*			PrettyName() const = 0;
};


#endif	// FUNCTION_DEBUG_INFO_H
