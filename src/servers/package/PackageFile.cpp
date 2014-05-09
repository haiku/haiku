/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "PackageFile.h"

#include <fcntl.h>

#include <File.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"
#include "PackageFileManager.h"


PackageFile::PackageFile()
	:
	fNodeRef(),
	fDirectoryRef(),
	fFileName(),
	fInfo(),
	fEntryRefHashTableNext(NULL),
// 	fNodeRefHashTableNext(NULL),
	fOwner(NULL),
	fIgnoreEntryCreated(0),
	fIgnoreEntryRemoved(0)
{
}


PackageFile::~PackageFile()
{
}


status_t
PackageFile::Init(const entry_ref& entryRef, PackageFileManager* owner)
{
	fDirectoryRef.device = entryRef.device;
	fDirectoryRef.node = entryRef.directory;

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

	if (fFileName != fInfo.CanonicalFileName())
		fInfo.SetFileName(fFileName);

	fOwner = owner;

	return B_OK;
}


BString
PackageFile::RevisionedName() const
{
	return BString().SetToFormat("%s-%s", fInfo.Name().String(),
		fInfo.Version().ToString().String());
}


BString
PackageFile::RevisionedNameThrows() const
{
	BString result(RevisionedName());
	if (result.IsEmpty())
		throw std::bad_alloc();
	return result;
}


void
PackageFile::LastReferenceReleased()
{
	if (fOwner != NULL)
		fOwner->RemovePackageFile(this);
	delete this;
}
