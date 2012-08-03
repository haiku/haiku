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
	fAttrCacheExpire(0),
	fCache(NULL),
	fFileCache(NULL),
	fWriteCookie(NULL),
	fWriteDirty(false)
{
	mutex_init(&fAttrCacheLock, NULL);
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

		if (inode->_HandleErrors(reply.NFS4Error(), serv))
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
	if (fFileCache != NULL)
		file_cache_delete(fFileCache);

	delete fCache;
	mutex_destroy(&fFileCacheLock);
	mutex_destroy(&fAttrCacheLock);
}


status_t
Inode::RevalidateFileCache()
{
	uint64 change;
	status_t result = GetChangeInfo(&change);
	if (result != B_OK)
		return result;

	MutexLocker _(fFileCacheLock);
	if (change == fChange)
		return B_OK;

	result = _UpdateAttrCache(true);
	if (result != B_OK)
		return result;

	file_cache_sync(fFileCache);
	Commit();
	file_cache_delete(fFileCache);

	fFileCache = file_cache_create(fFileSystem->DevId(), ID(),
		fAttrCache.st_size);

	change = fChange;
	return B_OK;
}


status_t
Inode::GetChangeInfo(uint64* change)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);

		Attribute attr[] = { FATTR4_CHANGE };
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
		if (result != B_OK || count < 1)
			return result;

		// FATTR4_CHANGE is mandatory
		*change = values[0].fData.fValue64;
		delete[] values;

		return B_OK;
	} while (true);
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
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);

		Attribute dirAttr[] = { FATTR4_CHANGE };
		req.GetAttr(dirAttr, sizeof(dirAttr) / sizeof(Attribute));

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

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		uint64 change = values[0].fData.fValue64;
		delete[] values;

		if (!strcmp(name, ".."))
			result = reply.LookUpUp();
		else
			result = reply.LookUp();
		if (result != B_OK)
			return result;

		FileHandle fh;
		reply.GetFH(&fh);

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

		uint64 fileId;
		if (count < 2 || values[1].fAttribute != FATTR4_FILEID)
			fileId = fFileSystem->AllocFileId();
		else
			fileId = values[1].fData.fValue64;
		delete[] values;

		*id = _FileIdToInoT(fileId);

		result = _ChildAdded(name, fileId, fh);
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
	} while (true);
}


status_t
Inode::Link(Inode* dir, const char* name)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
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

		// FileHandle has expired
		if (reply.NFS4Error() == NFS4ERR_FHEXPIRED) {
			fInfo.UpdateFileHandles(fFileSystem);
			dir->fInfo.UpdateFileHandles(dir->fFileSystem);
			continue;
		}

		// filesystem has been moved
		if (reply.NFS4Error() == NFS4ERR_MOVED) {
			fFileSystem->Migrate(serv);
			continue;
		}

		reply.PutFH();
		reply.SaveFH();
		reply.PutFH();

		uint64 before, after;
		bool atomic;
		result = reply.Link(&before, &after, atomic);
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
			if (atomic && dir->fCache->ChangeInfo() == before) {
				dir->fCache->AddEntry(name, fInfo.fFileId, true);
				dir->fCache->SetChangeInfo(after);
			} else if (dir->fCache->ChangeInfo() != before)
				dir->fCache->Trash();
			dir->fCache->Unlock();
		}

		return B_OK;
	} while (true);
}


// May cause problem similar to Rename (described below). When node's is has
// more than one hard link and we delete the name it stores for FileHandle
// restoration node will inocorectly become unavailable.
status_t
Inode::Remove(const char* name, FileType type)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
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

		uint64 before, after;
		bool atomic;
		result = reply.Remove(&before, &after, atomic);

		if (fCache->Lock() == B_OK) {
			if (atomic && fCache->ChangeInfo() == before) {
				fCache->RemoveEntry(name);
				fCache->SetChangeInfo(after);
			} else if (fCache->ChangeInfo() != before)
				fCache->Trash();
			fCache->Unlock();
		}

		fFileSystem->Root()->MakeInfoInvalid();

		return result;
	} while (true);
}


