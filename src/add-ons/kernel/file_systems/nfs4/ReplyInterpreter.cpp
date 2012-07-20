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

#include "Cookie.h"


FSLocation::~FSLocation()
{
	free(const_cast<char*>(fRootPath));
	for (uint32 i = 0; i < fCount; i++)
		free(const_cast<char*>(fLocations[i]));
	delete[] fLocations;
}


FSLocations::~FSLocations()
{
	free(const_cast<char*>(fRootPath));
	delete[] fLocations;
}


AttrValue::AttrValue()
	:
	fFreePointer(false)
{
}


AttrValue::~AttrValue()
{
	if (fFreePointer)
		free(fData.fPointer);
	if (fAttribute == FATTR4_FS_LOCATIONS)
		delete fData.fLocations;
}


DirEntry::DirEntry()
	:
	fName(NULL),
	fAttrs(NULL),
	fAttrCount(0)
{
}


DirEntry::~DirEntry()
{
	free(const_cast<char*>(fName));
	delete[] fAttrs;
}


ReplyInterpreter::ReplyInterpreter(RPC::Reply* reply)
	:
	fNFS4Error(NFS4_OK),
	fDecodeError(false),
	fReply(reply)
{
	if (reply != NULL)
		_ParseHeader();
}


ReplyInterpreter::~ReplyInterpreter()
{
	delete fReply;
}


