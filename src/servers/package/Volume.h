/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <Handler.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>

#include <package/ActivationTransaction.h>
#include <package/DaemonClient.h>
#include <package/packagefs.h>
#include <util/DoublyLinkedList.h>

#include "FSUtils.h"
#include "Package.h"


// Locking Policy
// ==============
//
// A Volume object is accessed by two threads:
// 1. The application thread: initially (c'tor and Init()) and when handling a
//    location info request (HandleGetLocationInfoRequest()).
// 2. The corresponding Root object's job thread (any other operation).
//
// The only thread synchronization needed is for the status information accessed
// by HandleGetLocationInfoRequest() and modified by the job thread. The data
// are encapsulated in a VolumeState object which contains a lock. The lock
// must be held by the app thread when accessing the data (it reads only) and
// by the job thread when modifying the data (not needed when reading).


using BPackageKit::BPrivate::BActivationTransaction;
using BPackageKit::BPrivate::BDaemonClient;

class BDirectory;

class Root;
class VolumeState;

namespace BPackageKit {
	class BSolver;
	class BSolverRepository;
}

using BPackageKit::BPackageInstallationLocation;
using BPackageKit::BSolver;
using BPackageKit::BSolverRepository;


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

			void				PackageJobPending();
			void				PackageJobFinished();
			bool				IsPackageJobPending() const;

			void				Unmounted();

	virtual	void				MessageReceived(BMessage* message);

			const BString&		Path() const
									{ return fPath; }
			PackageFSMountType	MountType() const
									{ return fMountType; }
			BPackageInstallationLocation Location() const;

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

			PackageFileNameHashTable::Iterator PackagesByFileNameIterator()
									const;

			int					OpenRootDirectory() const;

			void				ProcessPendingNodeMonitorEvents();

			bool				HasPendingPackageActivationChanges() const;
			void				ProcessPendingPackageActivationChanges();
			void				ClearPackageActivationChanges();
			const PackageSet&	PackagesToBeActivated() const
									{ return fPackagesToBeActivated; }
			const PackageSet&	PackagesToBeDeactivated() const
									{ return fPackagesToBeDeactivated; }

			status_t			CreateTransaction(
									BPackageInstallationLocation location,
									BActivationTransaction& _transaction,
									BDirectory& _transactionDirectory);
			void				CommitTransaction(
									const BActivationTransaction& transaction,
									const PackageSet& packagesAlreadyAdded,
									const PackageSet& packagesAlreadyRemoved,
									BDaemonClient::BCommitTransactionResult&
										_result);

private:
			struct NodeMonitorEvent;
			struct CommitTransactionHandler;

			friend struct CommitTransactionHandler;

			typedef FSUtils::RelativePath RelativePath;
			typedef DoublyLinkedList<NodeMonitorEvent> NodeMonitorEventList;

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

			status_t			_OpenSettingsRootDirectory(
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

			status_t			_ExtractPackageContent(Package* package,
									const char* contentPath,
									BDirectory& targetDirectory,
									BDirectory& _extractedFilesDirectory);

private:
			BString				fPath;
			PackageFSMountType	fMountType;
			node_ref			fRootDirectoryRef;
			node_ref			fPackagesDirectoryRef;
			Root*				fRoot;
			Listener*			fListener;
			VolumeState*		fState;
			BLocker				fPendingNodeMonitorEventsLock;
			NodeMonitorEventList fPendingNodeMonitorEvents;
			bigtime_t			fNodeMonitorEventHandleTime;
			PackageSet			fPackagesToBeActivated;
			PackageSet			fPackagesToBeDeactivated;
			BMessage			fLocationInfoReply;
									// only accessed in the application thread
};


class Volume::Listener {
public:
	virtual						~Listener();

	virtual	void				VolumeNodeMonitorEventOccurred(Volume* volume)
									= 0;
};


#endif	// VOLUME_H
