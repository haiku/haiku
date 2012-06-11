/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Filesystem.h"

#include <string.h>

#include "Request.h"

#include "Inode.h"


Filesystem::Filesystem()
	:
	fId(1)
{
}


Filesystem::~Filesystem()
{
}


status_t
Filesystem::Mount(Filesystem** pfs, RPC::Server* serv, const char* fsPath,
	dev_t id)
{
	Request request(serv);
	RequestBuilder& req = request.Builder();

	req.PutRootFH();

	// Better way of doing this will be needed
	uint32 lookupCount = 0;
	char* path = strdup(fsPath);
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

		lookupCount++;
	}
	free(path);

	req.GetFH();
	req.Access();
	Attribute attr[] = { FATTR4_FH_EXPIRE_TYPE };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	result = reply.PutRootFH();
	if (result != B_OK)
		return result;

	for (uint32 i = 0; i < lookupCount; i++) {
		result = reply.LookUp();
		if (result != B_OK)
			return result;
	}

	Filehandle fh;
	result = reply.GetFH(&fh);
	if (result != B_OK)
		return result;

	uint32 allowed;
	result = reply.Access(NULL, &allowed);
	if (result != B_OK)
		return result;
	else if (allowed & (ACCESS4_READ | ACCESS4_LOOKUP)
				!= (ACCESS4_READ | ACCESS4_LOOKUP))
		return B_PERMISSION_DENIED;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	if (count != 1 || values[0].fAttribute != FATTR4_FH_EXPIRE_TYPE) {
		delete[] values;
		return B_BAD_VALUE;
	}

	// Currently, only persistent filehandles are supported. That will be
	// changed soon.
	if (values[0].fData.fValue32 != FH4_PERSISTENT) {
		delete[] values;
		return B_UNSUPPORTED;
	}

	Filesystem* fs = new(std::nothrow) Filesystem;
	fs->fFHExpiryType = values[0].fData.fValue32;
	memcpy(&fs->fRootFH, &fh, sizeof(Filehandle));
	fs->fServer = serv;
	fs->fDevId = id;

	delete[] values;

	*pfs = fs;

	return B_OK;
}


status_t
Filesystem::GetInode(ino_t id, Inode** _inode)
{
	FileInfo fi;
	status_t result = fInoIdMap.GetFileInfo(&fi, id);
	if (result == B_ENTRY_NOT_FOUND)
		dprintf("NFS4: unknown inode: %llu\n", id);

	if (result != B_OK)
		return result;

	Inode* inode;
	result = Inode::CreateInode(this, fi, &inode);
	if (result != B_OK)
		return result;

	*_inode = inode;
	return B_OK;
}


Inode*
Filesystem::CreateRootInode()
{
	FileInfo fi;
	fi.fFH = fRootFH;
	fi.fParent = fRootFH;
	fi.fName = strdup("/");

	Inode* inode;
	status_t result = Inode::CreateInode(this, fi, &inode);
	if (result == B_OK)
		return inode;
	else
		return NULL;
}

