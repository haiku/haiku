/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include <Referenceable.h>


class SourceFile;


class SourceFileOwner {
public:
	virtual						~SourceFileOwner();

	virtual	void				SourceFileUnused(SourceFile* sourceFile) = 0;
	virtual	void				SourceFileDeleted(SourceFile* sourceFile) = 0;
};



class SourceFile : public BReferenceable {
public:
								SourceFile(SourceFileOwner* owner);
								~SourceFile();

			status_t			Init(const char* path);

			int32				CountLines() const;
			const char*			LineAt(int32 index) const;
			int32				LineLengthAt(int32 index) const;

protected:
	virtual	void				LastReferenceReleased();

private:
			SourceFileOwner*	fOwner;
			char*				fFileContent;
			int32*				fLineOffsets;
			int32				fLineCount;
};



#endif	// SOURCE_FILE_H
