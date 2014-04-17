/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <fs_interface.h>

#include <condition_variable.h>
#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/KMessage.h>

#include <packagefs.h>

#include "Index.h"
#include "Node.h"
#include "NodeListener.h"
#include "Package.h"
#include "PackageLinksListener.h"
#include "PackagesDirectory.h"
#include "PackageSettings.h"
#include "Query.h"


class Directory;
class PackageFSRoot;
class PackagesDirectory;
class UnpackingNode;

typedef IndexHashTable::Iterator IndexDirIterator;


typedef PackageFSMountType MountType;


class Volume : public DoublyLinkedListLinkImpl<Volume>,
	private PackageLinksListener {
public:
								Volume(fs_volume* fsVolume);
								~Volume();

	inline	bool				ReadLock() const;
	inline	void				ReadUnlock() const;
	inline	bool				WriteLock();
	inline	void				WriteUnlock();

			fs_volume*			FSVolume() const	{ return fFSVolume; }
			dev_t				ID() const			{ return fFSVolume->id; }
			Directory*			RootDirectory() const { return fRootDirectory; }

			::MountType			MountType() const	{ return fMountType; }

			void				SetPackageFSRoot(::PackageFSRoot* root)
									{ fPackageFSRoot = root; }
			::PackageFSRoot*	PackageFSRoot() const
									{ return fPackageFSRoot; }

			dev_t				MountPointDeviceID() const
									{ return fMountPoint.deviceID; }
			ino_t				MountPointNodeID() const
									{ return fMountPoint.nodeID; }

			status_t			Mount(const char* parameterString);
			void				Unmount();

			Node*				FindNode(ino_t nodeID) const
									{ return fNodes.Lookup(nodeID); }

			status_t			IOCtl(Node* node, uint32 operation,
									void* buffer, size_t size);

			// node listeners -- volume must be write-locked
			void				AddNodeListener(NodeListener* listener,
									Node* node);
			void				RemoveNodeListener(NodeListener* listener);

			// query support -- volume must be write-locked
			void				AddQuery(Query* query);
			void				RemoveQuery(Query* query);
			void				UpdateLiveQueries(Node* node,
									const char* attribute, int32 type,
									const void* oldKey, size_t oldLength,
									const void* newKey, size_t newLength);

			Index*				FindIndex(const StringKey& name) const
									{ return fIndices.Lookup(name); }
			IndexDirIterator	GetIndexDirIterator() const
									{ return fIndices.GetIterator(); }

			// VFS wrappers
			status_t			GetVNode(ino_t nodeID, Node*& _node);
			status_t			PutVNode(ino_t nodeID);
			status_t			RemoveVNode(ino_t nodeID);
			status_t			PublishVNode(Node* node);

private:
	// PackageLinksListener
	virtual	void				PackageLinkNodeAdded(Node* node);
	virtual	void				PackageLinkNodeRemoved(Node* node);
	virtual	void				PackageLinkNodeChanged(Node* node,
									uint32 statFields,
									const OldNodeAttributes& oldAttributes);

private:
			struct ShineThroughDirectory;
			struct ActivationChangeRequest;

private:
			status_t			_LoadOldPackagesStates(
									const char* packagesState);

			status_t			_AddInitialPackages();
			status_t			_AddInitialPackagesFromActivationFile(
									PackagesDirectory* packagesDirectory);
			status_t			_AddInitialPackagesFromDirectory();
			status_t			_LoadAndAddInitialPackage(
									PackagesDirectory* packagesDirectory,
									const char* name);

	inline	void				_AddPackage(Package* package);
	inline	void				_RemovePackage(Package* package);
			void				_RemoveAllPackages();
	inline	Package*			_FindPackage(const char* fileName) const;

			status_t			_AddPackageContent(Package* package,
									bool notify);
			void				_RemovePackageContent(Package* package,
									PackageNode* endNode, bool notify);

			status_t			_AddPackageContentRootNode(Package* package,
									PackageNode* node, bool notify);
			void				_RemovePackageContentRootNode(Package* package,
									PackageNode* packageNode,
									PackageNode* endPackageNode, bool notify);

			status_t			_AddPackageNode(Directory* directory,
									PackageNode* packageNode, bool notify,
									Node*& _node);
			void				_RemovePackageNode(Directory* directory,
									PackageNode* packageNode, Node* node,
									bool notify);

			status_t			_CreateUnpackingNode(mode_t mode,
									Directory* parent, const String& name,
									UnpackingNode*& _node);
									// does *not* return a reference
			void				_RemoveNode(Node* node);
			void				_RemoveNodeAndVNode(Node* node);
									// caller must hold a reference

			status_t			_LoadPackage(
									PackagesDirectory* packagesDirectory,
									const char* name, Package*& _package);

			status_t			_ChangeActivation(
									ActivationChangeRequest& request);

			status_t			_InitMountType(const char* mountType);
			status_t			_CreateShineThroughDirectory(Directory* parent,
									const char* name, Directory*& _directory);
			status_t			_CreateShineThroughDirectories(
									const char* shineThroughSetting);
			status_t			_PublishShineThroughDirectories();

			status_t			_AddPackageLinksDirectory();
			void				_RemovePackageLinksDirectory();
			void				_AddPackageLinksNode(Node* node);
			void				_RemovePackageLinksNode(Node* node);

	inline	Volume*				_SystemVolumeIfNotSelf() const;

			void				_NotifyNodeAdded(Node* node);
			void				_NotifyNodeRemoved(Node* node);
			void				_NotifyNodeChanged(Node* node,
									uint32 statFields,
									const OldNodeAttributes& oldAttributes);

private:
	mutable	rw_lock				fLock;
			fs_volume*			fFSVolume;
			Directory*			fRootDirectory;
			::PackageFSRoot*	fPackageFSRoot;
			::MountType			fMountType;
			PackagesDirectory*	fPackagesDirectory;
			PackagesDirectoryList fPackagesDirectories;
			PackagesDirectoryHashTable fPackagesDirectoriesByNodeRef;
			PackageSettings		fPackageSettings;

			struct {
				dev_t			deviceID;
				ino_t			nodeID;
			} fMountPoint;

			NodeIDHashTable		fNodes;
			NodeListenerHashTable fNodeListeners;
			PackageFileNameHashTable fPackages;
			QueryList			fQueries;
			IndexHashTable		fIndices;

			ino_t				fNextNodeID;
};


bool
Volume::ReadLock() const
{
	return rw_lock_read_lock(&fLock) == B_OK;
}


void
Volume::ReadUnlock() const
{
	rw_lock_read_unlock(&fLock);
}


bool
Volume::WriteLock()
{
	return rw_lock_write_lock(&fLock) == B_OK;
}


void
Volume::WriteUnlock()
{
	rw_lock_write_unlock(&fLock);
}


typedef AutoLocker<const Volume, AutoLockerReadLocking<const Volume> >
	VolumeReadLocker;
typedef AutoLocker<Volume, AutoLockerWriteLocking<Volume> > VolumeWriteLocker;


#endif	// VOLUME_H
