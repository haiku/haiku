/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "CommitTransactionHandler.h"

#include <errno.h>
#include <grp.h>
#include <pwd.h>

#include <File.h>
#include <Path.h>
#include <SymLink.h>

#include <AutoDeleter.h>
#include <CopyEngine.h>
#include <NotOwningEntryRef.h>
#include <package/CommitTransactionResult.h>
#include <package/DaemonDefs.h>
#include <RemoveEngine.h>

#include "Constants.h"
#include "DebugSupport.h"
#include "Exception.h"
#include "PackageFileManager.h"
#include "VolumeState.h"


using namespace BPackageKit::BPrivate;

using BPackageKit::BTransactionIssue;


// #pragma mark - TransactionIssueBuilder


struct CommitTransactionHandler::TransactionIssueBuilder {
	TransactionIssueBuilder(BTransactionIssue::BType type,
		Package* package = NULL)
		:
		fType(type),
		fPackageName(package != NULL ? package->FileName() : BString()),
		fPath1(),
		fPath2(),
		fSystemError(B_OK),
		fExitCode(0)
	{
	}

	TransactionIssueBuilder& SetPath1(const BString& path)
	{
		fPath1 = path;
		return *this;
	}

	TransactionIssueBuilder& SetPath1(const FSUtils::Entry& entry)
	{
		return SetPath1(entry.Path());
	}

	TransactionIssueBuilder& SetPath2(const BString& path)
	{
		fPath2 = path;
		return *this;
	}

	TransactionIssueBuilder& SetPath2(const FSUtils::Entry& entry)
	{
		return SetPath2(entry.Path());
	}

	TransactionIssueBuilder& SetSystemError(status_t error)
	{
		fSystemError = error;
		return *this;
	}

	TransactionIssueBuilder& SetExitCode(int exitCode)
	{
		fExitCode = exitCode;
		return *this;
	}

	BTransactionIssue BuildIssue(Package* package) const
	{
		BString packageName(fPackageName);
		if (packageName.IsEmpty() && package != NULL)
			packageName = package->FileName();

		return BTransactionIssue(fType, packageName, fPath1, fPath2,
			fSystemError, fExitCode);
	}

private:
	BTransactionIssue::BType	fType;
	BString						fPackageName;
	BString						fPath1;
	BString						fPath2;
	status_t					fSystemError;
	int							fExitCode;
};


// #pragma mark - CommitTransactionHandler


CommitTransactionHandler::CommitTransactionHandler(Volume* volume,
	PackageFileManager* packageFileManager, BCommitTransactionResult& result)
	:
	fVolume(volume),
	fPackageFileManager(packageFileManager),
	fVolumeState(NULL),
	fVolumeStateIsActive(false),
	fPackagesToActivate(),
	fPackagesToDeactivate(),
	fAddedPackages(),
	fRemovedPackages(),
	fPackagesAlreadyAdded(),
	fPackagesAlreadyRemoved(),
	fOldStateDirectory(),
	fOldStateDirectoryRef(),
	fOldStateDirectoryName(),
	fTransactionDirectoryRef(),
	fWritableFilesDirectory(),
	fAddedGroups(),
	fAddedUsers(),
	fFSTransaction(),
	fResult(result),
	fCurrentPackage(NULL)
{
}


CommitTransactionHandler::~CommitTransactionHandler()
{
	// Delete Package objects we created in case of error (on success
	// fPackagesToActivate will be empty).
	int32 count = fPackagesToActivate.CountItems();
	for (int32 i = 0; i < count; i++) {
		Package* package = fPackagesToActivate.ItemAt(i);
		if (fPackagesAlreadyAdded.find(package)
				== fPackagesAlreadyAdded.end()) {
			delete package;
		}
	}

	delete fVolumeState;
}


void
CommitTransactionHandler::Init(VolumeState* volumeState,
	bool isActiveVolumeState, const PackageSet& packagesAlreadyAdded,
	const PackageSet& packagesAlreadyRemoved)
{
	fVolumeState = volumeState->Clone();
	if (fVolumeState == NULL)
		throw std::bad_alloc();

	fVolumeStateIsActive = isActiveVolumeState;

	for (PackageSet::const_iterator it = packagesAlreadyAdded.begin();
			it != packagesAlreadyAdded.end(); ++it) {
		Package* package = fVolumeState->FindPackage((*it)->FileName());
		fPackagesAlreadyAdded.insert(package);
	}

	for (PackageSet::const_iterator it = packagesAlreadyRemoved.begin();
			it != packagesAlreadyRemoved.end(); ++it) {
		Package* package = fVolumeState->FindPackage((*it)->FileName());
		fPackagesAlreadyRemoved.insert(package);
	}
}


void
CommitTransactionHandler::HandleRequest(BMessage* request)
{
	status_t error;
	BActivationTransaction transaction(request, &error);
	if (error == B_OK)
		error = transaction.InitCheck();
	if (error != B_OK) {
		if (error == B_NO_MEMORY)
			throw Exception(B_TRANSACTION_NO_MEMORY);
		throw Exception(B_TRANSACTION_BAD_REQUEST);
	}

	HandleRequest(transaction);
}


void
CommitTransactionHandler::HandleRequest(
	const BActivationTransaction& transaction)
{
	// check the change count
	if (transaction.ChangeCount() != fVolume->ChangeCount())
		throw Exception(B_TRANSACTION_CHANGE_COUNT_MISMATCH);

	// collect the packages to deactivate
	_GetPackagesToDeactivate(transaction);

	// read the packages to activate
	_ReadPackagesToActivate(transaction);

	// anything to do at all?
	if (fPackagesToActivate.IsEmpty() &&  fPackagesToDeactivate.empty()) {
		WARN("Bad package activation request: no packages to activate or"
			" deactivate\n");
		throw Exception(B_TRANSACTION_BAD_REQUEST);
	}

	_ApplyChanges();
}


void
CommitTransactionHandler::HandleRequest()
{
	for (PackageSet::const_iterator it = fPackagesAlreadyAdded.begin();
		it != fPackagesAlreadyAdded.end(); ++it) {
		if (!fPackagesToActivate.AddItem(*it))
			throw std::bad_alloc();
	}

	fPackagesToDeactivate = fPackagesAlreadyRemoved;

	_ApplyChanges();
}


void
CommitTransactionHandler::Revert()
{
	// move packages to activate back to transaction directory
	_RevertAddPackagesToActivate();

	// move packages to deactivate back to packages directory
	_RevertRemovePackagesToDeactivate();

	// revert user and group changes
	_RevertUserGroupChanges();

	// Revert all other FS operations, i.e. the writable files changes as
	// well as the creation of the old state directory.
	fFSTransaction.RollBack();
}


VolumeState*
CommitTransactionHandler::DetachVolumeState()
{
	VolumeState* result = fVolumeState;
	fVolumeState = NULL;
	return result;
}


void
CommitTransactionHandler::_GetPackagesToDeactivate(
	const BActivationTransaction& transaction)
{
	// get the number of packages to deactivate
	const BStringList& packagesToDeactivate
		= transaction.PackagesToDeactivate();
	int32 packagesToDeactivateCount = packagesToDeactivate.CountStrings();
	if (packagesToDeactivateCount == 0)
		return;

	for (int32 i = 0; i < packagesToDeactivateCount; i++) {
		BString packageName = packagesToDeactivate.StringAt(i);
		Package* package = fVolumeState->FindPackage(packageName);
		if (package == NULL) {
			throw Exception(B_TRANSACTION_NO_SUCH_PACKAGE)
				.SetPackageName(packageName);
		}

		fPackagesToDeactivate.insert(package);
	}
}


