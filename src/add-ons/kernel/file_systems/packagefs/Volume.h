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
#include "NodeListener.h"
#include "PackageDomain.h"
#include "PackageLinksListener.h"


class Directory;
class PackageFSRoot;
class UnpackingNode;


enum MountType {
	MOUNT_TYPE_SYSTEM,
	MOUNT_TYPE_COMMON,
	MOUNT_TYPE_HOME,
	MOUNT_TYPE_CUSTOM
};


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

			// node listeners -- volume must be write-locked
			void				AddNodeListener(NodeListener* listener,
									Node* node);
			void				RemoveNodeListener(NodeListener* listener);

			// VFS wrappers
			status_t			GetVNode(ino_t nodeID, Node*& _node);
			status_t			PutVNode(ino_t nodeID);
			status_t			RemoveVNode(ino_t nodeID);
			status_t			PublishVNode(Node* node);

			status_t			AddPackageDomain(const char* path);

private:
	// PackageLinksListener
	virtual	void				PackageLinkNodeAdded(Node* node);
	virtual	void				PackageLinkNodeRemoved(Node* node);
	virtual	void				PackageLinkNodeChanged(Node* node,
									uint32 statFields);

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
			void				_RemovePackageDomain(PackageDomain* domain);
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

			status_t			_CreateUnpackingNode(mode_t mode,
									Directory* parent, const char* name,
									UnpackingNode*& _node);
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
			void				_RemovePackageLinksDirectory();
			void				_AddPackageLinksNode(Node* node);
			void				_RemovePackageLinksNode(Node* node);

	inline	Volume*				_SystemVolumeIfNotSelf() const;

			void				_NotifyNodeAdded(Node* node);
			void				_NotifyNodeRemoved(Node* node);
			void				_NotifyNodeChanged(Node* node,
									uint32 statFields);

private:
	mutable	rw_lock				fLock;
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
			NodeListenerHashTable fNodeListeners;

			JobList				fJobQueue;
			mutex				fJobQueueLock;
			ConditionVariable	fJobQueueCondition;

			ino_t				fNextNodeID;

	volatile bool				fTerminating;
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
