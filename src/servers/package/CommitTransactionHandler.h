/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef COMMIT_TRANSACTION_HANDLER_H
#define COMMIT_TRANSACTION_HANDLER_H


#include <set>
#include <string>

#include <Directory.h>

#include "FSTransaction.h"
#include "FSUtils.h"
#include "Volume.h"


typedef std::set<std::string> StringSet;


class CommitTransactionHandler {
public:
								CommitTransactionHandler(Volume* volume,
									PackageFileManager* packageFileManager,
									VolumeState* volumeState,
									bool isActiveVolumeState,
									const PackageSet& packagesAlreadyAdded,
									const PackageSet& packagesAlreadyRemoved);
								~CommitTransactionHandler();

			void				HandleRequest(BMessage* request,
									BMessage* reply);
			void				HandleRequest(
									const BActivationTransaction& transaction,
									BMessage* reply);
			void				HandleRequest(const PackageSet& packagesAdded,
									const PackageSet& packagesRemoved);

			void				Revert();

			const BString&		OldStateDirectoryName() const
									{ return fOldStateDirectoryName; }

			VolumeState*		DetachVolumeState();
			bool				IsActiveVolumeState() const
									{ return fVolumeStateIsActive; }

private:
			typedef BObjectList<Package> PackageList;
			typedef FSUtils::RelativePath RelativePath;

private:
			void				_GetPackagesToDeactivate(
									const BActivationTransaction& transaction);
			void				_ReadPackagesToActivate(
									const BActivationTransaction& transaction);
			void				_ApplyChanges(BMessage* reply);
			void				_CreateOldStateDirectory(BMessage* reply);
			void				_RemovePackagesToDeactivate();
			void				_AddPackagesToActivate();

			void				_PreparePackageToActivate(Package* package);
			void				_AddGroup(Package* package,
									const BString& groupName);
			void				_AddUser(Package* package, const BUser& user);
			void				_AddGlobalWritableFiles(Package* package);
			void				_AddGlobalWritableFile(Package* package,
									const BGlobalWritableFileInfo& file,
									const BDirectory& rootDirectory,
									const BDirectory& extractedFilesDirectory);
			void				_AddGlobalWritableFileRecurse(Package* package,
									const BDirectory& sourceDirectory,
									FSUtils::Path& relativeSourcePath,
									const BDirectory& targetDirectory,
									const char* targetName,
									BWritableFileUpdateType updateType);

			void				_RevertAddPackagesToActivate();
			void				_RevertRemovePackagesToDeactivate();
			void				_RevertUserGroupChanges();

			void				_RunPostInstallScripts();
			void				_RunPostInstallScript(Package* package,
									const BString& script);

			void				_ExtractPackageContent(Package* package,
									const BStringList& contentPaths,
									BDirectory& targetDirectory,
									BDirectory& _extractedFilesDirectory);

			status_t			_OpenPackagesSubDirectory(
									const RelativePath& path, bool create,
									BDirectory& _directory);
			status_t			_OpenPackagesFile(
									const RelativePath& subDirectoryPath,
									const char* fileName, uint32 openMode,
									BFile& _file, BEntry* _entry = NULL);

			status_t			_WriteActivationFile(
									const RelativePath& directoryPath,
									const char* fileName,
									const PackageSet& toActivate,
									const PackageSet& toDeactivate,
									BEntry& _entry);
			status_t			_CreateActivationFileContent(
									const PackageSet& toActivate,
									const PackageSet& toDeactivate,
									BString& _content);
			status_t			_WriteTextFile(
									const RelativePath& directoryPath,
									const char* fileName,
									const BString& content, BEntry& _entry);
			void				_ChangePackageActivation(
									const PackageSet& packagesToActivate,
									const PackageSet& packagesToDeactivate);
									// throws Exception
			void				_ChangePackageActivationIOCtl(
									const PackageSet& packagesToActivate,
									const PackageSet& packagesToDeactivate);
									// throws Exception
			void				_FillInActivationChangeItem(
									PackageFSActivationChangeItem* item,
									PackageFSActivationChangeType type,
									Package* package, char*& nameBuffer);

			bool				_IsSystemPackage(Package* package);

	static	BString				_GetPath(const FSUtils::Entry& entry,
									const BString& fallback);

	static	status_t			_TagPackageEntriesRecursively(
									BDirectory& directory, const BString& value,
									bool nonDirectoriesOnly);

private:
			Volume*				fVolume;
			PackageFileManager*	fPackageFileManager;
			VolumeState*		fVolumeState;
			bool				fVolumeStateIsActive;
			PackageList			fPackagesToActivate;
			PackageSet			fPackagesToDeactivate;
			PackageSet			fAddedPackages;
			PackageSet			fRemovedPackages;
			const PackageSet&	fPackagesAlreadyAdded;
			const PackageSet&	fPackagesAlreadyRemoved;
			BDirectory			fOldStateDirectory;
			node_ref			fOldStateDirectoryRef;
			BString				fOldStateDirectoryName;
			node_ref			fTransactionDirectoryRef;
			BDirectory			fWritableFilesDirectory;
			StringSet			fAddedGroups;
			StringSet			fAddedUsers;
			FSTransaction		fFSTransaction;
};


#endif	// COMMIT_TRANSACTION_HANDLER_H