void
CommitTransactionHandler::_ReadPackagesToActivate(
	const BActivationTransaction& transaction)
{
	// get the number of packages to activate
	const BStringList& packagesToActivate
		= transaction.PackagesToActivate();
	int32 packagesToActivateCount = packagesToActivate.CountStrings();
	if (packagesToActivateCount == 0)
		return;

	// check the transaction directory name -- we only allow a simple
	// subdirectory of the admin directory
	const BString& transactionDirectoryName
		= transaction.TransactionDirectoryName();
	if (transactionDirectoryName.IsEmpty()
		|| transactionDirectoryName.FindFirst('/') >= 0
		|| transactionDirectoryName == "."
		|| transactionDirectoryName == "..") {
		WARN("Bad package activation request: malformed transaction"
			" directory name: \"%s\"\n", transactionDirectoryName.String());
		throw Exception(B_TRANSACTION_BAD_REQUEST);
	}

	// open the directory
	RelativePath directoryPath(kAdminDirectoryName,
		transactionDirectoryName);
	BDirectory directory;
	status_t error = _OpenPackagesSubDirectory(directoryPath, false, directory);
	if (error == B_OK) {
		error = directory.GetNodeRef(&fTransactionDirectoryRef);
		if (error != B_OK) {
			ERROR("Failed to get transaction directory node ref: %s\n",
				strerror(error));
		}
	} else
		ERROR("Failed to open transaction directory: %s\n", strerror(error));

	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
			.SetPath1(_GetPath(
				FSUtils::Entry(fVolume->PackagesDirectoryRef(),
					directoryPath.ToString()),
				directoryPath.ToString()))
			.SetSystemError(error);
	}

	// read the packages
	for (int32 i = 0; i < packagesToActivateCount; i++) {
		BString packageName = packagesToActivate.StringAt(i);

		// make sure it doesn't clash with an already existing package
		Package* package = fVolumeState->FindPackage(packageName);
		if (package != NULL) {
			if (fPackagesAlreadyAdded.find(package)
					!= fPackagesAlreadyAdded.end()) {
				if (!fPackagesToActivate.AddItem(package))
					throw Exception(B_TRANSACTION_NO_MEMORY);
				continue;
			}

			if (fPackagesToDeactivate.find(package)
					== fPackagesToDeactivate.end()) {
				throw Exception(B_TRANSACTION_PACKAGE_ALREADY_EXISTS)
					.SetPackageName(packageName);
			}
		}

		// read the package
		error = fPackageFileManager->CreatePackage(
			NotOwningEntryRef(fTransactionDirectoryRef, packageName),
			package);
		if (error != B_OK) {
			if (error == B_NO_MEMORY)
				throw Exception(B_TRANSACTION_NO_MEMORY);
			throw Exception(B_TRANSACTION_FAILED_TO_READ_PACKAGE_FILE)
				.SetPackageName(packageName)
				.SetPath1(_GetPath(
					FSUtils::Entry(
						NotOwningEntryRef(fTransactionDirectoryRef,
							packageName)),
					packageName))
				.SetSystemError(error);
		}

		if (!fPackagesToActivate.AddItem(package)) {
			delete package;
			throw Exception(B_TRANSACTION_NO_MEMORY);
		}
	}
}


void
CommitTransactionHandler::_ApplyChanges()
{
	// create an old state directory
	_CreateOldStateDirectory();

	// move packages to deactivate to old state directory
	_RemovePackagesToDeactivate();

	// move packages to activate to packages directory
	_AddPackagesToActivate();

	// run pre-uninstall scripts, before their packages vanish.
	_RunPreUninstallScripts();

	// activate/deactivate packages
	_ChangePackageActivation(fAddedPackages, fRemovedPackages);

	// run post-installation scripts
	if (fVolumeStateIsActive) {
		_RunPostInstallScripts();
	} else {
		// need to reboot to finish installation so queue up scripts as
		// symbolic links in a work directory, which will run later.
		_QueuePostInstallScripts();
	}

	// removed packages have been deleted, new packages shall not be deleted
	fAddedPackages.clear();
	fRemovedPackages.clear();
	fPackagesToActivate.MakeEmpty(false);
	fPackagesToDeactivate.clear();
}


void
CommitTransactionHandler::_CreateOldStateDirectory()
{
	// construct a nice name from the current date and time
	time_t nowSeconds = time(NULL);
	struct tm now;
	BString baseName;
	if (localtime_r(&nowSeconds, &now) != NULL) {
		baseName.SetToFormat("state_%d-%02d-%02d_%02d:%02d:%02d",
			1900 + now.tm_year, now.tm_mon + 1, now.tm_mday, now.tm_hour,
			now.tm_min, now.tm_sec);
	} else
		baseName = "state";

	if (baseName.IsEmpty())
		throw Exception(B_TRANSACTION_NO_MEMORY);

	// make sure the directory doesn't exist yet
	BDirectory adminDirectory;
	status_t error = _OpenPackagesSubDirectory(
		RelativePath(kAdminDirectoryName), true, adminDirectory);
	if (error != B_OK) {
		ERROR("Failed to open administrative directory: %s\n", strerror(error));
		throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
			.SetPath1(_GetPath(
				FSUtils::Entry(fVolume->PackagesDirectoryRef(),
					kAdminDirectoryName),
				kAdminDirectoryName))
			.SetSystemError(error);
	}

	int uniqueId = 1;
	BString directoryName = baseName;
	while (BEntry(&adminDirectory, directoryName).Exists()) {
		directoryName.SetToFormat("%s-%d", baseName.String(), uniqueId++);
		if (directoryName.IsEmpty())
			throw Exception(B_TRANSACTION_NO_MEMORY);
	}

	// create the directory
	FSTransaction::CreateOperation createOldStateDirectoryOperation(
		&fFSTransaction, FSUtils::Entry(adminDirectory, directoryName));

	error = adminDirectory.CreateDirectory(directoryName,
		&fOldStateDirectory);
	if (error == B_OK) {
		createOldStateDirectoryOperation.Finished();

		fOldStateDirectoryName = directoryName;

		error = fOldStateDirectory.GetNodeRef(&fOldStateDirectoryRef);
		if (error != B_OK)
			ERROR("Failed get old state directory ref: %s\n", strerror(error));
	} else
		ERROR("Failed to create old state directory: %s\n", strerror(error));

	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_CREATE_DIRECTORY)
			.SetPath1(_GetPath(
				FSUtils::Entry(adminDirectory, directoryName),
				directoryName))
			.SetSystemError(error);
	}

	// write the old activation file
	BEntry activationFile;
	_WriteActivationFile(RelativePath(kAdminDirectoryName, directoryName),
		kActivationFileName, PackageSet(), PackageSet(), activationFile);

	fResult.SetOldStateDirectory(fOldStateDirectoryName);
}


void
CommitTransactionHandler::_RemovePackagesToDeactivate()
{
	if (fPackagesToDeactivate.empty())
		return;

	for (PackageSet::const_iterator it = fPackagesToDeactivate.begin();
		it != fPackagesToDeactivate.end(); ++it) {
		Package* package = *it;

		// When deactivating (or updating) a system package, don't do that live.
		if (_IsSystemPackage(package))
			fVolumeStateIsActive = false;

		if (fPackagesAlreadyRemoved.find(package)
				!= fPackagesAlreadyRemoved.end()) {
			fRemovedPackages.insert(package);
			continue;
		}

		// get a BEntry for the package
		NotOwningEntryRef entryRef(package->EntryRef());

		BEntry entry;
		status_t error = entry.SetTo(&entryRef);
		if (error != B_OK) {
			ERROR("Failed to get package entry for %s: %s\n",
				package->FileName().String(), strerror(error));
			throw Exception(B_TRANSACTION_FAILED_TO_GET_ENTRY_PATH)
				.SetPath1(package->FileName())
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}

		// move entry
		fRemovedPackages.insert(package);

		error = entry.MoveTo(&fOldStateDirectory);
		if (error != B_OK) {
			fRemovedPackages.erase(package);
			ERROR("Failed to move old package %s from packages directory: %s\n",
				package->FileName().String(), strerror(error));
			throw Exception(B_TRANSACTION_FAILED_TO_MOVE_FILE)
				.SetPath1(
					_GetPath(FSUtils::Entry(entryRef), package->FileName()))
				.SetPath2(_GetPath(
					FSUtils::Entry(fOldStateDirectory),
					fOldStateDirectoryName))
				.SetSystemError(error);
		}

		fPackageFileManager->PackageFileMoved(package->File(),
			fOldStateDirectoryRef);
		package->File()->IncrementEntryRemovedIgnoreLevel();
	}
}