// Rename may cause some problems if FileHandles are volatile and local Inode
// object exists for renamed node. It's stored filename will become invalid
// and, consequnetly, FileHandle restoration will fail. Probably, it will
// be much easier to solve this problem if more metadata is cached.
status_t
Inode::Rename(Inode* from, Inode* to, const char* fromName, const char* toName)
{
	if (from->fFileSystem != to->fFileSystem)
		return B_DONT_DO_THAT;

	do {
		RPC::Server* serv = from->fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(from->fInfo.fHandle);
		req.SaveFH();
		req.PutFH(to->fInfo.fHandle);
		req.Rename(fromName, toName);

		Attribute attr[] = { FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));
		req.LookUp(toName);

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

		uint64 fromBefore, fromAfter, toBefore, toAfter;
		bool fromAtomic, toAtomic;
		result = reply.Rename(&fromBefore, &fromAfter, fromAtomic, &toBefore,
			&toAfter, toAtomic);
		if (result != B_OK)
			return result;

		result = reply.LookUp();
		if (result != B_OK)
			return result;

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		uint32 fileID;
		if (count == 0)
			fileID = from->fFileSystem->AllocFileId();
		else
			fileID = values[1].fData.fValue64;

		from->fFileSystem->Root()->MakeInfoInvalid();

		if (from->fCache->Lock() == B_OK) {
			if (fromAtomic && from->fCache->ChangeInfo() == fromBefore) {
				from->fCache->RemoveEntry(fromName);
				from->fCache->SetChangeInfo(fromAfter);
			} else if (from->fCache->ChangeInfo() != fromBefore)
				from->fCache->Trash();
			from->fCache->Unlock();
		}

		if (to->fCache->Lock() == B_OK) {
			if (toAtomic && to->fCache->ChangeInfo() == toBefore) {
				to->fCache->AddEntry(toName, fileID, true);
				to->fCache->SetChangeInfo(toAfter);
			} else if (to->fCache->ChangeInfo() != toBefore)
				to->fCache->Trash();
			to->fCache->Unlock();
		}

		return B_OK;
	} while (true);
}


status_t
Inode::CreateLink(const char* name, const char* path, int mode)
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

		req.Create(NF4LNK, name, cattr, i, path);

		req.GetFH();
		Attribute attr[] = { FATTR4_FILEID };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

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
		if (result != B_OK)
			return result;

		FileHandle handle;
		reply.GetFH(&handle);

		AttrValue* values;
		uint32 count;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		uint32 fileID;
		if (count == 0)
			fileID = fFileSystem->AllocFileId();
		else
			fileID = values[0].fData.fValue64;

		fFileSystem->Root()->MakeInfoInvalid();

		result = _ChildAdded(name, fileID, handle);
		if (result != B_OK)
			return B_OK;

		if (fCache->Lock() == B_OK) {
			if (atomic && fCache->ChangeInfo() == before) {
				fCache->AddEntry(name, fileID, true);
				fCache->SetChangeInfo(after);
			} else if (fCache->ChangeInfo() != before)
				fCache->Trash();
			fCache->Unlock();
		}

		return B_OK;
	} while (true);
}


