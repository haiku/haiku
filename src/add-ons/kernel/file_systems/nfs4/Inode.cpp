/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"

#include <string.h>

#include <NodeMonitor.h>

#include "IdMap.h"
#include "Request.h"
#include "RootInode.h"


Inode::Inode()
{
}


status_t
Inode::CreateInode(Filesystem* fs, const FileInfo &fi, Inode** _inode)
{
	Inode* inode = NULL;
	if (fs->Root() == NULL)
		inode = new(std::nothrow) RootInode;
	else
		inode = new(std::nothrow) Inode;

	if (inode == NULL)
		return B_NO_MEMORY;

	inode->fInfo = fi;
	inode->fFilesystem = fs;

	do {
		RPC::Server* serv = fs->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(inode->fInfo.fHandle);

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
				inode->fInfo.fFileId = fs->AllocFileId();
			else
				inode->fInfo.fFileId = values[2].fData.fValue64;
		} else
			inode->fInfo.fFileId = fi.fFileId;

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
	free(const_cast<char*>(fInfo.fName));
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

		req.PutFH(fInfo.fHandle);

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
		fi.fHandle = fh;
		fi.fParent = fInfo.fHandle;
		fi.fName = strdup(name);

		char* path = reinterpret_cast<char*>(malloc(strlen(name) + 2 +
			strlen(fInfo.fPath)));
		strcpy(path, fInfo.fPath);
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

		req.PutFH(fInfo.fHandle);
		req.SaveFH();
		req.PutFH(dir->fInfo.fHandle);
		req.Link(name);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			fInfo.UpdateFileHandles(fFilesystem);
			dir->fInfo.UpdateFileHandles(dir->fFilesystem);
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFilesystem->Migrate(serv);
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

		req.PutFH(fInfo.fHandle);
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

		req.PutFH(from->fInfo.fHandle);
		req.SaveFH();
		req.PutFH(to->fInfo.fHandle);
		req.Rename(fromName, toName);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		// filehandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			from->fInfo.UpdateFileHandles(from->fFilesystem);
			to->fInfo.UpdateFileHandles(to->fFilesystem);
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			from->fFilesystem->Migrate(serv);
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

		req.Create(NF4LNK, name, cattr, i, path);

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

		req.PutFH(fInfo.fHandle);
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

		req.PutFH(fInfo.fHandle);

		Attribute attr[] = { FATTR4_SIZE, FATTR4_MODE, FATTR4_NUMLINKS,
							FATTR4_OWNER, FATTR4_OWNER_GROUP,
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
			request.Builder().PutFH(fInfo.fHandle);
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

		if (count >= next && values[next].fAttribute == FATTR4_OWNER) {
			char* owner = reinterpret_cast<char*>(values[next].fData.fPointer);
			st->st_uid = gIdMapper->GetUserId(owner);
			next++;
		} else
			st->st_uid = 0;

		if (count >= next && values[next].fAttribute == FATTR4_OWNER_GROUP) {
			char* group = reinterpret_cast<char*>(values[next].fData.fPointer);
			st->st_gid = gIdMapper->GetGroupId(group);
			next++;
		} else
			st->st_gid = 0;

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

		delete[] values;
		return B_OK;
	} while (true);
}


