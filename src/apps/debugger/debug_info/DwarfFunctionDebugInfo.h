/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_FUNCTION_DEBUG_INFO_H
#define DWARF_FUNCTION_DEBUG_INFO_H

#include <String.h>

#include "FunctionDebugInfo.h"


class DIESubprogram;
class DwarfImageDebugInfo;
class TargetAddressRangeList;


class DwarfFunctionDebugInfo : public FunctionDebugInfo {
public:
								DwarfFunctionDebugInfo(
									DwarfImageDebugInfo* imageDebugInfo,
									DIESubprogram* subprogramEntry,
									TargetAddressRangeList* addressRanges,
									const BString& name);
	virtual						~DwarfFunctionDebugInfo();

	virtual	SpecificImageDebugInfo* GetSpecificImageDebugInfo() const;
	virtual	target_addr_t		Address() const;
	virtual	target_size_t		Size() const;
	virtual	const char*			Name() const;
	virtual	const char*			PrettyName() const;

	virtual	const char*			SourceFileName() const;
	virtual	SourceLocation		SourceStartLocation() const;
	virtual	SourceLocation		SourceEndLocation() const;

private:
			DwarfImageDebugInfo* fImageDebugInfo;
			TargetAddressRangeList* fAddressRanges;
			const BString		fName;
};


#endif	// DWARF_FUNCTION_DEBUG_INFO_H
