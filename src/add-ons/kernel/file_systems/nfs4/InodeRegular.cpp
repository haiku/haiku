/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"

#include <string.h>

#include <fs_cache.h>
#include <NodeMonitor.h>

#include "IdMap.h"
#include "Request.h"
#include "RootInode.h"


status_t
Inode::_ConfirmOpen(const FileHandle& fh, OpenFileCookie* cookie)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);

		RequestBuilder& req = request.Builder();

		req.PutFH(fh);
		req.OpenConfirm(cookie->fSequence++, cookie->fStateId,
			cookie->fStateSeq);

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->RemoveOpenFile(cookie);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.OpenConfirm(&cookie->fStateSeq);
		if (result != B_OK) {
			fFileSystem->RemoveOpenFile(cookie);
			return result;
		}

		break;
	} while (true);

	return B_OK;
}


status_t
Inode::Create(const char* name, int mode, int perms, OpenFileCookie* cookie,
	ino_t* id)
{
	bool confirm;
	status_t result;

	cookie->fMode = mode;
	cookie->fSequence = 0;
	cookie->fLocks = NULL;

	bool badOwner = false;
	FileHandle fh;
	do {
		cookie->fClientId = fFileSystem->NFSServer()->ClientId();

		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		cookie->fOwnerId = atomic_add64(&cookie->fLastOwnerId, 1);

		req.PutFH(fInfo.fHandle);

		AttrValue cattr[4];
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

		if (!badOwner && fFileSystem->IsAttrSupported(FATTR4_OWNER)) {
			cattr[i].fAttribute = FATTR4_OWNER;
			cattr[i].fFreePointer = true;
			cattr[i].fData.fPointer = gIdMapper->GetOwner(getuid());
			i++;
		}

		if (!badOwner && fFileSystem->IsAttrSupported(FATTR4_OWNER_GROUP)) {
			cattr[i].fAttribute = FATTR4_OWNER_GROUP;
			cattr[i].fFreePointer = true;
			cattr[i].fData.fPointer = gIdMapper->GetOwnerGroup(getgid());
			i++;
		}

		req.Open(CLAIM_NULL, cookie->fSequence++, sModeToAccess(mode),
			cookie->fClientId, OPEN4_CREATE, cookie->fOwnerId, name, cattr,
			i, (mode & O_EXCL) == O_EXCL);

		req.GetFH();

		if (fFileSystem->IsAttrSupported(FATTR4_FILEID)) {
			Attribute attr[] = { FATTR4_FILEID };
			req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));
		}

		result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (reply.NFS4Error() == NFS4ERR_BADOWNER) {
			badOwner = true;
			continue;
		}
		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		bool atomic;
		uint64 before, after;
		result = reply.Open(cookie->fStateId, &cookie->fStateSeq, &confirm,
			&before, &after, &atomic);
		if (result != B_OK)
			return result;

		reply.GetFH(&fh);

		uint64 fileId;
		if (fFileSystem->IsAttrSupported(FATTR4_FILEID)) {
			AttrValue* values;
			uint32 count;
			result = reply.GetAttr(&values, &count);
			if (result != B_OK)
				return result;

			fileId = values[0].fData.fValue64;

			delete[] values;
		} else
			fileId = fFileSystem->AllocFileId();

		*id = _FileIdToInoT(fileId);

		FileInfo fi;
		fi.fFileId = fileId;
		fi.fHandle = fh;
		fi.fParent = fInfo.fHandle;
		fi.fName = strdup(name);

		char* path = reinterpret_cast<char*>(malloc(strlen(name) + 2 +
			strlen(fInfo.fPath)));
		strcpy(path, fInfo.fPath);
		strcat(path, "/");
		strcat(path, name);
		fi.fPath = path;

		fFileSystem->InoIdMap()->AddEntry(fi, *id);

		if (fCache->Lock() == B_OK) {
			if (atomic && fCache->ChangeInfo() == before) {
				fCache->AddEntry(name, fileId, true);
				fCache->SetChangeInfo(after);
			} else if (fCache->ChangeInfo() != before)
				fCache->Trash();
			fCache->Unlock();
		}

		cookie->fFileSystem = fFileSystem;
		cookie->fInfo = fi;

		break;
	} while (true);

	fFileSystem->AddOpenFile(cookie);
	fFileSystem->Root()->MakeInfoInvalid();

	if (confirm)
		return _ConfirmOpen(fh, cookie);

	return B_OK;
}


