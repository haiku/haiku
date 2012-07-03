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
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);

		uint32 i = 0;
		AttrValue cattr[3];
		cattr[i].fAttribute = FATTR4_MODE;
		cattr[i].fFreePointer = false;
		cattr[i].fData.fValue32 = mode;
		i++;

		if (fFilesystem->IsAttrSupported(FATTR4_OWNER)) {
			cattr[i].fAttribute = FATTR4_OWNER;
			cattr[i].fFreePointer = true;
			cattr[i].fData.fPointer = gIdMapper->GetOwner(getuid());
			i++;
		}

		if (fFilesystem->IsAttrSupported(FATTR4_OWNER_GROUP)) {
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

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		return reply.Create();
	} while (true);
}


status_t
Inode::OpenDir(OpenDirCookie* cookie)
{
	if (fType != NF4DIR)
		return B_NOT_A_DIRECTORY;

	do {
		RPC::Server* serv = fFilesystem->Server();
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

		cookie->fFilesystem = fFilesystem;
		cookie->fCookie = 0;
		cookie->fCookieVerf = 2;

		return B_OK;
	} while (true);
}


status_t
Inode::_ReadDirOnce(DirEntry** dirents, uint32* count, OpenDirCookie* cookie,
	bool* eof)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);

		Attribute attr[] = { FATTR4_FSID, FATTR4_FILEID };
		req.ReadDir(*count, cookie->fCookie, cookie->fCookieVerf, attr,
			sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send(cookie);
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		return reply.ReadDir(&cookie->fCookie, &cookie->fCookieVerf, dirents,
			count, eof);
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

	de->d_dev = fFilesystem->DevId();
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
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.LookUpUp();
		req.GetFH();

		if (fFilesystem->IsAttrSupported(FATTR4_FILEID)) {
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

		Filehandle fh;
		reply.GetFH(&fh);

		uint64 fileId;
		if (fFilesystem->IsAttrSupported(FATTR4_FILEID)) {
			AttrValue* values;
			uint32 count;
			reply.GetAttr(&values, &count);
			if (result != B_OK)
				return result;

			fileId = values[0].fData.fValue64;
			delete[] values;
		} else
			fileId = fFilesystem->AllocFileId();

		return _FillDirEntry(de, _FileIdToInoT(fileId), "..", pos, size);
	} while (true);
}

// TODO: Currently inode numbers returned by ReadDir are virtually random.
// Apparently Haiku does not use that information (contrary to inode number
// returned by LookUp) so fixing it can wait until directory caches are
// implemented.
// When directories are cached client should store inode numbers it assigned
// to directroy entries and use them consequently.
status_t
Inode::ReadDir(void* _buffer, uint32 size, uint32* _count,
	OpenDirCookie* cookie)
{
	uint32 count = 0;
	uint32 pos = 0;
	uint32 this_count;
	bool eof = false;

	char* buffer = reinterpret_cast<char*>(_buffer);

	if (cookie->fCookie == 0 && cookie->fCookieVerf == 2 && count < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);

		_FillDirEntry(de, fInfo.fFileId, ".", pos, size);

		pos += de->d_reclen;
		count++;
		cookie->fCookieVerf--;
	}

	if (cookie->fCookie == 0 && cookie->fCookieVerf == 1 && count < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);
		
		if (strcmp(fInfo.fName, "/"))
			_ReadDirUp(de, pos, size);
		else
			_FillDirEntry(de, _FileIdToInoT(fInfo.fFileId), "..", pos, size);

		pos += de->d_reclen;
		count++;
		cookie->fCookieVerf--;
	}

	bool overflow = false;
	while (count < *_count && !eof) {
		this_count = *_count - count;
		DirEntry* dirents;

		status_t result = _ReadDirOnce(&dirents, &this_count, cookie, &eof);
		if (result != B_OK)
			return result;

		uint32 i, entries = 0;
		for (i = 0; i < min_c(this_count, *_count - count); i++) {
			struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);

			// FATTR4_FSID is mandatory
			void* data = dirents[i].fAttrs[0].fData.fPointer;
			FilesystemId* fsid = reinterpret_cast<FilesystemId*>(data);
			if (*fsid != fFilesystem->FsId())
				continue;

			ino_t id;
			if (dirents[i].fAttrCount == 2)
				id = _FileIdToInoT(dirents[i].fAttrs[1].fData.fValue64);
			else
				id = _FileIdToInoT(fFilesystem->AllocFileId());

			const char* name = dirents[i].fName;
			if (_FillDirEntry(de, id, name, pos, size) == B_BUFFER_OVERFLOW) {
				eof = true;
				overflow = true;
				break;
			}

			pos += de->d_reclen;
			entries++;
		}
		delete[] dirents;
		count += entries;
	}

	if (count == 0 && overflow)
		return B_BUFFER_OVERFLOW;

	*_count = count;
	
	return B_OK;
}

