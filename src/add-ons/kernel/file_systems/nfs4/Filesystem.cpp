/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Filesystem.h"

#include <string.h>

#include "Request.h"

#include "Inode.h"


Filesystem::Filesystem()
	:
	fPath(NULL),
	fId(1)
{
}


Filesystem::~Filesystem()
{
	free(const_cast<char*>(fPath));
}


status_t
Filesystem::Mount(Filesystem** pfs, RPC::Server* serv, const char* fsPath,
	dev_t id)
{
	Request request(serv);
	RequestBuilder& req = request.Builder();

	req.PutRootFH();

	// Better way of doing this will be needed
	uint32 lookupCount = 0;
	char* path = strdup(fsPath);
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

		lookupCount++;
	}
	free(path);

	req.GetFH();
	req.Access();

	Attribute attr[] = { FATTR4_FSID };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

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

	Filehandle fh;
	result = reply.GetFH(&fh);
	if (result != B_OK)
		return result;

	uint32 allowed;
	result = reply.Access(NULL, &allowed);
	if (result != B_OK)
		return result;
	else if (allowed & (ACCESS4_READ | ACCESS4_LOOKUP)
				!= (ACCESS4_READ | ACCESS4_LOOKUP))
		return B_PERMISSION_DENIED;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK || count < 1)
		return result;

	// FATTR4_FSID is mandatory
	FilesystemId* fsid =
		reinterpret_cast<FilesystemId*>(values[0].fData.fPointer);

	Filesystem* fs = new(std::nothrow) Filesystem;
	fs->fPath = strdup(fsPath);
	memcpy(&fs->fRootFH, &fh, sizeof(Filehandle));
	fs->fServer = serv;
	fs->fDevId = id;
	fs->fFsId = *fsid;

	*pfs = fs;

	return B_OK;
}


status_t
Filesystem::GetInode(ino_t id, Inode** _inode)
{
	FileInfo fi;
	status_t result = fInoIdMap.GetFileInfo(&fi, id);
	if (result == B_ENTRY_NOT_FOUND)
		dprintf("NFS4: unknown inode: %llu\n", id);

	if (result != B_OK)
		return result;

	Inode* inode;
	result = Inode::CreateInode(this, fi, &inode);
	if (result != B_OK)
		return result;

	*_inode = inode;
	return B_OK;
}


Inode*
Filesystem::CreateRootInode()
{
	FileInfo fi;
	fi.fFH = fRootFH;
	fi.fParent = fRootFH;
	fi.fName = strdup("/");
	fi.fPath = strdup(fPath);

	Inode* inode;
	status_t result = Inode::CreateInode(this, fi, &inode);
	if (result == B_OK)
		return inode;
	else
		return NULL;
}


status_t
Filesystem::ReadInfo(struct fs_info* info)
{
	Request request(fServer);
	RequestBuilder& req = request.Builder();

	req.PutFH(fRootFH);
	Attribute attr[] = { FATTR4_FILES_FREE, FATTR4_FILES_TOTAL,
		FATTR4_MAXREAD, FATTR4_MAXWRITE, FATTR4_SPACE_FREE,
		FATTR4_SPACE_TOTAL };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	result = reply.PutFH();
	if (result != B_OK)
		return result;

	AttrValue* values;
	uint32 count, next = 0;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	if (count >= next && values[next].fAttribute == FATTR4_FILES_FREE) {
		info->free_nodes = values[next].fData.fValue64;
		next++;
	}

	if (count >= next && values[next].fAttribute == FATTR4_FILES_TOTAL) {
		info->total_nodes = values[next].fData.fValue64;
		next++;
	}

	uint64 io_size = LONGLONG_MAX;
	if (count >= next && values[next].fAttribute == FATTR4_MAXREAD) {
		io_size = min_c(io_size, values[next].fData.fValue64);
		next++;
	}

	if (count >= next && values[next].fAttribute == FATTR4_MAXWRITE) {
		io_size = min_c(io_size, values[next].fData.fValue64);
		next++;
	}

	if (io_size == LONGLONG_MAX)
		io_size = 32768;
	info->io_size = io_size;
	info->block_size = io_size;

	if (count >= next && values[next].fAttribute == FATTR4_SPACE_FREE) {
		info->free_blocks = values[next].fData.fValue64 / io_size;
		next++;
	}

	if (count >= next && values[next].fAttribute == FATTR4_SPACE_TOTAL) {
		info->total_blocks = values[next].fData.fValue64 / io_size;
		next++;
	}

	info->flags = B_FS_IS_READONLY;
	const char* name = strrchr(fPath, '/');
	if (name != NULL) {
		name++;
		strncpy(info->volume_name, name, B_FILE_NAME_LENGTH);
	} else
		strncpy(info->volume_name, fPath, B_FILE_NAME_LENGTH);

	return B_OK;
}

