/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer 	<laplace@users.sourceforge.net>
 *		Fredrik Modéen		<fredrik@modeen.se>
 */

#ifndef _FILEREADRWITE_
#define _FILEREADRWITE_

#include <stdio.h>
#include <File.h>
#include <String.h>

class FileReadWrite {
	public:
													// -1 defult encoding
   					FileReadWrite(BFile *file, int32 sourceEncoding = -1);
   		bool 		Next(BString& string);
   		status_t 	Write(const BString& contents)const;
   		void 		SetEncoding(int32 sourceEncoding);
   		uint32 		GetEncoding();

	private:
   		BFile*		fFile;
   		int32  		fSourceEncoding;
   		char    	fBuffer[4096];
   		off_t   	fPositionInBuffer;
   		ssize_t 	fAmtRead;
};

#endif //_FILEREADRWITE_
