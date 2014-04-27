/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Package.h"

#include <fcntl.h>

#include <File.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"


Package::Package(PackageFile* file)
	:
	fFile(file),
	fActive(false),
	fFileNameHashTableNext(NULL),
	fNodeRefHashTableNext(NULL)
{
	fFile->AcquireReference();
}


Package::~Package()
{
	fFile->ReleaseReference();
}


Package*
Package::Clone() const
{
	Package* clone = new(std::nothrow) Package(fFile);
	if (clone != NULL)
		clone->fActive = fActive;
	return clone;
}
