/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "IdMap.h"
#include "Inode.h"
#include "NFS4Inode.h"
#include "Request.h"


status_t
NFS4Inode::GetChangeInfo(uint64* change, bool attrDir)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		if (attrDir)
			req.PutFH(fInfo.fAttrDir);
		else
			req.PutFH(fInfo.fHandle);

		Attribute attr[] = { FATTR4_CHANGE };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK || count < 1)
			return result;

		// FATTR4_CHANGE is mandatory
		*change = values[0].fData.fValue64;
		delete[] values;

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::CommitWrites()
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Commit(0, 0);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		return reply.Commit();
	} while (true);
}


status_t
NFS4Inode::Access(uint32* allowed)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Access();

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		return reply.Access(NULL, allowed);
	} while (true);
}


status_t
NFS4Inode::LookUp(const char* name, uint64* change, uint64* fileID,
	FileHandle* handle, bool parent)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		if (parent)
			req.PutFH(fInfo.fParent);
		else
			req.PutFH(fInfo.fHandle);

		if (change != NULL) {
			Attribute dirAttr[] = { FATTR4_CHANGE };
			req.GetAttr(dirAttr, sizeof(dirAttr) / sizeof(Attribute));
		}

		if (!strcmp(name, ".."))
			req.LookUpUp();
		else
			req.LookUp(name);

		req.GetFH();

		Attribute attr[] = { FATTR4_FSID, FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		AttrValue* values;
		uint32 count;
		if (change != NULL) {
			result = reply.GetAttr(&values, &count);
			if (result != B_OK)
				return result;

			*change = values[0].fData.fValue64;
			delete[] values;
		}

		if (!strcmp(name, ".."))
			result = reply.LookUpUp();
		else
			result = reply.LookUp();
		if (result != B_OK)
			return result;

		reply.GetFH(handle);

		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		// FATTR4_FSID is mandatory
		FileSystemId* fsid =
			reinterpret_cast<FileSystemId*>(values[0].fData.fPointer);
		if (*fsid != fFileSystem->FsId()) {
			delete[] values;
			return B_ENTRY_NOT_FOUND;
		}

		if (fileID != NULL) {
			if (count < 2 || values[1].fAttribute != FATTR4_FILEID)
				*fileID = fFileSystem->AllocFileId();
			else
				*fileID = values[1].fData.fValue64;
		}

		delete[] values;

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::Link(Inode* dir, const char* name, ChangeInfo* changeInfo)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.SaveFH();
		req.PutFH(dir->fInfo.fHandle);
		req.Link(name);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		reply.SaveFH();
		reply.PutFH();

		return reply.Link(&changeInfo->fBefore, &changeInfo->fAfter,
			changeInfo->fAtomic);
	} while (true);
}


status_t
NFS4Inode::ReadLink(void* buffer, size_t* length)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.ReadLink();	

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		uint32 size;
		result = reply.ReadLink(buffer, &size, *length);
		*length = static_cast<size_t>(size);

		return result;
	} while (true);
}


status_t
NFS4Inode::GetStat(AttrValue** values, uint32* count, OpenAttrCookie* cookie)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		if (cookie != NULL)
			req.PutFH(cookie->fOpenState->fInfo.fHandle);
		else
			req.PutFH(fInfo.fHandle);

		Attribute attr[] = { FATTR4_SIZE, FATTR4_MODE, FATTR4_NUMLINKS,
							FATTR4_OWNER, FATTR4_OWNER_GROUP,
							FATTR4_TIME_ACCESS, FATTR4_TIME_CREATE,
							FATTR4_TIME_METADATA, FATTR4_TIME_MODIFY };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send(cookie);
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		return reply.GetAttr(values, count);
	} while (true);
}


