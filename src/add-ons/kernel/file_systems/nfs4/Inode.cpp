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

#include "Request.h"


vint64 OpenFileCookie::fLastOwnerId = 0;


Inode::Inode()
{
}


status_t
Inode::CreateInode(Filesystem* fs, const FileInfo &fi, Inode** _inode)
{
	Inode* inode = new(std::nothrow) Inode;
	if (inode == NULL)
		return B_NO_MEMORY;

	inode->fHandle = fi.fFH;
	inode->fFilesystem = fs;
	inode->fParentFH = fi.fParent;
	inode->fName = strdup(fi.fName);
	inode->fPath = strdup(fi.fPath);

	do {
		RPC::Server* serv = fs->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(inode->fHandle);

		Attribute attr[] = { FATTR4_TYPE, FATTR4_FSID, FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			inode->_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fs->Migrate(inode->fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK || count < 2)
			return result;

		if (fi.fFileId == 0) {
			if (count < 3 || values[2].fAttribute != FATTR4_FILEID)
				inode->fFileId = fs->AllocFileId();
			else
				inode->fFileId = values[2].fData.fValue64;
		} else
			inode->fFileId = fi.fFileId;

		// FATTR4_TYPE is mandatory
		inode->fType = values[0].fData.fValue32;

		// FATTR4_FSID is mandatory
		FilesystemId* fsid =
			reinterpret_cast<FilesystemId*>(values[1].fData.fPointer);
		if (*fsid != fs->FsId()) {
			delete[] values;
			return B_ENTRY_NOT_FOUND;
		}

		delete[] values;

		*_inode = inode;

		return B_OK;
	} while (true);
}


Inode::~Inode()
{
	free(const_cast<char*>(fName));
}


status_t
Inode::LookUp(const char* name, ino_t* id)
{
	if (fType != NF4DIR)
		return B_NOT_A_DIRECTORY;

	if (!strcmp(name, ".")) {
		*id = ID();
		return B_OK;
	}

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);

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

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		if (!strcmp(name, ".."))
			result = reply.LookUpUp();
		else
			result = reply.LookUp();
		if (result != B_OK)
			return result;

		Filehandle fh;
		result = reply.GetFH(&fh);
		if (result != B_OK)
			return result;

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		// FATTR4_FSID is mandatory
		FilesystemId* fsid =
			reinterpret_cast<FilesystemId*>(values[0].fData.fPointer);
		if (*fsid != fFilesystem->FsId()) {
			delete[] values;
			return B_ENTRY_NOT_FOUND;
		}

		uint64 fileId;
		if (count < 2 || values[1].fAttribute != FATTR4_FILEID)
			fileId = fFilesystem->AllocFileId();
		else
			fileId = values[1].fData.fValue64;
		delete[] values;

		*id = _FileIdToInoT(fileId);

		FileInfo fi;
		fi.fFileId = fileId;
		fi.fFH = fh;
		fi.fParent = fHandle;
		fi.fName = strdup(name);

		char* path = reinterpret_cast<char*>(malloc(strlen(name) + 2 +
			strlen(fPath)));
		strcpy(path, fPath);
		strcat(path, "/");
		strcat(path, name);
		fi.fPath = path;

		fFilesystem->InoIdMap()->AddEntry(fi, *id);

		return B_OK;
	} while (true);
}


status_t
Inode::ReadLink(void* buffer, size_t* length)
{
	if (fType != NF4LNK)
		return B_BAD_VALUE;

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.ReadLink();	

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		uint32 size;
		result = reply.ReadLink(buffer, &size, *length);
		*length = static_cast<size_t>(size);

		return result;
	} while (true);
}


status_t
Inode::Access(int mode)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.Access();

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		uint32 allowed;
		result = reply.Access(NULL, &allowed);
		if (result != B_OK)
			return result;

		int acc = 0;
		if ((allowed & ACCESS4_READ) != 0)
			acc |= R_OK;

		if (fType == NF4DIR && (allowed & ACCESS4_LOOKUP) != 0)
			acc |= X_OK | R_OK;

		if (fType != NF4DIR && (allowed & ACCESS4_EXECUTE) != 0)
			acc |= X_OK;

		if ((allowed & ACCESS4_MODIFY) != 0)
			acc |= W_OK;

		if ((mode & acc) != mode)
			return B_NOT_ALLOWED;

		return B_OK;
	} while (true);
}