void
CommitTransactionHandler::_AddPackagesToActivate()
{
	if (fPackagesToActivate.IsEmpty())
		return;

	// open packages directory
	BDirectory packagesDirectory;
	status_t error
		= packagesDirectory.SetTo(&fVolume->PackagesDirectoryRef());
	if (error != B_OK) {
		ERROR("Failed to open packages directory: %s\n", strerror(error));
		throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
			.SetPath1("<packages>")
			.SetSystemError(error);
	}

	int32 count = fPackagesToActivate.CountItems();
	for (int32 i = 0; i < count; i++) {
		Package* package = fPackagesToActivate.ItemAt(i);
		if (fPackagesAlreadyAdded.find(package)
				!= fPackagesAlreadyAdded.end()) {
			fAddedPackages.insert(package);
			_PreparePackageToActivate(package);
			continue;
		}

		// get a BEntry for the package
		NotOwningEntryRef entryRef(fTransactionDirectoryRef,
			package->FileName());
		BEntry entry;
		error = entry.SetTo(&entryRef);
		if (error != B_OK) {
			ERROR("Failed to get package entry for %s: %s\n",
				package->FileName().String(), strerror(error));
			throw Exception(B_TRANSACTION_FAILED_TO_GET_ENTRY_PATH)
				.SetPath1(package->FileName())
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}

		// move entry
		fAddedPackages.insert(package);

		error = entry.MoveTo(&packagesDirectory);
		if (error != B_OK) {
			fAddedPackages.erase(package);
			ERROR("Failed to move new package %s to packages directory: %s\n",
				package->FileName().String(), strerror(error));
			throw Exception(B_TRANSACTION_FAILED_TO_MOVE_FILE)
				.SetPath1(
					_GetPath(FSUtils::Entry(entryRef), package->FileName()))
				.SetPath2(_GetPath(
					FSUtils::Entry(packagesDirectory),
					"packages"))
				.SetSystemError(error);
		}

		fPackageFileManager->PackageFileMoved(package->File(),
			fVolume->PackagesDirectoryRef());
		package->File()->IncrementEntryCreatedIgnoreLevel();

		// also add the package to the volume
		fVolumeState->AddPackage(package);

		_PreparePackageToActivate(package);
	}
}


void
CommitTransactionHandler::_PreparePackageToActivate(Package* package)
{
	fCurrentPackage = package;

	// add groups
	const BStringList& groups = package->Info().Groups();
	int32 count = groups.CountStrings();
	for (int32 i = 0; i < count; i++)
		_AddGroup(package, groups.StringAt(i));

	// add users
	const BObjectList<BUser>& users = package->Info().Users();
	for (int32 i = 0; const BUser* user = users.ItemAt(i); i++)
		_AddUser(package, *user);

	// handle global writable files
	_AddGlobalWritableFiles(package);

	fCurrentPackage = NULL;
}


void
CommitTransactionHandler::_AddGroup(Package* package, const BString& groupName)
{
	// Check whether the group already exists.
	char buffer[256];
	struct group groupBuffer;
	struct group* groupFound;
	int error = getgrnam_r(groupName, &groupBuffer, buffer, sizeof(buffer),
		&groupFound);
	if ((error == 0 && groupFound != NULL) || error == ERANGE)
		return;

	// add it
	fAddedGroups.insert(groupName.String());

	std::string commandLine("groupadd ");
	commandLine += FSUtils::ShellEscapeString(groupName).String();

	if (system(commandLine.c_str()) != 0) {
		fAddedGroups.erase(groupName.String());
		ERROR("Failed to add group \"%s\".\n", groupName.String());
		throw Exception(B_TRANSACTION_FAILED_TO_ADD_GROUP)
			.SetPackageName(package->FileName())
			.SetString1(groupName);
	}
}


void
CommitTransactionHandler::_AddUser(Package* package, const BUser& user)
{
	// Check whether the user already exists.
	char buffer[256];
	struct passwd passwdBuffer;
	struct passwd* passwdFound;
	int error = getpwnam_r(user.Name(), &passwdBuffer, buffer,
		sizeof(buffer), &passwdFound);
	if ((error == 0 && passwdFound != NULL) || error == ERANGE)
		return;

	// add it
	fAddedUsers.insert(user.Name().String());

	std::string commandLine("useradd ");

	if (!user.RealName().IsEmpty()) {
		commandLine += std::string("-n ")
			+ FSUtils::ShellEscapeString(user.RealName()).String() + " ";
	}

	if (!user.Home().IsEmpty()) {
		commandLine += std::string("-d ")
			+ FSUtils::ShellEscapeString(user.Home()).String() + " ";
	}

	if (!user.Shell().IsEmpty()) {
		commandLine += std::string("-s ")
			+ FSUtils::ShellEscapeString(user.Shell()).String() + " ";
	}

	if (!user.Groups().IsEmpty()) {
		commandLine += std::string("-g ")
			+ FSUtils::ShellEscapeString(user.Groups().First()).String()
			+ " ";
	}

	commandLine += FSUtils::ShellEscapeString(user.Name()).String();

	if (system(commandLine.c_str()) != 0) {
		fAddedUsers.erase(user.Name().String());
		ERROR("Failed to add user \"%s\".\n", user.Name().String());
		throw Exception(B_TRANSACTION_FAILED_TO_ADD_USER)
			.SetPackageName(package->FileName())
			.SetString1(user.Name());

	}

	// add the supplementary groups
	int32 groupCount = user.Groups().CountStrings();
	for (int32 i = 1; i < groupCount; i++) {
		commandLine = std::string("groupmod -A ")
			+ FSUtils::ShellEscapeString(user.Name()).String()
			+ " "
			+ FSUtils::ShellEscapeString(user.Groups().StringAt(i))
				.String();
		if (system(commandLine.c_str()) != 0) {
			fAddedUsers.erase(user.Name().String());
			ERROR("Failed to add user \"%s\" to group \"%s\".\n",
				user.Name().String(), user.Groups().StringAt(i).String());
			throw Exception(B_TRANSACTION_FAILED_TO_ADD_USER_TO_GROUP)
				.SetPackageName(package->FileName())
				.SetString1(user.Name())
				.SetString2(user.Groups().StringAt(i));
		}
	}
}


void
CommitTransactionHandler::_AddGlobalWritableFiles(Package* package)
{
	// get the list of included files
	const BObjectList<BGlobalWritableFileInfo>& files
		= package->Info().GlobalWritableFileInfos();
	BStringList contentPaths;
	for (int32 i = 0; const BGlobalWritableFileInfo* file = files.ItemAt(i);
		i++) {
		if (file->IsIncluded() && !contentPaths.Add(file->Path()))
			throw std::bad_alloc();
	}

	if (contentPaths.IsEmpty())
		return;

	// Open the root directory of the installation location where we will
	// extract the files -- that's the volume's root directory.
	BDirectory rootDirectory;
	status_t error = rootDirectory.SetTo(&fVolume->RootDirectoryRef());
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
			.SetPath1(_GetPath(
				FSUtils::Entry(fVolume->RootDirectoryRef()),
				"<packagefs root>"))
			.SetSystemError(error);
	}

	// Open writable-files directory in the administrative directory.
	if (fWritableFilesDirectory.InitCheck() != B_OK) {
		RelativePath directoryPath(kAdminDirectoryName,
			kWritableFilesDirectoryName);
		error = _OpenPackagesSubDirectory(directoryPath, true,
			fWritableFilesDirectory);

		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
				.SetPath1(_GetPath(
					FSUtils::Entry(fVolume->PackagesDirectoryRef(),
						directoryPath.ToString()),
					directoryPath.ToString()))
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}
	}

	// extract files into a subdir of the writable-files directory
	BDirectory extractedFilesDirectory;
	_ExtractPackageContent(package, contentPaths,
		fWritableFilesDirectory, extractedFilesDirectory);

	for (int32 i = 0; const BGlobalWritableFileInfo* file = files.ItemAt(i);
		i++) {
		if (file->IsIncluded()) {
			_AddGlobalWritableFile(package, *file, rootDirectory,
				extractedFilesDirectory);
		}
	}
}