status_t
Inode::WriteStat(const struct stat* st, uint32 mask)
{
	status_t result;
	OpenFileCookie* cookie = NULL;
	AttrValue attr[6];
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

	if ((mask & B_STAT_UID) != 0) {
		attr[i].fAttribute = FATTR4_OWNER;
		attr[i].fFreePointer = true;
		attr[i].fData.fPointer = gIdMapper->GetOwner(st->st_uid);
		i++;
	}

	if ((mask & B_STAT_GID) != 0) {
		attr[i].fAttribute = FATTR4_OWNER_GROUP;
		attr[i].fFreePointer = true;
		attr[i].fData.fPointer = gIdMapper->GetOwnerGroup(st->st_gid);
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

		req.PutFH(fInfo.fHandle);
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


inline status_t
Inode::_CheckLockType(short ltype, uint32 mode)
{
	switch (ltype) {
		case F_UNLCK:
			return B_OK;

		case F_RDLCK:
			if ((mode & O_RDONLY) == 0 && (mode & O_RDWR) == 0)
				return EBADF;
			else
				return B_OK;

		case F_WRLCK:
			if ((mode & O_WRONLY) == 0 && (mode & O_RDWR) == 0)
				return EBADF;
			else
				return B_OK;

		default:
			return B_BAD_VALUE;
	}
}


status_t
Inode::TestLock(OpenFileCookie* cookie, struct flock* lock)
{
	if (lock->l_type == F_UNLCK)
		return B_OK;

	status_t result = _CheckLockType(lock->l_type, cookie->fMode);
	if (result != B_OK)
		return result;

	do {
		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
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
			if (len >= OFF_MAX)
				lock->l_len = OFF_MAX;
			else
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
	status_t result = _CheckLockType(lock->l_type, cookie->fMode);
	if (result != B_OK)
		return result;

	thread_info info;
	get_thread_info(find_thread(NULL), &info);

	MutexLocker locker(cookie->fOwnerLock);
	LockOwner* owner = cookie->GetLockOwner(info.team);
	if (owner == NULL)
		return B_NO_MEMORY;

	LockInfo* linfo = new LockInfo(owner);
	if (linfo == NULL)
		return B_NO_MEMORY;
	locker.Unlock();

	linfo->fStart = lock->l_start;
	if (lock->l_len + lock->l_start == OFF_MAX)
		linfo->fLength = UINT64_MAX;
	else
		linfo->fLength = lock->l_len;
	linfo->fType = sGetLockType(lock->l_type, wait);

	do {
		MutexLocker ownerLocker(linfo->fOwner->fLock);

		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Lock(cookie, linfo);

		status_t result = request.Send();
		if (result != B_OK) {
			cookie->DeleteLock(linfo);
			return result;
		}

		ReplyInterpreter &reply = request.Reply();

		reply.PutFH();
		result = reply.Lock(linfo);

		ownerLocker.Unlock();
		if (wait && reply.NFS4Error() == NFS4ERR_DENIED) {
			snooze_etc(sSecToBigTime(5), B_SYSTEM_TIMEBASE,
				B_RELATIVE_TIMEOUT);
			continue;
		}
		if (_HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		if (result != B_OK) {
			cookie->DeleteLock(linfo);
			return result;
		}
		break;
	} while (true);

	MutexLocker _(cookie->fLocksLock);
	cookie->AddLock(linfo);

	return B_OK;
}


status_t
Inode::ReleaseLock(OpenFileCookie* cookie, const struct flock* lock)
{
	LockInfo* prev = NULL;

	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	uint32 owner = info.team;

	MutexLocker locker(cookie->fLocksLock);
	LockInfo* linfo = cookie->fLocks;
	while (linfo != NULL) {
		if (linfo->fOwner->fOwner == owner && *linfo == *lock) {
			cookie->RemoveLock(linfo, prev);
			break;
		}

		prev = linfo;
		linfo = linfo->fNext;
	}
	locker.Unlock();

	if (linfo == NULL)
		return B_BAD_VALUE;

	do {
		MutexLocker ownerLocker(linfo->fOwner->fLock);

		RPC::Server* serv = fFilesystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.LockU(linfo);

		status_t result = request.Send();
		if (result != B_OK) {
			cookie->DeleteLock(linfo);
			return result;
		}

		ReplyInterpreter &reply = request.Reply();

		reply.PutFH();
		result = reply.LockU(linfo);

		ownerLocker.Unlock();
		if (_HandleErrors(reply.NFS4Error(), serv, cookie))
			continue;

		if (result != B_OK) {
			cookie->DeleteLock(linfo);
			return result;
		}

		break;
	} while (true);

	cookie->DeleteLock(linfo);

	return B_OK;
}


status_t
Inode::ReleaseAllLocks(OpenFileCookie* cookie)
{
	MutexLocker _(cookie->fLocksLock);
	LockInfo* linfo = cookie->fLocks;
	while (linfo != NULL) {
		do {
			MutexLocker ownerLocker(linfo->fOwner->fLock);

			RPC::Server* serv = fFilesystem->Server();
			Request request(serv);
			RequestBuilder& req = request.Builder();

			req.PutFH(fInfo.fHandle);
			req.LockU(linfo);

			status_t result = request.Send();
			if (result != B_OK)
				break;

			ReplyInterpreter& reply = request.Reply();

			reply.PutFH();
			reply.LockU(linfo);

			ownerLocker.Unlock();
			if (_HandleErrors(reply.NFS4Error(), serv, cookie))
				continue;

			break;
		} while (true);
	
		cookie->RemoveLock(linfo, NULL);
		cookie->DeleteLock(linfo);

		linfo = cookie->fLocks;
	}

	return B_OK;
}


bool
Inode::_HandleErrors(uint32 nfs4Error, RPC::Server* serv,
	OpenFileCookie* cookie)
{
	uint32 leaseTime;

	switch (nfs4Error) {
		case NFS4_OK:
			return false;

		// server needs more time, we need to wait
		case NFS4ERR_LOCKED:
		case NFS4ERR_DELAY:
			if (cookie == NULL) {
				snooze_etc(sSecToBigTime(5), B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				return true;
			} else if ((cookie->fMode & O_NONBLOCK) == 0) {
				status_t result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, sSecToBigTime(5));
				if (result == B_TIMED_OUT)
					return true;
				else {
					release_sem(cookie->fSnoozeCancel);
					return false;
				}
			} else
				return false;

		// server is in grace period, we need to wait
		case NFS4ERR_GRACE:
			leaseTime = fFilesystem->NFSServer()->LeaseTime();
			if (cookie == NULL) {
				snooze_etc(sSecToBigTime(leaseTime) / 3, B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				return true;
			} else if ((cookie->fMode & O_NONBLOCK) == 0) {
				status_t result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, sSecToBigTime(leaseTime) / 3);
				if (result == B_TIMED_OUT)
					return true;
				else {
					release_sem(cookie->fSnoozeCancel);
					return false;
				}
			} else
				return false;

		// server has rebooted, reclaim share and try again
		case NFS4ERR_STALE_CLIENTID:
		case NFS4ERR_STALE_STATEID:
			fFilesystem->NFSServer()->ServerRebooted(cookie->fClientId);
			return true;

		// filehandle has expired
		case NFS4ERR_FHEXPIRED:
			if (fInfo.UpdateFileHandles(fFilesystem) == B_OK)
				return true;
			else
				return false;

		// filesystem has been moved
		case NFS4ERR_LEASE_MOVED:
		case NFS4ERR_MOVED:
			fFilesystem->Migrate(serv);
			return true;

		// lease has expired
		case NFS4ERR_EXPIRED:
			if (cookie != NULL) {
				fFilesystem->NFSServer()->ClientId(cookie->fClientId, true);
				return true;
			} else
				return false;

		default:
			return false;
	}
}

