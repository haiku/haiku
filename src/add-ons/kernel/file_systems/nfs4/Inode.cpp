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

	Attribute attr[] = { FATTR4_TYPE, FATTR4_FILEID };
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
	if (result != B_OK || count < 1)
		return;

	if (count < 2 || values[1].fAttribute != FATTR4_FILEID) {
		// Server does not provide fileid. We need to make something up.
		fFileId = fs->GetId();
	} else
		fFileId = values[1].fData.fValue64;

	// FATTR4_TYPE is mandatory
	fType = values[0].fData.fValue32;

	delete values;
}


status_t
Inode::Stat(struct stat* st)
{
	RequestBuilder req(ProcCompound);
	req.PutFH(fHandle);

	Attribute attr[] = { FATTR4_SIZE, FATTR4_MODE, FATTR4_NUMLINKS };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	RPC::Reply *rpl;
	fFilesystem->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;
	result = reply.PutFH();
	if (result != B_OK)
		return result;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	// FATTR4_SIZE is mandatory
	if (count < 1 || values[0].fAttribute != FATTR4_SIZE)
		return B_BAD_VALUE;
	st->st_size = values[0].fData.fValue64;

	uint32 next = 1;
	st->st_mode = Type();
	if (count >= next && values[next].fAttribute == FATTR4_MODE) {
		st->st_mode |= values[next].fData.fValue32;
		next++;
	}
	// TODO: else: get some info from ACCESS

	if (count >= next && values[next].fAttribute == FATTR4_NUMLINKS) {
		st->st_nlink = values[next].fData.fValue32;
		next++;
	} else
		st->st_nlink = 1;	// TODO: if !link_support we dont have to ask

	st->st_uid = 0;
	st->st_gid = 0;

	delete values;
	return B_OK;
}