void
CommitTransactionHandler::_AddGlobalWritableFile(Package* package,
	const BGlobalWritableFileInfo& file, const BDirectory& rootDirectory,
	const BDirectory& extractedFilesDirectory)
{
	// Map the path name to the actual target location. Currently this only
	// concerns "settings/", which is mapped to "settings/global/".
	BString targetPath(file.Path());
	if (fVolume->MountType() == PACKAGE_FS_MOUNT_TYPE_HOME) {
		if (targetPath == "settings"
			|| targetPath.StartsWith("settings/")) {
			targetPath.Insert("/global", 8);
			if (targetPath.Length() == file.Path().Length())
				throw std::bad_alloc();
		}
	}

	// open parent directory of the source entry
	const char* lastSlash = strrchr(file.Path(), '/');
	const BDirectory* sourceDirectory;
	BDirectory stackSourceDirectory;
	if (lastSlash != NULL) {
		sourceDirectory = &stackSourceDirectory;
		BString sourceParentPath(file.Path(),
			lastSlash - file.Path().String());
		if (sourceParentPath.Length() == 0)
			throw std::bad_alloc();

		status_t error = stackSourceDirectory.SetTo(
			&extractedFilesDirectory, sourceParentPath);
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
				.SetPath1(_GetPath(
					FSUtils::Entry(extractedFilesDirectory, sourceParentPath),
					sourceParentPath))
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}
	} else {
		sourceDirectory = &extractedFilesDirectory;
	}

	// open parent directory of the target entry -- create, if necessary
	FSUtils::Path relativeSourcePath(file.Path());
	lastSlash = strrchr(targetPath, '/');
	if (lastSlash != NULL) {
		BString targetParentPath(targetPath,
			lastSlash - targetPath.String());
		if (targetParentPath.Length() == 0)
			throw std::bad_alloc();

		BDirectory targetDirectory;
		status_t error = FSUtils::OpenSubDirectory(rootDirectory,
			RelativePath(targetParentPath), true, targetDirectory);
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
				.SetPath1(_GetPath(
					FSUtils::Entry(rootDirectory, targetParentPath),
					targetParentPath))
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}
		_AddGlobalWritableFileRecurse(package, *sourceDirectory,
			relativeSourcePath, targetDirectory, lastSlash + 1,
			file.UpdateType());
	} else {
		_AddGlobalWritableFileRecurse(package, *sourceDirectory,
			relativeSourcePath, rootDirectory, targetPath,
			file.UpdateType());
	}
}