status_t
Inode::ReadLink(void* buffer, size_t* length)
{
	if (fType != NF4LNK)
		return B_BAD_VALUE;

	do {
		RPC::Server* serv = fFileSystem->Server();
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
	int acc = 0;

	if (fFileSystem->IsAttrSupported(FATTR4_MODE)) {
		status_t result = _UpdateAttrCache();
		if (result != B_OK)
			return result;

		// Root squashing and owner mapping problems may make this function
		// return false values. However, since due to race condition it can
		// not be used for anything critical, it is not a serius problem.

		mode_t nodeMode = fAttrCache.st_mode;
		uint8 userMode = (nodeMode & S_IRWXU) >> 6;
		uint8 groupMode = (nodeMode & S_IRWXG) >> 3;
		uint8 otherMode = (nodeMode & S_IRWXO);

		uid_t uid = geteuid();
		if (uid == 0)
			acc = userMode | groupMode | otherMode | R_OK | W_OK;
		else if (uid == fAttrCache.st_uid)
			acc = userMode;
		else if (getegid() == fAttrCache.st_gid)
			acc = groupMode;
		else
			acc = otherMode;
	} else {
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

			if ((allowed & ACCESS4_READ) != 0)
				acc |= R_OK;

			if (fType == NF4DIR && (allowed & ACCESS4_LOOKUP) != 0)
				acc |= X_OK | R_OK;

			if (fType != NF4DIR && (allowed & ACCESS4_EXECUTE) != 0)
				acc |= X_OK;

			if ((allowed & ACCESS4_MODIFY) != 0)
				acc |= W_OK;

			break;
		} while (true);
	}

	if ((mode & acc) != mode)
		return B_NOT_ALLOWED;

	return B_OK;
}


status_t
Inode::Stat(struct stat* st)
{
	status_t result = _UpdateAttrCache();
	if (result != B_OK)
		return result;

	// Do not touch other members of struct stat
	st->st_size = fAttrCache.st_size;
	st->st_mode = fAttrCache.st_mode;
	st->st_nlink = fAttrCache.st_nlink;
	st->st_uid = fAttrCache.st_uid;
	st->st_gid = fAttrCache.st_gid;
	st->st_atim = fAttrCache.st_atim;
	st->st_ctim = fAttrCache.st_ctim;
	st->st_crtim = fAttrCache.st_crtim;
	st->st_mtim = fAttrCache.st_mtim;

	return B_OK;
}


