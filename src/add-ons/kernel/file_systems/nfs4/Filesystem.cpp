/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Filesystem.h"

#include <string.h>

#include "ReplyInterpreter.h"
#include "RequestBuilder.h"

#include "Inode.h"


Filesystem::Filesystem()
	:
	fId(0)
{
}


Filesystem::~Filesystem()
{
}


status_t
Filesystem::Mount(Filesystem** pfs, RPC::Server* serv, const char* fsPath,
	dev_t id)
{
	RequestBuilder req(ProcCompound);
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

	RPC::Reply *rpl;
	serv->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;

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


Inode*
Filesystem::CreateRootInode()
{
	return new(std::nothrow)Inode(this, fRootFH, NULL);
}

