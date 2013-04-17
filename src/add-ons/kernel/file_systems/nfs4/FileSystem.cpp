/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "FileSystem.h"

#include <string.h>

#include <AutoDeleter.h>
#include <lock.h>
#include <util/Random.h>

#include "Request.h"
#include "RootInode.h"


extern RPC::ServerManager* gRPCServerManager;
extern RPC::ProgramData* CreateNFS4Server(RPC::Server* serv);


FileSystem::FileSystem(const MountConfiguration& configuration)
	:
	fOpenCount(0),
	fOpenOwnerSequence(0),
	fNamedAttrs(true),
	fPath(NULL),
	fRoot(NULL),
	fId(1),
	fConfiguration(configuration)
{
	fOpenOwner = get_random<uint64>();

	mutex_init(&fOpenOwnerLock, NULL);
	mutex_init(&fOpenLock, NULL);
	mutex_init(&fDelegationLock, NULL);
	mutex_init(&fCreateFileLock, NULL);
}


FileSystem::~FileSystem()
{
	NFS4Server* server = reinterpret_cast<NFS4Server*>(fServer->PrivateData());
	if (server != NULL)
		server->RemoveFileSystem(this);

	mutex_destroy(&fDelegationLock);
	mutex_destroy(&fOpenLock);
	mutex_destroy(&fOpenOwnerLock);
	mutex_destroy(&fCreateFileLock);

	if (fPath != NULL) {
		for (uint32 i = 0; fPath[i] != NULL; i++)
			free(const_cast<char*>(fPath[i]));
	}
	delete[] fPath;

	delete fRoot;
}


static InodeNames*
GetInodeNames(const char** root, const char* _path)
{
	ASSERT(_path != NULL);

	int i;
	char* path = strdup(_path);
	if (path == NULL)
		return NULL;
	MemoryDeleter _(path);

	if (root != NULL) {
		for (i = 0; root[i] != NULL; i++) {
			char* pathEnd = strchr(path, '/');
			if (pathEnd == path) {
				path++;
				i--;
				continue;
			}

			if (pathEnd == NULL) {
				path = NULL;
				break;
			} else
				path = pathEnd + 1;
		}
	}

	InodeNames* names = NULL;
	if (path == NULL) {
		names = new InodeNames;
		if (names == NULL)
			return NULL;

		names->AddName(NULL, "");
		return names;
	}

	do {
		char* pathEnd = strchr(path, '/');
		if (pathEnd != NULL)
			*pathEnd = '\0';

		InodeNames* name = new InodeNames;
		if (name == NULL) {
			delete names;
			return NULL;
		}

		name->AddName(names, path);
		names = name;
		if (pathEnd == NULL)
			break;

		path = pathEnd + 1;
	} while (*path != '\0');

	return names;
}


status_t
FileSystem::Mount(FileSystem** _fs, RPC::Server* serv, const char* fsPath,
	dev_t id, const MountConfiguration& configuration)
{
	ASSERT(_fs != NULL);
	ASSERT(serv != NULL);
	ASSERT(fsPath != NULL);

	FileSystem* fs = new(std::nothrow) FileSystem(configuration);
	if (fs == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<FileSystem> fsDeleter(fs);

	Request request(serv, fs);
	RequestBuilder& req = request.Builder();

	req.PutRootFH();

	uint32 lookupCount = 0;
	status_t result = _ParsePath(req, lookupCount, fsPath);
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
	else if ((allowed & (ACCESS4_READ | ACCESS4_LOOKUP))
		!= (ACCESS4_READ | ACCESS4_LOOKUP))
		return B_PERMISSION_DENIED;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK || count < 2)
		return result;

	// FATTR4_SUPPORTED_ATTRS is mandatory
	memcpy(fs->fSupAttrs, &values[0].fData.fValue64, sizeof(fs->fSupAttrs));

	// FATTR4_FH_EXPIRE_TYPE is mandatory
	fs->fExpireType = values[1].fData.fValue32;

	// FATTR4_FSID is mandatory
	FileSystemId* fsid
		= reinterpret_cast<FileSystemId*>(values[2].fData.fPointer);

	if (count == 4 && values[3].fAttribute == FATTR4_FS_LOCATIONS) {
		FSLocations* locs
			= reinterpret_cast<FSLocations*>(values[3].fData.fLocations);

		fs->fPath = locs->fRootPath;
		locs->fRootPath = NULL;
	} else
		fs->fPath = NULL;

	FileInfo fi;

	fs->fServer = serv;
	fs->fDevId = id;
	fs->fFsId = *fsid;

	fi.fHandle = fh;

	fi.fNames = GetInodeNames(fs->fPath, fsPath);
	if (fi.fNames == NULL) {
		delete[] values;
		return B_NO_MEMORY;
	}
	fi.fNames->fHandle = fh;

	delete[] values;

	Inode* inode;
	result = Inode::CreateInode(fs, fi, &inode);
	if (result != B_OK)
		return result;

	char* name = strrchr(fsPath, '/');
	if (name != NULL) {
		name++;
		reinterpret_cast<RootInode*>(inode)->SetName(name);
	} else if (fsPath[0] != '\0')
		reinterpret_cast<RootInode*>(inode)->SetName(fsPath);
	else {
		char* address = serv->ID().UniversalAddress();
		if (address != NULL)
			reinterpret_cast<RootInode*>(inode)->SetName(address);
		else
			reinterpret_cast<RootInode*>(inode)->SetName("NFS4 Share");
		free(address);
	}

	fs->fRoot = reinterpret_cast<RootInode*>(inode);

	fs->NFSServer()->AddFileSystem(fs);
	*_fs = fs;

	fsDeleter.Detach();
	return B_OK;
}