void
CommitTransactionHandler::_AddGlobalWritableFileRecurse(Package* package,
	const BDirectory& sourceDirectory, FSUtils::Path& relativeSourcePath,
	const BDirectory& targetDirectory, const char* targetName,
	BWritableFileUpdateType updateType)
{
	// * If the file doesn't exist, just copy the extracted one.
	// * If the file does exist, compare with the previous original version:
	//   * If unchanged, just overwrite it.
	//   * If changed, leave it to the user for now. When we support merging
	//     first back the file up, then try the merge.

	// Check whether the target location exists and what type the entry at
	// both locations are.
	struct stat targetStat;
	if (targetDirectory.GetStatFor(targetName, &targetStat) != B_OK) {
		// target doesn't exist -- just copy
		PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
			"couldn't get stat for writable file, copying...\n");
		FSTransaction::CreateOperation copyOperation(&fFSTransaction,
			FSUtils::Entry(targetDirectory, targetName));
		status_t error = BCopyEngine(BCopyEngine::COPY_RECURSIVELY)
			.CopyEntry(
				FSUtils::Entry(sourceDirectory, relativeSourcePath.Leaf()),
				FSUtils::Entry(targetDirectory, targetName));
		if (error != B_OK) {
			if (targetDirectory.GetStatFor(targetName, &targetStat) == B_OK)
				copyOperation.Finished();

			throw Exception(B_TRANSACTION_FAILED_TO_COPY_FILE)
				.SetPath1(_GetPath(
					FSUtils::Entry(sourceDirectory,
						relativeSourcePath.Leaf()),
					relativeSourcePath))
				.SetPath2(_GetPath(
					FSUtils::Entry(targetDirectory, targetName),
					targetName))
				.SetSystemError(error);
		}
		copyOperation.Finished();
		return;
	}

	struct stat sourceStat;
	status_t error = sourceDirectory.GetStatFor(relativeSourcePath.Leaf(),
		&sourceStat);
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_ACCESS_ENTRY)
			.SetPath1(_GetPath(
				FSUtils::Entry(sourceDirectory,
					relativeSourcePath.Leaf()),
				relativeSourcePath))
			.SetSystemError(error);
	}

	if ((sourceStat.st_mode & S_IFMT) != (targetStat.st_mode & S_IFMT)
		|| (!S_ISDIR(sourceStat.st_mode) && !S_ISREG(sourceStat.st_mode)
			&& !S_ISLNK(sourceStat.st_mode))) {
		// Source and target entry types don't match or this is an entry
		// we cannot handle. The user must handle this manually.
		PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
			"writable file exists, but type doesn't match previous type\n");
		_AddIssue(TransactionIssueBuilder(
				BTransactionIssue::B_WRITABLE_FILE_TYPE_MISMATCH)
			.SetPath1(FSUtils::Entry(targetDirectory, targetName))
			.SetPath2(FSUtils::Entry(sourceDirectory,
				relativeSourcePath.Leaf())));
		return;
	}

	if (S_ISDIR(sourceStat.st_mode)) {
		// entry is a directory -- recurse
		BDirectory sourceSubDirectory;
		error = sourceSubDirectory.SetTo(&sourceDirectory,
			relativeSourcePath.Leaf());
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
				.SetPath1(_GetPath(
					FSUtils::Entry(sourceDirectory,
						relativeSourcePath.Leaf()),
					relativeSourcePath))
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}

		BDirectory targetSubDirectory;
		error = targetSubDirectory.SetTo(&targetDirectory, targetName);
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
				.SetPath1(_GetPath(
					FSUtils::Entry(targetDirectory, targetName),
					targetName))
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}

		entry_ref entry;
		while (sourceSubDirectory.GetNextRef(&entry) == B_OK) {
			relativeSourcePath.AppendComponent(entry.name);
			_AddGlobalWritableFileRecurse(package, sourceSubDirectory,
				relativeSourcePath, targetSubDirectory, entry.name,
				updateType);
			relativeSourcePath.RemoveLastComponent();
		}

		PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
			"writable directory, recursion done\n");
		return;
	}

	// get the package the target file originated from
	BString originalPackage;
	if (BNode(&targetDirectory, targetName).ReadAttrString(
			kPackageFileAttribute, &originalPackage) != B_OK) {
		// Can't determine the original package. The user must handle this
		// manually.
		PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
			"failed to get SYS:PACKAGE attribute\n");
		if (updateType != B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD) {
			_AddIssue(TransactionIssueBuilder(
					BTransactionIssue::B_WRITABLE_FILE_NO_PACKAGE_ATTRIBUTE)
				.SetPath1(FSUtils::Entry(targetDirectory, targetName)));
		}
		return;
	}

	// If that's our package, we're happy.
	if (originalPackage == package->RevisionedNameThrows()) {
		PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
			"file tagged with same package version we're activating\n");
		return;
	}

	// Check, whether the writable-files directory for the original package
	// exists.
	BString originalRelativeSourcePath = BString().SetToFormat("%s/%s",
		originalPackage.String(), relativeSourcePath.ToCString());
	if (originalRelativeSourcePath.IsEmpty())
		throw std::bad_alloc();

	struct stat originalPackageStat;
	error = fWritableFilesDirectory.GetStatFor(originalRelativeSourcePath,
		&originalPackageStat);
	if (error != B_OK
		|| (sourceStat.st_mode & S_IFMT)
			!= (originalPackageStat.st_mode & S_IFMT)) {
		// Original entry doesn't exist (either we don't have the data from
		// the original package or the entry really didn't exist) or its
		// type differs from the expected one. The user must handle this
		// manually.
		PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
			"original \"%s\" doesn't exist or has other type\n",
			_GetPath(FSUtils::Entry(fWritableFilesDirectory,
					originalRelativeSourcePath),
				originalRelativeSourcePath).String());
		if (error != B_OK) {
			_AddIssue(TransactionIssueBuilder(
					BTransactionIssue
						::B_WRITABLE_FILE_OLD_ORIGINAL_FILE_MISSING)
				.SetPath1(FSUtils::Entry(targetDirectory, targetName))
				.SetPath2(FSUtils::Entry(fWritableFilesDirectory,
					originalRelativeSourcePath)));
		} else {
			_AddIssue(TransactionIssueBuilder(
					BTransactionIssue
						::B_WRITABLE_FILE_OLD_ORIGINAL_FILE_TYPE_MISMATCH)
				.SetPath1(FSUtils::Entry(targetDirectory, targetName))
				.SetPath2(FSUtils::Entry(fWritableFilesDirectory,
					originalRelativeSourcePath)));
		}
		return;
	}

	if (S_ISREG(sourceStat.st_mode)) {
		// compare file content
		bool equal;
		error = FSUtils::CompareFileContent(
			FSUtils::Entry(fWritableFilesDirectory,
				originalRelativeSourcePath),
			FSUtils::Entry(targetDirectory, targetName),
			equal);
		// TODO: Merge support!
		if (error != B_OK || !equal) {
			// The comparison failed or the files differ. The user must
			// handle this manually.
			PRINT("Volume::CommitTransactionHandler::"
				"_AddGlobalWritableFile(): "
				"file comparison failed (%s) or files aren't equal\n",
				strerror(error));
			if (updateType != B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD) {
				if (error != B_OK) {
					_AddIssue(TransactionIssueBuilder(
							BTransactionIssue
								::B_WRITABLE_FILE_COMPARISON_FAILED)
						.SetPath1(FSUtils::Entry(targetDirectory, targetName))
						.SetPath2(FSUtils::Entry(fWritableFilesDirectory,
							originalRelativeSourcePath))
						.SetSystemError(error));
				} else {
					_AddIssue(TransactionIssueBuilder(
							BTransactionIssue
								::B_WRITABLE_FILE_NOT_EQUAL)
						.SetPath1(FSUtils::Entry(targetDirectory, targetName))
						.SetPath2(FSUtils::Entry(fWritableFilesDirectory,
							originalRelativeSourcePath)));
				}
			}
			return;
		}
	} else {
		// compare symlinks
		bool equal;
		error = FSUtils::CompareSymLinks(
			FSUtils::Entry(fWritableFilesDirectory,
				originalRelativeSourcePath),
			FSUtils::Entry(targetDirectory, targetName),
			equal);
		if (error != B_OK || !equal) {
			// The comparison failed or the symlinks differ. The user must
			// handle this manually.
			PRINT("Volume::CommitTransactionHandler::"
				"_AddGlobalWritableFile(): "
				"symlink comparison failed (%s) or symlinks aren't equal\n",
				strerror(error));
			if (updateType != B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD) {
				if (error != B_OK) {
					_AddIssue(TransactionIssueBuilder(
							BTransactionIssue
								::B_WRITABLE_SYMLINK_COMPARISON_FAILED)
						.SetPath1(FSUtils::Entry(targetDirectory, targetName))
						.SetPath2(FSUtils::Entry(fWritableFilesDirectory,
							originalRelativeSourcePath))
						.SetSystemError(error));
				} else {
					_AddIssue(TransactionIssueBuilder(
							BTransactionIssue
								::B_WRITABLE_SYMLINK_NOT_EQUAL)
						.SetPath1(FSUtils::Entry(targetDirectory, targetName))
						.SetPath2(FSUtils::Entry(fWritableFilesDirectory,
							originalRelativeSourcePath)));
				}
			}
			return;
		}
	}

	// Replace the existing file/symlink. We do that in two steps: First
	// copy the new file to a neighoring location, then move-replace the
	// old file.
	BString tempTargetName;
	tempTargetName.SetToFormat("%s.%s", targetName,
		package->RevisionedNameThrows().String());
	if (tempTargetName.IsEmpty())
		throw std::bad_alloc();

	// copy
	FSTransaction::CreateOperation copyOperation(&fFSTransaction,
		FSUtils::Entry(targetDirectory, tempTargetName));

	error = BCopyEngine(BCopyEngine::UNLINK_DESTINATION).CopyEntry(
		FSUtils::Entry(sourceDirectory, relativeSourcePath.Leaf()),
		FSUtils::Entry(targetDirectory, tempTargetName));
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_COPY_FILE)
			.SetPath1(_GetPath(
				FSUtils::Entry(sourceDirectory,
					relativeSourcePath.Leaf()),
				relativeSourcePath))
			.SetPath2(_GetPath(
				FSUtils::Entry(targetDirectory, tempTargetName),
				tempTargetName))
			.SetSystemError(error);
	}

	copyOperation.Finished();

	// rename
	FSTransaction::RemoveOperation renameOperation(&fFSTransaction,
		FSUtils::Entry(targetDirectory, targetName),
		FSUtils::Entry(fWritableFilesDirectory,
			originalRelativeSourcePath));

	BEntry targetEntry;
	error = targetEntry.SetTo(&targetDirectory, tempTargetName);
	if (error == B_OK)
		error = targetEntry.Rename(targetName, true);
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_MOVE_FILE)
			.SetPath1(_GetPath(
				FSUtils::Entry(targetDirectory, tempTargetName),
				tempTargetName))
			.SetPath2(targetName)
			.SetSystemError(error);
	}

	renameOperation.Finished();
	copyOperation.Unregister();
}


void
CommitTransactionHandler::_RevertAddPackagesToActivate()
{
	if (fAddedPackages.empty())
		return;

	// open transaction directory
	BDirectory transactionDirectory;
	status_t error = transactionDirectory.SetTo(&fTransactionDirectoryRef);
	if (error != B_OK) {
		ERROR("failed to open transaction directory: %s\n",
			strerror(error));
	}

	for (PackageSet::iterator it = fAddedPackages.begin();
		it != fAddedPackages.end(); ++it) {
		// remove package from the volume
		Package* package = *it;

		if (fPackagesAlreadyAdded.find(package)
				!= fPackagesAlreadyAdded.end()) {
			continue;
		}

		fVolumeState->RemovePackage(package);

		if (transactionDirectory.InitCheck() != B_OK)
			continue;

		// get BEntry for the package
		NotOwningEntryRef entryRef(package->EntryRef());
		BEntry entry;
		error = entry.SetTo(&entryRef);
		if (error != B_OK) {
			ERROR("failed to get entry for package \"%s\": %s\n",
				package->FileName().String(), strerror(error));
			continue;
		}

		// move entry
		error = entry.MoveTo(&transactionDirectory);
		if (error != B_OK) {
			ERROR("failed to move new package \"%s\" back to transaction "
				"directory: %s\n", package->FileName().String(),
				strerror(error));
			continue;
		}

		fPackageFileManager->PackageFileMoved(package->File(),
			fTransactionDirectoryRef);
		package->File()->IncrementEntryRemovedIgnoreLevel();
	}
}


