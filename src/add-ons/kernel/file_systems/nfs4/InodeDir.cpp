/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"

#include <dirent.h>
#include <string.h>

#include "IdMap.h"
#include "Request.h"
#include "RootInode.h"


status_t
Inode::CreateDir(const char* name, int mode)
{
	bool badOwner = false;

	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);

		uint32 i = 0;
		AttrValue cattr[3];
		cattr[i].fAttribute = FATTR4_MODE;
		cattr[i].fFreePointer = false;
		cattr[i].fData.fValue32 = mode;
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

		req.Create(NF4DIR, name, cattr, i);

		status_t result = request.Send();
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

		uint64 before, after;
		bool atomic;
		result = reply.Create(&before, &after, atomic);

		fFileSystem->Root()->MakeInfoInvalid();

		if (fCache->Lock() == B_OK) {
			if (atomic && fCache->ChangeInfo() == before) {
				// TODO: update cache
				//fCache->AddEntry(name, );
				fCache->SetChangeInfo(after);
			} else if (fCache->ChangeInfo() != before)
				fCache->Trash();
			fCache->Unlock();
		}

		return result;
	} while (true);
}


status_t
Inode::OpenDir(OpenDirCookie* cookie)
{
	if (fType != NF4DIR)
		return B_NOT_A_DIRECTORY;

	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Access();

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		uint32 allowed;
		result = reply.Access(NULL, &allowed);
		if (result != B_OK)
			return result;

		if (allowed & ACCESS4_READ != ACCESS4_READ)
			return B_PERMISSION_DENIED;

		cookie->fFileSystem = fFileSystem;
		cookie->fSpecial = 0;
		cookie->fSnapshot = NULL;
		cookie->fCurrent = NULL;
		cookie->fEOF = false;

		fFileSystem->Root()->MakeInfoInvalid();

		return B_OK;
	} while (true);
}


status_t
Inode::_ReadDirOnce(DirEntry** dirents, uint32* count, OpenDirCookie* cookie,
	bool* eof, uint64* change, uint64* dirCookie, uint64* dirCookieVerf)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

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

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		AttrValue* before = NULL;
		uint32 attrCount;
		if (*change == 0) {
			result = reply.GetAttr(&before, &attrCount);
			if (result != B_OK)
				return result;
		}

		result = reply.ReadDir(dirCookie, dirCookieVerf, dirents,
			count, eof);
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

		if (*change == 0 && before[0].fData.fValue64 == after[0].fData.fValue64
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
Inode::_FillDirEntry(struct dirent* de, ino_t id, const char* name, uint32 pos,
	uint32 size)
{
	uint32 nameSize = strlen(name);
	const uint32 entSize = sizeof(struct dirent);

	if (pos + entSize + nameSize > size)
		return B_BUFFER_OVERFLOW;

	de->d_dev = fFileSystem->DevId();
	de->d_ino = id;
	de->d_reclen = entSize + nameSize;
	if (de->d_reclen % 8 != 0)
		de->d_reclen += 8 - de->d_reclen % 8;

	strcpy(de->d_name, name);

	return B_OK;
}


status_t
Inode::_ReadDirUp(struct dirent* de, uint32 pos, uint32 size)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.LookUpUp();
		req.GetFH();

		if (fFileSystem->IsAttrSupported(FATTR4_FILEID)) {
			Attribute attr[] = { FATTR4_FILEID };
			req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));
		}

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.LookUpUp();
		if (result != B_OK)
			return result;

		FileHandle fh;
		reply.GetFH(&fh);

		uint64 fileId;
		if (fFileSystem->IsAttrSupported(FATTR4_FILEID)) {
			AttrValue* values;
			uint32 count;
			reply.GetAttr(&values, &count);
			if (result != B_OK)
				return result;

			fileId = values[0].fData.fValue64;
			delete[] values;
		} else
			fileId = fFileSystem->AllocFileId();

		return _FillDirEntry(de, _FileIdToInoT(fileId), "..", pos, size);
	} while (true);
}


