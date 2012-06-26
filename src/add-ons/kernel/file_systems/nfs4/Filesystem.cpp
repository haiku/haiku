/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Filesystem.h"

#include <string.h>

#include <arpa/inet.h>
#include <net/dns_resolver.h>

#include "Request.h"
#include "Inode.h"


extern RPC::ServerManager* gRPCServerManager;
extern RPC::ProgramData* CreateNFS4Server(RPC::Server* serv);


Filesystem::Filesystem()
	:
	fPath(NULL),
	fName(NULL),
	fId(1)
{
}


Filesystem::~Filesystem()
{
	free(const_cast<char*>(fName));
	free(const_cast<char*>(fPath));
}


static const char*
sGetPath(const char* root, const char* path)
{
	int slash = 0;
	for (int i = 0; path[i] != '\0'; i++) {
		if (path[i] != root[i] || root[i] == '\0')
			break;

		if (path[i] == '/')
			slash = i;
	}

	return path + slash;
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
	while (pathStart != NULL && pathStart[0] != '\0') {
		pathEnd = strpbrk(pathStart, "/");
		if (pathEnd != NULL)
			*pathEnd = '\0';
		if (pathEnd == pathStart) {
			pathStart++;
			continue;
		}

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

	Attribute attr[] = { FATTR4_FSID, FATTR4_FS_LOCATIONS };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	reply.PutRootFH();

	for (uint32 i = 0; i < lookupCount; i++)
		reply.LookUp();

	Filehandle fh;
	reply.GetFH(&fh);

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

	if (count == 2 && values[1].fAttribute == FATTR4_FS_LOCATIONS) {
		FSLocations* locs =
			reinterpret_cast<FSLocations*>(values[1].fData.fLocations);

		fs->fPath = strdup(locs->fRootPath);

		delete locs;
	} else
		fs->fPath = NULL;

	const char* name;
	if (fsPath != NULL && fsPath[0] == '/')
		fsPath++;
	name = strrchr(fsPath, '/');
	if (name != NULL) {
		name++;
		fs->fName = strdup(name);
	} else
		fs->fName = strdup(fsPath);

	memcpy(&fs->fRootFH, &fh, sizeof(Filehandle));
	fs->fServer = serv;
	fs->fDevId = id;
	fs->fFsId = *fsid;

	FileInfo fi;
	fi.fFH = fh;
	fi.fParent = fh;
	fi.fName = strdup("/");
	fi.fPath = strdup(sGetPath(fs->fPath, fsPath));
	fs->fRoot = fi;

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
	Inode* inode;
	status_t result = Inode::CreateInode(this, fRoot, &inode);
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

	reply.PutFH();

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
	strncpy(info->volume_name, fName, B_FILE_NAME_LENGTH);

	return B_OK;
}


status_t
Filesystem::Migrate(const Filehandle& fh, const RPC::Server* serv)
{
	mutex_lock(&fMigrationLock);
	if (serv != fServer) {
		mutex_unlock(&fMigrationLock);
		return B_OK;
	}

	Request request(fServer);
	RequestBuilder& req = request.Builder();

	req.PutFH(fh);
	Attribute attr[] = { FATTR4_FS_LOCATIONS };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	status_t result = request.Send();
	if (result != B_OK) {
		mutex_unlock(&fMigrationLock);
		return result;
	}

	ReplyInterpreter& reply = request.Reply();

	reply.PutFH();

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK || count < 1) {
		mutex_unlock(&fMigrationLock);
		return result;
	}

	FSLocations* locs =
		reinterpret_cast<FSLocations*>(values[0].fData.fLocations);

	dns_resolver_module* dns;
	result = get_module(DNS_RESOLVER_MODULE_NAME,
		reinterpret_cast<module_info**>(&dns));
	if (result != B_OK) {
		mutex_unlock(&fMigrationLock);
		return result;
	}

	RPC::Server* server = fServer;
	for (uint32 i = 0; i < locs->fCount; i++) {
		for (uint32 j = 0; j < locs->fLocations[i].fCount; j++) {
			uint32 ip;
			struct in_addr addr;
			if (inet_aton(locs->fLocations[i].fLocations[j], &addr) == 0) {
					result = dns->dns_resolve(locs->fLocations[i].fLocations[j],
						&ip);
					if (result != B_OK)
						continue;
			} else
				ip = addr.s_addr;

			if (gRPCServerManager->Acquire(&fServer, ip, 2049,
				ProtocolUDP, CreateNFS4Server) == B_OK) {

				free(const_cast<char*>(fPath));
				fPath = strdup(locs->fLocations[j].fRootPath);
				break;
			}
		}
	}

	put_module(DNS_RESOLVER_MODULE_NAME);
	delete locs;

	if (server == fServer) {
		mutex_unlock(&fMigrationLock);
		return B_ERROR;
	}

	gRPCServerManager->Release(server);

	mutex_unlock(&fMigrationLock);

	return B_OK;
}

