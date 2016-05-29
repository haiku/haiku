/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_CODE_H
#define SOURCE_CODE_H


#include <Referenceable.h>

#include "LineDataSource.h"
#include "TargetAddressRange.h"


class LocatableFile;
class SourceLanguage;
class SourceLocation;
class Statement;


class SourceCode : public LineDataSource {
public:
	virtual						~SourceCode();

	// Locking needed for GetStatementLocationRange(), since that might use
	// mutable data.
	virtual	bool				Lock() = 0;
	virtual	void				Unlock() = 0;

	virtual	SourceLanguage*		GetSourceLanguage() const = 0;

	virtual	bool				GetStatementLocationRange(
									const SourceLocation& location,
									SourceLocation& _start,
									SourceLocation& _end) const = 0;

	virtual	LocatableFile*		GetSourceFile() const = 0;
};


#endif	// SOURCE_CODE_H