status_t
NFS4Inode::WriteStat(OpenState* state, AttrValue* attrs, uint32 attrCount)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		if (state != NULL) {
			req.PutFH(state->fInfo.fHandle);
			req.SetAttr(state->fStateID, state->fStateSeq, attrs, attrCount);
		} else {
			req.PutFH(fInfo.fHandle);
			req.SetAttr(NULL, 0, attrs, attrCount);
		}

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.SetAttr();

		if (result != B_OK)
			return result;

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::RenameNode(Inode* from, Inode* to, const char* fromName,
	const char* toName, ChangeInfo* fromChange, ChangeInfo* toChange,
	uint64* fileID, bool attribute)
{
	do {
		RPC::Server* serv = from->fFileSystem->Server();
		Request request(serv, from->fFileSystem);
		RequestBuilder& req = request.Builder();

		if (attribute)
			req.PutFH(from->fInfo.fAttrDir);
		else
			req.PutFH(from->fInfo.fHandle);
		req.SaveFH();
		if (attribute)
			req.PutFH(to->fInfo.fAttrDir);
		else
			req.PutFH(to->fInfo.fHandle);
		req.Rename(fromName, toName);

		if (!attribute) {
			req.LookUp(toName);
			Attribute attr[] = { FATTR4_FILEID };
			req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));
		}

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// FileHandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			from->fInfo.UpdateFileHandles(from->fFileSystem);
			to->fInfo.UpdateFileHandles(to->fFileSystem);
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			from->fFileSystem->Migrate(serv);
			continue;
		}

		reply.PutFH();
		reply.SaveFH();
		reply.PutFH();

		result = reply.Rename(&fromChange->fBefore, &fromChange->fAfter,
			fromChange->fAtomic, &toChange->fBefore, &toChange->fAfter,
			toChange->fAtomic);
		if (result != B_OK)
			return result;

		if (!attribute) {
			result = reply.LookUp();
			if (result != B_OK)
				return result;

			AttrValue* values;
			uint32 count;
			result = reply.GetAttr(&values, &count);
			if (result != B_OK)
				return result;

			if (count == 0)
				*fileID = from->fFileSystem->AllocFileId();
			else
				*fileID = values[0].fData.fValue64;

			delete[] values;
		}

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::CreateFile(const char* name, int mode, int perms, OpenState* state,
	ChangeInfo* changeInfo, uint64* fileID, FileHandle* handle,
	OpenDelegationData* delegation)
{
	bool confirm;
	status_t result;

	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		state->fClientID = fFileSystem->NFSServer()->ClientId();

		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);

		AttrValue cattr[2];
		uint32 i = 0;
		if ((mode & O_TRUNC) == O_TRUNC) {
			cattr[i].fAttribute = FATTR4_SIZE;
			cattr[i].fFreePointer = false;
			cattr[i].fData.fValue64 = 0;
			i++;
		}
		cattr[i].fAttribute = FATTR4_MODE;
		cattr[i].fFreePointer = false;
		cattr[i].fData.fValue32 = perms;
		i++;

		req.Open(CLAIM_NULL, sequence, sModeToAccess(mode),
			state->fClientID, OPEN4_CREATE, fFileSystem->OpenOwner(), name,
			cattr, i, (mode & O_EXCL) == O_EXCL);

		req.GetFH();

		if (fFileSystem->IsAttrSupported(FATTR4_FILEID)) {
			Attribute attr[] = { FATTR4_FILEID };
			req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));
		}

		result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		sequence += IncrementSequence(reply.NFS4Error());

		if (HandleErrors(reply.NFS4Error(), serv, NULL, state, &sequence))
			continue;

		reply.PutFH();

		result = reply.Open(state->fStateID, &state->fStateSeq, &confirm,
			delegation, changeInfo);
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		reply.GetFH(handle);

		if (fFileSystem->IsAttrSupported(FATTR4_FILEID)) {
			AttrValue* values;
			uint32 count;
			result = reply.GetAttr(&values, &count);
			if (result != B_OK) {
				fFileSystem->OpenOwnerSequenceUnlock(sequence);
				return result;
			}

			*fileID = values[0].fData.fValue64;

			delete[] values;
		} else
			*fileID = fFileSystem->AllocFileId();

		break;
	} while (true);

	state->fOpened = true;

	if (confirm)
		result = ConfirmOpen(*handle, state, &sequence);

	fFileSystem->OpenOwnerSequenceUnlock(sequence);
	return result;
}


