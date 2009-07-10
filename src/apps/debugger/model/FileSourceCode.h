/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_SOURCE_CODE_H
#define FILE_SOURCE_CODE_H


#include "Array.h"
#include "SourceCode.h"


class LocatableFile;
class SourceFile;


class FileSourceCode : public SourceCode {
public:
								FileSourceCode(LocatableFile* file,
									SourceFile* sourceFile);
	virtual						~FileSourceCode();

			status_t			Init();
			status_t			AddSourceLocation(
									const SourceLocation& location);

	virtual	int32				CountLines() const;
	virtual	const char*			LineAt(int32 index) const;

	virtual	bool				GetStatementLocationRange(
									const SourceLocation& location,
									SourceLocation& _start,
									SourceLocation& _end) const;

	virtual	LocatableFile*		GetSourceFile() const;

	virtual	status_t			GetStatementAtLocation(
									const SourceLocation& location,
									Statement*& _statement);

private:
			int32				_FindSourceLocationIndex(
									const SourceLocation& location,
									bool& _foundMatch) const;

private:
			LocatableFile*		fFile;
			SourceFile*			fSourceFile;
			Array<SourceLocation> fSourceLocations;
};


#endif	// FILE_BASED_SOURCE_CODE_H
