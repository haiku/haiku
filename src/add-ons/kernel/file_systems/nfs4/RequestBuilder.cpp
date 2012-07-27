/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RequestBuilder.h"

#include <errno.h>
#include <string.h>

#include "Cookie.h"


RequestBuilder::RequestBuilder(Procedure proc)
	:
	fOpCount(0),
	fProcedure(proc),
	fRequest(NULL)
{
	_InitHeader();
}


RequestBuilder::~RequestBuilder()
{
	delete fRequest;
}


void
RequestBuilder::_InitHeader()
{
	fRequest = RPC::Call::Create(fProcedure, RPC::Auth::CreateSys(),
					RPC::Auth::CreateNone());

	if (fRequest == NULL)
		return;

	if (fProcedure == ProcCompound) {
		fRequest->Stream().AddOpaque(NULL, 0);
		fRequest->Stream().AddUInt(0);

		fOpCountPosition = fRequest->Stream().Current();
		fRequest->Stream().AddUInt(0);
	}
}


status_t
RequestBuilder::Access()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpAccess);
	fRequest->Stream().AddUInt(ACCESS4_READ | ACCESS4_LOOKUP | ACCESS4_MODIFY
								| ACCESS4_EXTEND | ACCESS4_DELETE
								| ACCESS4_EXECUTE);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Close(uint32 seq, const uint32* id, uint32 stateSeq)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpClose);
	fRequest->Stream().AddUInt(seq);
	fRequest->Stream().AddUInt(stateSeq);
	fRequest->Stream().AddUInt(id[0]);
	fRequest->Stream().AddUInt(id[1]);
	fRequest->Stream().AddUInt(id[2]);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Commit(uint64 offset, uint32 count)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpCommit);
	fRequest->Stream().AddUHyper(offset);
	fRequest->Stream().AddUInt(count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Create(FileType type, const char* name, AttrValue* attr,
	uint32 count, const char* path)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;
	if (type == NF4LNK && path == NULL)
		return B_BAD_VALUE;
	if (name == NULL)
		return B_BAD_VALUE;
	if (type == NF4BLK || type == NF4CHR)
		return B_BAD_VALUE;

	fRequest->Stream().AddUInt(OpCreate);
	fRequest->Stream().AddUInt(type);
	if (type == NF4LNK)
		fRequest->Stream().AddString(path);
	fRequest->Stream().AddString(name);
	_EncodeAttrs(fRequest->Stream(), attr, count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::GetAttr(Attribute* attrs, uint32 count)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpGetAttr);
	_AttrBitmap(fRequest->Stream(), attrs, count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::GetFH()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpGetFH);
	fOpCount++;

	return B_OK;
}


void
RequestBuilder::_GenerateLockOwner(XDR::WriteStream& stream,
	OpenFileCookie* cookie, LockOwner* owner)
{
	stream.AddUHyper(cookie->fClientId);

	uint64 lockOwner[2];
	lockOwner[0] = owner->fOwner;
	lockOwner[1] = cookie->fInfo.fFileId;
	stream.AddOpaque(lockOwner, sizeof(lockOwner));
}


