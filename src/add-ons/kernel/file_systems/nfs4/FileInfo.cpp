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


status_t
FileInfo::ParsePath(RequestBuilder& req, uint32& count, const char* _path,
	bool getFileHandle)
{
	ASSERT(_path != NULL);

	char* path = strdup(_path);
	if (path == NULL)
		return B_NO_MEMORY;

	char* pathStart = path;
	char* pathEnd;

	while (pathStart != NULL) {
		pathEnd = strchr(pathStart, '/');
		if (pathEnd != NULL)
			*pathEnd = '\0';

		if (pathEnd != pathStart) {
			if (!strcmp(pathStart, "..")) {
				req.LookUpUp();
				count++;
			} else if (strcmp(pathStart, ".")) {
				req.LookUp(pathStart);
				if (getFileHandle)
					req.GetFH();
				count++;
			}
		}

		if (pathEnd != NULL && pathEnd[1] != '\0')
			pathStart = pathEnd + 1;
		else
			pathStart = NULL;
	}
	free(path);

	return B_OK;
}


status_t
FileInfo::CreateName(const char* dirPath, const char* name)
{
	ASSERT(name != NULL);

	free(const_cast<char*>(fName));
	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	free(const_cast<char*>(fPath));
	fPath = NULL;
	if (dirPath != NULL) {
		char* path = reinterpret_cast<char*>(malloc(strlen(name) + 2
			+ strlen(dirPath)));
		if (path == NULL)
			return B_NO_MEMORY;

		strcpy(path, dirPath);
		strcat(path, "/");
		strcat(path, name);

		fPath = path;
	} else
		fPath = strdup(name);

	if (fPath == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
FileInfo::UpdateFileHandles(FileSystem* fs)
{
	ASSERT(fs != NULL);

	Request request(fs->Server(), fs);
	RequestBuilder& req = request.Builder();

	req.PutRootFH();
	req.GetFH();

	uint32 lookupCount = 0;
	status_t result = ParsePath(req, lookupCount, fs->Path(), true);
	if (result != B_OK)
		return result;

	result = ParsePath(req, lookupCount, fPath, true);
	if (result != B_OK)
		return result;

	if (fs->IsAttrSupported(FATTR4_FILEID)) {
		AttrValue attr;
		attr.fAttribute = FATTR4_FILEID;
		attr.fFreePointer = false;
		attr.fData.fValue64 = fFileId;
		req.Verify(&attr, 1);
	}

	result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	FileHandle parent;
	FileHandle child;

	reply.PutRootFH();
	reply.GetFH(&child);
	parent = child;
	for (uint32 i = 0; i < lookupCount; i++) {
		reply.LookUp();
		parent = child;
		result = reply.GetFH(&child);
		if (result !=  B_OK)
			return result;
	}

	if (fs->IsAttrSupported(FATTR4_FILEID)) {
		result = reply.Verify();
		if (result != B_OK)
			return result;
	}

	fHandle = child;
	fParent = parent;

	return B_OK;
}

