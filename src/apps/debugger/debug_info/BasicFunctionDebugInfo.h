/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASIC_FUNCTION_DEBUG_INFO_H
#define BASIC_FUNCTION_DEBUG_INFO_H

#include <String.h>

#include "FunctionDebugInfo.h"


class BasicFunctionDebugInfo : public FunctionDebugInfo {
public:
								BasicFunctionDebugInfo(
									DebugInfo* debugInfo,
									target_addr_t address,
									target_size_t size,
									const BString& name,
									const BString& prettyName);
	virtual						~BasicFunctionDebugInfo();

	virtual	DebugInfo*			GetDebugInfo() const;
	virtual	target_addr_t		Address() const;
	virtual	target_size_t		Size() const;
	virtual	const char*			Name() const;
	virtual	const char*			PrettyName() const;

private:
			DebugInfo*			fDebugInfo;
			target_addr_t		fAddress;
			target_size_t		fSize;
			const BString		fName;
			const BString		fPrettyName;
};


#endif	// BASIC_FUNCTION_DEBUG_INFO_H
