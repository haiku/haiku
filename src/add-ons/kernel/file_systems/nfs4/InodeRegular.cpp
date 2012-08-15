/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"

#include <string.h>

#include <AutoDeleter.h>
#include <fs_cache.h>
#include <NodeMonitor.h>

#include "IdMap.h"
#include "Request.h"
#include "RootInode.h"


status_t
Inode::CreateState(const char* name, int mode, int perms, OpenState* state,
	OpenDelegationData* delegationData) {
	uint64 fileID;
	FileHandle handle;
	ChangeInfo changeInfo;

	status_t result = CreateFile(name, mode, perms, state, &changeInfo,
		&fileID, &handle, delegationData);
	if (result != B_OK)
		return result;

	RevalidateFileCache();

	FileInfo fi;
	fi.fFileId = fileID;
	fi.fHandle = handle;
	fi.fParent = fInfo.fHandle;
	fi.fName = strdup(name);

	char* path = reinterpret_cast<char*>(malloc(strlen(name) + 2 +
		strlen(fInfo.fPath)));
	strcpy(path, fInfo.fPath);
	strcat(path, "/");
	strcat(path, name);
	fi.fPath = path;

	fFileSystem->InoIdMap()->AddEntry(fi, FileIdToInoT(fileID));

	if (fCache->Lock() == B_OK) {
		if (changeInfo.fAtomic
			&& fCache->ChangeInfo() == changeInfo.fBefore) {
			fCache->AddEntry(name, fileID, true);
			fCache->SetChangeInfo(changeInfo.fAfter);
		} else if (fCache->ChangeInfo() != changeInfo.fBefore)
			fCache->Trash();
		fCache->Unlock();
	}

	state->fFileSystem = fFileSystem;
	state->fInfo = fi;

	return B_OK;
}


status_t
Inode::Create(const char* name, int mode, int perms, OpenFileCookie* cookie,
	OpenDelegationData* data, ino_t* id)
{
	cookie->fMode = mode;
	cookie->fLocks = NULL;

	MutexLocker _(fStateLock);

	OpenState* state = new OpenState;
	status_t result = CreateState(name, mode, perms, state, data);
	if (result != B_OK)
		return result;

	cookie->fOpenState = state;
	fOpenState = state;

	*id = FileIdToInoT(state->fInfo.fFileId);
	cookie->fOpenState = fOpenState;

	cookie->fFileSystem = fFileSystem;

	fFileSystem->AddOpenFile(state);
	fFileSystem->Root()->MakeInfoInvalid();

	notify_entry_created(fFileSystem->DevId(), ID(), name, *id);

	return B_OK;
}


status_t
Inode::Open(int mode, OpenFileCookie* cookie)
{
	MutexLocker locker(fStateLock);

	OpenDelegationData data;
	data.fType = OPEN_DELEGATE_NONE;
	if (fOpenState == NULL) {
		RevalidateFileCache();

		OpenState* state = new OpenState;
		if (state == NULL)
			return B_NO_MEMORY;

		state->fInfo = fInfo;
		state->fFileSystem = fFileSystem;
		status_t result = OpenFile(state, mode, &data);
		if (result != B_OK)
			return result;

		fFileSystem->AddOpenFile(state);
		fOpenState = state;
		locker.Unlock();
	} else {
		fOpenState->AcquireReference();
		locker.Unlock();

		int newMode = mode & O_RWMASK;
		int oldMode = fOpenState->fMode & O_RWMASK;
		if (oldMode != newMode && oldMode != O_RDWR) {
			if (oldMode == O_RDONLY)
				RecallReadDelegation();

			status_t result = OpenFile(fOpenState, O_RDWR, &data);
			if (result != B_OK) {
				locker.Lock();
				ReleaseOpenState();
				return result;
			}
			fOpenState->fMode = O_RDWR;
		} else {
			int newMode = mode & O_RWMASK;
			uint32 allowed = 0;
			if (newMode == O_RDWR || newMode == O_RDONLY)
				allowed |= R_OK;
			if (newMode == O_RDWR || newMode == O_WRONLY)
				allowed |= W_OK;

			status_t result = Access(allowed);
			if (result != B_OK) {
				locker.Lock();
				ReleaseOpenState();
				return result;
			}
		}
	}

	if ((mode & O_TRUNC) == O_TRUNC) {
		struct stat st;
		st.st_size = 0;
		WriteStat(&st, B_STAT_SIZE);
		file_cache_set_size(fFileCache, 0);
	}

	cookie->fOpenState = fOpenState;

	cookie->fFileSystem = fFileSystem;
	cookie->fMode = mode;
	cookie->fLocks = NULL;

	if (data.fType != OPEN_DELEGATE_NONE) {
		Delegation* delegation
			= new(std::nothrow) Delegation(data, this, fOpenState->fClientID);
		if (delegation != NULL) {
			delegation->fInfo = fOpenState->fInfo;
			delegation->fFileSystem = fFileSystem;
			SetDelegation(delegation);
		}
	}

	return B_OK;
}


status_t
Inode::Close(OpenFileCookie* cookie)
{
	SyncAndCommit();

	MutexLocker _(fStateLock);
	ReleaseOpenState();

	return B_OK;
}


static char*
AttrToFileName(const char* path)
{
	char* name = strdup(path);
	if (name == NULL)
		return NULL;

	char* current = strpbrk(name, "/:");
	while (current != NULL) {
		switch (*current) {
			case '/':
				*current = '#';
				break;
			case ':':
				*current = '$';
				break;
		}
		current = strpbrk(name, "/:");
	}

	return name;
}


