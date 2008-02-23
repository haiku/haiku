/*
 * Copyright 2008, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "IStreamWrapper.h"

IStreamWrapper::IStreamWrapper(const char *filename, BPositionIO *stream)
	: IStream(filename),
	fStream(stream, 2048)
{
}


IStreamWrapper::~IStreamWrapper()
{
}


bool
IStreamWrapper::read(char c[/*n*/], int n)
{
	int actual = fStream.Read(c, n);
	if (actual < B_OK) {
		
	}
	return (actual == n);
}


Int64
IStreamWrapper::tellg()
{
	return fStream.Position();
}


void
IStreamWrapper::seekg(Int64 pos)
{
	fStream.Seek(pos, SEEK_SET);
}