void
ReplyInterpreter::_ParseHeader()
{
	fNFS4Error = fReply->Stream().GetUInt();
	fReply->Stream().GetOpaque(NULL);
	fReply->Stream().GetUInt();
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

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Close()
{
	status_t res = _OperationError(OpClose);
	if (res != B_OK)
		return res;

	fReply->Stream().GetUInt();
	fReply->Stream().GetUInt();
	fReply->Stream().GetUInt();
	fReply->Stream().GetUInt();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Create(uint64* before, uint64* after, bool& atomic)
{
	status_t res = _OperationError(OpCreate);
	if (res != B_OK)
		return res;

	atomic = fReply->Stream().GetBoolean();
	*before = fReply->Stream().GetUHyper();
	*after = fReply->Stream().GetUHyper();
	uint32 count = fReply->Stream().GetUInt();
	for (uint32 i; i < count; i++)
		fReply->Stream().GetUInt();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}



// Bit Twiddling Hacks
// http://graphics.stanford.edu/~seander/bithacks.html
static inline uint32 sCountBits(uint32 v)
{
	v = v - ((v >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}


status_t
ReplyInterpreter::GetAttr(AttrValue** attrs, uint32* count)
{
	status_t res = _OperationError(OpGetAttr);
	if (res != B_OK)
		return res;

	return _DecodeAttrs(fReply->Stream(), attrs, count);
}


status_t
ReplyInterpreter::GetFH(FileHandle* fh)
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
		memcpy(fh->fData, ptr, size);
	}

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Link(uint64* before, uint64* after, bool& atomic)
{
	status_t res = _OperationError(OpLink);
	if (res != B_OK)
		return res;

	atomic = fReply->Stream().GetBoolean();
	*before = fReply->Stream().GetUHyper();
	*after = fReply->Stream().GetUHyper();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Lock(LockInfo* linfo)
{
	status_t res = _OperationError(OpLock);
	if (res != B_OK)
		return res;

	linfo->fOwner->fStateSeq = fReply->Stream().GetUInt();
	linfo->fOwner->fStateId[0] = fReply->Stream().GetUInt();
	linfo->fOwner->fStateId[1] = fReply->Stream().GetUInt();
	linfo->fOwner->fStateId[2] = fReply->Stream().GetUInt();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::LockT(uint64* pos, uint64* len, LockType* type)
{
	status_t res = _OperationError(OpLockU);
	if (res != B_WOULD_BLOCK || NFS4Error() != NFS4ERR_DENIED)
		return res;

	*pos = fReply->Stream().GetUHyper();
	*len = fReply->Stream().GetUHyper();
	*type = static_cast<LockType>(fReply->Stream().GetInt());

	fReply->Stream().GetUHyper();
	fReply->Stream().GetOpaque(NULL);

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::LockU(LockInfo* linfo)
{
	status_t res = _OperationError(OpLockU);
	if (res != B_OK)
		return res;

	linfo->fOwner->fStateSeq = fReply->Stream().GetUInt();
	linfo->fOwner->fStateId[0] = fReply->Stream().GetUInt();
	linfo->fOwner->fStateId[1] = fReply->Stream().GetUInt();
	linfo->fOwner->fStateId[2] = fReply->Stream().GetUInt();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Open(uint32* id, uint32* seq, bool* confirm)
{
	status_t res = _OperationError(OpOpen);
	if (res != B_OK)
		return res;

	*seq = fReply->Stream().GetUInt();
	id[0] = fReply->Stream().GetUInt();
	id[1] = fReply->Stream().GetUInt();
	id[2] = fReply->Stream().GetUInt();

	// change info
	fReply->Stream().GetBoolean();
	fReply->Stream().GetUHyper();
	fReply->Stream().GetUHyper();

	uint32 flags = fReply->Stream().GetUInt();
	*confirm = (flags & OPEN4_RESULT_CONFIRM) == OPEN4_RESULT_CONFIRM;

	// attrmask
	uint32 bcount = fReply->Stream().GetUInt();
	for (uint32 i = 0; i < bcount; i++)
		fReply->Stream().GetUInt();

	// delegation info
	fReply->Stream().GetUInt();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::OpenConfirm(uint32* stateSeq)
{
	status_t res = _OperationError(OpOpenConfirm);
	if (res != B_OK)
		return res;

	*stateSeq = fReply->Stream().GetUInt();
	fReply->Stream().GetUInt();
	fReply->Stream().GetUInt();
	fReply->Stream().GetUInt();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Read(void* buffer, uint32* size, bool* eof)
{
	status_t res = _OperationError(OpRead);
	if (res != B_OK)
		return res;

	*eof = fReply->Stream().GetBoolean();
	const void* ptr = fReply->Stream().GetOpaque(size);
	memcpy(buffer, ptr, *size);

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::ReadDir(uint64* cookie, uint64* cookieVerf,
	DirEntry** dirents, uint32* _count,	bool* eof)
{
	status_t res = _OperationError(OpReadDir);
	if (res != B_OK)
		return res;

	*cookieVerf = fReply->Stream().GetUHyper();

	bool isNext;
	uint32 count = 0;

	// TODO: using  list instead of array would make this much more elegant
	// and efficient
	XDR::Stream::Position dataStart = fReply->Stream().Current();
	isNext = fReply->Stream().GetBoolean();
	while (isNext) {
		fReply->Stream().GetUHyper();

		free(fReply->Stream().GetString());
		AttrValue* values;
		uint32 attrCount;
		_DecodeAttrs(fReply->Stream(), &values,	&attrCount);
		delete[] values;

		count++;

		isNext = fReply->Stream().GetBoolean();
	}

	DirEntry* entries = new(std::nothrow) DirEntry[count];
	if (entries == NULL)
		return B_NO_MEMORY;

	count = 0;
	fReply->Stream().SetPosition(dataStart);
	isNext = fReply->Stream().GetBoolean();
	while (isNext) {
		*cookie = fReply->Stream().GetUHyper();

		entries[count].fName = fReply->Stream().GetString();
		_DecodeAttrs(fReply->Stream(), &entries[count].fAttrs,
			&entries[count].fAttrCount);

		count++;

		isNext = fReply->Stream().GetBoolean();
	}
	if (!isNext)
		*eof = fReply->Stream().GetBoolean();
	else
		*eof = false;

	*_count = count;
	*dirents = entries;

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::ReadLink(void* buffer, uint32* size, uint32 maxSize)
{
	status_t res = _OperationError(OpReadLink);
	if (res != B_OK)
		return res;

	const void* ptr = fReply->Stream().GetOpaque(size);
	memcpy(buffer, ptr, min_c(*size, maxSize));

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Remove(uint64* before, uint64* after, bool& atomic)
{
	status_t res = _OperationError(OpRemove);
	if (res != B_OK)
		return res;

	atomic = fReply->Stream().GetBoolean();
	*before = fReply->Stream().GetUHyper();
	*after = fReply->Stream().GetUHyper();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Rename(uint64* fromBefore, uint64* fromAfter,
	bool& fromAtomic, uint64* toBefore, uint64* toAfter, bool& toAtomic)
{
	status_t res = _OperationError(OpRename);
	if (res != B_OK)
		return res;

	fromAtomic = fReply->Stream().GetBoolean();
	*fromBefore = fReply->Stream().GetUHyper();
	*fromAfter = fReply->Stream().GetUHyper();

	toAtomic = fReply->Stream().GetBoolean();
	*toBefore = fReply->Stream().GetUHyper();
	*toAfter = fReply->Stream().GetUHyper();
	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::SetAttr()
{
	status_t res = _OperationError(OpSetAttr);
	if (res != B_OK)
		return res;

	uint32 bcount = fReply->Stream().GetUInt();
	for (uint32 i = 0; i < bcount; i++)
		fReply->Stream().GetUInt();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::SetClientID(uint64* clientid, uint64* verifier)
{
	status_t res = _OperationError(OpSetClientID);
	if (res != B_OK)
		return res;

	*clientid = fReply->Stream().GetUHyper();
	*verifier = fReply->Stream().GetUHyper();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::Write(uint32* size)
{
	status_t res = _OperationError(OpWrite);
	if (res != B_OK)
		return res;

	*size = fReply->Stream().GetUInt();
	fReply->Stream().GetInt();
	fReply->Stream().GetUHyper();

	return fReply->Stream().IsEOF() ? B_BAD_VALUE : B_OK;
}


static const char*
sFlattenPathname(XDR::ReadStream& str)
{
	uint32 count = str.GetUInt();
	char* pathname = NULL;
	uint32 size = 0;
	for (uint32 i = 0; i < count; i++) {
		const char* path = str.GetString();
		size += strlen(path) + 1;
		if (pathname == NULL) {
			pathname = reinterpret_cast<char*>(malloc(strlen(path + 1)));
			pathname[0] = '\0';
		} else {
			*pathname++ = '/';
			pathname = reinterpret_cast<char*>(realloc(pathname, size));
		}
		strcat(pathname, path);
		free(const_cast<char*>(path));
	}

	return pathname;
}


status_t
ReplyInterpreter::_DecodeAttrs(XDR::ReadStream& str, AttrValue** attrs,
	uint32* count)
{
	uint32 bcount = fReply->Stream().GetUInt();
	uint32 *bitmap = new(std::nothrow) uint32[bcount];
	if (bitmap == NULL)
		return B_NO_MEMORY;

	uint32 attr_count = 0;
	for (uint32 i = 0; i < bcount; i++) {
		bitmap[i] = str.GetUInt();
		attr_count += sCountBits(bitmap[i]);
	}

	if (attr_count == 0) {
		*attrs = NULL;
		*count = 0;
		return B_OK;
	} else if (attr_count > FATTR4_MAXIMUM_ATTR_ID)
		return B_BAD_VALUE;

	uint32 size;
	const void* ptr = str.GetOpaque(&size);
	XDR::ReadStream stream(const_cast<void*>(ptr), size);

	AttrValue* values = new(std::nothrow) AttrValue[attr_count];
	if (values == NULL) {
		delete[] bitmap;
		return B_NO_MEMORY;
	}

	uint32 current = 0;

	if (sIsAttrSet(FATTR4_SUPPORTED_ATTRS, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_SUPPORTED_ATTRS;
		uint32 count = stream.GetInt();
		uint32 i;
		// two uint32 are enough for NFS4, not for NFS4.1
		for (i = 0; i < min_c(count, 2); i++)
			((uint32*)&values[current].fData.fValue64)[i] = stream.GetUInt();
		for (; i < count; i++)
			stream.GetUInt();
		current++;
	}

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

	if (sIsAttrSet(FATTR4_CHANGE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_CHANGE;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_SIZE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_SIZE;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_FSID, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_FSID;
		values[current].fFreePointer = true;

		FileSystemId fsid;
		fsid.fMajor = stream.GetUHyper();
		fsid.fMinor = stream.GetUHyper();
		
		values[current].fData.fPointer = malloc(sizeof(fsid));
		memcpy(values[current].fData.fPointer, &fsid, sizeof(fsid));
		current++;
	}

	if (sIsAttrSet(FATTR4_LEASE_TIME, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_LEASE_TIME;
		values[current].fData.fValue32 = stream.GetUInt();
		current++;
	}

	if (sIsAttrSet(FATTR4_FILEID, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_FILEID;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_FILES_FREE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_FILES_FREE;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_FILES_TOTAL, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_FILES_TOTAL;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_FS_LOCATIONS, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_FS_LOCATIONS;

		FSLocations* locs = new FSLocations;
		locs->fRootPath = sFlattenPathname(stream);
		locs->fCount = stream.GetUInt();
		locs->fLocations = new FSLocation[locs->fCount];
		for (uint32 i = 0; i < locs->fCount; i++) {
			locs->fLocations[i].fRootPath = sFlattenPathname(stream);
			locs->fLocations[i].fCount = stream.GetUInt();
			locs->fLocations[i].fLocations =
				new const char*[locs->fLocations[i].fCount];
			for (uint32 j = 0; j < locs->fLocations[i].fCount; j++)
				locs->fLocations[i].fLocations[j] = stream.GetString();
		}
		values[current].fData.fLocations = locs;
		current++;
	}

	if (sIsAttrSet(FATTR4_MAXREAD, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_MAXREAD;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_MAXWRITE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_MAXWRITE;
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

	if (sIsAttrSet(FATTR4_OWNER, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_OWNER;
		values[current].fFreePointer = true;
		values[current].fData.fPointer = stream.GetString();
		current++;
	}

	if (sIsAttrSet(FATTR4_OWNER_GROUP, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_OWNER_GROUP;
		values[current].fFreePointer = true;
		values[current].fData.fPointer = stream.GetString();
		current++;
	}

	if (sIsAttrSet(FATTR4_SPACE_FREE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_SPACE_FREE;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_SPACE_TOTAL, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_SPACE_TOTAL;
		values[current].fData.fValue64 = stream.GetUHyper();
		current++;
	}

	if (sIsAttrSet(FATTR4_TIME_ACCESS, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_TIME_ACCESS;
		values[current].fFreePointer = true;

		struct timespec ts;
		ts.tv_sec = static_cast<time_t>(stream.GetHyper());
		ts.tv_nsec = static_cast<long>(stream.GetUInt());
		
		values[current].fData.fPointer = malloc(sizeof(ts));
		memcpy(values[current].fData.fPointer, &ts, sizeof(ts));
		current++;
	}

	if (sIsAttrSet(FATTR4_TIME_CREATE, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_TIME_CREATE;
		values[current].fFreePointer = true;

		struct timespec ts;
		ts.tv_sec = static_cast<time_t>(stream.GetHyper());
		ts.tv_nsec = static_cast<long>(stream.GetUInt());
		
		values[current].fData.fPointer = malloc(sizeof(ts));
		memcpy(values[current].fData.fPointer, &ts, sizeof(ts));
		current++;
	}

	if (sIsAttrSet(FATTR4_TIME_METADATA, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_TIME_METADATA;
		values[current].fFreePointer = true;

		struct timespec ts;
		ts.tv_sec = static_cast<time_t>(stream.GetHyper());
		ts.tv_nsec = static_cast<long>(stream.GetUInt());
		
		values[current].fData.fPointer = malloc(sizeof(ts));
		memcpy(values[current].fData.fPointer, &ts, sizeof(ts));
		current++;
	}

	if (sIsAttrSet(FATTR4_TIME_MODIFY, bitmap, bcount)) {
		values[current].fAttribute = FATTR4_TIME_MODIFY;
		values[current].fFreePointer = true;

		struct timespec ts;
		ts.tv_sec = static_cast<time_t>(stream.GetHyper());
		ts.tv_nsec = static_cast<long>(stream.GetUInt());
		
		values[current].fData.fPointer = malloc(sizeof(ts));
		memcpy(values[current].fData.fPointer, &ts, sizeof(ts));
		current++;
	}

	delete[] bitmap;

	*count = attr_count;
	*attrs = values;
	return str.IsEOF() ? B_BAD_VALUE : B_OK;
}


status_t
ReplyInterpreter::_OperationError(Opcode op)
{
	if (fDecodeError)
		return B_BAD_VALUE;

	if (fReply == NULL)
		return B_NOT_INITIALIZED;

	if (fReply->Error() != B_OK || fReply->Stream().IsEOF()) {
		fDecodeError = true;
		return fReply->Error();
	}

	if (fReply->Stream().GetInt() != op) {
		fDecodeError = true;
		return B_BAD_VALUE;
	}

	status_t result = _NFS4ErrorToHaiku(fReply->Stream().GetUInt());
	if (result != B_OK)
		fDecodeError = true;
	return result;
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
		case NFS4ERR_ACCESS:	return B_NOT_ALLOWED;
		case NFS4ERR_EXIST:		return B_FILE_EXISTS;
		case NFS4ERR_XDEV:		return B_CROSS_DEVICE_LINK;
		case NFS4ERR_NOTDIR:	return B_NOT_A_DIRECTORY;
		case NFS4ERR_ISDIR:		return B_IS_A_DIRECTORY;
		case NFS4ERR_INVAL:		return B_BAD_VALUE;
		case NFS4ERR_FBIG:		return B_FILE_TOO_LARGE;
		// ...
		case NFS4ERR_DELAY:
		case NFS4ERR_DENIED:
		case NFS4ERR_LOCKED:
		case NFS4ERR_GRACE:
								return B_WOULD_BLOCK;
		case NFS4ERR_FHEXPIRED:	return B_ENTRY_NOT_FOUND;
		// ...
		default:				return B_ERROR;
	}
}