status_t
Inode::Stat(struct stat* st)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);

		Attribute attr[] = { FATTR4_SIZE, FATTR4_MODE, FATTR4_NUMLINKS,
							FATTR4_TIME_ACCESS, FATTR4_TIME_CREATE,
							FATTR4_TIME_METADATA, FATTR4_TIME_MODIFY };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		// FATTR4_SIZE is mandatory
		if (count < 1 || values[0].fAttribute != FATTR4_SIZE) {
			delete[] values;
			return B_BAD_VALUE;
		}
		st->st_size = values[0].fData.fValue64;

		uint32 next = 1;
		st->st_mode = Type();
		if (count >= next && values[next].fAttribute == FATTR4_MODE) {
			st->st_mode |= values[next].fData.fValue32;
			next++;
		} else {
			// Try to guess using ACCESS request
			request.Reset();
			request.Builder().PutFH(fHandle);
			request.Builder().Access();
			result = request.Send();
			if (result != B_OK)
				return result;
			result = request.Reply().PutFH();
			if (result != B_OK)
				return result;
			uint32 prvl;
			result = request.Reply().Access(NULL, &prvl);
			if (result != B_OK)
				return result;

			if ((prvl & ACCESS4_READ) != 0)
				st->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;

			if ((prvl & ACCESS4_MODIFY) != 0)
				st->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;

			if (fType == NF4DIR && (prvl & ACCESS4_LOOKUP) != 0)
				st->st_mode |= S_IXUSR | S_IXGRP | S_IXOTH;

			if (fType != NF4DIR && (prvl & ACCESS4_EXECUTE) != 0)
				st->st_mode |= S_IXUSR | S_IXGRP | S_IXOTH;
		}

		if (count >= next && values[next].fAttribute == FATTR4_NUMLINKS) {
			st->st_nlink = values[next].fData.fValue32;
			next++;
		} else
			st->st_nlink = 1;

		if (count >= next && values[next].fAttribute == FATTR4_TIME_ACCESS) {
			memcpy(&st->st_atim, values[next].fData.fPointer, sizeof(timespec));
			next++;
		} else
			memset(&st->st_atim, 0, sizeof(timespec));

		if (count >= next && values[next].fAttribute == FATTR4_TIME_CREATE) {
			memcpy(&st->st_crtim, values[next].fData.fPointer,
				sizeof(timespec));
			next++;
		} else
			memset(&st->st_crtim, 0, sizeof(timespec));

		if (count >= next && values[next].fAttribute == FATTR4_TIME_METADATA) {
			memcpy(&st->st_ctim, values[next].fData.fPointer, sizeof(timespec));
			next++;
		} else
			memset(&st->st_ctim, 0, sizeof(timespec));

		if (count >= next && values[next].fAttribute == FATTR4_TIME_MODIFY) {
			memcpy(&st->st_mtim, values[next].fData.fPointer, sizeof(timespec));
			next++;
		} else
			memset(&st->st_mtim, 0, sizeof(timespec));

		st->st_uid = 0;
		st->st_gid = 0;

		delete[] values;
		return B_OK;
	} while (true);
}