status_t
NFS4Inode::OpenFile(OpenState* state, int mode, OpenDelegationData* delegation)
{
	bool confirm;
	status_t result;
	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		state->fClientID = fFileSystem->NFSServer()->ClientId();

		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		// Since we are opening the file using a pair (parentFH, name) we
		// need to check for race conditions.
		if (fFileSystem->IsAttrSupported(FATTR4_FILEID)) {
			req.PutFH(fInfo.fParent);
			req.LookUp(fInfo.fName);
			AttrValue attr;
			attr.fAttribute = FATTR4_FILEID;
			attr.fFreePointer = false;
			attr.fData.fValue64 = fInfo.fFileId;
			req.Verify(&attr, 1);
		} else if (fFileSystem->ExpireType() == FH4_PERSISTENT) {
			req.PutFH(fInfo.fParent);
			req.LookUp(fInfo.fName);
			AttrValue attr;
			attr.fAttribute = FATTR4_FILEHANDLE;
			attr.fFreePointer = true;
			attr.fData.fPointer = malloc(sizeof(fInfo.fHandle));
			memcpy(attr.fData.fPointer, &fInfo.fHandle, sizeof(fInfo.fHandle));
			req.Verify(&attr, 1);
		}

		req.PutFH(fInfo.fParent);
		req.Open(CLAIM_NULL, sequence, sModeToAccess(mode), state->fClientID,
			OPEN4_NOCREATE, fFileSystem->OpenOwner(), fInfo.fName);
		req.GetFH();

		result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		sequence += IncrementSequence(reply.NFS4Error());

		if (HandleErrors(reply.NFS4Error(), serv, NULL, state, &sequence))
			continue;

		// Verify if the file we want to open is the file this Inode
		// represents.
		if (fFileSystem->IsAttrSupported(FATTR4_FILEID) ||
			fFileSystem->ExpireType() == FH4_PERSISTENT) {
			reply.PutFH();
			result = reply.LookUp();
			if (result != B_OK) {
				fFileSystem->OpenOwnerSequenceUnlock(sequence);
				return result;
			}

			result = reply.Verify();
			if (result != B_OK) {
				fFileSystem->OpenOwnerSequenceUnlock(sequence);
				return result;
			}

			if (result != B_OK && reply.NFS4Error() == NFS4ERR_NOT_SAME)
				return B_ENTRY_NOT_FOUND;
			else if (result != B_OK)
				return result;
		}

		reply.PutFH();
		result = reply.Open(state->fStateID, &state->fStateSeq, &confirm,
			delegation);

		FileHandle handle;
		reply.GetFH(&handle);

		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		break;
	} while (true);

	state->fOpened = true;

	if (confirm)
		result =  ConfirmOpen(fInfo.fHandle, state, &sequence);

	fFileSystem->OpenOwnerSequenceUnlock(sequence);
	return result;
}


status_t
NFS4Inode::OpenAttr(OpenState* state, const char* name, int mode,
	OpenDelegationData* delegation, bool create)
{
	bool confirm;
	status_t result;
	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		state->fClientID = fFileSystem->NFSServer()->ClientId();

		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fAttrDir);
		req.Open(CLAIM_NULL, sequence, sModeToAccess(mode), state->fClientID,
			create ? OPEN4_CREATE : OPEN4_NOCREATE, fFileSystem->OpenOwner(),
			name);
		req.GetFH();

		result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		sequence += IncrementSequence(reply.NFS4Error());

		if (HandleErrors(reply.NFS4Error(), serv, NULL, state, &sequence))
			continue;

		reply.PutFH();
		result = reply.Open(state->fStateID, &state->fStateSeq, &confirm,
			delegation);

		reply.GetFH(&state->fInfo.fHandle);

		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		break;
	} while (true);

	state->fOpened = true;

	if (confirm)
		result = ConfirmOpen(state->fInfo.fHandle, state, &sequence);

	fFileSystem->OpenOwnerSequenceUnlock(sequence);
	return result;
}