status_t
FileSystem::GetInode(ino_t id, Inode** _inode)
{
	ASSERT(_inode != NULL);

	FileInfo fi;
	status_t result = fInoIdMap.GetFileInfo(&fi, id);
	ASSERT(result != B_ENTRY_NOT_FOUND);

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
	ASSERT(serv != NULL);

	MutexLocker _(fOpenLock);
	if (serv != fServer)
		return B_OK;

	if (!fRoot->ProbeMigration())
		return B_OK;

	AttrValue* values;
	status_t result = fRoot->GetLocations(&values);
	if (result != B_OK)
		return result;

	FSLocations* locs
		= reinterpret_cast<FSLocations*>(values[0].fData.fLocations);

	RPC::Server* server = fServer;
	for (uint32 i = 0; i < locs->fCount; i++) {
		for (uint32 j = 0; j < locs->fLocations[i].fCount; j++) {
			AddressResolver resolver(locs->fLocations[i].fLocations[j]);

			if (gRPCServerManager->Acquire(&fServer, &resolver,
					CreateNFS4Server) == B_OK) {

				if (fPath != NULL) {
					for (uint32 i = 0; fPath[i] != NULL; i++)
						free(const_cast<char*>(fPath[i]));
				}
				delete[] fPath;

				fPath = locs->fLocations[i].fRootPath;
				locs->fLocations[i].fRootPath = NULL;

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


DoublyLinkedList<OpenState>&
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
	ASSERT(state != NULL);

	MutexLocker _(fOpenLock);

	fOpenFiles.InsertBefore(fOpenFiles.Head(), state);

	NFSServer()->IncUsage();
}


void
FileSystem::RemoveOpenFile(OpenState* state)
{
	ASSERT(state != NULL);

	MutexLocker _(fOpenLock);

	fOpenFiles.Remove(state);

	NFSServer()->DecUsage();
}


DoublyLinkedList<Delegation>&
FileSystem::DelegationsLock()
{
	mutex_lock(&fDelegationLock);
	return fDelegationList;
}


void
FileSystem::DelegationsUnlock()
{
	mutex_unlock(&fDelegationLock);
}


void
FileSystem::AddDelegation(Delegation* delegation)
{
	ASSERT(delegation != NULL);

	MutexLocker _(fDelegationLock);

	fDelegationList.InsertBefore(fDelegationList.Head(), delegation);

	fHandleToDelegation.Remove(delegation->fInfo.fHandle);
	fHandleToDelegation.Insert(delegation->fInfo.fHandle, delegation);
}


void
FileSystem::RemoveDelegation(Delegation* delegation)
{
	ASSERT(delegation != NULL);

	MutexLocker _(fDelegationLock);

	fDelegationList.Remove(delegation);
	fHandleToDelegation.Remove(delegation->fInfo.fHandle);
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


status_t
FileSystem::_ParsePath(RequestBuilder& req, uint32& count, const char* _path)
{
	ASSERT(_path != NULL);

	char* path = strdup(_path);
	if (path == NULL)
		return B_NO_MEMORY;

	char* pathStart = path;
	char* pathEnd;

	while (pathStart != NULL) {
		pathEnd = strchr(pathStart, '/');
		if (pathEnd != NULL)
			*pathEnd = '\0';

		if (pathEnd != pathStart) {
			if (!strcmp(pathStart, "..")) {
				req.LookUpUp();
				count++;
			} else if (strcmp(pathStart, ".")) {
				req.LookUp(pathStart);
				count++;
			}
		}

		if (pathEnd != NULL && pathEnd[1] != '\0')
			pathStart = pathEnd + 1;
		else
			pathStart = NULL;
	}
	free(path);

	return B_OK;
}