status_t
RequestBuilder::Lock(OpenFileCookie* cookie, LockInfo* lock, bool reclaim)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpLock);

	fRequest->Stream().AddInt(lock->fType);
	fRequest->Stream().AddBoolean(reclaim);

	fRequest->Stream().AddUHyper(lock->fStart);
	fRequest->Stream().AddUHyper(lock->fLength);

	if (lock->fOwner->fStateId[0] == 0 && lock->fOwner->fStateId[1] == 0
		&& lock->fOwner->fStateId[2] == 0) {

		fRequest->Stream().AddBoolean(true);			// new lock owner

		// open seq stateid
		fRequest->Stream().AddUInt(cookie->fSequence++);
		fRequest->Stream().AddUInt(cookie->fStateSeq);
		fRequest->Stream().AddUInt(cookie->fStateId[0]);
		fRequest->Stream().AddUInt(cookie->fStateId[1]);
		fRequest->Stream().AddUInt(cookie->fStateId[2]);

		// lock seq owner
		fRequest->Stream().AddUInt(lock->fOwner->fSequence++);
		_GenerateLockOwner(fRequest->Stream(), cookie, lock->fOwner);

	} else {
		fRequest->Stream().AddBoolean(false);			// old lock owner

		// lock stateid seq
		fRequest->Stream().AddUInt(lock->fOwner->fStateSeq);
		fRequest->Stream().AddUInt(lock->fOwner->fStateId[0]);
		fRequest->Stream().AddUInt(lock->fOwner->fStateId[1]);
		fRequest->Stream().AddUInt(lock->fOwner->fStateId[2]);

		fRequest->Stream().AddUInt(lock->fOwner->fSequence++);
	}

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::LockT(LockType type, uint64 pos, uint64 len,
	OpenFileCookie* cookie)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpLockT);

	fRequest->Stream().AddInt(type);

	fRequest->Stream().AddUHyper(pos);
	fRequest->Stream().AddUHyper(len);

	fRequest->Stream().AddUHyper(cookie->fClientId);

	uint32 owner = find_thread(NULL);
	fRequest->Stream().AddOpaque(&owner, sizeof(owner));

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::LockU(LockInfo* lock)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpLockU);

	fRequest->Stream().AddInt(lock->fType);

	fRequest->Stream().AddUInt(lock->fOwner->fSequence++);
	fRequest->Stream().AddUInt(lock->fOwner->fStateSeq);
	fRequest->Stream().AddUInt(lock->fOwner->fStateId[0]);
	fRequest->Stream().AddUInt(lock->fOwner->fStateId[1]);
	fRequest->Stream().AddUInt(lock->fOwner->fStateId[2]);

	fRequest->Stream().AddUHyper(lock->fStart);
	fRequest->Stream().AddUHyper(lock->fLength);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Link(const char* name)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;
	if (name == NULL)
		return B_BAD_VALUE;

	fRequest->Stream().AddUInt(OpLink);
	fRequest->Stream().AddString(name);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::LookUp(const char* name)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;
	if (name == NULL)
		return B_BAD_VALUE;

	fRequest->Stream().AddUInt(OpLookUp);
	fRequest->Stream().AddString(name, strlen(name));
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::LookUpUp()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpLookUpUp);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Nverify(AttrValue* attr, uint32 count)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpNverify);
	_EncodeAttrs(fRequest->Stream(), attr, count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Open(OpenClaim claim, uint32 seq, uint32 access, uint64 id,
	OpenCreate oc, uint64 ownerId, const char* name, AttrValue* attr,
	uint32 count, bool excl)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpOpen);
	fRequest->Stream().AddUInt(seq);
	fRequest->Stream().AddUInt(access);
	fRequest->Stream().AddUInt(0);			// deny none
	fRequest->Stream().AddUHyper(id);

	char owner[128];
	int pos = 0;
	*(uint64*)(owner + pos) = ownerId;
	pos += sizeof(uint64);

	fRequest->Stream().AddOpaque(owner, pos);

	fRequest->Stream().AddUInt(oc);
	if (oc == OPEN4_CREATE) {
		fRequest->Stream().AddInt(excl ? GUARDED4 : UNCHECKED4);
		_EncodeAttrs(fRequest->Stream(), attr, count);
	}

	fRequest->Stream().AddUInt(claim);
	switch (claim) {
		case CLAIM_NULL:
			fRequest->Stream().AddString(name, strlen(name));
			break;
		case CLAIM_PREVIOUS:
			fRequest->Stream().AddUInt(0);
			break;
		default:
			return B_UNSUPPORTED;
	}

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::OpenConfirm(uint32 seq, const uint32* id, uint32 stateSeq)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpOpenConfirm);
	fRequest->Stream().AddUInt(stateSeq);
	fRequest->Stream().AddUInt(id[0]);
	fRequest->Stream().AddUInt(id[1]);
	fRequest->Stream().AddUInt(id[2]);
	fRequest->Stream().AddUInt(seq);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::PutFH(const FileHandle& fh)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpPutFH);
	fRequest->Stream().AddOpaque(fh.fData, fh.fSize);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::PutRootFH()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpPutRootFH);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Read(const uint32* id, uint32 stateSeq, uint64 pos, uint32 len)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpRead);
	fRequest->Stream().AddUInt(stateSeq);
	fRequest->Stream().AddUInt(id[0]);
	fRequest->Stream().AddUInt(id[1]);
	fRequest->Stream().AddUInt(id[2]);
	fRequest->Stream().AddUHyper(pos);
	fRequest->Stream().AddUInt(len);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::ReadDir(uint32 count, uint64 cookie, uint64 cookieVerf,
	Attribute* attrs, uint32 attrCount)
{
	(void)count;

	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpReadDir);
	fRequest->Stream().AddUHyper(cookie);
	fRequest->Stream().AddUHyper(cookieVerf);

	// consider predicting this values basing on count or buffer size
	fRequest->Stream().AddUInt(0x2000);
	fRequest->Stream().AddUInt(0x8000);
	_AttrBitmap(fRequest->Stream(), attrs, attrCount);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::ReadLink()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpReadLink);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Remove(const char* file)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpRemove);
	fRequest->Stream().AddString(file);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Rename(const char* from, const char* to)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpRename);
	fRequest->Stream().AddString(from);
	fRequest->Stream().AddString(to);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Renew(uint64 clientId)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpRenew);
	fRequest->Stream().AddUHyper(clientId);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::SaveFH()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpSaveFH);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::SetAttr(const uint32* id, uint32 stateSeq, AttrValue* attr,
	uint32 count)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpSetAttr);
	fRequest->Stream().AddUInt(stateSeq);
	if (id != NULL) {
		fRequest->Stream().AddUInt(id[0]);
		fRequest->Stream().AddUInt(id[1]);
		fRequest->Stream().AddUInt(id[2]);
	} else {
		fRequest->Stream().AddUInt(0);
		fRequest->Stream().AddUInt(0);
		fRequest->Stream().AddUInt(0);
	}
	_EncodeAttrs(fRequest->Stream(), attr, count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::SetClientID(const RPC::Server* server)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpSetClientID);
	uint64 verifier = rand();
	verifier = verifier << 32 | rand();
	fRequest->Stream().AddUHyper(verifier);

	status_t result = _GenerateClientId(fRequest->Stream(), server);
	if (result != B_OK)
		return result;

	// Callbacks are currently not supported
	fRequest->Stream().AddUInt(0);
	fRequest->Stream().AddOpaque(NULL, 0);
	fRequest->Stream().AddOpaque(NULL, 0);
	fRequest->Stream().AddUInt(0);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::_GenerateClientId(XDR::WriteStream& stream,
	const RPC::Server* server)
{
	char id[512] = "HAIKU:kernel:";
	int pos = strlen(id);

	const sockaddr* remoteAddress =
		reinterpret_cast<const sockaddr*>(&server->ID().fAddress);

	ServerAddress local = server->LocalID();
	const sockaddr* localAddress = reinterpret_cast<sockaddr*>(&local.fAddress);
	
	const sockaddr_in* address4;
	const sockaddr_in6* address6;
	switch (remoteAddress->sa_family) {
		case AF_INET:
			address4 = reinterpret_cast<const sockaddr_in*>(remoteAddress);

			memcpy(id + pos, &address4->sin_addr, sizeof(address4->sin_addr));
			pos += sizeof(address4->sin_addr);

			memcpy(id + pos,
				&reinterpret_cast<const sockaddr_in*>(localAddress)->sin_addr,
				sizeof(address4->sin_addr));
			pos += sizeof(address4->sin_addr);

			*(uint16*)(id + pos) = address4->sin_port;
			break;

		case AF_INET6:
			address6 = reinterpret_cast<const sockaddr_in6*>(remoteAddress);

			memcpy(id + pos, &address6->sin6_addr, sizeof(address6->sin6_addr));
			pos += sizeof(address6->sin6_addr);

			memcpy(id + pos,
				&reinterpret_cast<const sockaddr_in6*>(localAddress)->sin6_addr,
				sizeof(address6->sin6_addr));
			pos += sizeof(address6->sin6_addr);

			*(uint16*)(id + pos) = address6->sin6_port;
			break;

		default:
			return B_BAD_VALUE;
	}
	pos += sizeof(uint16);

	*(uint16*)(id + pos) = server->ID().fProtocol;
	pos += sizeof(uint16);
	
	stream.AddOpaque(id, pos);

	return B_OK;
}


