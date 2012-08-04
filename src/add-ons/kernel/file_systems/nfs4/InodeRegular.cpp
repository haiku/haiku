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
Inode::CreateState(const char* name, int mode, int perms, OpenState* state) {
	uint64 fileID;
	FileHandle handle;
	ChangeInfo changeInfo;

	status_t result = CreateFile(name, mode, perms, state, &changeInfo,
		&fileID, &handle);
	if (result != B_OK)
		return result;

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
	ino_t* id)
{
	cookie->fMode = mode;
	cookie->fLocks = NULL;

	MutexLocker _(fStateLock);
	int openMode = mode & O_RWMASK;

	if (openMode == O_WRONLY || openMode == O_RDWR) {
		OpenState* state = new OpenState;
		status_t result = CreateState(name, O_WRONLY, perms, state);
		if (result != B_OK)
			return result;

		fWriteState = state;
		*id = FileIdToInoT(cookie->fWriteState->fInfo.fFileId);
		cookie->fWriteState = fWriteState;
	}

	if (openMode == O_RDONLY || openMode == O_RDWR) {
		OpenState* state = new OpenState;
		state->fInfo = fInfo;
		state->fFileSystem = fFileSystem;

		status_t result;
		if (openMode == O_RDWR)
			result = OpenFile(state, O_RDONLY);
		else
			result = CreateState(name, O_RDONLY, perms, state);

		if (result != B_OK) {
			if (fWriteState != NULL)
				if (fWriteState->ReleaseReference() == 1)
					fWriteState = NULL;
			return result;
		}

		fReadState = state;
		*id = FileIdToInoT(cookie->fReadState->fInfo.fFileId);
		cookie->fReadState = fReadState;
	}

	cookie->fFileSystem = fFileSystem;
	cookie->fClientID = fFileSystem->NFSServer()->ClientId();

	fFileSystem->AddOpenFile(cookie);
	fFileSystem->Root()->MakeInfoInvalid();

	return B_OK;
}


status_t
Inode::Open(int mode, OpenFileCookie* cookie)
{
	MutexLocker _(fStateLock);
	int openMode = mode & O_RWMASK;

	if (openMode == O_WRONLY || openMode == O_RDWR) {
		if (fWriteState == NULL) {
			OpenState* state = new OpenState;
			if (state == NULL)
				return B_NO_MEMORY;

			state->fInfo = fInfo;
			state->fFileSystem = fFileSystem;
			status_t result = OpenFile(state, O_WRONLY);
			if (result != B_OK)
				return result;

			fWriteState = state;
		} else
			fWriteState->AcquireReference();

		cookie->fWriteState = fWriteState;
	}

	if (openMode == O_RDONLY || openMode == O_RDWR) {
		if (fReadState == NULL) {
			OpenState* state = new OpenState;
			if (state == NULL) {
				if (fWriteState != NULL)
					if (fWriteState->ReleaseReference() == 1)
						fWriteState = NULL;
				return B_NO_MEMORY;
			}

			state->fInfo = fInfo;
			state->fFileSystem = fFileSystem;
			status_t result = OpenFile(state, O_RDONLY);
			if (result != B_OK) {
				if (fWriteState != NULL)
					if (fWriteState->ReleaseReference() == 1)
						fWriteState = NULL;
				return result;
			}

			fReadState = state;
		} else
			fReadState->AcquireReference();

		cookie->fReadState = fReadState;
	}

	cookie->fClientID = fFileSystem->NFSServer()->ClientId();
	cookie->fFileSystem = fFileSystem;
	cookie->fMode = mode;
	cookie->fLocks = NULL;

	fFileSystem->AddOpenFile(cookie);

	return B_OK;
}


status_t
Inode::Close(OpenFileCookie* cookie)
{
	fFileSystem->RemoveOpenFile(cookie);

	MutexLocker _(fStateLock);
	if (cookie->fWriteState != NULL)
		if (cookie->fWriteState->ReleaseReference() == 1)
			fWriteState = NULL;

	if (cookie->fReadState != NULL)
		if (cookie->fReadState->ReleaseReference() == 1)
			fReadState = NULL;

	return B_OK;
}


status_t
Inode::Read(OpenFileCookie* cookie, off_t pos, void* buffer, size_t* _length,
	bool* eof)
{
	*eof = false;
	uint32 size = 0;

	status_t result;
	while (size < *_length && !*eof) {
		uint32 len = *_length - size;
		result = ReadFile(cookie, fReadState, pos + size, &len,
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
Inode::Write(OpenFileCookie* cookie, off_t pos, const void* _buffer,
	size_t *_length)
{
	uint32 size = 0;
	const char* buffer = reinterpret_cast<const char*>(_buffer);

	fWriteDirty = true;

	while (size < *_length) {
		uint32 len = *_length - size;
		status_t result = WriteFile(cookie, fWriteState, pos + size, &len,
			buffer + size);
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

