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

#include <NodeMonitor.h>

#include "Request.h"


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

		if (inode->_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

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

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		if (!strcmp(name, ".."))
			result = reply.LookUpUp();
		else
			result = reply.LookUp();
		if (result != B_OK)
			return result;

		Filehandle fh;
		reply.GetFH(&fh);

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
Inode::Link(Inode* dir, const char* name)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.SaveFH();
		req.PutFH(dir->fHandle);
		req.Link(name);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			_LookUpFilehandle();
			dir->_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(fHandle, serv);
			dir->fFilesystem->Migrate(dir->fHandle, serv);
			continue;
		}

		reply.PutFH();
		reply.SaveFH();
		reply.PutFH();

		return reply.Link();
	} while (true);
}


// May cause problem similar to Rename (described below). When node's is has
// more than one hard link and we delete the name it stores for filehandle
// restoration node will inocorectly become unavailable.
status_t
Inode::Remove(const char* name, FileType type)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.LookUp(name);
		AttrValue attr;
		attr.fAttribute = FATTR4_TYPE;
		attr.fFreePointer = false;
		attr.fData.fValue32 = NF4DIR;
		if (type == NF4DIR)
			req.Verify(&attr, 1);
		else
			req.Nverify(&attr, 1);

		req.PutFH(fHandle);
		req.Remove(name);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
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

		reply.PutFH();
		return reply.Remove();
	} while (true);
}


// Rename may cause some problems if filehandles are volatile and local Inode
// object exists for renamed node. It's stored filename will become invalid
// and, consequnetly, filehandle restoration will fail. Probably, it will
// be much easier to solve this problem if more metadata is cached.
status_t
Inode::Rename(Inode* from, Inode* to, const char* fromName, const char* toName)
{
	if (from->fFilesystem != to->fFilesystem)
		return B_DONT_DO_THAT;

	do {
		RPC::Server* serv = from->fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(from->fHandle);
		req.SaveFH();
		req.PutFH(to->fHandle);
		req.Rename(fromName, toName);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			from->_LookUpFilehandle();
			to->_LookUpFilehandle();
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			from->fFilesystem->Migrate(from->fHandle, serv);
			to->fFilesystem->Migrate(to->fHandle, serv);
			continue;
		}

		reply.PutFH();
		reply.SaveFH();
		reply.PutFH();

		return reply.Rename();
	} while (true);
}


status_t
Inode::CreateLink(const char* name, const char* path, int mode)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);

		AttrValue attr;
		attr.fAttribute = FATTR4_MODE;
		attr.fFreePointer = false;
		attr.fData.fValue32 = mode;
		req.Create(NF4LNK, name, &attr, 1, path);

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

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

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

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

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

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

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
			request.Reply().PutFH();
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
Inode::WriteStat(const struct stat* st, uint32 mask)
{
	status_t result;
	OpenFileCookie* cookie = NULL;
	AttrValue attr[4];
	uint32 i = 0;

	if ((mask & B_STAT_SIZE) != 0) {
		delete cookie;
		cookie = new OpenFileCookie;
		if (cookie == NULL)
			return B_NO_MEMORY;

		result = Open(O_WRONLY, cookie);
		if (result != B_OK) {
			delete cookie;
			return result;
		}

		attr[i].fAttribute = FATTR4_SIZE;
		attr[i].fFreePointer = false;
		attr[i].fData.fValue64 = st->st_size;
		i++;
	}

	if ((mask & B_STAT_MODE) != 0) {
		attr[i].fAttribute = FATTR4_MODE;
		attr[i].fFreePointer = false;
		attr[i].fData.fValue32 = st->st_mode;
		i++;
	}

	if ((mask & B_STAT_ACCESS_TIME) != 0) {
		attr[i].fAttribute = FATTR4_TIME_ACCESS_SET;
		attr[i].fFreePointer = true;
		attr[i].fData.fPointer = malloc(sizeof(st->st_atime));
		memcpy(attr[i].fData.fPointer, &st->st_atime, sizeof(st->st_atim));
		i++;
	}

	if ((mask & B_STAT_MODIFICATION_TIME) != 0) {
		attr[i].fAttribute = FATTR4_TIME_MODIFY_SET;
		attr[i].fFreePointer = true;
		attr[i].fData.fPointer = malloc(sizeof(st->st_mtime));
		memcpy(attr[i].fData.fPointer, &st->st_mtime, sizeof(st->st_mtim));
		i++;
	}

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		if ((mask & B_STAT_SIZE) != 0)
			req.SetAttr(cookie->fStateId, cookie->fStateSeq, attr, i);
		else
			req.SetAttr(NULL, 0, attr, i);

		status_t result = request.Send();
		if (result != B_OK) {
			Close(cookie);
			delete cookie;
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.SetAttr();

		if ((mask & B_STAT_SIZE) != 0) {
			Close(cookie);
			delete cookie;
		}

		return result;
	} while (true);

	return B_OK;
}


