/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "CharacterClasses.h"
#include "Scanner.h"

#include <stdio.h>

int FileBuffer::Read()
{
	if (fIndex >= fSize) {
		fSize = fFile->Read(fBuffer, kReadBufferSize);
		fIndex = 0;
	}
	if (fSize <= 0) {
		return -1;
	}
	return (int)fBuffer[fIndex ++];
}

PPDFile::PPDFile(const char* file, PPDFile* previousFile)
	: fFileName(file)
	, fFile(file, B_READ_ONLY)
	, fPreviousFile(previousFile)
	, fCurrentPosition(0, 1)
	, fCurrentChar(-1)
	, fBuffer(&fFile)
{
}

PPDFile::~PPDFile()
{
	// nothing to do
}

status_t PPDFile::InitCheck()
{
	return fFile.InitCheck();
}

int PPDFile::GetCurrentChar()
{
	return fCurrentChar;
}

void PPDFile::NextChar() {
	fCurrentChar = fBuffer.Read();
	if (fCurrentChar != -1) {
#if TRACE_SCANNER
		fprintf(stderr, "%c ", fCurrentChar);
#endif
		if (fCurrentChar == kCr) {
			fCurrentPosition.x = 0;
			fCurrentPosition.y ++;
		} else {
			fCurrentPosition.x ++;
		}
	}
}

Position PPDFile::GetPosition()
{
	return fCurrentPosition;
}

PPDFile* PPDFile::GetPreviousFile()
{
	return fPreviousFile;
}

const char* PPDFile::GetFileName()
{
	return fFileName.String();
}
