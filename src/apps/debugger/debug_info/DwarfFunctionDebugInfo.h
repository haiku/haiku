/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_FUNCTION_DEBUG_INFO_H
#define DWARF_FUNCTION_DEBUG_INFO_H

#include <String.h>

#include "FunctionDebugInfo.h"
#include "SourceLocation.h"


class CompilationUnit;
class DIESubprogram;
class DwarfImageDebugInfo;
class TargetAddressRangeList;


class DwarfFunctionDebugInfo : public FunctionDebugInfo {
public:
								DwarfFunctionDebugInfo(
									DwarfImageDebugInfo* imageDebugInfo,
									CompilationUnit* compilationUnit,
									DIESubprogram* subprogramEntry,
									TargetAddressRangeList* addressRanges,
									const BString& name,
									LocatableFile* sourceFile,
									const SourceLocation& sourceLocation);
	virtual						~DwarfFunctionDebugInfo();

	virtual	SpecificImageDebugInfo* GetSpecificImageDebugInfo() const;
	virtual	target_addr_t		Address() const;
	virtual	target_size_t		Size() const;
	virtual	const BString&		Name() const;
	virtual	const BString&		PrettyName() const;

	virtual	LocatableFile*		SourceFile() const;
	virtual	SourceLocation		SourceStartLocation() const;
	virtual	SourceLocation		SourceEndLocation() const;

			CompilationUnit*	GetCompilationUnit() const
									{ return fCompilationUnit; }
			DIESubprogram*		SubprogramEntry() const
									{ return fSubprogramEntry; }

private:
			DwarfImageDebugInfo* fImageDebugInfo;
			CompilationUnit*	fCompilationUnit;
			DIESubprogram*		fSubprogramEntry;
			TargetAddressRangeList* fAddressRanges;
			BString				fName;
			LocatableFile*		fSourceFile;
			SourceLocation		fSourceLocation;
};


#endif	// DWARF_FUNCTION_DEBUG_INFO_H