status_t
Inode::Open(int mode, OpenFileCookie* cookie)
{
	bool confirm;
	status_t result;

	cookie->fHandle = fHandle;
	cookie->fMode = mode;
	cookie->fSequence = 0;

	do {
		cookie->fClientId = fFilesystem->NFSServer()->ClientId();

		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		cookie->fOwnerId = atomic_add64(&cookie->fLastOwnerId, 1);

		req.PutFH(fParentFH);
		req.Open(CLAIM_NULL, cookie->fSequence++, OPEN4_SHARE_ACCESS_READ,
			cookie->fClientId, OPEN4_NOCREATE, cookie->fOwnerId, fName);

		result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		// server is in grace period, we need to wait
		if (reply.NFS4Error() == NFS4ERR_GRACE) {
			fFilesystem->NFSServer()->ReleaseCID(cookie->fClientId);
			snooze_etc(fFilesystem->NFSServer()->LeaseTime() / 3,
				B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		result = reply.Open(cookie->fStateId, &cookie->fStateSeq, &confirm);
		if (result != B_OK)
			return result;

		break;
	} while (true);

	fFilesystem->NFSServer()->AddOpenFile(cookie);

	if (confirm) {
		Request request(fFilesystem->Server());

		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.OpenConfirm(cookie->fSequence++, cookie->fStateId,
			cookie->fStateSeq);

		result = request.Send();
		if (result != B_OK) {
			fFilesystem->NFSServer()->RemoveOpenFile(cookie);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		result = reply.PutFH();
		if (result != B_OK) {
			fFilesystem->NFSServer()->RemoveOpenFile(cookie);
			return result;
		}

		result = reply.OpenConfirm(&cookie->fStateSeq);
		if (result != B_OK) {
			fFilesystem->NFSServer()->RemoveOpenFile(cookie);
			return result;
		}
	}

	return B_OK;
}


status_t
Inode::Close(OpenFileCookie* cookie)
{
	fFilesystem->NFSServer()->RemoveOpenFile(cookie);

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.Close(cookie->fSequence++, cookie->fStateId,
			cookie->fStateSeq);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		// server is in grace period, we need to wait
		if (reply.NFS4Error() == NFS4ERR_GRACE) {
			snooze_etc(fFilesystem->NFSServer()->LeaseTime() / 3,
				B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		result = reply.Close();
		if (result != B_OK)
			return result;

		fFilesystem->NFSServer()->ReleaseCID(cookie->fClientId);

		return B_OK;
	} while (true);

	return B_OK;
}


status_t
Inode::Read(OpenFileCookie* cookie, off_t pos, void* buffer, size_t* _length)
{
	bool eof = false;
	uint32 size = 0;
	uint32 len = 0;

	while (size < *_length && !eof) {
		do {
			RPC::Server* serv = fFilesystem->Server();
			Request request(serv);
			RequestBuilder& req = request.Builder();

			req.PutFH(fHandle);
			req.Read(cookie->fStateId, cookie->fStateSeq, pos + size,
				*_length - size);

			status_t result = request.Send();
			if (result != B_OK)
				return result;

			ReplyInterpreter& reply = request.Reply();

			// filehandle has expired
			if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
				_LookUpFilehandle();
				continue;
			}

			// filesystem has been moved
			if (reply.NFS4Error() == NFS4ERR_MOVED) {
				fFilesystem->Migrate(fHandle, serv);
				continue;
			}

			// server is in grace period, we need to wait
			if (reply.NFS4Error() == NFS4ERR_GRACE) {
				snooze_etc(fFilesystem->NFSServer()->LeaseTime() / 3,
					B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
				continue;
			}

			// server has rebooted, reclaim share and try again
			if (reply.NFS4Error() == NFS4ERR_STALE_CLIENTID ||
				reply.NFS4Error() == NFS4ERR_STALE_STATEID) {
				fFilesystem->NFSServer()->ServerRebooted(cookie->fClientId);
				continue;
			}

			result = reply.PutFH();
			if (result != B_OK)
				return result;

			result = reply.Read(reinterpret_cast<char*>(buffer) + size, &len,
						&eof);
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
Inode::OpenDir(uint64* cookie)
{
	if (fType != NF4DIR)
		return B_NOT_A_DIRECTORY;

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.Access();

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		uint32 allowed;
		result = reply.Access(NULL, &allowed);
		if (result != B_OK)
			return result;

		if (allowed & ACCESS4_READ != ACCESS4_READ)
			return B_PERMISSION_DENIED;

		cookie[0] = 0;
		cookie[1] = 2;

		return B_OK;
	} while (true);
}


status_t
Inode::_ReadDirOnce(DirEntry** dirents, uint32* count, uint64* cookie,
	bool* eof)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);

		Attribute attr[] = { FATTR4_FSID, FATTR4_FILEID };
		req.ReadDir(*count, cookie, attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		return reply.ReadDir(cookie, dirents, count, eof);
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

		req.PutFH(fHandle);
		req.LookUpUp();
		req.GetFH();
		Attribute attr[] = { FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			continue;
		}

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		result = reply.LookUpUp();
		if (result != B_OK)
			return result;

		Filehandle fh;
		result = reply.GetFH(&fh);
		if (result != B_OK)
			return result;

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		uint64 fileId;
		if (count < 1 || values[0].fAttribute != FATTR4_FILEID) {
			fileId = fFilesystem->AllocFileId();
		} else
			fileId = values[0].fData.fValue64;

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
Inode::ReadDir(void* _buffer, uint32 size, uint32* _count, uint64* cookie)
{
	uint32 count = 0;
	uint32 pos = 0;
	uint32 this_count;
	bool eof = false;

	char* buffer = reinterpret_cast<char*>(_buffer);

	if (cookie[0] == 0 && cookie[1] == 2 && count < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);

		_FillDirEntry(de, fFileId, ".", pos, size);

		pos += de->d_reclen;
		count++;
		cookie[1]--;
	}

	if (cookie[0] == 0 && cookie[1] == 1 && count < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);
		
		if (strcmp(fName, "/"))
			_ReadDirUp(de, pos, size);
		else
			_FillDirEntry(de, _FileIdToInoT(fFileId), "..", pos, size);

		pos += de->d_reclen;
		count++;
		cookie[1]--;
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


static status_t
sParsePath(RequestBuilder& req, uint32* count, const char* _path)
{
	char* path = strdup(_path);
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

		(*count)++;
	}
	free(path);

	return B_OK;
}


status_t
Inode::_LookUpFilehandle()
{
	Request request(fFilesystem->Server());
	RequestBuilder& req = request.Builder();

	req.PutRootFH();

	uint32 lookupCount = 0;

	sParsePath(req, &lookupCount, fFilesystem->Path());
	sParsePath(req, &lookupCount, fPath);

	req.GetFH();

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	result = reply.PutRootFH();
	if (result != B_OK)
		return result;

	for (uint32 i = 0; i < lookupCount; i++) {
		result = reply.LookUp();
		if (result != B_OK)
			return result;
	}

	return reply.GetFH(&fHandle);
}

