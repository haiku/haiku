/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"

#include <string.h>

#include "ReplyInterpreter.h"
#include "RequestBuilder.h"


// Creating Inode object from Filehandle probably is not a good idea when
// filehandles are volatile.
Inode::Inode(Filesystem* fs, const Filehandle &fh)
	:
	fFilesystem(fs)
{
	memcpy(&fHandle, &fh, sizeof(fh));

	RequestBuilder req(ProcCompound);
	req.PutFH(fh);

	Attribute attr[] = { FATTR4_FILEID };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	RPC::Reply *rpl;
	fs->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;
	result = reply.PutFH();
	if (result != B_OK)
		return;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return;

	if (count < 1 || values[0].fAttribute != FATTR4_FILEID) {
		// Server does not provide fileid. We need to make something up.
		fFileId = fs->GetId();
	} else
		fFileId = values[0].fData.fValue64;

	delete values;
}

