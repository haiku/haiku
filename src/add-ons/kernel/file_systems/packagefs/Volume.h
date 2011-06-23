/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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

#include "Node.h"
#include "PackageDomain.h"


class Directory;
class Node;
class PackageFSRoot;


enum MountType {
	MOUNT_TYPE_SYSTEM,
	MOUNT_TYPE_COMMON,
	MOUNT_TYPE_HOME,
	MOUNT_TYPE_CUSTOM
};


class Volume : public DoublyLinkedListLinkImpl<Volume> {
public:
								Volume(fs_volume* fsVolume);
								~Volume();

	inline	bool				ReadLock();
	inline	void				ReadUnlock();
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

			// VFS wrappers
			status_t			GetVNode(ino_t nodeID, Node*& _node);
			status_t			PutVNode(ino_t nodeID);
			status_t			RemoveVNode(ino_t nodeID);
			status_t			PublishVNode(Node* node);

			status_t			AddPackageDomain(const char* path);

private:
			struct Job;
			struct AddPackageDomainJob;
			struct DomainDirectoryEventJob;
			struct PackageLoaderErrorOutput;
			struct PackageLoaderContentHandler;
			struct DomainDirectoryListener;

			friend struct AddPackageDomainJob;
			friend struct DomainDirectoryEventJob;
			friend struct DomainDirectoryListener;

			typedef DoublyLinkedList<Job> JobList;
			typedef DoublyLinkedList<PackageDomain> PackageDomainList;

private:
	static	status_t			_PackageLoaderEntry(void* data);
			status_t			_PackageLoader();

			void				_TerminatePackageLoader();

			void				_PushJob(Job* job);

			status_t			_AddInitialPackageDomain(const char* path);
			status_t			_AddPackageDomain(PackageDomain* domain,
									bool notify);
			status_t			_LoadPackage(Package* package);

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

			status_t			_CreateNode(mode_t mode, Directory* parent,
									const char* name, Node*& _node);
									// does *not* return a reference
			void				_RemoveNode(Node* node);

			void				_DomainListenerEventOccurred(
									PackageDomain* domain,
									const KMessage* event);
			void				_DomainEntryCreated(PackageDomain* domain,
									dev_t deviceID, ino_t directoryID,
									ino_t nodeID, const char* name,
									bool addContent, bool notify);
			void				_DomainEntryRemoved(PackageDomain* domain,
									dev_t deviceID, ino_t directoryID,
									ino_t nodeID, const char* name,
									bool notify);
			void				_DomainEntryMoved(PackageDomain* domain,
									dev_t deviceID, ino_t fromDirectoryID,
									ino_t toDirectoryID, dev_t nodeDeviceID,
									ino_t nodeID, const char* fromName,
									const char* name, bool notify);

			status_t			_InitMountType(const char* mountType);
			status_t			_CreateShineThroughDirectories(
									const char* shineThroughSetting);
			status_t			_AddPackageLinksDirectory();
private:
			rw_lock				fLock;
			fs_volume*			fFSVolume;
			Directory*			fRootDirectory;
			::PackageFSRoot*	fPackageFSRoot;
			::MountType			fMountType;
			thread_id			fPackageLoader;
			PackageDomainList	fPackageDomains;

			struct {
				dev_t			deviceID;
				ino_t			nodeID;
			} fMountPoint;

			NodeIDHashTable		fNodes;

			JobList				fJobQueue;
			mutex				fJobQueueLock;
			ConditionVariable	fJobQueueCondition;

			ino_t				fNextNodeID;

	volatile bool				fTerminating;
};


bool
Volume::ReadLock()
{
	return rw_lock_read_lock(&fLock) == B_OK;
}


void
Volume::ReadUnlock()
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


typedef AutoLocker<Volume, AutoLockerReadLocking<Volume> > VolumeReadLocker;
typedef AutoLocker<Volume, AutoLockerWriteLocking<Volume> > VolumeWriteLocker;


#endif	// VOLUME_H
