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

#include "Node.h"
#include "PackageDomain.h"


class Directory;
class Node;


class Volume {
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

			status_t			Mount();
			void				Unmount();

			Node*				FindNode(ino_t nodeID) const
									{ return fNodes.Lookup(nodeID); }

			// VFS wrappers
			status_t			GetVNode(ino_t nodeID, Node*& _node);
			status_t			PublishVNode(Node* node);

			status_t			AddPackageDomain(const char* path);

private:
			struct Job;
			struct AddPackageDomainJob;
			struct PackageLoaderErrorOutput;
			struct PackageLoaderContentHandler;

			typedef DoublyLinkedList<Job> JobList;
			typedef DoublyLinkedList<PackageDomain> PackageDomainList;

private:
	static	status_t			_PackageLoaderEntry(void* data);
			status_t			_PackageLoader();

			void				_TerminatePackageLoader();

			void				_PushJob(Job* job);

			status_t			_AddInitialPackageDomain(const char* path);
			status_t			_AddPackageDomain(PackageDomain* domain);
			status_t			_LoadPackage(Package* package);
			status_t			_AddPackageContent(Package* package);
			status_t			_AddPackageContentRootNode(Package* package,
									PackageNode* node);
			status_t			_AddPackageNode(Directory* directory,
									PackageNode* packageNode, Node*& _node);

			status_t			_CreateNode(mode_t mode, Directory* parent,
									const char* name, Node*& _node);

private:
			rw_lock				fLock;
			fs_volume*			fFSVolume;
			Directory*			fRootDirectory;
			thread_id			fPackageLoader;
			PackageDomainList	fPackageDomains;

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