status_t
RequestBuilder::SetClientIDConfirm(uint64 id, uint64 ver)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpSetClientIDConfirm);
	fRequest->Stream().AddUHyper(id);
	fRequest->Stream().AddUHyper(ver);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Verify(AttrValue* attr, uint32 count)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpVerify);
	_EncodeAttrs(fRequest->Stream(), attr, count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Write(const uint32* id, uint32 stateSeq, const void* buffer,
	uint64 pos, uint32 len)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpWrite);
	fRequest->Stream().AddUInt(stateSeq);
	fRequest->Stream().AddUInt(id[0]);
	fRequest->Stream().AddUInt(id[1]);
	fRequest->Stream().AddUInt(id[2]);
	fRequest->Stream().AddUHyper(pos);
	fRequest->Stream().AddInt(UNSTABLE4);
	fRequest->Stream().AddOpaque(buffer, len);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::ReleaseLockOwner(OpenFileCookie* cookie, LockOwner* owner)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpReleaseLockOwner);
	_GenerateLockOwner(fRequest->Stream(), cookie, owner);

	fOpCount++;

	return B_OK;
}


RPC::Call*
RequestBuilder::Request()
{
	if (fProcedure == ProcCompound)
		fRequest->Stream().InsertUInt(fOpCountPosition, fOpCount);

	if (fRequest == NULL || fRequest->Stream().Error() == B_OK)
		return fRequest;
	else
		return NULL;
}


