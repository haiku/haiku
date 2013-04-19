/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
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


Package::Package()
	:
	fNodeRef(),
	fFileName(),
	fInfo(),
	fActive(false),
	fFileNameHashTableNext(NULL),
	fNodeRefHashTableNext(NULL),
	fIgnoreEntryCreated(0),
	fIgnoreEntryRemoved(0)
{
}


Package::~Package()
{
}


status_t
Package::Init(const entry_ref& entryRef)
{
	// init the file name
	fFileName = entryRef.name;
	if (fFileName.IsEmpty())
		RETURN_ERROR(B_NO_MEMORY);

	// open the file and get the node_ref
	BFile file;
	status_t error = file.SetTo(&entryRef, B_READ_ONLY);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = file.GetNodeRef(&fNodeRef);
	if (error != B_OK)
		RETURN_ERROR(error);

	// get the package info
	int fd = file.Dup();
	if (fd < 0)
		RETURN_ERROR(error);
	FileDescriptorCloser fdCloser(fd);

	error = fInfo.ReadFromPackageFile(fd);
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}