status_t
NFS4Inode::ReadFile(OpenStateCookie* cookie, OpenState* state, uint64 position,
	uint32* length, void* buffer, bool* eof)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(state->fInfo.fHandle);
		req.Read(state->fStateID, state->fStateSeq, position, *length);

		status_t result = request.Send(cookie);
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv, cookie, state))
			continue;

		reply.PutFH();
		result = reply.Read(buffer, length, eof);
		if (result != B_OK)
			return result;

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::WriteFile(OpenStateCookie* cookie, OpenState* state, uint64 position,
	uint32* length, const void* buffer, bool commit)
{

	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(state->fInfo.fHandle);

		req.Write(state->fStateID, state->fStateSeq, buffer, position, *length,
			commit);

		status_t result = request.Send(cookie);
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv, cookie, state))
			continue;

		reply.PutFH();

		result = reply.Write(length);
		if (result != B_OK)
			return result;

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::CreateObject(const char* name, const char* path, int mode,
	FileType type, ChangeInfo* changeInfo, uint64* fileID, FileHandle* handle,
	bool parent)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		if (parent)
			req.PutFH(fInfo.fParent);
		else
			req.PutFH(fInfo.fHandle);

		uint32 i = 0;
		AttrValue cattr[1];
		cattr[i].fAttribute = FATTR4_MODE;
		cattr[i].fFreePointer = false;
		cattr[i].fData.fValue32 = mode;
		i++;

		switch (type) {
			case NF4DIR:
				req.Create(NF4DIR, name, cattr, i);
				break;
			case NF4LNK:
				req.Create(NF4LNK, name, cattr, i, path);
				break;
			default:
				return B_BAD_VALUE;
		}

		req.GetFH();

		if (fileID != NULL) {
			Attribute attr[] = { FATTR4_FILEID };
			req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));
		}

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.Create(&changeInfo->fBefore, &changeInfo->fAfter,
			changeInfo->fAtomic);
		if (result != B_OK)
			return result;

		result = reply.GetFH(handle);
		if (result != B_OK)
			return result;

		if (fileID != NULL) {
			AttrValue* values;
			uint32 count;
			result = reply.GetAttr(&values, &count);
			if (result != B_OK)
				return result;

			if (count == 0)
				*fileID = fFileSystem->AllocFileId();
			else
				*fileID = values[0].fData.fValue64;

			delete[] values;
		}

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::RemoveObject(const char* name, FileType type, ChangeInfo* changeInfo,
	uint64* fileID)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.LookUp(name);
		AttrValue attr;
		attr.fAttribute = FATTR4_TYPE;
		attr.fFreePointer = false;
		attr.fData.fValue32 = NF4DIR;
		if (type == NF4DIR)
			req.Verify(&attr, 1);
		else
			req.Nverify(&attr, 1);

		if (type != NF4NAMEDATTR) {
			Attribute idAttr[] = { FATTR4_FILEID };
			req.GetAttr(idAttr, sizeof(idAttr) / sizeof(Attribute));
		}

		req.PutFH(fInfo.fHandle);
		req.Remove(name);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.LookUp();
		if (result != B_OK)
			return result;

		if (type == NF4DIR)
			result = reply.Verify();
		else
			result = reply.Nverify();

		if (result == NFS4ERR_SAME && type != NF4DIR)
			return B_IS_A_DIRECTORY;
		if (result == NFS4ERR_NOT_SAME && type == NF4DIR)
			return B_NOT_A_DIRECTORY;
		if (result != B_OK)
			return result;

		if (type != NF4NAMEDATTR) {
			AttrValue* values;
			uint32 count;
			result = reply.GetAttr(&values, &count);
			if (result != B_OK)
				return result;

			if (count == 0)
				*fileID = fFileSystem->AllocFileId();
			else
				*fileID = values[0].fData.fValue64;
			delete[] values;
		}

		reply.PutFH();
		return reply.Remove(&changeInfo->fBefore, &changeInfo->fAfter,
			changeInfo->fAtomic);
	} while (true);
}


