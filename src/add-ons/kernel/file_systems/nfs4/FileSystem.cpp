/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "FileSystem.h"

#include <string.h>

#include <lock.h>

#include "Request.h"
#include "RootInode.h"


extern RPC::ServerManager* gRPCServerManager;
extern RPC::ProgramData* CreateNFS4Server(RPC::Server* serv);


FileSystem::FileSystem()
	:
	fNext(NULL),
	fPrev(NULL),
	fOpenFiles(NULL),
	fOpenCount(0),
	fOpenOwnerSequence(0),
	fPath(NULL),
	fRoot(NULL),
	fId(1)
{
	fOpenOwner = rand();
	fOpenOwner <<= 32;
	fOpenOwner |= rand();

	mutex_init(&fOpenOwnerLock, NULL);
	mutex_init(&fOpenLock, NULL);
	mutex_init(&fDelegationLock, NULL);
}


FileSystem::~FileSystem()
{
	NFSServer()->RemoveFileSystem(this);

	mutex_destroy(&fDelegationLock);
	mutex_destroy(&fOpenLock);
	mutex_destroy(&fOpenOwnerLock);

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
FileSystem::Mount(FileSystem** pfs, RPC::Server* serv, const char* fsPath,
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

	FileHandle fh;
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

	FileSystem* fs = new(std::nothrow) FileSystem;
	if (fs == NULL)
		return B_NO_MEMORY;

	// FATTR4_SUPPORTED_ATTRS is mandatory
	memcpy(fs->fSupAttrs, &values[0].fData.fValue64, sizeof(fs->fSupAttrs));

	// FATTR4_FH_EXPIRE_TYPE is mandatory
	fs->fExpireType = values[1].fData.fValue32;

	// FATTR4_FSID is mandatory
	FileSystemId* fsid =
		reinterpret_cast<FileSystemId*>(values[2].fData.fPointer);

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

	fs->NFSServer()->AddFileSystem(fs);
	*pfs = fs;

	return B_OK;
}


status_t
FileSystem::GetInode(ino_t id, Inode** _inode)
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
FileSystem::Migrate(const RPC::Server* serv)
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

	RPC::Server* server = fServer;
	ServerAddress addr = fServer->ID();
	for (uint32 i = 0; i < locs->fCount; i++) {
		for (uint32 j = 0; j < locs->fLocations[i].fCount; j++) {
			if (ServerAddress::ResolveName(locs->fLocations[i].fLocations[j],
				&addr) != B_OK)
				continue;

			if (gRPCServerManager->Acquire(&fServer, addr,
					CreateNFS4Server) == B_OK) {

				free(const_cast<char*>(fPath));
				fPath = strdup(locs->fLocations[i].fRootPath);

				if (fPath == NULL) {
					gRPCServerManager->Release(fServer);
					fServer = server;

					delete[] values;
					return B_NO_MEMORY;
				}

				break;
			}
		}
	}

	delete[] values;

	if (server == fServer) {
		gRPCServerManager->Release(server);
		return B_ERROR;
	}

	NFS4Server* old = reinterpret_cast<NFS4Server*>(server->PrivateData());
	old->RemoveFileSystem(this);
	NFSServer()->AddFileSystem(this);

	gRPCServerManager->Release(server);

	return B_OK;
}


OpenState*
FileSystem::OpenFilesLock()
{
	mutex_lock(&fOpenLock);
	return fOpenFiles;
}


void
FileSystem::OpenFilesUnlock()
{
	mutex_unlock(&fOpenLock);
}


void
FileSystem::AddOpenFile(OpenState* state)
{
	MutexLocker _(fOpenLock);

	state->fPrev = NULL;
	state->fNext = fOpenFiles;
	if (fOpenFiles != NULL)
		fOpenFiles->fPrev = state;
	fOpenFiles = state;
	NFSServer()->IncUsage();
}


void
FileSystem::RemoveOpenFile(OpenState* state)
{
	MutexLocker _(fOpenLock);
	if (state == fOpenFiles)
		fOpenFiles = state->fNext;

	if (state->fNext)
		state->fNext->fPrev = state->fPrev;
	if (state->fPrev)
		state->fPrev->fNext = state->fNext;
	NFSServer()->DecUsage();
}


void
FileSystem::AddDelegation(Delegation* delegation)
{
	MutexLocker _(fDelegationLock);

	fHandleToDelegation.Remove(delegation->fInfo.fHandle);
	fHandleToDelegation.Insert(delegation->fInfo.fHandle, delegation);

	NFSServer()->IncUsage();
}


void
FileSystem::RemoveDelegation(Delegation* delegation)
{
	MutexLocker _(fDelegationLock);

	fHandleToDelegation.Remove(delegation->fInfo.fHandle);

	NFSServer()->DecUsage();
}


Delegation*
FileSystem::GetDelegation(const FileHandle& handle)
{
	MutexLocker _(fDelegationLock);

	AVLTreeMap<FileHandle, Delegation*>::Iterator it;
	it = fHandleToDelegation.Find(handle);
	if (!it.HasCurrent())
		return NULL;

	return it.Current();
}