void
CommitTransactionHandler::_RevertRemovePackagesToDeactivate()
{
	if (fRemovedPackages.empty())
		return;

	// open packages directory
	BDirectory packagesDirectory;
	status_t error
		= packagesDirectory.SetTo(&fVolume->PackagesDirectoryRef());
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
			.SetPath1("<packages>")
			.SetSystemError(error);
	}

	for (PackageSet::iterator it = fRemovedPackages.begin();
		it != fRemovedPackages.end(); ++it) {
		Package* package = *it;
		if (fPackagesAlreadyRemoved.find(package)
				!= fPackagesAlreadyRemoved.end()) {
			continue;
		}

		// get a BEntry for the package
		BEntry entry;
		status_t error = entry.SetTo(&fOldStateDirectory,
			package->FileName());
		if (error != B_OK) {
			ERROR("failed to get entry for package \"%s\": %s\n",
				package->FileName().String(), strerror(error));
			continue;
		}

		// move entry
		error = entry.MoveTo(&packagesDirectory);
		if (error != B_OK) {
			ERROR("failed to move old package \"%s\" back to packages "
				"directory: %s\n", package->FileName().String(),
				strerror(error));
			continue;
		}

		fPackageFileManager->PackageFileMoved(package->File(),
			fVolume->PackagesDirectoryRef());
		package->File()->IncrementEntryCreatedIgnoreLevel();
	}
}


void
CommitTransactionHandler::_RevertUserGroupChanges()
{
	// delete users
	for (StringSet::const_iterator it = fAddedUsers.begin();
		it != fAddedUsers.end(); ++it) {
		std::string commandLine("userdel ");
		commandLine += FSUtils::ShellEscapeString(it->c_str()).String();
		if (system(commandLine.c_str()) != 0)
			ERROR("failed to remove user \"%s\"\n", it->c_str());
	}

	// delete groups
	for (StringSet::const_iterator it = fAddedGroups.begin();
		it != fAddedGroups.end(); ++it) {
		std::string commandLine("groupdel ");
		commandLine += FSUtils::ShellEscapeString(it->c_str()).String();
		if (system(commandLine.c_str()) != 0)
			ERROR("failed to remove group \"%s\"\n", it->c_str());
	}
}


void
CommitTransactionHandler::_RunPostInstallScripts()
{
	for (PackageSet::iterator it = fAddedPackages.begin();
		it != fAddedPackages.end(); ++it) {
		Package* package = *it;
		fCurrentPackage = package;
		const BStringList& scripts = package->Info().PostInstallScripts();
		int32 count = scripts.CountStrings();
		for (int32 i = 0; i < count; i++)
			_RunPostOrPreScript(package, scripts.StringAt(i), true);
	}

	fCurrentPackage = NULL;
}


void
CommitTransactionHandler::_RunPreUninstallScripts()
{
	// Note this runs in the correct order, so dependents get uninstalled before
	// the packages they depend on.  No need for a reversed loop.
	for (PackageSet::iterator it = fPackagesToDeactivate.begin();
		it != fPackagesToDeactivate.end(); ++it) {
		Package* package = *it;
		fCurrentPackage = package;
		const BStringList& scripts = package->Info().PreUninstallScripts();
		int32 count = scripts.CountStrings();
		for (int32 i = 0; i < count; i++)
			_RunPostOrPreScript(package, scripts.StringAt(i), false);
	}

	fCurrentPackage = NULL;
}


void
CommitTransactionHandler::_RunPostOrPreScript(Package* package,
	const BString& script, bool postNotPre)
{
	const char *postOrPreInstallWording = postNotPre
		? "post-installation" : "pre-uninstall";
	BDirectory rootDir(&fVolume->RootDirectoryRef());
	BPath scriptPath(&rootDir, script);
	status_t error = scriptPath.InitCheck();
	if (error != B_OK) {
		ERROR("Volume::CommitTransactionHandler::_RunPostOrPreScript(): "
			"failed get path of %s script \"%s\" of package "
			"%s: %s\n",
			postOrPreInstallWording, script.String(),
			package->FileName().String(), strerror(error));
		_AddIssue(TransactionIssueBuilder(postNotPre
				? BTransactionIssue::B_POST_INSTALL_SCRIPT_NOT_FOUND
				: BTransactionIssue::B_PRE_UNINSTALL_SCRIPT_NOT_FOUND)
			.SetPath1(script)
			.SetSystemError(error));
		return;
	}

	errno = 0;
	int result = system(scriptPath.Path());
	if (result != 0) {
		ERROR("Volume::CommitTransactionHandler::_RunPostOrPreScript(): "
			"running %s script \"%s\" of package %s "
			"failed: %d (errno: %s)\n",
			postOrPreInstallWording, script.String(),
			package->FileName().String(), result, strerror(errno));
		if (result < 0 || result == 127) { // bash shell returns 127 on failure.
			_AddIssue(TransactionIssueBuilder(postNotPre
					? BTransactionIssue::B_STARTING_POST_INSTALL_SCRIPT_FAILED
					: BTransactionIssue::B_STARTING_PRE_UNINSTALL_SCRIPT_FAILED)
				.SetPath1(BString(scriptPath.Path()))
				.SetSystemError(errno));
		} else { // positive is an exit code from the script itself.
			_AddIssue(TransactionIssueBuilder(postNotPre
					? BTransactionIssue::B_POST_INSTALL_SCRIPT_FAILED
					: BTransactionIssue::B_PRE_UNINSTALL_SCRIPT_FAILED)
				.SetPath1(BString(scriptPath.Path()))
				.SetExitCode(result));
		}
	}
}


void
CommitTransactionHandler::_QueuePostInstallScripts()
{
	BDirectory adminDirectory;
	status_t error = _OpenPackagesSubDirectory(
		RelativePath(kAdminDirectoryName), true, adminDirectory);
	if (error != B_OK) {
		ERROR("Failed to open administrative directory: %s\n", strerror(error));
		return;
	}

	BDirectory scriptsDirectory;
	error = scriptsDirectory.SetTo(&adminDirectory, kQueuedScriptsDirectoryName);
	if (error == B_ENTRY_NOT_FOUND)
		error = adminDirectory.CreateDirectory(kQueuedScriptsDirectoryName, &scriptsDirectory);
	if (error != B_OK) {
		ERROR("Failed to open queued scripts directory: %s\n", strerror(error));
		return;
	}

	BDirectory rootDir(&fVolume->RootDirectoryRef());
	for (PackageSet::iterator it = fAddedPackages.begin();
		it != fAddedPackages.end(); ++it) {
		Package* package = *it;
		const BStringList& scripts = package->Info().PostInstallScripts();
		for (int32 i = 0; i < scripts.CountStrings(); ++i) {
			BPath scriptPath(&rootDir, scripts.StringAt(i));
			status_t error = scriptPath.InitCheck();
			if (error != B_OK) {
				ERROR("Can't find script: %s\n", scripts.StringAt(i).String());
				continue;
			}

			// symlink to the script
			BSymLink scriptLink;
			scriptsDirectory.CreateSymLink(scriptPath.Leaf(),
				scriptPath.Path(), &scriptLink);
			if (scriptLink.InitCheck() != B_OK) {
				ERROR("Creating symlink failed: %s\n", strerror(scriptLink.InitCheck()));
				continue;
			}
		}
	}
}