status_t
NFS4Inode::ReadDirOnce(DirEntry** dirents, uint32* count, OpenDirCookie* cookie,
	bool* eof, uint64* change, uint64* dirCookie, uint64* dirCookieVerf,
	bool attribute)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		if (attribute)
			req.PutFH(fInfo.fAttrDir);
		else
			req.PutFH(fInfo.fHandle);

		Attribute dirAttr[] = { FATTR4_CHANGE };
		if (*change == 0)
			req.GetAttr(dirAttr, sizeof(dirAttr) / sizeof(Attribute));

		Attribute attr[] = { FATTR4_FSID, FATTR4_FILEID };
		req.ReadDir(*count, *dirCookie, *dirCookieVerf, attr,
			sizeof(attr) / sizeof(Attribute));

		req.GetAttr(dirAttr, sizeof(dirAttr) / sizeof(Attribute));

		status_t result = request.Send(cookie);
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		AttrValue* before = NULL;
		uint32 attrCount;
		if (*change == 0) {
			result = reply.GetAttr(&before, &attrCount);
			if (result != B_OK)
				return result;
		}

		result = reply.ReadDir(dirCookie, dirCookieVerf, dirents, count, eof);
		if (result != B_OK) {
			delete[] before;
			return result;
		}

		AttrValue* after;
		result = reply.GetAttr(&after, &attrCount);
		if (result != B_OK) {
			delete[] before;
			return result;
		}

		if ((*change == 0
				&& before[0].fData.fValue64 == after[0].fData.fValue64)
			|| *change == after[0].fData.fValue64)
			*change = after[0].fData.fValue64;
		else
			return B_ERROR;

		delete[] before;
		delete[] after;

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::OpenAttrDir(FileHandle* handle)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.OpenAttrDir(true);
		req.GetFH();

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.OpenAttrDir();
		if (result != B_OK)
			return result;

		return reply.GetFH(handle);
	} while (true);
}


status_t
NFS4Inode::TestLock(OpenFileCookie* cookie, LockType* type, uint64* position,
	uint64* length, bool& conflict)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.LockT(*type, *position, *length, cookie->fOpenState);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter &reply = request.Reply();
		if (HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		reply.PutFH();
		result = reply.LockT(position, length, type);
		if (reply.NFS4Error() == NFS4ERR_DENIED) {
			conflict = true;
			result = B_OK;
		} else if (reply.NFS4Error() == NFS4_OK)
			conflict = false;

		return result;
	} while (true);

	return B_OK;
}


status_t
NFS4Inode::AcquireLock(OpenFileCookie* cookie, LockInfo* lockInfo, bool wait)
{
	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		MutexLocker ownerLocker(lockInfo->fOwner->fLock);

		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Lock(cookie->fOpenState, lockInfo, sequence);

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		ReplyInterpreter &reply = request.Reply();

		sequence += IncrementSequence(reply.NFS4Error());

		reply.PutFH();
		result = reply.Lock(lockInfo);

		ownerLocker.Unlock();
		if (wait && reply.NFS4Error() == NFS4ERR_DENIED) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			snooze_etc(sSecToBigTime(5), B_SYSTEM_TIMEBASE,
				B_RELATIVE_TIMEOUT);
			sequence = fFileSystem->OpenOwnerSequenceLock();
			continue;
		}
		if (HandleErrors(reply.NFS4Error(), serv, cookie, NULL, &sequence))
			continue;

		fFileSystem->OpenOwnerSequenceUnlock(sequence);
		if (result != B_OK)
			return result;

		return B_OK;
	} while (true);
}


status_t
NFS4Inode::ReleaseLock(OpenFileCookie* cookie, LockInfo* lockInfo)
{
	do {
		MutexLocker ownerLocker(lockInfo->fOwner->fLock);

		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.LockU(lockInfo);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter &reply = request.Reply();

		reply.PutFH();
		result = reply.LockU(lockInfo);

		ownerLocker.Unlock();
		if (HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		if (result != B_OK)
			return result;

		return B_OK;
	} while (true);
}

