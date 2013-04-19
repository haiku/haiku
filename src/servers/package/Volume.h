/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <set>

#include <Handler.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>

#include <package/packagefs.h>
#include <util/DoublyLinkedList.h>

#include "Package.h"


class BDirectory;

class Root;

namespace BPackageKit {
	class BSolver;
	class BSolverRepository;
}


class Volume : public BHandler {
public:
			class Listener;

public:
								Volume(BLooper* looper);
	virtual						~Volume();

			status_t			Init(const node_ref& rootDirectoryRef,
									node_ref& _packageRootRef);
			status_t			InitPackages(Listener* listener);

			status_t			AddPackagesToRepository(
									BSolverRepository& repository,
									bool activeOnly);
			void				InitialVerify(Volume* nextVolume,
									Volume* nextNextVolume);
			void				HandleGetLocationInfoRequest(BMessage* message);
			void				HandleCommitTransactionRequest(
									BMessage* message);

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

			bool				HasPendingPackageActivationChanges() const;
			void				ProcessPendingPackageActivationChanges();

private:
			struct NodeMonitorEvent;
			struct RelativePath;
			struct Exception;
			struct CommitTransactionHandler;

			friend struct CommitTransactionHandler;

			typedef DoublyLinkedList<NodeMonitorEvent> NodeMonitorEventList;
			typedef std::set<Package*> PackageSet;

private:
			void				_HandleEntryCreatedOrRemoved(
									const BMessage* message, bool created);
			void				_HandleEntryMoved(const BMessage* message);
			void				_QueueNodeMonitorEvent(const BString& name,
									bool wasCreated);

			void				_PackagesEntryCreated(const char* name);
			void				_PackagesEntryRemoved(const char* name);

			void				_FillInActivationChangeItem(
									PackageFSActivationChangeItem* item,
									PackageFSActivationChangeType type,
									Package* package, char*& nameBuffer);

			void				_AddPackage(Package* package);
			void				_RemovePackage(Package* package);

			status_t			_ReadPackagesDirectory();
			status_t			_GetActivePackages(int fd);

			status_t			_AddRepository(BSolver* solver,
									BSolverRepository& repository,
							 		bool activeOnly, bool installed);

			status_t			_OpenPackagesFile(
									const RelativePath& subDirectoryPath,
									const char* fileName, uint32 openMode,
									BFile& _file, BEntry* _entry = NULL);
			status_t			_OpenPackagesSubDirectory(
									const RelativePath& path, bool create,
									BDirectory& _directory);

			status_t			_CreateActivationFileContent(
									const PackageSet& toActivate,
									const PackageSet& toDeactivate,
									BString& _content);
			status_t			_WriteActivationFile(
									const RelativePath& directoryPath,
									const char* fileName,
									const PackageSet& toActivate,
									const PackageSet& toDeactivate,
									BEntry& _entry);

			status_t			_WriteTextFile(
									const RelativePath& directoryPath,
									const char* fileName,
									const BString& content, BEntry& _entry);

			void				_ChangePackageActivation(
									const PackageSet& packagesToActivate,
									const PackageSet& packagesToDeactivate);
									// throws Exception

private:
			BString				fPath;
			PackageFSMountType	fMountType;
			node_ref			fRootDirectoryRef;
			node_ref			fPackagesDirectoryRef;
			Root*				fRoot;
			Listener*			fListener;
			PackageFileNameHashTable fPackagesByFileName;
			PackageNodeRefHashTable fPackagesByNodeRef;
			BLocker				fPendingNodeMonitorEventsLock;
			NodeMonitorEventList fPendingNodeMonitorEvents;
			PackageSet			fPackagesToBeActivated;
			PackageSet			fPackagesToBeDeactivated;
			int64				fChangeCount;
			BMessage			fLocationInfoReply;
};


class Volume::Listener {
public:
	virtual						~Listener();

	virtual	void				VolumeNodeMonitorEventOccurred(Volume* volume)
									= 0;
};


#endif	// VOLUME_H
