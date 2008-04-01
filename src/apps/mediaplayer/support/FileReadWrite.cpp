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

FileReadWrite::FileReadWrite(BFile *file, int32 sourceEncoding)
	: fFile(file),
	fSourceEncoding(sourceEncoding)
{}


void
FileReadWrite::SetEncoding(int32 sourceEncoding)
{
	fSourceEncoding = sourceEncoding;
}


uint32 
FileReadWrite::GetEncoding()
{
	return fSourceEncoding;
}


status_t 
FileReadWrite::Write(const BString& contents)const
{
	ssize_t sz = fFile->Write(contents.String(), contents.Length());
	if (sz != contents.Length())
		return sz < 0 ? sz : B_IO_ERROR;
	else
		return B_OK;
}


bool
FileReadWrite::Next(BString& string)
{
	// Fill up the buffer with the first chunk of code
	if (fPositionInBuffer == 0)
		fAmtRead = fFile->Read(&fBuffer, sizeof(fBuffer));
	while (fAmtRead > 0) {
		while (fPositionInBuffer < fAmtRead) {
			// Return true if we hit a newline or the end of the file
			if (fBuffer[fPositionInBuffer] == '\n') {
				fPositionInBuffer++;
				//Convert begin
				int32 state = 0;
				int32 bufferLen = string.Length();
				int32 destBufferLen = bufferLen;
				char destination[destBufferLen];
				if (fSourceEncoding)
					convert_to_utf8(fSourceEncoding, string.String(), &bufferLen, destination, &destBufferLen, &state);
				string = destination;
				return true;
			}
			string += fBuffer[fPositionInBuffer]; 
			fPositionInBuffer++;
		} 

		// Once the buffer runs out, grab some more and start again
		fAmtRead = fFile->Read(&fBuffer, sizeof(fBuffer)); 
		fPositionInBuffer = 0;
	}	
	return false;
}
