/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FS_ROOT_H
#define PACKAGE_FS_ROOT_H


#include <Referenceable.h>

#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <lock.h>

#include "Volume.h"


class PackageLinksDirectory;


class PackageFSRoot : private BReferenceable,
	public DoublyLinkedListLinkImpl<PackageFSRoot> {
public:
	// constructor and destructor are conceptually private
								PackageFSRoot(dev_t deviceID, ino_t nodeID);
	virtual						~PackageFSRoot();

	static	status_t			GlobalInit();
	static	void				GlobalUninit();

	inline	bool				ReadLock() const;
	inline	void				ReadUnlock() const;
	inline	bool				WriteLock();
	inline	void				WriteUnlock();

			status_t			Init();

	static	status_t			RegisterVolume(Volume* volume);
			void				UnregisterVolume(Volume* volume);

			status_t			AddPackage(Package* package);
			void				RemovePackage(Package* package);

			dev_t				DeviceID() const	{ return fDeviceID; }
			ino_t				NodeID() const		{ return fNodeID; }
			bool				IsCustom() const	{ return fDeviceID < 0; }

			Volume*				SystemVolume() const;
			PackageLinksDirectory* GetPackageLinksDirectory() const
									{ return fPackageLinksDirectory; }

private:
			typedef DoublyLinkedList<PackageFSRoot> RootList;
			typedef DoublyLinkedList<Volume> VolumeList;

private:
			status_t			_AddVolume(Volume* volume);
			void				_RemoveVolume(Volume* volume);

	static	status_t			_GetOrCreateRoot(dev_t deviceID, ino_t nodeID,
									PackageFSRoot*& _root);
	static	PackageFSRoot*		_FindRootLocked(dev_t deviceID, ino_t nodeID);
	static	void				_PutRoot(PackageFSRoot* root);

private:
	static	mutex				sRootListLock;
	static	RootList			sRootList;

	mutable	rw_lock				fLock;
			dev_t				fDeviceID;
			ino_t				fNodeID;
			VolumeList			fVolumes;
			Volume*				fSystemVolume;
			PackageLinksDirectory* fPackageLinksDirectory;
};


bool
PackageFSRoot::ReadLock() const
{
	return rw_lock_read_lock(&fLock) == B_OK;
}


void
PackageFSRoot::ReadUnlock() const
{
	rw_lock_read_unlock(&fLock);
}


bool
PackageFSRoot::WriteLock()
{
	return rw_lock_write_lock(&fLock) == B_OK;
}


void
PackageFSRoot::WriteUnlock()
{
	rw_lock_write_unlock(&fLock);
}


typedef AutoLocker<const PackageFSRoot,
	AutoLockerReadLocking<const PackageFSRoot> > PackageFSRootReadLocker;
typedef AutoLocker<PackageFSRoot, AutoLockerWriteLocking<PackageFSRoot> >
	PackageFSRootWriteLocker;


#endif	// PACKAGE_FS_ROOT_H
