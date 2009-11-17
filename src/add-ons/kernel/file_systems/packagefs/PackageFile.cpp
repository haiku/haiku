/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageFile.h"


PackageFile::PackageFile(mode_t mode, const PackageData& data)
	:
	PackageLeafNode(mode),
	fData(data)
{
}


PackageFile::~PackageFile()
{
}