void
CommitTransactionHandler::_ExtractPackageContent(Package* package,
	const BStringList& contentPaths, BDirectory& targetDirectory,
	BDirectory& _extractedFilesDirectory)
{
	// check whether the subdirectory already exists
	BString targetName(package->RevisionedNameThrows());

	BEntry targetEntry;
	status_t error = targetEntry.SetTo(&targetDirectory, targetName);
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_ACCESS_ENTRY)
			.SetPath1(_GetPath(
				FSUtils::Entry(targetDirectory, targetName),
				targetName))
			.SetPackageName(package->FileName())
			.SetSystemError(error);
	}
	if (targetEntry.Exists()) {
		// nothing to do -- the very same version of the package has already
		// been extracted
		error = _extractedFilesDirectory.SetTo(&targetDirectory,
			targetName);
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
				.SetPath1(_GetPath(
					FSUtils::Entry(targetDirectory, targetName),
					targetName))
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}
		return;
	}

	// create the subdirectory with a temporary name (remove, if it already
	// exists)
	BString temporaryTargetName = BString().SetToFormat("%s.tmp",
		targetName.String());
	if (temporaryTargetName.IsEmpty())
		throw std::bad_alloc();

	error = targetEntry.SetTo(&targetDirectory, temporaryTargetName);
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_ACCESS_ENTRY)
			.SetPath1(_GetPath(
				FSUtils::Entry(targetDirectory, temporaryTargetName),
				temporaryTargetName))
			.SetPackageName(package->FileName())
			.SetSystemError(error);
	}

	if (targetEntry.Exists()) {
		// remove pre-existing
		error = BRemoveEngine().RemoveEntry(FSUtils::Entry(targetEntry));
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_REMOVE_DIRECTORY)
				.SetPath1(_GetPath(
					FSUtils::Entry(targetDirectory, temporaryTargetName),
					temporaryTargetName))
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}
	}

	BDirectory& subDirectory = _extractedFilesDirectory;
	FSTransaction::CreateOperation createSubDirectoryOperation(
		&fFSTransaction,
		FSUtils::Entry(targetDirectory, temporaryTargetName));
	error = targetDirectory.CreateDirectory(temporaryTargetName,
		&subDirectory);
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_CREATE_DIRECTORY)
			.SetPath1(_GetPath(
				FSUtils::Entry(targetDirectory, temporaryTargetName),
				temporaryTargetName))
			.SetPackageName(package->FileName())
			.SetSystemError(error);
	}

	createSubDirectoryOperation.Finished();

	// extract
	NotOwningEntryRef packageRef(package->EntryRef());

	int32 contentPathCount = contentPaths.CountStrings();
	for (int32 i = 0; i < contentPathCount; i++) {
		const char* contentPath = contentPaths.StringAt(i);

		error = FSUtils::ExtractPackageContent(FSUtils::Entry(packageRef),
			contentPath, FSUtils::Entry(subDirectory));
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_EXTRACT_PACKAGE_FILE)
				.SetPath1(contentPath)
				.SetPackageName(package->FileName())
				.SetSystemError(error);
		}
	}

	// tag all entries with the package attribute
	_TagPackageEntriesRecursively(subDirectory, targetName, true);

	// rename the subdirectory
	error = targetEntry.Rename(targetName);
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_MOVE_FILE)
			.SetPath1(_GetPath(
				FSUtils::Entry(targetDirectory, temporaryTargetName),
				temporaryTargetName))
			.SetPath2(targetName)
			.SetPackageName(package->FileName())
			.SetSystemError(error);
	}

	// keep the directory, regardless of whether the transaction is rolled
	// back
	createSubDirectoryOperation.Unregister();
}


status_t
CommitTransactionHandler::_OpenPackagesSubDirectory(const RelativePath& path,
	bool create, BDirectory& _directory)
{
	// open the packages directory
	BDirectory directory;
	status_t error = directory.SetTo(&fVolume->PackagesDirectoryRef());
	if (error != B_OK) {
		ERROR("CommitTransactionHandler::_OpenPackagesSubDirectory(): failed "
			"to open packages directory: %s\n", strerror(error));
		RETURN_ERROR(error);
	}

	return FSUtils::OpenSubDirectory(directory, path, create, _directory);
}


status_t
CommitTransactionHandler::_OpenPackagesFile(
	const RelativePath& subDirectoryPath, const char* fileName, uint32 openMode,
	BFile& _file, BEntry* _entry)
{
	BDirectory directory;
	if (!subDirectoryPath.IsEmpty()) {
		status_t error = _OpenPackagesSubDirectory(subDirectoryPath,
			(openMode & B_CREATE_FILE) != 0, directory);
		if (error != B_OK) {
			ERROR("CommitTransactionHandler::_OpenPackagesFile(): failed to "
				"open packages subdirectory \"%s\": %s\n",
				subDirectoryPath.ToString().String(), strerror(error));
			RETURN_ERROR(error);
		}
	} else {
		status_t error = directory.SetTo(&fVolume->PackagesDirectoryRef());
		if (error != B_OK) {
			ERROR("CommitTransactionHandler::_OpenPackagesFile(): failed to "
				"open packages directory: %s\n", strerror(error));
			RETURN_ERROR(error);
		}
	}

	BEntry stackEntry;
	BEntry& entry = _entry != NULL ? *_entry : stackEntry;
	status_t error = entry.SetTo(&directory, fileName);
	if (error != B_OK) {
		ERROR("CommitTransactionHandler::_OpenPackagesFile(): failed to get "
			"entry for file: %s", strerror(error));
		RETURN_ERROR(error);
	}

	return _file.SetTo(&entry, openMode);
}


void
CommitTransactionHandler::_WriteActivationFile(
	const RelativePath& directoryPath, const char* fileName,
	const PackageSet& toActivate, const PackageSet& toDeactivate,
	BEntry& _entry)
{
	// create the content
	BString activationFileContent;
	_CreateActivationFileContent(toActivate, toDeactivate,
		activationFileContent);

	// write the file
	status_t error = _WriteTextFile(directoryPath, fileName,
		activationFileContent, _entry);
	if (error != B_OK) {
		BString filePath = directoryPath.ToString() << '/' << fileName;
		throw Exception(B_TRANSACTION_FAILED_TO_WRITE_ACTIVATION_FILE)
			.SetPath1(_GetPath(
				FSUtils::Entry(fVolume->PackagesDirectoryRef(), filePath),
				filePath))
			.SetSystemError(error);
	}
}


void
CommitTransactionHandler::_CreateActivationFileContent(
	const PackageSet& toActivate, const PackageSet& toDeactivate,
	BString& _content)
{
	BString activationFileContent;
	for (PackageFileNameHashTable::Iterator it
			= fVolumeState->ByFileNameIterator();
		Package* package = it.Next();) {
		if (package->IsActive()
			&& toDeactivate.find(package) == toDeactivate.end()) {
			int32 length = activationFileContent.Length();
			activationFileContent << package->FileName() << '\n';
			if (activationFileContent.Length()
					< length + package->FileName().Length() + 1) {
				throw Exception(B_TRANSACTION_NO_MEMORY);
			}
		}
	}

	for (PackageSet::const_iterator it = toActivate.begin();
		it != toActivate.end(); ++it) {
		Package* package = *it;
		int32 length = activationFileContent.Length();
		activationFileContent << package->FileName() << '\n';
		if (activationFileContent.Length()
				< length + package->FileName().Length() + 1) {
			throw Exception(B_TRANSACTION_NO_MEMORY);
		}
	}

	_content = activationFileContent;
}


status_t
CommitTransactionHandler::_WriteTextFile(const RelativePath& directoryPath,
	const char* fileName, const BString& content, BEntry& _entry)
{
	BFile file;
	status_t error = _OpenPackagesFile(directoryPath,
		fileName, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE, file, &_entry);
	if (error != B_OK) {
		ERROR("CommitTransactionHandler::_WriteTextFile(): failed to create "
			"file \"%s/%s\": %s\n", directoryPath.ToString().String(), fileName,
			strerror(error));
		return error;
	}

	ssize_t bytesWritten = file.Write(content.String(),
		content.Length());
	if (bytesWritten < 0) {
		ERROR("CommitTransactionHandler::_WriteTextFile(): failed to write "
			"file \"%s/%s\": %s\n", directoryPath.ToString().String(), fileName,
			strerror(bytesWritten));
		return bytesWritten;
	}

	return B_OK;
}


