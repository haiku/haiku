/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "FileInfo.h"

#include "Filesystem.h"
#include "Request.h"


// TODO: This function probably needs more strict checking against incorrect
// paths. Correct handling of '..' and '.' also may be useful.
status_t
FileInfo::ParsePath(RequestBuilder& req, uint32& count, const char* _path)
{
	char* path = strdup(_path);
	if (path == NULL)
		return B_NO_MEMORY;

	char* pathStart = path;
	char* pathEnd;

	while (pathStart != NULL) {
		pathEnd = strchr(pathStart, '/');
		if (pathEnd != NULL)
			*pathEnd = '\0';

		req.LookUp(pathStart);

		if (pathEnd != NULL && pathEnd[1] != '\0')
			pathStart = pathEnd + 1;
		else
			pathStart = NULL;

		count++;
	}
	free(path);

	return B_OK;
}


status_t
FileInfo::UpdateFileHandles(Filesystem* fs)
{
	Request request(fs->Server());
	RequestBuilder& req = request.Builder();

	req.PutRootFH();

	uint32 lookupCount = 0;
	status_t result;

	result = ParsePath(req, lookupCount, fs->Path());
	if (result != B_OK)
		return result;

	result = ParsePath(req, lookupCount, fPath);
	if (result != B_OK)
		return result;

	if (fs->IsAttrSupported(FATTR4_FILEID)) {
		AttrValue attr;
		attr.fAttribute = FATTR4_FILEID;
		attr.fFreePointer = false;
		attr.fData.fValue64 = fFileId;
		req.Verify(&attr, 1);
	}

	req.GetFH();
	req.LookUpUp();
	req.GetFH();

	result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	reply.PutRootFH();
	for (uint32 i = 0; i < lookupCount; i++)
		reply.LookUp();

	if (fs->IsAttrSupported(FATTR4_FILEID)) {
		result = reply.Verify();
		if (result != B_OK)
			return result;
	}

	reply.GetFH(&fHandle);
	if (reply.LookUpUp() == B_ENTRY_NOT_FOUND) {
		fParent = fHandle;
		return B_OK;
	}

	return reply.GetFH(&fParent);
}

