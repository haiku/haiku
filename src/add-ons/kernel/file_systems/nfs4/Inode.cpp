/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"

#include <ctype.h>
#include <string.h>

#include <fs_cache.h>
#include <NodeMonitor.h>

#include "IdMap.h"
#include "Request.h"
#include "RootInode.h"


Inode::Inode()
	:
	fMetaCache(this),
	fCache(NULL),
	fAttrCache(NULL),
	fDelegation(NULL),
	fFileCache(NULL),
	fMaxFileSize(0),
	fOpenState(NULL),
	fWriteDirty(false)
{
	rw_lock_init(&fDelegationLock, NULL);
	mutex_init(&fStateLock, NULL);
	mutex_init(&fFileCacheLock, NULL);
}


status_t
Inode::CreateInode(FileSystem* fs, const FileInfo &fi, Inode** _inode)
{
	Inode* inode = NULL;
	if (fs->Root() == NULL)
		inode = new(std::nothrow) RootInode;
	else
		inode = new(std::nothrow) Inode;

	if (inode == NULL)
		return B_NO_MEMORY;

	inode->fInfo = fi;
	inode->fFileSystem = fs;

	uint64 size;
	do {
		RPC::Server* serv = fs->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(inode->fInfo.fHandle);

		Attribute attr[] = { FATTR4_TYPE, FATTR4_CHANGE, FATTR4_SIZE,
			FATTR4_FSID, FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (inode->HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK || count < 4)
			return result;

		if (fi.fFileId == 0) {
			if (count < 5 || values[4].fAttribute != FATTR4_FILEID)
				inode->fInfo.fFileId = fs->AllocFileId();
			else
				inode->fInfo.fFileId = values[4].fData.fValue64;
		} else
			inode->fInfo.fFileId = fi.fFileId;

		// FATTR4_TYPE is mandatory
		inode->fType = values[0].fData.fValue32;

		if (inode->fType == NF4DIR)
			inode->fCache = new DirectoryCache(inode);
		inode->fAttrCache = new DirectoryCache(inode, true);

		// FATTR4_CHANGE is mandatory
		inode->fChange = values[1].fData.fValue64;

		// FATTR4_SIZE is mandatory
		size = values[2].fData.fValue64;

		// FATTR4_FSID is mandatory
		FileSystemId* fsid =
			reinterpret_cast<FileSystemId*>(values[3].fData.fPointer);
		if (*fsid != fs->FsId()) {
			delete[] values;
			return B_ENTRY_NOT_FOUND;
		}

		delete[] values;

		*_inode = inode;

		break;
	} while (true);

	if (inode->fType == NF4REG)
		inode->fFileCache = file_cache_create(fs->DevId(), inode->ID(), size);

	return B_OK;
}


Inode::~Inode()
{
	if (fDelegation != NULL)
		RecallDelegation();

	if (fFileCache != NULL)
		file_cache_delete(fFileCache);

	delete fCache;
	delete fAttrCache;

	mutex_destroy(&fStateLock);
	mutex_destroy(&fFileCacheLock);
	rw_lock_destroy(&fDelegationLock);
}


status_t
Inode::RevalidateFileCache()
{
	if (fDelegation != NULL)
		return B_OK;

	uint64 change;
	status_t result = GetChangeInfo(&change);
	if (result != B_OK)
		return result;

	MutexLocker _(fFileCacheLock);
	if (change == fChange)
		return B_OK;

	struct stat st;
	result = Stat(&st);
	if (result != B_OK)
		return result;

	SyncAndCommit(true);
	file_cache_delete(fFileCache);

	fFileCache = file_cache_create(fFileSystem->DevId(), ID(), st.st_size);

	change = fChange;
	return B_OK;
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

	uint64 change;
	uint64 fileID;
	FileHandle handle;
	status_t result = NFS4Inode::LookUp(name, &change, &fileID, &handle);
	if (result != B_OK)
		return result;

	*id = FileIdToInoT(fileID);

	result = ChildAdded(name, fileID, handle);
	if (result != B_OK)
		return result;

	fFileSystem->Revalidator().Lock();
	if (fCache->Lock() != B_OK) {
		fCache->ResetAndLock();
		fCache->SetChangeInfo(change);
	} else {
		fFileSystem->Revalidator().RemoveDirectory(fCache);
		fCache->ValidateChangeInfo(change);
	}

	fCache->AddEntry(name, *id);
	fFileSystem->Revalidator().AddDirectory(fCache);
	fCache->Unlock();
	fFileSystem->Revalidator().Unlock();

	return B_OK;
}


status_t
Inode::Link(Inode* dir, const char* name)
{
	ChangeInfo changeInfo;
	status_t result = NFS4Inode::Link(dir, name, &changeInfo);
	if (result != B_OK)
		return result;

	fFileSystem->Root()->MakeInfoInvalid();

	FileInfo fi = fInfo;
	fi.fParent = dir->fInfo.fHandle;
	free(const_cast<char*>(fi.fName));
	fi.fName = strdup(name);
	if (fi.fName == NULL)
		return B_NO_MEMORY;

	char* path = reinterpret_cast<char*>(malloc(strlen(name) + 2 +
		strlen(fInfo.fPath)));
	if (path == NULL)
		return B_NO_MEMORY;

	strcpy(path, dir->fInfo.fPath);
	strcat(path, "/");
	strcat(path, name);
	free(const_cast<char*>(fi.fPath));
	fi.fPath = path;

	fFileSystem->InoIdMap()->AddEntry(fi, fInfo.fFileId);

	if (dir->fCache->Lock() == B_OK) {
		if (changeInfo.fAtomic
			&& dir->fCache->ChangeInfo() == changeInfo.fBefore) {
			dir->fCache->AddEntry(name, fInfo.fFileId, true);
			dir->fCache->SetChangeInfo(changeInfo.fAfter);
		} else if (dir->fCache->ChangeInfo() != changeInfo.fBefore)
			dir->fCache->Trash();
		dir->fCache->Unlock();
	}

	notify_entry_created(fFileSystem->DevId(), dir->ID(), name, ID());

	return B_OK;
}


status_t
Inode::Remove(const char* name, FileType type)
{
	if (type == NF4NAMEDATTR) {
		status_t result = LoadAttrDirHandle();
		if (result != B_OK)
			return result;
	}

	ChangeInfo changeInfo;
	uint64 fileID;
	status_t result = NFS4Inode::RemoveObject(name, type, &changeInfo, &fileID);
	if (result != B_OK)
		return result;

	DirectoryCache* cache = type != NF4NAMEDATTR ? fCache : fAttrCache;
	if (cache->Lock() == B_OK) {
		if (changeInfo.fAtomic
			&& fCache->ChangeInfo() == changeInfo.fBefore) {
			cache->RemoveEntry(name);
			cache->SetChangeInfo(changeInfo.fAfter);
		} else if (cache->ChangeInfo() != changeInfo.fBefore)
			cache->Trash();
		cache->Unlock();
	}

	fFileSystem->Root()->MakeInfoInvalid();

	if (type == NF4NAMEDATTR) {
		notify_attribute_changed(fFileSystem->DevId(), ID(), name,
			B_ATTR_REMOVED);
	} else {
		notify_entry_removed(fFileSystem->DevId(), ID(), name,
			FileIdToInoT(fileID));
	}

	return B_OK;
}


status_t
Inode::Rename(Inode* from, Inode* to, const char* fromName, const char* toName,
	bool attribute)
{
	if (from->fFileSystem != to->fFileSystem)
		return B_DONT_DO_THAT;

	if (attribute) {
		status_t result = from->LoadAttrDirHandle();
		if (result != B_OK)
			return result;

		result = to->LoadAttrDirHandle();
		if (result != B_OK)
			return result;
	}

	ChangeInfo fromChange, toChange;
	uint64 fileID;
	status_t result = NFS4Inode::Rename(from, to, fromName, toName, &fromChange,
		&toChange, &fileID, attribute);
	if (result != B_OK)
		return result;

	from->fFileSystem->Root()->MakeInfoInvalid();

	DirectoryCache* cache = attribute ? from->fAttrCache : from->fCache;
	if (cache->Lock() == B_OK) {
		if (fromChange.fAtomic
			&& cache->ChangeInfo() == fromChange.fBefore) {
			cache->RemoveEntry(fromName);
			cache->SetChangeInfo(fromChange.fAfter);
		} else if (cache->ChangeInfo() != fromChange.fBefore)
			cache->Trash();
		cache->Unlock();
	}

	cache = attribute ? to->fAttrCache : to->fCache;
	if (cache->Lock() == B_OK) {
		if (toChange.fAtomic
			&& cache->ChangeInfo() == toChange.fBefore) {
			cache->AddEntry(toName, fileID, true);
			cache->SetChangeInfo(toChange.fAfter);
		} else if (to->fCache->ChangeInfo() != toChange.fBefore)
			cache->Trash();
		cache->Unlock();
	}

	if (attribute) {
		notify_attribute_changed(from->fFileSystem->DevId(), from->ID(),
			fromName, B_ATTR_REMOVED);
		notify_attribute_changed(to->fFileSystem->DevId(), to->ID(), toName,
			B_ATTR_CREATED);
	} else {
		notify_entry_moved(from->fFileSystem->DevId(), from->ID(), fromName,
			to->ID(), toName, FileIdToInoT(fileID));
	}

	return B_OK;
}


status_t
Inode::CreateLink(const char* name, const char* path, int mode)
{
	return CreateObject(name, path, mode, NF4LNK);
}


status_t
Inode::CreateObject(const char* name, const char* path, int mode, FileType type)
{
	ChangeInfo changeInfo;
	uint64 fileID;
	FileHandle handle;

	status_t result = NFS4Inode::CreateObject(name, path, mode, type, &changeInfo,
		&fileID, &handle);
	if (result != B_OK)
		return B_OK;

	fFileSystem->Root()->MakeInfoInvalid();

	result = ChildAdded(name, fileID, handle);
	if (result != B_OK)
		return B_OK;

	if (fCache->Lock() == B_OK) {
		if (changeInfo.fAtomic && fCache->ChangeInfo() == changeInfo.fBefore) {
			fCache->AddEntry(name, fileID, true);
			fCache->SetChangeInfo(changeInfo.fAfter);
		} else if (fCache->ChangeInfo() != changeInfo.fBefore)
			fCache->Trash();
		fCache->Unlock();
	}

	notify_entry_created(fFileSystem->DevId(), ID(), name,
		FileIdToInoT(fileID));

	return B_OK;
}


status_t
Inode::Access(int mode)
{
	int acc = 0;

	uint32 allowed;
	status_t result = fMetaCache.GetAccess(geteuid(), &allowed);
	if (result != B_OK) {
		result = NFS4Inode::Access(&allowed);
		if (result != B_OK)
			return result;
		fMetaCache.SetAccess(geteuid(), allowed);
	}

	if ((allowed & ACCESS4_READ) != 0)
		acc |= R_OK;

	if ((allowed & ACCESS4_LOOKUP) != 0)
		acc |= X_OK | R_OK;

	if ((allowed & ACCESS4_EXECUTE) != 0)
		acc |= X_OK;

	if ((allowed & ACCESS4_MODIFY) != 0)
		acc |= W_OK;

	if ((mode & acc) != mode)
		return B_NOT_ALLOWED;

	return B_OK;
}


status_t
Inode::Stat(struct stat* st, OpenAttrCookie* attr)
{
	if (attr != NULL)
		return GetStat(st, attr);

	status_t result = fMetaCache.GetStat(st);
	if (result != B_OK) {
		struct stat temp;
		result = GetStat(&temp);
		if (result != B_OK)
			return result;
		fMetaCache.SetStat(temp);
		fMetaCache.GetStat(st);
	}

	return B_OK;
}


status_t
Inode::GetStat(struct stat* st, OpenAttrCookie* attr)
{
	AttrValue* values;
	uint32 count;
	status_t result = NFS4Inode::GetStat(&values, &count, attr);
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
	} else
		st->st_mode = 777;

	if (count >= next && values[next].fAttribute == FATTR4_NUMLINKS) {
		st->st_nlink = values[next].fData.fValue32;
		next++;
	} else
		st->st_nlink = 1;

	if (count >= next && values[next].fAttribute == FATTR4_OWNER) {
		char* owner = reinterpret_cast<char*>(values[next].fData.fPointer);
		if (owner != NULL && isdigit(owner[0]))
			st->st_uid = atoi(owner);
		else
			st->st_uid = gIdMapper->GetUserId(owner);
		next++;
	} else
		st->st_uid = 0;

	if (count >= next && values[next].fAttribute == FATTR4_OWNER_GROUP) {
		char* group = reinterpret_cast<char*>(values[next].fData.fPointer);
		if (group != NULL && isdigit(group[0]))
			st->st_gid = atoi(group);
		else
			st->st_gid = gIdMapper->GetGroupId(group);
		next++;
	} else
		st->st_gid = 0;

	if (count >= next && values[next].fAttribute == FATTR4_TIME_ACCESS) {
		memcpy(&st->st_atim, values[next].fData.fPointer,
			sizeof(timespec));
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
		memcpy(&st->st_ctim, values[next].fData.fPointer,
			sizeof(timespec));
		next++;
	} else
		memset(&st->st_ctim, 0, sizeof(timespec));

	if (count >= next && values[next].fAttribute == FATTR4_TIME_MODIFY) {
		memcpy(&st->st_mtim, values[next].fData.fPointer,
			sizeof(timespec));
		next++;
	} else
		memset(&st->st_mtim, 0, sizeof(timespec));
	delete[] values;

	st->st_blksize = fFileSystem->Root()->IOSize();
	st->st_blocks = st->st_size / st->st_blksize;
	st->st_blocks += st->st_size % st->st_blksize == 0 ? 0 : 1;

	return B_OK;
}


