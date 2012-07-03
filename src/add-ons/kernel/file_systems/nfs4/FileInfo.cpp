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


static status_t
sParsePath(RequestBuilder& req, uint32* count, const char* _path)
{
	char* path = strdup(_path);
	char* pathStart = path;
	char* pathEnd;
	while (pathStart != NULL) {
		pathEnd = strpbrk(pathStart, "/");
		if (pathEnd != NULL)
			*pathEnd = '\0';

		req.LookUp(pathStart);

		if (pathEnd != NULL && pathEnd[1] != '\0')
			pathStart = pathEnd + 1;
		else
			pathStart = NULL;

		(*count)++;
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

	sParsePath(req, &lookupCount, fs->Path());
	sParsePath(req, &lookupCount, fPath);

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

	status_t result = request.Send();
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
	} else
		return reply.GetFH(&fParent);
}