void
CommitTransactionHandler::_ChangePackageActivation(
	const PackageSet& packagesToActivate,
	const PackageSet& packagesToDeactivate)
{
	INFORM("CommitTransactionHandler::_ChangePackageActivation(): activating "
		"%zu, deactivating %zu packages\n", packagesToActivate.size(),
		packagesToDeactivate.size());

	// write the temporary package activation file
	BEntry activationFileEntry;
	_WriteActivationFile(RelativePath(kAdminDirectoryName),
		kTemporaryActivationFileName, packagesToActivate, packagesToDeactivate,
		activationFileEntry);

	// notify packagefs
	if (fVolumeStateIsActive) {
		_ChangePackageActivationIOCtl(packagesToActivate, packagesToDeactivate);
	} else {
		// TODO: Notify packagefs that active packages have been moved or do
		// node monitoring in packagefs!
	}

	// rename the temporary activation file to the final file
	status_t error = activationFileEntry.Rename(kActivationFileName, true);
	if (error != B_OK) {
		throw Exception(B_TRANSACTION_FAILED_TO_MOVE_FILE)
			.SetPath1(_GetPath(
				FSUtils::Entry(activationFileEntry),
				activationFileEntry.Name()))
			.SetPath2(kActivationFileName)
			.SetSystemError(error);

// TODO: We should probably try to revert the activation changes, though that
// will fail, if this method has been called in response to node monitoring
// events. Alternatively moving the package activation file could be made part
// of the ioctl(), since packagefs should be able to undo package changes until
// the very end, unless running out of memory. In the end the situation would be
// bad anyway, though, since the activation file may refer to removed packages
// and things would be in an inconsistent state after rebooting.
	}

	// Update our state, i.e. remove deactivated packages and mark activated
	// packages accordingly.
	fVolumeState->ActivationChanged(packagesToActivate, packagesToDeactivate);
}


void
CommitTransactionHandler::_ChangePackageActivationIOCtl(
	const PackageSet& packagesToActivate,
	const PackageSet& packagesToDeactivate)
{
	// compute the size of the allocation we need for the activation change
	// request
	int32 itemCount = packagesToActivate.size() + packagesToDeactivate.size();
	size_t requestSize = sizeof(PackageFSActivationChangeRequest)
		+ itemCount * sizeof(PackageFSActivationChangeItem);

	for (PackageSet::iterator it = packagesToActivate.begin();
		 it != packagesToActivate.end(); ++it) {
		requestSize += (*it)->FileName().Length() + 1;
	}

	for (PackageSet::iterator it = packagesToDeactivate.begin();
		 it != packagesToDeactivate.end(); ++it) {
		requestSize += (*it)->FileName().Length() + 1;
	}

	// allocate and prepare the request
	PackageFSActivationChangeRequest* request
		= (PackageFSActivationChangeRequest*)malloc(requestSize);
	if (request == NULL)
		throw Exception(B_TRANSACTION_NO_MEMORY);
	MemoryDeleter requestDeleter(request);

	request->itemCount = itemCount;

	PackageFSActivationChangeItem* item = &request->items[0];
	char* nameBuffer = (char*)(item + itemCount);

	for (PackageSet::iterator it = packagesToActivate.begin();
		it != packagesToActivate.end(); ++it, item++) {
		_FillInActivationChangeItem(item, PACKAGE_FS_ACTIVATE_PACKAGE, *it,
			nameBuffer);
	}

	for (PackageSet::iterator it = packagesToDeactivate.begin();
		it != packagesToDeactivate.end(); ++it, item++) {
		_FillInActivationChangeItem(item, PACKAGE_FS_DEACTIVATE_PACKAGE, *it,
			nameBuffer);
	}

	// issue the request
	int fd = fVolume->OpenRootDirectory();
	if (fd < 0) {
		throw Exception(B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY)
			.SetPath1(_GetPath(
				FSUtils::Entry(fVolume->RootDirectoryRef()),
				"<packagefs root>"))
			.SetSystemError(fd);
	}
	FileDescriptorCloser fdCloser(fd);

	if (ioctl(fd, PACKAGE_FS_OPERATION_CHANGE_ACTIVATION, request, requestSize)
			!= 0) {
// TODO: We need more error information and error handling!
		throw Exception(B_TRANSACTION_FAILED_TO_CHANGE_PACKAGE_ACTIVATION)
			.SetSystemError(errno);
	}
}


void
CommitTransactionHandler::_FillInActivationChangeItem(
	PackageFSActivationChangeItem* item, PackageFSActivationChangeType type,
	Package* package, char*& nameBuffer)
{
	item->type = type;
	item->packageDeviceID = package->NodeRef().device;
	item->packageNodeID = package->NodeRef().node;
	item->nameLength = package->FileName().Length();
	item->parentDeviceID = fVolume->PackagesDeviceID();
	item->parentDirectoryID = fVolume->PackagesDirectoryID();
	item->name = nameBuffer;
	strcpy(nameBuffer, package->FileName());
	nameBuffer += package->FileName().Length() + 1;
}


bool
CommitTransactionHandler::_IsSystemPackage(Package* package)
{
	// package name should be "haiku[_<arch>]"
	const BString& name = package->Info().Name();
	if (!name.StartsWith("haiku"))
		return false;
	if (name.Length() == 5)
		return true;
	if (name[5] != '_')
		return false;

	BPackageArchitecture architecture;
	return BPackageInfo::GetArchitectureByName(name.String() + 6, architecture)
		== B_OK;
}


void
CommitTransactionHandler::_AddIssue(const TransactionIssueBuilder& builder)
{
	fResult.AddIssue(builder.BuildIssue(fCurrentPackage));
}


/*static*/ BString
CommitTransactionHandler::_GetPath(const FSUtils::Entry& entry,
	const BString& fallback)
{
	BString path = entry.Path();
	return path.IsEmpty() ? fallback : path;
}


/*static*/ void
CommitTransactionHandler::_TagPackageEntriesRecursively(BDirectory& directory,
	const BString& value, bool nonDirectoriesOnly)
{
	char buffer[sizeof(dirent) + B_FILE_NAME_LENGTH];
	dirent *entry = (dirent*)buffer;
	while (directory.GetNextDirents(entry, sizeof(buffer), 1) == 1) {
		if (strcmp(entry->d_name, ".") == 0
			|| strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		// determine type
		struct stat st;
		status_t error = directory.GetStatFor(entry->d_name, &st);
		if (error != B_OK) {
			throw Exception(B_TRANSACTION_FAILED_TO_ACCESS_ENTRY)
				.SetPath1(_GetPath(
					FSUtils::Entry(directory, entry->d_name),
					entry->d_name))
				.SetSystemError(error);
		}
		bool isDirectory = S_ISDIR(st.st_mode);

		// open the node and set the attribute
		BNode stackNode;
		BDirectory stackDirectory;
		BNode* node;
		if (isDirectory) {
			node = &stackDirectory;
			error = stackDirectory.SetTo(&directory, entry->d_name);
		} else {
			node = &stackNode;
			error = stackNode.SetTo(&directory, entry->d_name);
		}

		if (error != B_OK) {
			throw Exception(isDirectory
					? B_TRANSACTION_FAILED_TO_OPEN_DIRECTORY
					: B_TRANSACTION_FAILED_TO_OPEN_FILE)
				.SetPath1(_GetPath(
					FSUtils::Entry(directory, entry->d_name),
					entry->d_name))
				.SetSystemError(error);
		}

		if (!isDirectory || !nonDirectoriesOnly) {
			error = node->WriteAttrString(kPackageFileAttribute, &value);
			if (error != B_OK) {
				throw Exception(B_TRANSACTION_FAILED_TO_WRITE_FILE_ATTRIBUTE)
					.SetPath1(_GetPath(
						FSUtils::Entry(directory, entry->d_name),
						entry->d_name))
					.SetSystemError(error);
			}
		}

		// recurse
		if (isDirectory) {
			_TagPackageEntriesRecursively(stackDirectory, value,
				nonDirectoriesOnly);
		}
	}
}
