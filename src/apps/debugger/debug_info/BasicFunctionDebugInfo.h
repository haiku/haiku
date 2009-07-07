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
									SpecificImageDebugInfo* imageDebugInfo,
									target_addr_t address,
									target_size_t size,
									const BString& name,
									const BString& prettyName);
	virtual						~BasicFunctionDebugInfo();

	virtual	SpecificImageDebugInfo* GetSpecificImageDebugInfo() const;
	virtual	target_addr_t		Address() const;
	virtual	target_size_t		Size() const;
	virtual	const BString&		Name() const;
	virtual	const BString&		PrettyName() const;

	virtual	LocatableFile*		SourceFile() const;
	virtual	SourceLocation		SourceStartLocation() const;
	virtual	SourceLocation		SourceEndLocation() const;

private:
			SpecificImageDebugInfo* fImageDebugInfo;
			target_addr_t		fAddress;
			target_size_t		fSize;
			const BString		fName;
			const BString		fPrettyName;
};


#endif	// BASIC_FUNCTION_DEBUG_INFO_H
