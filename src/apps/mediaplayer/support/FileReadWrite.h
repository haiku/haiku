/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer 	<laplace@users.sourceforge.net>
 *		Fredrik Modéen		<fredrik@modeen.se>
 */
#ifndef FILE_READ_WRITE_H
#define FILE_READ_WRITE_H

#include <stdio.h>
#include <File.h>
#include <String.h>


class FileReadWrite {
public:
								FileReadWrite(BFile* file,
									int32 sourceEncoding = -1);
									// -1 means "default" encoding
			bool				Next(BString& string);
			status_t			Write(const BString& contents) const;
			void				SetEncoding(int32 sourceEncoding);
			uint32				GetEncoding() const;

private:
			BFile*				fFile;
			int32				fSourceEncoding;
			char				fBuffer[4096];
			off_t				fPositionInBuffer;
			ssize_t				fAmtRead;
};

#endif //FILE_READ_WRITE_H
