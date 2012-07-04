/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Filesystem.h"

#include <arpa/inet.h>
#include <string.h>

#include <lock.h>
#include <net/dns_resolver.h>

#include "Request.h"
#include "RootInode.h"


extern RPC::ServerManager* gRPCServerManager;
extern RPC::ProgramData* CreateNFS4Server(RPC::Server* serv);


Filesystem::Filesystem()
	:
	fNext(NULL),
	fPrev(NULL),
	fOpenFiles(NULL),
	fOpenCount(0),
	fPath(NULL),
	fRoot(NULL),
	fId(1)
{
	mutex_init(&fOpenLock, NULL);
}


Filesystem::~Filesystem()
{
	NFSServer()->RemoveFilesystem(this);

	mutex_destroy(&fOpenLock);

	free(const_cast<char*>(fPath));
	delete fRoot;
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

	uint32 lookupCount = 0;
	status_t result = FileInfo::ParsePath(req, lookupCount, fsPath);
	if (result != B_OK)
		return result;

	req.GetFH();
	req.Access();

	Attribute attr[] = { FATTR4_SUPPORTED_ATTRS, FATTR4_FH_EXPIRE_TYPE,
		FATTR4_FSID, FATTR4_FS_LOCATIONS };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	result = request.Send();
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
	if (result != B_OK || count < 2)
		return result;

	Filesystem* fs = new(std::nothrow) Filesystem;
	if (fs == NULL)
		return B_NO_MEMORY;

	// FATTR4_SUPPORTED_ATTRS is mandatory
	memcpy(fs->fSupAttrs, &values[0].fData.fValue64, sizeof(fs->fSupAttrs));

	// FATTR4_FH_EXPIRE_TYPE is mandatory
	fs->fExpireType = values[1].fData.fValue32;

	// FATTR4_FSID is mandatory
	FilesystemId* fsid =
		reinterpret_cast<FilesystemId*>(values[2].fData.fPointer);

	if (count == 4 && values[3].fAttribute == FATTR4_FS_LOCATIONS) {
		FSLocations* locs =
			reinterpret_cast<FSLocations*>(values[3].fData.fLocations);

		fs->fPath = strdup(locs->fRootPath);
	} else
		fs->fPath = NULL;

	FileInfo fi;
	const char* name;
	if (fsPath != NULL && fsPath[0] == '/')
		fsPath++;
	name = strrchr(fsPath, '/');
	if (name != NULL) {
		name++;
		fi.fName = strdup(name);
	} else
		fi.fName = strdup(fsPath);

	fs->fServer = serv;
	fs->fDevId = id;
	fs->fFsId = *fsid;

	fi.fHandle = fh;
	fi.fParent = fh;
	fi.fPath = strdup(sGetPath(fs->fPath, fsPath));

	delete[] values;

	if (fi.fName == NULL || fi.fPath == NULL) {
		delete fs;
		return B_NO_MEMORY;
	}

	Inode* inode;
	result = Inode::CreateInode(fs, fi, &inode);
	if (result != B_OK) {
		delete fs;
		return result;
	}

	fs->fRoot = reinterpret_cast<RootInode*>(inode);

	fs->NFSServer()->AddFilesystem(fs);
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


status_t
Filesystem::Migrate(const RPC::Server* serv)
{
	MutexLocker _(fOpenLock);
	if (serv != fServer)
		return B_OK;

	if (!fRoot->ProbeMigration())
		return B_OK;

	AttrValue* values;
	status_t result = fRoot->GetLocations(&values);
	if (result != B_OK)
		return result;

	FSLocations* locs =
		reinterpret_cast<FSLocations*>(values[0].fData.fLocations);

	dns_resolver_module* dns;
	result = get_module(DNS_RESOLVER_MODULE_NAME,
		reinterpret_cast<module_info**>(&dns));
	if (result != B_OK) {
		delete[] values;
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

				if (fPath == NULL) {
					gRPCServerManager->Release(fServer);
					fServer = server;
					put_module(DNS_RESOLVER_MODULE_NAME);
					delete[] values;
					return B_NO_MEMORY;
				}

				break;
			}
		}
	}

	put_module(DNS_RESOLVER_MODULE_NAME);
	delete[] values;

	if (server == fServer) {
		gRPCServerManager->Release(server);
		return B_ERROR;
	}

	NFS4Server* old = reinterpret_cast<NFS4Server*>(server->PrivateData());
	old->RemoveFilesystem(this);
	NFSServer()->AddFilesystem(this);

	gRPCServerManager->Release(server);

	return B_OK;
}


OpenFileCookie*
Filesystem::OpenFilesLock()
{
	mutex_lock(&fOpenLock);
	return fOpenFiles;
}


void
Filesystem::OpenFilesUnlock()
{
	mutex_unlock(&fOpenLock);
}


void
Filesystem::AddOpenFile(OpenFileCookie* cookie)
{
	MutexLocker _(fOpenLock);

	cookie->fPrev = NULL;
	cookie->fNext = fOpenFiles;
	if (fOpenFiles != NULL)
		fOpenFiles->fPrev = cookie;
	fOpenFiles = cookie;
	NFSServer()->IncUsage();
}


void
Filesystem::RemoveOpenFile(OpenFileCookie* cookie)
{
	MutexLocker _(fOpenLock);
	if (cookie == fOpenFiles)
		fOpenFiles = cookie->fNext;

	if (cookie->fNext)
		cookie->fNext->fPrev = cookie->fPrev;
	if (cookie->fPrev)
		cookie->fPrev->fNext = cookie->fNext;
	NFSServer()->DecUsage();
}

