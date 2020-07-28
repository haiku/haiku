/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TarArchiveHeader.h"


TarArchiveHeader::TarArchiveHeader()
	:
	fFileName(),
	fLength(0),
	fFileType(TAR_FILE_TYPE_NORMAL)
{
}


TarArchiveHeader::~TarArchiveHeader()
{
}


const BString&
TarArchiveHeader::FileName() const
{
	return fFileName;
}

size_t
TarArchiveHeader::Length() const
{
	return fLength;
}


tar_file_type
TarArchiveHeader::FileType() const
{
	return fFileType;
}


void
TarArchiveHeader::SetFileName(const BString& value)
{
	fFileName = value;
}


void
TarArchiveHeader::SetLength(size_t value)
{
	fLength = value;
}


void
TarArchiveHeader::SetFileType(tar_file_type value)
{
	fFileType = value;
}