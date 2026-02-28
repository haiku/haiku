/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer 	<laplace@users.sourceforge.net>
 *		Fredrik Modéen		<fredrik@modeen.se>
 */
#include "FileReadWrite.h"

#include <UTF8.h>
#include <Path.h>


FileReadWrite::FileReadWrite(BFile* file, int32 sourceEncoding)
	: fFile(file),
	  fSourceEncoding(sourceEncoding),
	  fPositionInBuffer(0),
	  fAmtRead(0)
{
}


void
FileReadWrite::SetEncoding(int32 sourceEncoding)
{
	fSourceEncoding = sourceEncoding;
}


uint32 
FileReadWrite::GetEncoding() const
{
	return fSourceEncoding;
}


status_t 
FileReadWrite::Write(const BString& contents) const
{
	ssize_t written = fFile->Write(contents.String(), contents.Length());
	if (written != contents.Length())
		return written < 0 ? (status_t)written : B_IO_ERROR;

	return B_OK;
}


// returns true when '\n' found and false otherwise
bool
FileReadWrite::Next(BString& string)
{
	// Fill up the buffer with the first chunk of data
	if (fPositionInBuffer == 0)
		fAmtRead = fFile->Read(&fBuffer, sizeof(fBuffer));
	while (fAmtRead > 0) {
		// TODO: If the source file is expected to have different encoding,
		// don't we need to check for the line endings in that encoding?!

		int32 remainingLength = fAmtRead - fPositionInBuffer;
		char* currentPosition = fBuffer + fPositionInBuffer;
		char* endLinePosition = (char*)memchr(currentPosition, '\n', remainingLength);

		if (endLinePosition != NULL) {
			int32 appendLength = endLinePosition - currentPosition;
			string.Append(currentPosition, appendLength);

			if (fSourceEncoding != -1) {
				int32 state = 0;
				int32 bufferLen = string.Length();
				int32 destBufferLen = bufferLen;
				char destination[destBufferLen];
				convert_to_utf8(fSourceEncoding, string.String(), &bufferLen, destination,
					&destBufferLen, &state);
				string = destination;
			}

			fPositionInBuffer += (appendLength + 1) % fAmtRead;

			return true;
		} else
			string.Append(currentPosition, remainingLength);

		// Once the buffer runs out, grab some more and start again
		fAmtRead = fFile->Read(&fBuffer, sizeof(fBuffer));
		fPositionInBuffer = 0;
	}
	return false;
}
