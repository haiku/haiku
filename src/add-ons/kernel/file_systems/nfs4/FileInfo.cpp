/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "FileInfo.h"

#include "FileSystem.h"
#include "Request.h"


InodeName::InodeName(InodeNames* parent, const char* name)
	:
	fParent(parent),
	fName(strdup(name))
{
	if (fParent != NULL)
		fParent->AcquireReference();
}


InodeName::~InodeName()
{
	if (fParent != NULL)
		fParent->ReleaseReference();
	free(const_cast<char*>(fName));
}


InodeNames::InodeNames()
{
	mutex_init(&fLock, NULL);
}


InodeNames::~InodeNames()
{
	while (!fNames.IsEmpty())
		delete fNames.RemoveHead();
	mutex_destroy(&fLock);
}


status_t
InodeNames::AddName(InodeNames* parent, const char* name)
{
	MutexLocker _(fLock);

	InodeName* current = fNames.Head();
	while (current != NULL) {
		if (current->fParent == parent && !strcmp(current->fName, name))
			return B_OK;
		current = fNames.GetNext(current);
	}

	InodeName* newName = new InodeName(parent, name);
	if (newName == NULL)
		return B_NO_MEMORY;
	fNames.Add(newName);
	return B_OK;
}


bool
InodeNames::RemoveName(InodeNames* parent, const char* name)
{
	MutexLocker _(fLock);

	InodeName* previous = NULL;
	InodeName* current = fNames.Head();
	while (current != NULL) {
		if (current->fParent == parent && !strcmp(current->fName, name)) {
			fNames.Remove(previous, current);
			delete current;
			break;
		}

		previous = current;
		current = fNames.GetNext(current);
	}

	return fNames.IsEmpty();
}


FileInfo::FileInfo()
	:
	fFileId(0),
	fNames(NULL)
{
}


FileInfo::~FileInfo()
{
	if (fNames != NULL)
		fNames->ReleaseReference();
}


FileInfo::FileInfo(const FileInfo& fi)
	:
	fFileId(fi.fFileId),
	fHandle(fi.fHandle),
	fNames(fi.fNames)
{
	if (fNames != NULL)
		fNames->AcquireReference();
}


FileInfo&
FileInfo::operator=(const FileInfo& fi)
{
	fFileId = fi.fFileId;
	fHandle = fi.fHandle;

	if (fNames != NULL)
		fNames->ReleaseReference();
	fNames = fi.fNames;
	if (fNames != NULL)
		fNames->AcquireReference();

	return *this;
}


status_t
FileInfo::UpdateFileHandles(FileSystem* fs)
{
	ASSERT(fs != NULL);

	Request request(fs->Server(), fs);
	RequestBuilder& req = request.Builder();

	req.PutRootFH();

	uint32 lookupCount = 0;
	const char** path = fs->Path();
	if (path != NULL) {
		for (; path[lookupCount] != NULL; lookupCount++)
			req.LookUp(path[lookupCount]);
	}

	uint32 i;
	InodeNames* names = fNames;
	for (i = 0; names != NULL; i++) {
		if (names->fNames.IsEmpty())
			return B_ENTRY_NOT_FOUND;

		names = names->fNames.Head()->fParent;
	}

	if (i > 0) {
		names = fNames;
		InodeNames** pathNames = new InodeNames*[i];
		if (pathNames == NULL)
			return B_NO_MEMORY;

		for (i = 0; names != NULL; i++) {
			pathNames[i] = names;
			names = names->fNames.Head()->fParent;
		}

		for (; i > 0; i--) {
			if (!strcmp(pathNames[i - 1]->fNames.Head()->fName, ""))
				continue;

			req.LookUp(pathNames[i - 1]->fNames.Head()->fName);
			lookupCount++;
		}
		delete[] pathNames;
	}

	req.GetFH();

	if (fs->IsAttrSupported(FATTR4_FILEID)) {
		AttrValue attr;
		attr.fAttribute = FATTR4_FILEID;
		attr.fFreePointer = false;
		attr.fData.fValue64 = fFileId;
		req.Verify(&attr, 1);
	}

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	reply.PutRootFH();
	for (uint32 i = 0; i < lookupCount; i++)
		reply.LookUp();

	FileHandle handle;
	result = reply.GetFH(&handle);
	if (result != B_OK)
		return result;

	if (fs->IsAttrSupported(FATTR4_FILEID)) {
		result = reply.Verify();
		if (result != B_OK)
			return result;
	}

	fHandle = handle;
	fNames->fHandle = handle;

	return B_OK;
}