status_t
Inode::OpenAttr(const char* _name, int mode, OpenAttrCookie* cookie,
	bool create, int32 type)
{
	(void)type;

	status_t result = LoadAttrDirHandle();
	if (result != B_OK)
		return result;

	char* name = AttrToFileName(_name);
	if (name == NULL)
		return B_NO_MEMORY;
	MemoryDeleter nameDeleter(name);

	OpenDelegationData data;
	data.fType = OPEN_DELEGATE_NONE;

	OpenState* state = new OpenState;
	if (state == NULL)
		return B_NO_MEMORY;

	state->fInfo.fName = strdup(name);
	state->fInfo.fParent = fInfo.fAttrDir;
	state->fFileSystem = fFileSystem;
	result = NFS4Inode::OpenAttr(state, name, mode, &data, create);
	if (result != B_OK) {
		delete state;
		return result;
	}

	fFileSystem->AddOpenFile(state);

	cookie->fOpenState = state;
	cookie->fFileSystem = fFileSystem;
	cookie->fMode = mode;

	if (data.fType != OPEN_DELEGATE_NONE) {
		Delegation* delegation
			= new(std::nothrow) Delegation(data, this, state->fClientID, true);
		if (delegation != NULL) {
			delegation->fInfo = state->fInfo;
			delegation->fFileSystem = fFileSystem;
			state->fDelegation = delegation;
			fFileSystem->AddDelegation(delegation);
		}
	}

	if (create || (mode & O_TRUNC) == O_TRUNC) {
		struct stat st;
		st.st_size = 0;
		WriteStat(&st, B_STAT_SIZE, cookie);
	}

	return B_OK;
}


status_t
Inode::CloseAttr(OpenAttrCookie* cookie)
{
	if (cookie->fOpenState->fDelegation != NULL) {
		cookie->fOpenState->fDelegation->GiveUp();
		fFileSystem->RemoveDelegation(cookie->fOpenState->fDelegation);
	}

	delete cookie->fOpenState->fDelegation;
	delete cookie->fOpenState;
	return B_OK;
}


status_t
Inode::ReadDirect(OpenStateCookie* cookie, off_t pos, void* buffer, size_t* _length,
	bool* eof)
{
	*eof = false;
	uint32 size = 0;

	uint32 ioSize = fFileSystem->Root()->IOSize();
	*_length = min_c(ioSize, *_length);

	status_t result;
	OpenState* state = cookie != NULL ? cookie->fOpenState : fOpenState;
	while (size < *_length && !*eof) {
		uint32 len = *_length - size;
		result = ReadFile(cookie, state, pos + size, &len,
			reinterpret_cast<char*>(buffer) + size, eof);
		if (result != B_OK) {
			if (size == 0)
				return result;
			else
				break;
		}

		size += len;
	}

	*_length = size;

	return B_OK;
}


status_t
Inode::Read(OpenFileCookie* cookie, off_t pos, void* buffer, size_t* _length)
{
	bool eof = false;
	if ((cookie->fMode & O_NOCACHE) != 0)
		return ReadDirect(cookie, pos, buffer, _length, &eof);
	return file_cache_read(fFileCache, cookie, pos, buffer, _length);
}


status_t
Inode::WriteDirect(OpenStateCookie* cookie, off_t pos, const void* _buffer,
	size_t *_length)
{
	uint32 size = 0;
	const char* buffer = reinterpret_cast<const char*>(_buffer);

	uint32 ioSize = fFileSystem->Root()->IOSize();
	*_length = min_c(ioSize, *_length);

	bool attribute = false;
	OpenState* state = fOpenState;
	if (cookie != NULL) {
		attribute = cookie->fOpenState->fInfo.fHandle != fInfo.fHandle;
		state = cookie->fOpenState;
	}

	if (!attribute)
		fWriteDirty = true;

	while (size < *_length) {
		uint32 len = *_length - size;
		status_t result = WriteFile(cookie, state, pos + size, &len,
			buffer + size, attribute);
		if (result != B_OK) {
			if (size == 0)
				return result;
			else
				break;
		}

		size += len;
	}

	*_length = size;

	fMetaCache.GrowFile(size + pos);
	fFileSystem->Root()->MakeInfoInvalid();

	return B_OK;
}


status_t
Inode::Write(OpenFileCookie* cookie, off_t pos, const void* _buffer,
	size_t *_length)
{
	struct stat st;
	status_t result = Stat(&st);
	if (result != B_OK)
		return result;

	if ((cookie->fMode & O_APPEND) != 0)
		pos = st.st_size;

	uint64 fileSize = max_c(st.st_size, pos + *_length);
	fMaxFileSize = max_c(fMaxFileSize, fileSize);

	if ((cookie->fMode & O_NOCACHE) != 0) {
		WriteDirect(cookie, pos, _buffer, _length);
		Commit();
	}

	result = file_cache_set_size(fFileCache, fileSize);
	if (result != B_OK)
		return result;

	return file_cache_write(fFileCache, cookie, pos, _buffer, _length);
}


status_t
Inode::Commit()
{
	if (!fWriteDirty)
		return B_OK;
	status_t result = CommitWrites();
	if (result != B_OK)
		return result;
	fWriteDirty = false;
	return B_OK;
}

