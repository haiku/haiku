/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FUSEVolume.h"

#include "FUSEFileSystem.h"


inline FUSEFileSystem*
FUSEVolume::_FileSystem() const
{
	return static_cast<FUSEFileSystem*>(fFileSystem);
}


FUSEVolume::FUSEVolume(FUSEFileSystem* fileSystem, dev_t id)
	:
	Volume(fileSystem, id)
{
}


FUSEVolume::~FUSEVolume()
{
}


// #pragma mark - FS


status_t
FUSEVolume::Mount(const char* device, uint32 flags, const char* parameters,
	ino_t* rootID)
{
printf("FUSEVolume::Mount()\n");
	status_t error = _FileSystem()->InitClientFS(parameters);
	if (error != B_OK)
		RETURN_ERROR(error);

	_FileSystem()->ExitClientFS(B_UNSUPPORTED);

	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Unmount()
{
printf("FUSEVolume::Unmount()\n");
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Sync()
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::ReadFSInfo(fs_info* info)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::WriteFSInfo(const struct fs_info* info, uint32 mask)
{
	return B_UNSUPPORTED;
}


// #pragma mark - vnodes


status_t
FUSEVolume::Lookup(void* dir, const char* entryName, ino_t* vnid)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::GetVNodeType(void* node, int* type)
{
	return B_NOT_SUPPORTED;
}


status_t
FUSEVolume::GetVNodeName(void* node, char* buffer, size_t bufferSize)
{
	return B_NOT_SUPPORTED;
}


status_t
FUSEVolume::ReadVNode(ino_t vnid, bool reenter, void** node, int* type,
	uint32* flags, FSVNodeCapabilities* _capabilities)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::WriteVNode(void* node, bool reenter)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::RemoveVNode(void* node, bool reenter)
{
	return B_UNSUPPORTED;
}


// #pragma mark - asynchronous I/O


status_t
FUSEVolume::DoIO(void* node, void* cookie, const IORequestInfo& requestInfo)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::CancelIO(void* node, void* cookie, int32 ioRequestID)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::IterativeIOGetVecs(void* cookie, int32 requestID, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::IterativeIOFinished(void* cookie, int32 requestID, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	return B_UNSUPPORTED;
}


// #pragma mark - nodes


status_t
FUSEVolume::IOCtl(void* node, void* cookie, uint32 command, void *buffer,
	size_t size)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::SetFlags(void* node, void* cookie, int flags)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Select(void* node, void* cookie, uint8 event, selectsync* sync)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Deselect(void* node, void* cookie, uint8 event, selectsync* sync)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::FSync(void* node)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::ReadSymlink(void* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::CreateSymlink(void* dir, const char* name, const char* target,
	int mode)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Link(void* dir, const char* name, void* node)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Unlink(void* dir, const char* name)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Rename(void* oldDir, const char* oldName, void* newDir,
	const char* newName)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Access(void* node, int mode)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::ReadStat(void* node, struct stat* st)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::WriteStat(void* node, const struct stat *st, uint32 mask)
{
	return B_UNSUPPORTED;
}


// #pragma mark - files


status_t
FUSEVolume::Create(void* dir, const char* name, int openMode, int mode,
	void** cookie, ino_t* vnid)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Open(void* node, int openMode, void** cookie)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Close(void* node, void* cookie)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::FreeCookie(void* node, void* cookie)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Read(void* node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::Write(void* node, void* cookie, off_t pos, const void* buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	return B_UNSUPPORTED;
}


// #pragma mark - directories


status_t
FUSEVolume::CreateDir(void* dir, const char* name, int mode, ino_t *newDir)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::RemoveDir(void* dir, const char* name)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::OpenDir(void* node, void** cookie)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::CloseDir(void* node, void* cookie)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::FreeDirCookie(void* node, void* cookie)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::ReadDir(void* node, void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	return B_UNSUPPORTED;
}


status_t
FUSEVolume::RewindDir(void* node, void* cookie)
{
	return B_UNSUPPORTED;
}