status_t
Inode::WriteStat(const struct stat* st, uint32 mask, OpenAttrCookie* cookie)
{
	status_t result;
	AttrValue attr[6];
	uint32 i = 0;

	if ((mask & B_STAT_SIZE) != 0) {
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

	if (cookie == NULL) {
		MutexLocker stateLocker(fStateLock);
		result = NFS4Inode::WriteStat(fOpenState, attr, i);
		stateLocker.Unlock();

		fMetaCache.InvalidateStat();
		if ((mask & B_STAT_MODE) != 0 || (mask & B_STAT_UID) != 0
			|| (mask & B_STAT_GID) != 0) {
			fMetaCache.InvalidateAccess();
		}
	} else
		result = NFS4Inode::WriteStat(cookie->fOpenState, attr, i);

	return result;
}


inline status_t
Inode::CheckLockType(short ltype, uint32 mode)
{
	switch (ltype) {
		case F_UNLCK:
			return B_OK;

		case F_RDLCK:
			if ((mode & O_RDONLY) == 0 && (mode & O_RDWR) == 0)
				return EBADF;
			return B_OK;

		case F_WRLCK:
			if ((mode & O_WRONLY) == 0 && (mode & O_RDWR) == 0)
				return EBADF;
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

	status_t result = CheckLockType(lock->l_type, cookie->fMode);
	if (result != B_OK)
		return result;

	LockType ltype = sGetLockType(lock->l_type, false);
	uint64 position = lock->l_start;
	uint64 length = lock->l_len;
	bool conflict;

	result = NFS4Inode::TestLock(cookie, &ltype, &position, &length, conflict);
	if (result != B_OK)
		return result;

	if (conflict) {
		lock->l_type = sLockTypeToHaiku(ltype);
		lock->l_start = static_cast<off_t>(position);
		if (length >= OFF_MAX)
			lock->l_len = OFF_MAX;
		else
			lock->l_len = static_cast<off_t>(length);
	} else
		lock->l_type = F_UNLCK;

	return B_OK;
}

status_t
Inode::AcquireLock(OpenFileCookie* cookie, const struct flock* lock,
	bool wait)
{
	OpenState* state = cookie->fOpenState;

	status_t result = CheckLockType(lock->l_type, cookie->fMode);
	if (result != B_OK)
		return result;

	thread_info info;
	get_thread_info(find_thread(NULL), &info);

	MutexLocker locker(state->fOwnerLock);
	LockOwner* owner = state->GetLockOwner(info.team);
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

	result = NFS4Inode::AcquireLock(cookie, linfo, wait);
	if (result != B_OK)
		return result;

	MutexLocker _(state->fLocksLock);
	state->AddLock(linfo);
	cookie->AddLock(linfo);

	return B_OK;
}


status_t
Inode::ReleaseLock(OpenFileCookie* cookie, const struct flock* lock)
{
	SyncAndCommit();

	LockInfo* prev = NULL;

	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	uint32 owner = info.team;

	OpenState* state = cookie->fOpenState;
	MutexLocker locker(state->fLocksLock);
	LockInfo* linfo = state->fLocks;
	while (linfo != NULL) {
		if (linfo->fOwner->fOwner == owner && *linfo == *lock) {
			state->RemoveLock(linfo, prev);
			break;
		}

		prev = linfo;
		linfo = linfo->fNext;
	}

	prev = NULL;
	linfo = cookie->fLocks;
	while (linfo != NULL) {
		if (linfo->fOwner->fOwner == owner && *linfo == *lock) {
			cookie->RemoveLock(linfo, prev);
			break;
		}

		prev = linfo;
		linfo = linfo->fCookieNext;
	}
	locker.Unlock();

	if (linfo == NULL)
		return B_BAD_VALUE;

	status_t result = NFS4Inode::ReleaseLock(cookie, linfo);
	if (result != B_OK)
		return result;

	state->DeleteLock(linfo);

	return B_OK;
}


status_t
Inode::ReleaseAllLocks(OpenFileCookie* cookie)
{
	SyncAndCommit();

	OpenState* state = cookie->fOpenState;
	MutexLocker _(state->fLocksLock);
	LockInfo* linfo = cookie->fLocks;
	while (linfo != NULL) {
		cookie->RemoveLock(linfo, NULL);

		LockInfo* prev = NULL;
		LockInfo* stateLock = state->fLocks;
		while (stateLock != NULL) {
			if (*linfo == *stateLock) {
				state->RemoveLock(stateLock, prev);
				break;
			}

			prev = stateLock;
			stateLock = stateLock->fNext;
		}

		NFS4Inode::ReleaseLock(cookie, linfo);
		state->DeleteLock(linfo);

		linfo = cookie->fLocks;
	}

	return B_OK;
}


status_t
Inode::ChildAdded(const char* name, uint64 fileID,
	const FileHandle& fileHandle)
{
	fFileSystem->Root()->MakeInfoInvalid();

	FileInfo fi;
	fi.fFileId = fileID;
	fi.fHandle = fileHandle;
	fi.fParent = fInfo.fHandle;
	fi.fName = strdup(name);
	if (fi.fName == NULL)
		return B_NO_MEMORY;

	char* path = reinterpret_cast<char*>(malloc(strlen(name) + 2 +
		strlen(fInfo.fPath)));
	if (path == NULL)
		return B_NO_MEMORY;

	strcpy(path, fInfo.fPath);
	strcat(path, "/");
	strcat(path, name);
	fi.fPath = path;

	return fFileSystem->InoIdMap()->AddEntry(fi, FileIdToInoT(fileID));
}


void
Inode::SetDelegation(Delegation* delegation)
{
	WriteLocker _(fDelegationLock);

	fMetaCache.InvalidateStat();
	struct stat st;
	Stat(&st);
	fMetaCache.LockValid();	

	fDelegation = delegation;
	fOpenState->AcquireReference();
	fOpenState->fDelegation = delegation;
	fFileSystem->AddDelegation(delegation);
}


void
Inode::RecallDelegation(bool truncate)
{
	WriteLocker _(fDelegationLock);
	if (fDelegation == NULL)
		return;
	ReturnDelegation(truncate);
}


void
Inode::RecallReadDelegation()
{
	WriteLocker _(fDelegationLock);
	if (fDelegation == NULL || fDelegation->Type() != OPEN_DELEGATE_READ)
		return;
	ReturnDelegation(false);
}


void
Inode::ReturnDelegation(bool truncate)
{
	fDelegation->GiveUp(truncate);

	fMetaCache.UnlockValid();
	fFileSystem->RemoveDelegation(fDelegation);

	MutexLocker stateLocker(fStateLock);
	fOpenState->fDelegation = NULL;
	ReleaseOpenState();

	delete fDelegation;
	fDelegation = NULL;
}


void
Inode::ReleaseOpenState()
{
	if (fOpenState->ReleaseReference() == 1)
		fOpenState = NULL;
}


status_t
Inode::SyncAndCommit(bool force)
{
	if (!force && fDelegation != NULL)
		return B_OK;

	file_cache_sync(fFileCache);
	return Commit();
}