status_t
Inode::_UpdateAttrCache(bool force)
{
	if (!force && fAttrCacheExpire > time(NULL))
		return B_OK;

	MutexLocker _(fAttrCacheLock);

	if (fAttrCacheExpire > time(NULL))
		return B_OK;

	do {
		RPC::Server* serv = fFileSystem->Server();
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
		fAttrCache.st_size = values[0].fData.fValue64;

		uint32 next = 1;
		fAttrCache.st_mode = Type();
		if (count >= next && values[next].fAttribute == FATTR4_MODE) {
			fAttrCache.st_mode |= values[next].fData.fValue32;
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
				fAttrCache.st_mode |= S_IRUSR | S_IRGRP | S_IROTH;

			if ((prvl & ACCESS4_MODIFY) != 0)
				fAttrCache.st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;

			if (fType == NF4DIR && (prvl & ACCESS4_LOOKUP) != 0)
				fAttrCache.st_mode |= S_IXUSR | S_IXGRP | S_IXOTH;

			if (fType != NF4DIR && (prvl & ACCESS4_EXECUTE) != 0)
				fAttrCache.st_mode |= S_IXUSR | S_IXGRP | S_IXOTH;
		}

		if (count >= next && values[next].fAttribute == FATTR4_NUMLINKS) {
			fAttrCache.st_nlink = values[next].fData.fValue32;
			next++;
		} else
			fAttrCache.st_nlink = 1;

		if (count >= next && values[next].fAttribute == FATTR4_OWNER) {
			char* owner = reinterpret_cast<char*>(values[next].fData.fPointer);
			if (owner != NULL && isdigit(owner[0]))
				fAttrCache.st_uid = atoi(owner);
			else
				fAttrCache.st_uid = gIdMapper->GetUserId(owner);
			next++;
		} else
			fAttrCache.st_uid = 0;

		if (count >= next && values[next].fAttribute == FATTR4_OWNER_GROUP) {
			char* group = reinterpret_cast<char*>(values[next].fData.fPointer);
			if (group != NULL && isdigit(group[0]))
				fAttrCache.st_gid = atoi(group);
			else
				fAttrCache.st_gid = gIdMapper->GetGroupId(group);
			next++;
		} else
			fAttrCache.st_gid = 0;

		if (count >= next && values[next].fAttribute == FATTR4_TIME_ACCESS) {
			memcpy(&fAttrCache.st_atim, values[next].fData.fPointer,
				sizeof(timespec));
			next++;
		} else
			memset(&fAttrCache.st_atim, 0, sizeof(timespec));

		if (count >= next && values[next].fAttribute == FATTR4_TIME_CREATE) {
			memcpy(&fAttrCache.st_crtim, values[next].fData.fPointer,
				sizeof(timespec));
			next++;
		} else
			memset(&fAttrCache.st_crtim, 0, sizeof(timespec));

		if (count >= next && values[next].fAttribute == FATTR4_TIME_METADATA) {
			memcpy(&fAttrCache.st_ctim, values[next].fData.fPointer,
				sizeof(timespec));
			next++;
		} else
			memset(&fAttrCache.st_ctim, 0, sizeof(timespec));

		if (count >= next && values[next].fAttribute == FATTR4_TIME_MODIFY) {
			memcpy(&fAttrCache.st_mtim, values[next].fData.fPointer,
				sizeof(timespec));
			next++;
		} else
			memset(&fAttrCache.st_mtim, 0, sizeof(timespec));

		delete[] values;

		fAttrCacheExpire = time(NULL) + kAttrCacheExpirationTime;

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
		RPC::Server* serv = fFileSystem->Server();
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

		if (result != B_OK)
			return result;

		break;
	} while (true);

	_UpdateAttrCache(true);

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

	status_t result = _CheckLockType(lock->l_type, cookie->fMode);
	if (result != B_OK)
		return result;

	do {
		RPC::Server* serv = fFileSystem->Server();
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

		RPC::Server* serv = fFileSystem->Server();
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

		RPC::Server* serv = fFileSystem->Server();
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

			RPC::Server* serv = fFileSystem->Server();
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
				if (result != B_TIMED_OUT) {
					release_sem(cookie->fSnoozeCancel);
					return false;
				}
				return true;
			}
			return false;

		// server is in grace period, we need to wait
		case NFS4ERR_GRACE:
			leaseTime = fFileSystem->NFSServer()->LeaseTime();
			if (cookie == NULL) {
				snooze_etc(sSecToBigTime(leaseTime) / 3, B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				return true;
			} else if ((cookie->fMode & O_NONBLOCK) == 0) {
				status_t result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, sSecToBigTime(leaseTime) / 3);
				if (result != B_TIMED_OUT) {
					release_sem(cookie->fSnoozeCancel);
					return false;
				}
				return true;
			}
			return false;

		// server has rebooted, reclaim share and try again
		case NFS4ERR_STALE_CLIENTID:
		case NFS4ERR_STALE_STATEID:
			fFileSystem->NFSServer()->ServerRebooted(cookie->fClientId);
			return true;

		// FileHandle has expired
		case NFS4ERR_FHEXPIRED:
			if (fInfo.UpdateFileHandles(fFileSystem) == B_OK)
				return true;
			return false;

		// filesystem has been moved
		case NFS4ERR_LEASE_MOVED:
		case NFS4ERR_MOVED:
			fFileSystem->Migrate(serv);
			return true;

		// lease has expired
		case NFS4ERR_EXPIRED:
			if (cookie != NULL) {
				fFileSystem->NFSServer()->ClientId(cookie->fClientId, true);
				return true;
			}
			return false;

		default:
			return false;
	}
}


status_t
Inode::_ChildAdded(const char* name, uint64 fileID,
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

	return fFileSystem->InoIdMap()->AddEntry(fi, _FileIdToInoT(fileID));
}