status_t
Inode::_GetDirSnapshot(DirectoryCacheSnapshot** _snapshot,
	OpenDirCookie* cookie, uint64* _change)
{
	DirectoryCacheSnapshot* snapshot = new DirectoryCacheSnapshot;
	if (snapshot == NULL)
		return B_NO_MEMORY;

	uint64 change = 0;
	uint64 dirCookie = 0;
	uint64 dirCookieVerf = 0;
	bool eof = false;

	while (!eof) {
		uint32 count;
		DirEntry* dirents;

		status_t result = _ReadDirOnce(&dirents, &count, cookie, &eof, &change,
			&dirCookie, &dirCookieVerf);
		if (result != B_OK) {
			delete snapshot;
			return result;
		}

		uint32 i;
		for (i = 0; i < count; i++) {

			// FATTR4_FSID is mandatory
			void* data = dirents[i].fAttrs[0].fData.fPointer;
			FileSystemId* fsid = reinterpret_cast<FileSystemId*>(data);
			if (*fsid != fFileSystem->FsId())
				continue;

			ino_t id;
			if (dirents[i].fAttrCount == 2)
				id = _FileIdToInoT(dirents[i].fAttrs[1].fData.fValue64);
			else
				id = _FileIdToInoT(fFileSystem->AllocFileId());
	
			NameCacheEntry* entry = new NameCacheEntry(dirents[i].fName, id);
			if (entry == NULL || entry->fName == NULL) {
				if (entry->fName == NULL)
					delete entry;
				delete snapshot;
				delete[] dirents;
				return B_NO_MEMORY;
			}
			snapshot->fEntries.Add(entry);
		}

		delete[] dirents;
	}

	*_snapshot = snapshot;
	*_change = change;

	return B_OK;
}


status_t
Inode::ReadDir(void* _buffer, uint32 size, uint32* _count,
	OpenDirCookie* cookie)
{
	if (cookie->fEOF) {
		*_count = 0;
		return B_OK;
	}

	status_t result;
	if (cookie->fSnapshot == NULL) {
		fFileSystem->Revalidator().Lock();
		if (fCache->Lock() != B_OK) {
			fCache->ResetAndLock();
		} else {
			fFileSystem->Revalidator().RemoveDirectory(fCache);
		}

		cookie->fSnapshot = fCache->GetSnapshot();
		if (cookie->fSnapshot == NULL) {
			uint64 change;
			result = _GetDirSnapshot(&cookie->fSnapshot, cookie, &change);
			if (result != B_OK) {
				fCache->Unlock();
				fFileSystem->Revalidator().Unlock();
				return result;
			}
			fCache->ValidateChangeInfo(change);
			fCache->SetSnapshot(cookie->fSnapshot);
		}
		cookie->fSnapshot->AcquireReference();
		fFileSystem->Revalidator().AddDirectory(fCache);
		fCache->Unlock();
		fFileSystem->Revalidator().Unlock();
	}

	char* buffer = reinterpret_cast<char*>(_buffer);
	uint32 pos = 0;
	uint32 i = 0;
	bool overflow = false;

	if (cookie->fSpecial == 0 && i < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);

		status_t result;
		result = _FillDirEntry(de, fInfo.fFileId, ".", pos, size);

		if (result == B_BUFFER_OVERFLOW)
			overflow = true;
		else if (result == B_OK) {
			pos += de->d_reclen;
			i++;
			cookie->fSpecial++;
		} else
			return result;
	}

	if (cookie->fSpecial == 1 && i < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);
		
		status_t result;
		if (strcmp(fInfo.fName, "/"))
			result = _ReadDirUp(de, pos, size);
		else
			result = _FillDirEntry(de, _FileIdToInoT(fInfo.fFileId), "..", pos, size);

		if (result == B_BUFFER_OVERFLOW)
			overflow = true;
		else if (result == B_OK) {
			pos += de->d_reclen;
			i++;
			cookie->fSpecial++;
		} else
			return result;
	}

	MutexLocker _(cookie->fSnapshot->fLock);
	for (; !overflow && i < *_count; i++) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);

		if (cookie->fCurrent == NULL)
			cookie->fCurrent = cookie->fSnapshot->fEntries.Head();
		else {
			cookie->fCurrent
				= cookie->fSnapshot->fEntries.GetNext(cookie->fCurrent);
		}

		if (cookie->fCurrent == NULL) {
			cookie->fEOF = true;
			break;
		}

		if (_FillDirEntry(de, cookie->fCurrent->fNode, cookie->fCurrent->fName,
			pos, size) == B_BUFFER_OVERFLOW) {
			overflow = true;
			break;
		}

		pos += de->d_reclen;
	}

	if (i == 0 && overflow)
		return B_BUFFER_OVERFLOW;

	*_count = i;

	return B_OK;
}

