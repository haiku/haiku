/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_CODE_H
#define SOURCE_CODE_H


#include <Referenceable.h>

#include "TargetAddressRange.h"


class LocatableFile;
class SourceLocation;
class Statement;


class SourceCode : public Referenceable {
public:
	virtual						~SourceCode();

	virtual	int32				CountLines() const = 0;
	virtual	const char*			LineAt(int32 index) const = 0;

	virtual	bool				GetStatementLocationRange(
									const SourceLocation& location,
									SourceLocation& _start,
									SourceLocation& _end) const = 0;

	virtual	LocatableFile*		GetSourceFile() const = 0;

	virtual	status_t			GetStatementAtLocation(
									const SourceLocation& location,
									Statement*& _statement) = 0;
									// returns a reference,
									// may return B_UNSUPPORTED, when
									// SourceFile() returns non-NULL
};


#endif	// SOURCE_CODE_H