status_t
Inode::Open(int mode, OpenFileCookie* cookie)
{
	bool confirm;
	status_t result;

	cookie->fFileSystem = fFileSystem;
	cookie->fInfo = fInfo;
	cookie->fMode = mode;
	cookie->fSequence = 0;
	cookie->fLocks = NULL;

	do {
		cookie->fClientId = fFileSystem->NFSServer()->ClientId();

		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		cookie->fOwnerId = atomic_add64(&cookie->fLastOwnerId, 1);

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
		if ((mode & O_TRUNC) == O_TRUNC) {
			AttrValue attr;
			attr.fAttribute = FATTR4_SIZE;
			attr.fFreePointer = false;
			attr.fData.fValue64 = 0;
			req.Open(CLAIM_NULL, cookie->fSequence++, sModeToAccess(mode),
				cookie->fClientId, OPEN4_CREATE, cookie->fOwnerId, fInfo.fName,
				&attr, 1, false);
		} else
		req.Open(CLAIM_NULL, cookie->fSequence++, sModeToAccess(mode),
			cookie->fClientId, OPEN4_NOCREATE, cookie->fOwnerId, fInfo.fName);

		result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		// Verify if the file we want to open is the file this Inode
		// represents.
		if (fFileSystem->IsAttrSupported(FATTR4_FILEID) ||
			fFileSystem->ExpireType() == FH4_PERSISTENT) {
			reply.PutFH();
			result = reply.LookUp();
			if (result != B_OK)
				return result;
			result = reply.Verify();
			if (result != B_OK && reply.NFS4Error() == NFS4ERR_NOT_SAME)
				return B_ENTRY_NOT_FOUND;
			else if (result != B_OK)
				return result;
		}

		reply.PutFH();
		result = reply.Open(cookie->fStateId, &cookie->fStateSeq, &confirm);
		if (result != B_OK)
			return result;

		break;
	} while (true);

	fFileSystem->AddOpenFile(cookie);

	if (confirm)
		return _ConfirmOpen(fInfo.fHandle, cookie);

	return B_OK;
}


status_t
Inode::Close(OpenFileCookie* cookie)
{
	fFileSystem->RemoveOpenFile(cookie);

	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Close(cookie->fSequence++, cookie->fStateId,
			cookie->fStateSeq);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		reply.PutFH();
		result = reply.Close();
		if (result != B_OK)
			return result;

		break;
	} while (true);

	return B_OK;
}


status_t
Inode::Read(OpenFileCookie* cookie, off_t pos, void* buffer, size_t* _length,
	bool* eof)
{
	*eof = false;
	uint32 size = 0;
	uint32 len = 0;

	while (size < *_length && !*eof) {
		do {
			RPC::Server* serv = fFileSystem->Server();
			Request request(serv);
			RequestBuilder& req = request.Builder();

			req.PutFH(fInfo.fHandle);
			req.Read(cookie->fStateId, cookie->fStateSeq, pos + size,
				*_length - size);

			status_t result = request.Send(cookie);
			if (result != B_OK)
				return result;

			ReplyInterpreter& reply = request.Reply();

			if (_HandleErrors(reply.NFS4Error(), serv, cookie))
				continue;

			reply.PutFH();
			result = reply.Read(reinterpret_cast<char*>(buffer) + size, &len,
						eof);
			if (result != B_OK)
				return result;

			size += len;

			break;
		} while (true);
	}

	*_length = size;

	return B_OK;
}


status_t
Inode::Write(OpenFileCookie* cookie, off_t pos, const void* _buffer,
	size_t *_length)
{
	uint32 size = 0;
	uint32 len = 0;
	uint64 fileSize;
	const char* buffer = reinterpret_cast<const char*>(_buffer);

	fWriteDirty = true;

	while (size < *_length) {
		do {
			RPC::Server* serv = fFileSystem->Server();
			Request request(serv);
			RequestBuilder& req = request.Builder();

			if (size == 0 && (cookie->fMode & O_APPEND) == O_APPEND) {
				struct stat st;
				status_t result = Stat(&st);
				if (result != B_OK)
					return result;

				fileSize = st.st_size;
				pos = fileSize;
			}

			req.PutFH(fInfo.fHandle);
			if ((cookie->fMode & O_APPEND) == O_APPEND) {
				AttrValue attr;
				attr.fAttribute = FATTR4_SIZE;
				attr.fFreePointer = false;
				attr.fData.fValue64 = fileSize + size;
				req.Verify(&attr, 1);
			}
			req.Write(cookie->fStateId, cookie->fStateSeq, buffer + size,
				pos + size, *_length - size);

			status_t result = request.Send(cookie);
			if (result != B_OK)
				return result;

			ReplyInterpreter& reply = request.Reply();

			// append: race condition
			if (reply.NFS4Error() == NFS4ERR_NOT_SAME) {
				if (size == 0)
					continue;
				else {
					*_length = size;
					return B_OK;
				}
			}

			if (_HandleErrors(reply.NFS4Error(), serv, cookie))
				continue;

			reply.PutFH();

			if ((cookie->fMode & O_APPEND) == O_APPEND) {
				result = reply.Verify();
				if (result != B_OK)
					return result;
			}

			result = reply.Write(&len);
			if (result != B_OK)
				return result;

			size += len;

			break;
		} while (true);
	}

	*_length = size;

	fAttrCache.st_size = max_c(fAttrCache.st_size, *_length + pos);
	fFileSystem->Root()->MakeInfoInvalid();

	return B_OK;
}


status_t
Inode::Commit()
{
	if (!fWriteDirty)
		return B_OK;

	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Commit(0, 0);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		return reply.Commit();
	} while (true);
}