static OpenAccess
sModeToAccess(int mode)
{
	switch (mode & O_RWMASK) {
		case O_RDONLY:
			return OPEN4_SHARE_ACCESS_READ;
		case O_WRONLY:
			return OPEN4_SHARE_ACCESS_WRITE;
		case O_RDWR:
			return OPEN4_SHARE_ACCESS_BOTH;
	}

	return OPEN4_SHARE_ACCESS_READ;
}


static status_t
sConfirmOpen(Filesystem* fs, Filehandle& fh, OpenFileCookie* cookie)
{
	Request request(fs->Server());

	RequestBuilder& req = request.Builder();

	req.PutFH(fh);
	req.OpenConfirm(cookie->fSequence++, cookie->fStateId,
		cookie->fStateSeq);

	status_t result = request.Send();
	if (result != B_OK) {
		fs->NFSServer()->RemoveOpenFile(cookie);
		return result;
	}

	ReplyInterpreter& reply = request.Reply();

	reply.PutFH();

	result = reply.OpenConfirm(&cookie->fStateSeq);
	if (result != B_OK) {
		fs->NFSServer()->RemoveOpenFile(cookie);
		return result;
	}

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

	Filehandle fh;
	do {
		cookie->fClientId = fFilesystem->NFSServer()->ClientId();

		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		cookie->fOwnerId = atomic_add64(&cookie->fLastOwnerId, 1);

		req.PutFH(fHandle);

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
		req.Open(CLAIM_NULL, cookie->fSequence++, sModeToAccess(mode),
			cookie->fClientId, OPEN4_CREATE, cookie->fOwnerId, name, cattr,
			i + 1, (mode & O_EXCL) == O_EXCL);

		req.GetFH();

		Attribute attr[] = { FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (reply.NFS4Error() == NFS4ERR_GRACE)
			fFilesystem->NFSServer()->ReleaseCID(cookie->fClientId);
		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		reply.Open(cookie->fStateId, &cookie->fStateSeq, &confirm);
		reply.GetFH(&fh);

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		uint64 fileId;
		if (count == 1 && values[1].fAttribute == FATTR4_FILEID)
			fileId = values[1].fData.fValue64;
		else
			fileId = fFilesystem->AllocFileId();
			
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

		cookie->fFilesystem = fFilesystem;
		cookie->fHandle = fh;

		break;
	} while (true);

	fFilesystem->NFSServer()->AddOpenFile(cookie);

	if (confirm)
		return sConfirmOpen(fFilesystem, fh, cookie);
	else
		return B_OK;
}


status_t
Inode::Open(int mode, OpenFileCookie* cookie)
{
	bool confirm;
	status_t result;

	cookie->fFilesystem = fFilesystem;
	cookie->fHandle = fHandle;
	cookie->fMode = mode;
	cookie->fSequence = 0;
	cookie->fLocks = NULL;

	do {
		cookie->fClientId = fFilesystem->NFSServer()->ClientId();

		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		cookie->fOwnerId = atomic_add64(&cookie->fLastOwnerId, 1);

		req.PutFH(fParentFH);

		if ((mode & O_TRUNC) == O_TRUNC) {
			AttrValue attr;
			attr.fAttribute = FATTR4_SIZE;
			attr.fFreePointer = false;
			attr.fData.fValue64 = 0;
			req.Open(CLAIM_NULL, cookie->fSequence++, sModeToAccess(mode),
				cookie->fClientId, OPEN4_CREATE, cookie->fOwnerId, fName, &attr,
				1, false);
		} else
		req.Open(CLAIM_NULL, cookie->fSequence++, sModeToAccess(mode),
			cookie->fClientId, OPEN4_NOCREATE, cookie->fOwnerId, fName);

		result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (reply.NFS4Error() == NFS4ERR_GRACE)
			fFilesystem->NFSServer()->ReleaseCID(cookie->fClientId);
		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();
		result = reply.Open(cookie->fStateId, &cookie->fStateSeq, &confirm);
		if (result != B_OK)
			return result;

		break;
	} while (true);

	fFilesystem->NFSServer()->AddOpenFile(cookie);

	if (confirm)
		return sConfirmOpen(fFilesystem, fHandle, cookie);
	else
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

		if (_HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		reply.PutFH();
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

			status_t result = request.Send(cookie);
			if (result != B_OK)
				return result;

			ReplyInterpreter& reply = request.Reply();

			if (_HandleErrors(reply.NFS4Error(), serv, cookie))
				continue;

			reply.PutFH();
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
Inode::Write(OpenFileCookie* cookie, off_t pos, const void* _buffer,
	size_t *_length)
{
	uint32 size = 0;
	uint32 len = 0;
	uint64 fileSize;
	const char* buffer = reinterpret_cast<const char*>(_buffer);

	while (size < *_length) {
		do {
			RPC::Server* serv = fFilesystem->Server();
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

			req.PutFH(fHandle);
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

	return B_OK;
}


status_t
Inode::CreateDir(const char* name, int mode)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);

		AttrValue attr;
		attr.fAttribute = FATTR4_MODE;
		attr.fFreePointer = false;
		attr.fData.fValue32 = mode;
		req.Create(NF4DIR, name, &attr, 1);

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

		req.PutFH(fHandle);
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

		req.PutFH(fHandle);

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

		req.PutFH(fHandle);
		req.LookUpUp();
		req.GetFH();
		Attribute attr[] = { FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

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

		AttrValue* values;
		uint32 count;
		reply.GetAttr(&values, &count);
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

		_FillDirEntry(de, fFileId, ".", pos, size);

		pos += de->d_reclen;
		count++;
		cookie->fCookieVerf--;
	}

	if (cookie->fCookie == 0 && cookie->fCookieVerf == 1 && count < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);
		
		if (strcmp(fName, "/"))
			_ReadDirUp(de, pos, size);
		else
			_FillDirEntry(de, _FileIdToInoT(fFileId), "..", pos, size);

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


status_t
Inode::TestLock(OpenFileCookie* cookie, struct flock* lock)
{
	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.LockT(sGetLockType(lock->l_type, false), lock->l_start,
			lock->l_len, cookie);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter &reply = request.Reply();
		if (_HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		reply.PutFH();
		LockType ltype;
		uint64 pos, len;
		result = reply.LockT(&pos, &len, &ltype);
		if (reply.NFS4Error() == NFS4ERR_DENIED) {
			lock->l_type = sLockTypeToHaiku(ltype);
			lock->l_start = static_cast<off_t>(pos);
			lock->l_len = static_cast<off_t>(len);
			result = B_OK;
		} else if (reply.NFS4Error() == NFS4_OK)
			lock->l_type = F_UNLCK;

		return result;
	} while (true);

	return B_OK;
}


status_t
Inode::AcquireLock(OpenFileCookie* cookie, const struct flock* lock,
	bool wait)
{
	LockInfo* linfo = new LockInfo;
	if (linfo == NULL)
		return B_NO_MEMORY;

	linfo->fSequence = 0;
	linfo->fStart = lock->l_start;
	if (lock->l_len == OFF_MAX)
		linfo->fLength = UINT64_MAX;
	else
		linfo->fLength = lock->l_len;
	linfo->fType = sGetLockType(lock->l_type, wait);
	linfo->fOwner = find_thread(NULL);

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.Lock(cookie, linfo);

		status_t result = request.Send();
		if (result != B_OK) {
			delete linfo;
			return result;
		}

		ReplyInterpreter &reply = request.Reply();
		if (wait && reply.NFS4Error() == NFS4ERR_DENIED) {
			snooze_etc(5 * 1000000, B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
			continue;
		}

		if (_HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		reply.PutFH();
		result = reply.Lock(linfo);
		if (result != B_OK) {
			delete linfo;
			return result;
		}

		break;
	} while (true);

	mutex_lock(&cookie->fLocksLock);
	linfo->fNext = cookie->fLocks;
	cookie->fLocks = linfo;
	mutex_unlock(&cookie->fLocksLock);

	return B_OK;
}


status_t
Inode::ReleaseLock(OpenFileCookie* cookie, const struct flock* lock)
{
	LockInfo* prev = NULL;
	uint32 owner = find_thread(NULL);

	mutex_lock(&cookie->fLocksLock);
	LockInfo* linfo = cookie->fLocks;
	while (linfo != NULL) {
		if (linfo->fOwner == owner &&
			linfo->fStart == static_cast<uint64>(lock->l_start) &&
			(linfo->fLength == static_cast<uint64>(lock->l_len) ||
				(linfo->fLength == UINT64_MAX && lock->l_len == OFF_MAX))) {
			if (prev != NULL)
				prev->fNext = linfo->fNext;
			else
				cookie->fLocks = linfo->fNext;
			break;
		}

		prev = linfo;
		linfo = linfo->fNext;
	}
	mutex_unlock(&cookie->fLocksLock);

	if (linfo == NULL)
		return B_BAD_VALUE;

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fHandle);
		req.LockU(linfo);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter &reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		reply.PutFH();
		result = reply.LockU();
		if (result != B_OK)
			return result;

		break;
	} while (true);

	delete linfo;

	return B_OK;
}


status_t
Inode::ReleaseAllLocks(OpenFileCookie* cookie)
{
	mutex_lock(&cookie->fLocksLock);
	while (cookie->fLocks != NULL) {
		do {
			RPC::Server* serv = fFilesystem->Server();
			Request request(serv);
			RequestBuilder& req = request.Builder();

			req.PutFH(fHandle);
			req.LockU(cookie->fLocks);

			status_t result = request.Send();
			if (result != B_OK) {
				mutex_unlock(&cookie->fLocksLock);
				return result;
			}

			ReplyInterpreter &reply = request.Reply();

			if (_HandleErrors(reply.NFS4Error(), serv, cookie))
				continue;

			reply.PutFH();
			result = reply.LockU();
			if (result != B_OK) {
				mutex_unlock(&cookie->fLocksLock);
				return result;
			}

			break;
		} while (true);

		LockInfo* linfo = cookie->fLocks->fNext;
		delete cookie->fLocks;
		cookie->fLocks = linfo;
	}
	mutex_unlock(&cookie->fLocksLock);

	return B_OK;
}



bool
Inode::_HandleErrors(uint32 nfs4Error, RPC::Server* serv,
	OpenFileCookie* cookie)
{
	switch (nfs4Error) {
		case NFS4_OK:
			return false;

		// server needs more time, we need to wait
		case NFS4ERR_LOCKED:
		case NFS4ERR_DELAY:
			if (cookie == NULL || (cookie->fMode & O_NONBLOCK) == 0) {
				snooze_etc(5 * 1000000, B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
				return true;
			} else
				return false;

		// server is in grace period, we need to wait
		case NFS4ERR_GRACE:
			if (cookie == NULL || (cookie->fMode & O_NONBLOCK) == 0) {
				snooze_etc(fFilesystem->NFSServer()->LeaseTime() / 3,
					B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
				return true;
			} else
				return false;

		// server has rebooted, reclaim share and try again
		case NFS4ERR_STALE_CLIENTID:
		case NFS4ERR_STALE_STATEID:
			fFilesystem->NFSServer()->ServerRebooted(cookie->fClientId);
			return true;

		// filehandle has expired
		case NFS4ERR_FHEXPIRED:
			_LookUpFilehandle();
			return true;

		// filesystem has been moved
		case NFS4ERR_MOVED:
			fFilesystem->Migrate(fHandle, serv);
			return true;

		// lease has been moved, provoke server to return NFS4ERR_MOVED
		case NFS4ERR_LEASE_MOVED:
			Access(0);
			return true;

		default:
			return false;
	}
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

	reply.PutRootFH();
	for (uint32 i = 0; i < lookupCount; i++)
		reply.LookUp();

	return reply.GetFH(&fHandle);
}