void
RequestBuilder::_AttrBitmap(XDR::WriteStream& stream, Attribute* attrs,
	uint32 count)
{
	// 2 is safe in NFS4, not in NFS4.1 though
	uint32 bitmap[2];
	memset(bitmap, 0, sizeof(bitmap));
	for (uint32 i = 0; i < count; i++) {
		bitmap[attrs[i] / 32] |= 1 << attrs[i] % 32;
	}

	uint32 bcount = bitmap[1] != 0 ? 2 : 1;
	stream.AddUInt(bcount);
	for (uint32 i = 0; i < bcount; i++)
		stream.AddUInt(bitmap[i]);
}


void
RequestBuilder::_EncodeAttrs(XDR::WriteStream& stream, AttrValue* attr,
	uint32 count)
{
	Attribute* attrs =
		reinterpret_cast<Attribute*>(malloc(sizeof(Attribute) * count));
	for (uint32 i = 0; i < count; i++)
		attrs[i] = static_cast<Attribute>(attr[i].fAttribute);
	_AttrBitmap(stream, attrs, count);
	free(attrs);

	uint32 i = 0;
	XDR::WriteStream str;
	if (i < count && attr[i].fAttribute == FATTR4_TYPE) {
		str.AddUInt(attr[i].fData.fValue32);
		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_SIZE) {
		str.AddUHyper(attr[i].fData.fValue64);
		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_FILEHANDLE) {
		FileHandle* fh = reinterpret_cast<FileHandle*>(attr[i].fData.fPointer);
		str.AddOpaque(fh->fData, fh->fSize);
		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_FILEID) {
		str.AddUHyper(attr[i].fData.fValue64);
		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_MODE) {
		str.AddUInt(attr[i].fData.fValue32);
		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_OWNER) {
		str.AddString(reinterpret_cast<char*>(attr[i].fData.fPointer));
		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_OWNER_GROUP) {
		str.AddString(reinterpret_cast<char*>(attr[i].fData.fPointer));
		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_TIME_ACCESS_SET) {
		str.AddInt(1);		// SET_TO_CLIENT_TIME4

		struct timespec* ts =
			reinterpret_cast<timespec*>(attr[i].fData.fPointer);
		str.AddHyper(ts->tv_sec);
		str.AddUInt(ts->tv_nsec);

		i++;
	}

	if (i < count && attr[i].fAttribute == FATTR4_TIME_MODIFY_SET) {
		str.AddInt(1);		// SET_TO_CLIENT_TIME4

		struct timespec* ts =
			reinterpret_cast<timespec*>(attr[i].fData.fPointer);
		str.AddHyper(ts->tv_sec);
		str.AddUInt(ts->tv_nsec);

		i++;
	}

	stream.AddOpaque(str);
}

