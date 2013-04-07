/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <Handler.h>
#include <Locker.h>
#include <String.h>

#include <packagefs.h>
#include <util/DoublyLinkedList.h>

#include "Package.h"


class BDirectory;

class Root;


class Volume : public BHandler {
public:
								Volume(BLooper* looper);
	virtual						~Volume();

			status_t			Init(const node_ref& rootDirectoryRef,
									node_ref& _packageRootRef);
			status_t			InitPackages();

			void				Unmounted();

	virtual	void				MessageReceived(BMessage* message);

			const BString&		Path() const
									{ return fPath; }
			PackageFSMountType	MountType() const
									{ return fMountType; }

			const node_ref&		RootDirectoryRef() const
									{ return fRootDirectoryRef; }
			dev_t				DeviceID() const
									{ return fRootDirectoryRef.device; }
			ino_t				RootDirectoryID() const
									{ return fRootDirectoryRef.node; }

			const node_ref&		PackagesDirectoryRef() const
									{ return fPackagesDirectoryRef; }
			dev_t				PackagesDeviceID() const
									{ return fPackagesDirectoryRef.device; }
			ino_t				PackagesDirectoryID() const
									{ return fPackagesDirectoryRef.node; }

			Root*				GetRoot() const
									{ return fRoot; }
			void				SetRoot(Root* root)
									{ fRoot = root; }

			int					OpenRootDirectory() const;

			void				ProcessPendingNodeMonitorEvents();

private:
			struct NodeMonitorEvent;
			typedef DoublyLinkedList<NodeMonitorEvent> NodeMonitorEventList;

private:
			void				_HandleEntryCreatedOrRemoved(
									const BMessage* message, bool created);
			void				_HandleEntryMoved(const BMessage* message);
			void				_QueueNodeMonitorEvent(const BString& name,
									bool wasCreated);

			void				_PackagesEntryCreated(const char* name);
			void				_PackagesEntryRemoved(const char* name);

			status_t			_ReadPackagesDirectory();
			status_t			_GetActivePackages(int fd);

private:
			BString				fPath;
			PackageFSMountType	fMountType;
			node_ref			fRootDirectoryRef;
			node_ref			fPackagesDirectoryRef;
			Root*				fRoot;
			PackageFileNameHashTable fPackagesByFileName;
			PackageNodeRefHashTable fPackagesByNodeRef;
			BLocker				fPendingNodeMonitorEventsLock;
			NodeMonitorEventList fPendingNodeMonitorEvents;
};


#endif	// VOLUME_H
