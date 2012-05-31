/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "ReplyInterpreter.h"

#include <string.h>

#include <util/kernel_cpp.h>


ReplyInterpreter::ReplyInterpreter(RPC::Reply* reply)
	:
	fReply(reply)
{
	fReply->Stream().GetUInt();
	fReply->Stream().GetOpaque(NULL);
	fReply->Stream().GetUInt();
}


ReplyInterpreter::~ReplyInterpreter()
{
	delete fReply;
}


status_t
ReplyInterpreter::Access(uint32* supported, uint32* allowed)
{
	status_t res = _OperationError(OpAccess);
	if (res != B_OK)
		return res;

	uint32 support = fReply->Stream().GetUInt();
	uint32 allow = fReply->Stream().GetUInt();

	if (supported != NULL)
		*supported = support;
	if (allowed != NULL)
		*allowed = allow;

	return B_OK;
}


// Bit Twiddling Hacks
// http://graphics.stanford.edu/~seander/bithacks.html
static inline uint32 sCountBits(uint32 v)
{
	v = v - ((v >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}


static inline bool sIsAttrSet(Attribute attr, uint32* bitmap, uint32 count)
{
	if ((uint32)attr / 32 >= count)
		return false;

	return (bitmap[attr / 32] & 1 << attr % 32) != 0;
}


status_t
ReplyInterpreter::GetAttr(AttrValue** attrs, uint32* count)
{
	status_t res = _OperationError(OpGetAttr);
	if (res != B_OK)
		return res;

	uint32 bcount = fReply->Stream().GetUInt();
	uint32 *bitmap = new(std::nothrow) uint32[bcount];
	if (bitmap == NULL)
		return B_NO_MEMORY;

	uint32 attr_count = 0;
	for (uint32 i = 0; i < bcount; i++) {
		bitmap[i] = fReply->Stream().GetUInt();
		attr_count += sCountBits(bitmap[i]);
	}

	uint32 size;
	const void* ptr = fReply->Stream().GetOpaque(&size);
	XDR::ReadStream stream(const_cast<void*>(ptr), size);

	AttrValue* values = new(std::nothrow) AttrValue[attr_count];
	if (values == NULL) {
		delete bitmap;
		return B_NO_MEMORY;
	}

	uint32 current = 0;

	if (sIsAttrSet(FATTR4_TYPE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_TYPE;
		values[current].fData.fValue32 = stream.GetInt();
		current++;
	}

	if (sIsAttrSet(FATTR4_FH_EXPIRE_TYPE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_FH_EXPIRE_TYPE;
		values[current].fData.fValue32 = stream.GetUInt();
		current++;
	}

	if (sIsAttrSet(FATTR4_SIZE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_SIZE;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_FILEID, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_FILEID;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_MODE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_MODE;
		values[current].fData.fValue32 = stream.GetUInt();
		current++;
	}

	if (sIsAttrSet(FATTR4_NUMLINKS, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_NUMLINKS;
		values[current].fData.fValue32 = stream.GetUInt();
		current++;
	}

	delete bitmap;

	*count = attr_count;
	*attrs = values;
	return B_OK;
}


status_t
ReplyInterpreter::GetFH(Filehandle* fh)
{
	status_t res = _OperationError(OpGetFH);
	if (res != B_OK)
		return res;

	uint32 size;
	const void* ptr = fReply->Stream().GetOpaque(&size);
	if (ptr == NULL || size > NFS4_FHSIZE)
		return B_BAD_VALUE;

	if (fh != NULL) {
		fh->fSize = size;
		memcpy(fh->fFH, ptr, size);
	}

	return B_OK;
}


status_t
ReplyInterpreter::_OperationError(Opcode op)
{
	if (fReply->Error() != B_OK || fReply->Stream().IsEOF())
		return fReply->Error();

	if (fReply->Stream().GetInt() != op)
		return B_BAD_VALUE;

	return _NFS4ErrorToHaiku(fReply->Stream().GetUInt());
}


status_t
ReplyInterpreter::_NFS4ErrorToHaiku(uint32 x)
{
	switch (x) {
		case NFS4_OK:			return B_OK;
		case NFS4ERR_PERM:		return B_PERMISSION_DENIED;
		case NFS4ERR_NOENT:		return B_ENTRY_NOT_FOUND;
		case NFS4ERR_IO:		return B_IO_ERROR;
		case NFS4ERR_NXIO:		return B_DEVICE_NOT_FOUND;
		case NFS4ERR_ACCESS:	return B_PERMISSION_DENIED;
		case NFS4ERR_EXIST:		return B_FILE_EXISTS;
		case NFS4ERR_XDEV:		return B_CROSS_DEVICE_LINK;
		case NFS4ERR_NOTDIR:	return B_NOT_A_DIRECTORY;
		case NFS4ERR_ISDIR:		return B_IS_A_DIRECTORY;
		case NFS4ERR_INVAL:		return B_BAD_VALUE;
		case NFS4ERR_FBIG:		return B_FILE_TOO_LARGE;
		// ...
		default:				return B_ERROR;
	}
}

