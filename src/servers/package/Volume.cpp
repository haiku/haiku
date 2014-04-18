/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Volume.h"

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <string>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <Path.h>

#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverRepository.h>
#include <package/solver/SolverResult.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <CopyEngine.h>
#include <NotOwningEntryRef.h>
#include <package/DaemonDefs.h>
#include <package/PackagesDirectoryDefs.h>
#include <RemoveEngine.h>

#include "DebugSupport.h"
#include "Exception.h"
#include "FSTransaction.h"


using namespace BPackageKit::BPrivate;

typedef std::set<std::string> StringSet;


static const char* const kPackageFileNameExtension = ".hpkg";
static const char* const kAdminDirectoryName
	= PACKAGES_DIRECTORY_ADMIN_DIRECTORY;
static const char* const kActivationFileName
	= PACKAGES_DIRECTORY_ACTIVATION_FILE;
static const char* const kTemporaryActivationFileName
	= PACKAGES_DIRECTORY_ACTIVATION_FILE ".tmp";
static const char* const kWritableFilesDirectoryName = "writable-files";
static const char* const kPackageFileAttribute = "SYS:PACKAGE";

static const bigtime_t kHandleNodeMonitorEvents = 'nmon';

static const bigtime_t kNodeMonitorEventHandlingDelay = 500000;
static const bigtime_t kCommunicationTimeout = 1000000;


// #pragma mark - Listener


Volume::Listener::~Listener()
{
}


// #pragma mark - NodeMonitorEvent


struct Volume::NodeMonitorEvent
	: public DoublyLinkedListLinkImpl<NodeMonitorEvent> {
public:
	NodeMonitorEvent(const BString& entryName, bool created)
		:
		fEntryName(entryName),
		fCreated(created)
	{
	}

	const BString& EntryName() const
	{
		return fEntryName;
	}

	bool WasCreated() const
	{
		return fCreated;
	}

private:
	BString	fEntryName;
	bool	fCreated;
};


// #pragma mark - State


struct Volume::State {

	State()
		:
		fLock("volume state"),
		fPackagesByFileName(),
		fPackagesByNodeRef(),
		fChangeCount(0),
		fPendingPackageJobCount(0)
	{
	}

	~State()
	{
		fPackagesByFileName.Clear();

		Package* package = fPackagesByNodeRef.Clear(true);
		while (package != NULL) {
			Package* next = package->NodeRefHashTableNext();
			delete package;
			package = next;
		}
	}

	bool Init()
	{
		return fLock.InitCheck() == B_OK && fPackagesByFileName.Init() == B_OK
			&& fPackagesByNodeRef.Init() == B_OK;
	}

	bool Lock()
	{
		return fLock.Lock();
	}

	void Unlock()
	{
		fLock.Unlock();
	}

	int64 ChangeCount() const
	{
		return fChangeCount;
	}

	Package* FindPackage(const char* name) const
	{
		return fPackagesByFileName.Lookup(name);
	}

	Package* FindPackage(const node_ref& nodeRef) const
	{
		return fPackagesByNodeRef.Lookup(nodeRef);
	}

	PackageFileNameHashTable::Iterator ByFileNameIterator() const
	{
		return fPackagesByFileName.GetIterator();
	}

	PackageNodeRefHashTable::Iterator ByNodeRefIterator() const
	{
		return fPackagesByNodeRef.GetIterator();
	}

	void AddPackage(Package* package)
	{
		AutoLocker<BLocker> locker(fLock);
		fPackagesByFileName.Insert(package);
		fPackagesByNodeRef.Insert(package);
	}

	void RemovePackage(Package* package)
	{
		AutoLocker<BLocker> locker(fLock);
		_RemovePackage(package);
	}

	void SetPackageActive(Package* package, bool active)
	{
		AutoLocker<BLocker> locker(fLock);
		package->SetActive(active);
	}

	void ActivationChanged(const PackageSet& activatedPackage,
		const PackageSet& deactivatePackages)
	{
		AutoLocker<BLocker> locker(fLock);

		for (PackageSet::iterator it = activatedPackage.begin();
				it != activatedPackage.end(); ++it) {
			(*it)->SetActive(true);
			fChangeCount++;
		}

		for (PackageSet::iterator it = deactivatePackages.begin();
				it != deactivatePackages.end(); ++it) {
			Package* package = *it;
			_RemovePackage(package);
			delete package;
		}
	}

	void PackageJobPending()
	{
		atomic_add(&fPendingPackageJobCount, 1);
	}


	void PackageJobFinished()
	{
		atomic_add(&fPendingPackageJobCount, -1);
	}


	bool IsPackageJobPending() const
	{
		return fPendingPackageJobCount != 0;
	}

private:
	void _RemovePackage(Package* package)
	{
		fPackagesByFileName.Remove(package);
		fPackagesByNodeRef.Remove(package);
		fChangeCount++;
	}

private:
	BLocker						fLock;
	PackageFileNameHashTable	fPackagesByFileName;
	PackageNodeRefHashTable		fPackagesByNodeRef;
	int64						fChangeCount;
	int32						fPendingPackageJobCount;
};


// #pragma mark - CommitTransactionHandler


struct Volume::CommitTransactionHandler {
	CommitTransactionHandler(Volume* volume,
		const PackageSet& packagesAlreadyAdded,
		const PackageSet& packagesAlreadyRemoved)
		:
		fVolume(volume),
		fPackagesToActivate(),
		fPackagesToDeactivate(),
		fAddedPackages(),
		fRemovedPackages(),
		fPackagesAlreadyAdded(packagesAlreadyAdded),
		fPackagesAlreadyRemoved(packagesAlreadyRemoved),
		fAddedGroups(),
		fAddedUsers(),
		fFSTransaction()
	{
	}

	~CommitTransactionHandler()
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
	}

	void HandleRequest(BMessage* request, BMessage* reply)
	{
		status_t error;
		BActivationTransaction transaction(request, &error);
		if (error == B_OK)
			error = transaction.InitCheck();
		if (error != B_OK) {
			if (error == B_NO_MEMORY)
				throw Exception(B_NO_MEMORY);
			throw Exception(B_DAEMON_BAD_REQUEST);
		}

		HandleRequest(transaction, reply);
	}

	void HandleRequest(const BActivationTransaction& transaction,
		BMessage* reply)
	{
		// check the change count
		if (transaction.ChangeCount() != fVolume->fState->ChangeCount())
			throw Exception(B_DAEMON_CHANGE_COUNT_MISMATCH);

		// collect the packages to deactivate
		_GetPackagesToDeactivate(transaction);

		// read the packages to activate
		_ReadPackagesToActivate(transaction);

		// anything to do at all?
		if (fPackagesToActivate.IsEmpty() &&  fPackagesToDeactivate.empty()) {
			throw Exception(B_DAEMON_BAD_REQUEST,
				"no packages to activate or deactivate");
		}

		_ApplyChanges(reply);
	}

	void HandleRequest(const PackageSet& packagesAdded,
		const PackageSet& packagesRemoved)
	{
		// Copy package sets to fPackagesToActivate/fPackagesToDeactivate. The
		// given sets are assumed to be identical to the ones specified in the
		// constructor invocation (fPackagesAlreadyAdded,
		// fPackagesAlreadyRemoved).
		for (PackageSet::const_iterator it = packagesAdded.begin();
			it != packagesAdded.end(); ++it) {
			if (!fPackagesToActivate.AddItem(*it))
				throw std::bad_alloc();
		}

		fPackagesToDeactivate = packagesRemoved;

		_ApplyChanges(NULL);
	}

	void Revert()
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

	const BString& OldStateDirectoryName() const
	{
		return fOldStateDirectoryName;
	}

private:
	typedef BObjectList<Package> PackageList;

	void _GetPackagesToDeactivate(const BActivationTransaction& transaction)
	{
		// get the number of packages to deactivate
		const BStringList& packagesToDeactivate
			= transaction.PackagesToDeactivate();
		int32 packagesToDeactivateCount = packagesToDeactivate.CountStrings();
		if (packagesToDeactivateCount == 0)
			return;

		for (int32 i = 0; i < packagesToDeactivateCount; i++) {
			BString packageName = packagesToDeactivate.StringAt(i);
			Package* package = fVolume->fState->FindPackage(packageName);
			if (package == NULL) {
				throw Exception(B_DAEMON_NO_SUCH_PACKAGE, "no such package",
					packageName);
			}

			fPackagesToDeactivate.insert(package);

			if (fPackagesAlreadyRemoved.find(package)
					== fPackagesAlreadyRemoved.end()) {
				package->IncrementEntryRemovedIgnoreLevel();
			}
		}
	}

	void _ReadPackagesToActivate(const BActivationTransaction& transaction)
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
			throw Exception(B_DAEMON_BAD_REQUEST);
		}

		// open the directory
		RelativePath directoryPath(kAdminDirectoryName,
			transactionDirectoryName);
		BDirectory directory;
		status_t error = fVolume->_OpenPackagesSubDirectory(directoryPath,
			false, directory);
		if (error != B_OK)
			throw Exception(error, "failed to open transaction directory");

		error = directory.GetNodeRef(&fTransactionDirectoryRef);
		if (error != B_OK) {
			throw Exception(error,
				"failed to get transaction directory node ref");
		}

		// read the packages
		for (int32 i = 0; i < packagesToActivateCount; i++) {
			BString packageName = packagesToActivate.StringAt(i);

			// make sure it doesn't clash with an already existing package
			Package* package = fVolume->fState->FindPackage(packageName);
			if (package != NULL) {
				if (fPackagesAlreadyAdded.find(package)
						!= fPackagesAlreadyAdded.end()) {
					if (!fPackagesToActivate.AddItem(package))
						throw Exception(B_NO_MEMORY);
					continue;
				}

				if (fPackagesToDeactivate.find(package)
						== fPackagesToDeactivate.end()) {
					throw Exception(B_DAEMON_PACKAGE_ALREADY_EXISTS, NULL,
						packageName);
				}
			}

			// read the package
			entry_ref entryRef;
			entryRef.device = fTransactionDirectoryRef.device;
			entryRef.directory = fTransactionDirectoryRef.node;
			if (entryRef.set_name(packageName) != B_OK)
				throw Exception(B_NO_MEMORY);

			package = new(std::nothrow) Package;
			if (package == NULL || !fPackagesToActivate.AddItem(package)) {
				delete package;
				throw Exception(B_NO_MEMORY);
			}

			error = package->Init(entryRef);
			if (error != B_OK)
				throw Exception(error, "failed to read package", packageName);

			package->IncrementEntryCreatedIgnoreLevel();
		}
	}

	void _ApplyChanges(BMessage* reply)
	{
		// create an old state directory
		_CreateOldStateDirectory(reply);

		// move packages to deactivate to old state directory
		_RemovePackagesToDeactivate();

		// move packages to activate to packages directory
		_AddPackagesToActivate();

		// activate/deactivate packages
		fVolume->_ChangePackageActivation(fAddedPackages, fRemovedPackages);

		// run post-installation scripts
		_RunPostInstallScripts();

		// removed packages have been deleted, new packages shall not be deleted
		fAddedPackages.clear();
		fRemovedPackages.clear();
		fPackagesToActivate.MakeEmpty(false);
		fPackagesToDeactivate.clear();
	}

	void _CreateOldStateDirectory(BMessage* reply)
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
			throw Exception(B_NO_MEMORY);

		// make sure the directory doesn't exist yet
		BDirectory adminDirectory;
		status_t error = fVolume->_OpenPackagesSubDirectory(
			RelativePath(kAdminDirectoryName), true, adminDirectory);
		if (error != B_OK)
			throw Exception(error, "failed to open administrative directory");

		int uniqueId = 1;
		BString directoryName = baseName;
		while (BEntry(&adminDirectory, directoryName).Exists()) {
			directoryName.SetToFormat("%s-%d", baseName.String(), uniqueId++);
			if (directoryName.IsEmpty())
				throw Exception(B_NO_MEMORY);
		}

		// create the directory
		FSTransaction::CreateOperation createOldStateDirectoryOperation(
			&fFSTransaction, FSUtils::Entry(adminDirectory, directoryName));

		error = adminDirectory.CreateDirectory(directoryName,
			&fOldStateDirectory);
		if (error != B_OK)
			throw Exception(error, "failed to create old state directory");

		createOldStateDirectoryOperation.Finished();

		fOldStateDirectoryName = directoryName;

		// write the old activation file
		BEntry activationFile;
		error = fVolume->_WriteActivationFile(
			RelativePath(kAdminDirectoryName, directoryName),
			kActivationFileName, PackageSet(), PackageSet(), activationFile);
		if (error != B_OK)
			throw Exception(error, "failed to write old activation file");

		// add the old state directory to the reply
		if (reply != NULL) {
			error = reply->AddString("old state", fOldStateDirectoryName);
			if (error != B_OK)
				throw Exception(error, "failed to add field to reply");
		}
	}

	void _RemovePackagesToDeactivate()
	{
		if (fPackagesToDeactivate.empty())
			return;

		for (PackageSet::const_iterator it = fPackagesToDeactivate.begin();
			it != fPackagesToDeactivate.end(); ++it) {
			Package* package = *it;
			if (fPackagesAlreadyRemoved.find(package)
					!= fPackagesAlreadyRemoved.end()) {
				fRemovedPackages.insert(package);
				continue;
			}

			// get a BEntry for the package
			NotOwningEntryRef entryRef(fVolume->fPackagesDirectoryRef,
				package->FileName());

			BEntry entry;
			status_t error = entry.SetTo(&entryRef);
			if (error != B_OK) {
				throw Exception(error, "failed to get package entry",
					package->FileName());
			}

			// move entry
			fRemovedPackages.insert(package);

			error = entry.MoveTo(&fOldStateDirectory);
			if (error != B_OK) {
				fRemovedPackages.erase(package);
				throw Exception(error,
					"failed to move old package from packages directory",
					package->FileName());
			}
		}
	}

	void _AddPackagesToActivate()
	{
		if (fPackagesToActivate.IsEmpty())
			return;

		// open packages directory
		BDirectory packagesDirectory;
		status_t error
			= packagesDirectory.SetTo(&fVolume->fPackagesDirectoryRef);
		if (error != B_OK)
			throw Exception(error, "failed to open packages directory");

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
			entry_ref entryRef;
			entryRef.device = fTransactionDirectoryRef.device;
			entryRef.directory = fTransactionDirectoryRef.node;
			if (entryRef.set_name(package->FileName()) != B_OK)
				throw Exception(B_NO_MEMORY);

			BEntry entry;
			error = entry.SetTo(&entryRef);
			if (error != B_OK) {
				throw Exception(error, "failed to get package entry",
					package->FileName());
			}

			// move entry
			fAddedPackages.insert(package);

			error = entry.MoveTo(&packagesDirectory);
			if (error != B_OK) {
				fAddedPackages.erase(package);
				throw Exception(error,
					"failed to move new package to packages directory",
					package->FileName());
			}

			// also add the package to the volume
			fVolume->_AddPackage(package);

			_PreparePackageToActivate(package);
		}
	}

	void _PreparePackageToActivate(Package* package)
	{
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
	}

	void _AddGroup(Package* package, const BString& groupName)
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
			throw Exception(error,
				BString().SetToFormat("failed to add group \%s\"",
					groupName.String()),
				package->FileName());
		}
	}

	void _AddUser(Package* package, const BUser& user)
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
			throw Exception(error,
				BString().SetToFormat("failed to add user \%s\"",
					user.Name().String()),
				package->FileName());
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
				throw Exception(error,
					BString().SetToFormat("failed to add user \%s\" to group "
						"\"%s\"", user.Name().String(),
						user.Groups().StringAt(i).String()),
					package->FileName());
			}
		}
	}

	void _AddGlobalWritableFiles(Package* package)
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
		status_t error = rootDirectory.SetTo(&fVolume->fRootDirectoryRef);
		if (error != B_OK) {
			throw Exception(error,
				BString().SetToFormat("failed to get the root directory "
					"for writable files"),
				package->FileName());
		}

		// Open writable-files directory in the administrative directory.
		if (fWritableFilesDirectory.InitCheck() != B_OK) {
			error = fVolume->_OpenPackagesSubDirectory(
				RelativePath(kAdminDirectoryName, kWritableFilesDirectoryName),
				true, fWritableFilesDirectory);

			if (error != B_OK) {
				throw Exception(error,
					BString().SetToFormat("failed to get the backup directory "
						"for writable files"),
					package->FileName());
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

	void _AddGlobalWritableFile(Package* package,
		const BGlobalWritableFileInfo& file, const BDirectory& rootDirectory,
		const BDirectory& extractedFilesDirectory)
	{
		// Map the path name to the actual target location. Currently this only
		// concerns "settings/", which is mapped to "settings/global/".
		BString targetPath(file.Path());
		if (fVolume->fMountType == PACKAGE_FS_MOUNT_TYPE_HOME) {
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
				throw Exception(error,
					BString().SetToFormat("failed to open directory \"%s\"",
						_GetPath(
							FSUtils::Entry(extractedFilesDirectory,
								sourceParentPath),
							sourceParentPath).String()),
					package->FileName());
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
				throw Exception(error,
					BString().SetToFormat("failed to open/create directory "
						"\"%s\"",
						_GetPath(
							FSUtils::Entry(rootDirectory,targetParentPath),
							targetParentPath).String()),
					package->FileName());
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

	void _AddGlobalWritableFileRecurse(Package* package,
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

				throw Exception(error,
					BString().SetToFormat("failed to copy entry \"%s\"",
						_GetPath(
							FSUtils::Entry(sourceDirectory,
								relativeSourcePath.Leaf()),
							relativeSourcePath).String()),
					package->FileName());
			}
			copyOperation.Finished();
			return;
		}

		struct stat sourceStat;
		status_t error = sourceDirectory.GetStatFor(relativeSourcePath.Leaf(),
			&sourceStat);
		if (error != B_OK) {
			throw Exception(error,
				BString().SetToFormat("failed to get stat data for entry "
					"\"%s\"",
					_GetPath(
						FSUtils::Entry(targetDirectory, targetName),
						targetName).String()),
				package->FileName());
		}

		if ((sourceStat.st_mode & S_IFMT) != (targetStat.st_mode & S_IFMT)
			|| (!S_ISDIR(sourceStat.st_mode) && !S_ISREG(sourceStat.st_mode)
				&& !S_ISLNK(sourceStat.st_mode))) {
			// Source and target entry types don't match or this is an entry
			// we cannot handle. The user must handle this manually.
			PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
				"writable file exists, but type doesn't match previous type\n");
// TODO: Notify user!
			return;
		}

		if (S_ISDIR(sourceStat.st_mode)) {
			// entry is a directory -- recurse
			BDirectory sourceSubDirectory;
			error = sourceSubDirectory.SetTo(&sourceDirectory,
				relativeSourcePath.Leaf());
			if (error != B_OK) {
				throw Exception(error,
					BString().SetToFormat("failed to open directory \"%s\"",
						_GetPath(
							FSUtils::Entry(sourceDirectory,
								relativeSourcePath.Leaf()),
							relativeSourcePath).String()),
					package->FileName());
			}

			BDirectory targetSubDirectory;
			error = targetSubDirectory.SetTo(&targetDirectory, targetName);
			if (error != B_OK) {
				throw Exception(error,
					BString().SetToFormat("failed to open directory \"%s\"",
						_GetPath(
							FSUtils::Entry(targetDirectory, targetName),
							targetName).String()),
					package->FileName());
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
// TODO: Notify user, if not B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD!
			PRINT("Volume::CommitTransactionHandler::_AddGlobalWritableFile(): "
				"failed to get SYS:PACKAGE attribute\n");
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
		if (fWritableFilesDirectory.GetStatFor(originalRelativeSourcePath,
				&originalPackageStat) != B_OK
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
			return;
// TODO: Notify user!
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
				return;
// TODO: Notify user, if not B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD!
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
				return;
// TODO: Notify user, if not B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD!
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
			throw Exception(error,
				BString().SetToFormat("failed to copy entry \"%s\"",
					_GetPath(
						FSUtils::Entry(sourceDirectory,
							relativeSourcePath.Leaf()),
						relativeSourcePath).String()),
				package->FileName());
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
			throw Exception(error,
				BString().SetToFormat("failed to rename entry \"%s\" to \"%s\"",
					_GetPath(
						FSUtils::Entry(targetDirectory, tempTargetName),
						tempTargetName).String(),
					targetName),
				package->FileName());
		}

		renameOperation.Finished();
		copyOperation.Unregister();
	}

	void _RevertAddPackagesToActivate()
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

			fVolume->_RemovePackage(package);

			if (transactionDirectory.InitCheck() != B_OK)
				continue;

			// get BEntry for the package
			NotOwningEntryRef entryRef(fVolume->fPackagesDirectoryRef,
				package->FileName());

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
		}
	}

	void _RevertRemovePackagesToDeactivate()
	{
		if (fRemovedPackages.empty())
			return;

		// open packages directory
		BDirectory packagesDirectory;
		status_t error
			= packagesDirectory.SetTo(&fVolume->fPackagesDirectoryRef);
		if (error != B_OK) {
			throw Exception(error, "failed to open packages directory");
			ERROR("failed to open packages directory: %s\n",
				strerror(error));
			return;
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
		}
	}

	void _RevertUserGroupChanges()
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

	void _RunPostInstallScripts()
	{
		for (PackageSet::iterator it = fAddedPackages.begin();
			it != fAddedPackages.end(); ++it) {
			Package* package = *it;
			const BStringList& scripts = package->Info().PostInstallScripts();
			int32 count = scripts.CountStrings();
			for (int32 i = 0; i < count; i++)
				_RunPostInstallScript(package, scripts.StringAt(i));
		}
	}

	void _RunPostInstallScript(Package* package, const BString& script)
	{
		BDirectory rootDir(&fVolume->fRootDirectoryRef);
		BPath scriptPath(&rootDir, script);
		status_t error = scriptPath.InitCheck();
		if (error != B_OK) {
			ERROR("Volume::CommitTransactionHandler::_RunPostInstallScript(): "
				"failed get path of post-installation script \"%s\" of package "
				"%s: %s\n", script.String(), package->FileName().String(),
				strerror(error));
// TODO: Notify the user!
			return;
		}

		if (system(scriptPath.Path()) != 0) {
			ERROR("Volume::CommitTransactionHandler::_RunPostInstallScript(): "
				"running post-installation script \"%s\" of package %s "
				"failed: %s\n", script.String(), package->FileName().String(),
				strerror(error));
// TODO: Notify the user!
		}
	}

	static BString _GetPath(const FSUtils::Entry& entry,
		const BString& fallback)
	{
		BString path = entry.Path();
		return path.IsEmpty() ? fallback : path;
	}

	void _ExtractPackageContent(Package* package,
		const BStringList& contentPaths, BDirectory& targetDirectory,
		BDirectory& _extractedFilesDirectory)
	{
		// check whether the subdirectory already exists
		BString targetName(package->RevisionedNameThrows());

		BEntry targetEntry;
		status_t error = targetEntry.SetTo(&targetDirectory, targetName);
		if (error != B_OK) {
			throw Exception(error,
				BString().SetToFormat("failed to init entry \"%s\"",
					_GetPath(
						FSUtils::Entry(targetDirectory, targetName),
						targetName).String()),
				package->FileName());
		}
		if (targetEntry.Exists()) {
			// nothing to do -- the very same version of the package has already
			// been extracted
			error = _extractedFilesDirectory.SetTo(&targetDirectory,
				targetName);
			if (error != B_OK) {
				throw Exception(error,
					BString().SetToFormat("failed to open directory \"%s\"",
						_GetPath(
							FSUtils::Entry(targetDirectory, targetName),
							targetName).String()),
					package->FileName());
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
			throw Exception(error,
				BString().SetToFormat("failed to init entry \"%s\"",
					_GetPath(
						FSUtils::Entry(targetDirectory, temporaryTargetName),
						temporaryTargetName).String()),
				package->FileName());
		}

		if (targetEntry.Exists()) {
			// remove pre-existing
			error = BRemoveEngine().RemoveEntry(FSUtils::Entry(targetEntry));
			if (error != B_OK) {
				throw Exception(error,
					BString().SetToFormat("failed to remove directory \"%s\"",
						_GetPath(
							FSUtils::Entry(targetDirectory,
								temporaryTargetName),
							temporaryTargetName).String()),
					package->FileName());
			}
		}

		BDirectory& subDirectory = _extractedFilesDirectory;
		FSTransaction::CreateOperation createSubDirectoryOperation(
			&fFSTransaction,
			FSUtils::Entry(targetDirectory, temporaryTargetName));
		error = targetDirectory.CreateDirectory(temporaryTargetName,
			&subDirectory);
		if (error != B_OK) {
			throw Exception(error,
				BString().SetToFormat("failed to create directory \"%s\"",
					_GetPath(
						FSUtils::Entry(targetDirectory, temporaryTargetName),
						temporaryTargetName).String()),
				package->FileName());
		}

		createSubDirectoryOperation.Finished();

		// extract
		NotOwningEntryRef packageRef(fVolume->fPackagesDirectoryRef,
			package->FileName());

		int32 contentPathCount = contentPaths.CountStrings();
		for (int32 i = 0; i < contentPathCount; i++) {
			const char* contentPath = contentPaths.StringAt(i);

			error = FSUtils::ExtractPackageContent(FSUtils::Entry(packageRef),
				contentPath, FSUtils::Entry(subDirectory));
			if (error != B_OK) {
				throw Exception(error,
					BString().SetToFormat(
						"failed to extract \"%s\" from package", contentPath),
					package->FileName());
			}
		}

		// tag all entries with the package attribute
		error = _TagPackageEntriesRecursively(subDirectory, targetName, true);
		if (error != B_OK) {
			throw Exception(error,
				BString().SetToFormat("failed to tag extract files in \"%s\" "
					"with package attribute",
					_GetPath(
						FSUtils::Entry(targetDirectory, temporaryTargetName),
						temporaryTargetName).String()),
				package->FileName());
		}

		// rename the subdirectory
		error = targetEntry.Rename(targetName);
		if (error != B_OK) {
			throw Exception(error,
				BString().SetToFormat("failed to rename entry \"%s\" to \"%s\"",
					_GetPath(
						FSUtils::Entry(targetDirectory, temporaryTargetName),
						temporaryTargetName).String(),
					targetName.String()),
				package->FileName());
		}

		// keep the directory, regardless of whether the transaction is rolled
		// back
		createSubDirectoryOperation.Unregister();
	}

	static status_t _TagPackageEntriesRecursively(BDirectory& directory,
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
			if (error != B_OK)
				return error;
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

			if (error != B_OK)
				return error;

			if (!isDirectory || !nonDirectoriesOnly) {
				error = node->WriteAttrString(kPackageFileAttribute, &value);
				if (error != B_OK)
					return error;
			}

			// recurse
			if (isDirectory) {
				error = _TagPackageEntriesRecursively(stackDirectory, value,
					nonDirectoriesOnly);
				if (error != B_OK)
					return error;
			}
		}

		return B_OK;
	}

private:
	Volume*				fVolume;
	PackageList			fPackagesToActivate;
	PackageSet			fPackagesToDeactivate;
	PackageSet			fAddedPackages;
	PackageSet			fRemovedPackages;
	const PackageSet&	fPackagesAlreadyAdded;
	const PackageSet&	fPackagesAlreadyRemoved;
	BDirectory			fOldStateDirectory;
	BString				fOldStateDirectoryName;
	node_ref			fTransactionDirectoryRef;
	BDirectory			fWritableFilesDirectory;
	StringSet			fAddedGroups;
	StringSet			fAddedUsers;
	FSTransaction		fFSTransaction;
};


// #pragma mark - Volume


Volume::Volume(BLooper* looper)
	:
	BHandler(),
	fPath(),
	fMountType(PACKAGE_FS_MOUNT_TYPE_CUSTOM),
	fRootDirectoryRef(),
	fPackagesDirectoryRef(),
	fRoot(NULL),
	fListener(NULL),
	fState(NULL),
	fPendingNodeMonitorEventsLock("pending node monitor events"),
	fPendingNodeMonitorEvents(),
	fNodeMonitorEventHandleTime(0),
	fPackagesToBeActivated(),
	fPackagesToBeDeactivated(),
	fLocationInfoReply(B_MESSAGE_GET_INSTALLATION_LOCATION_INFO_REPLY)
{
	looper->AddHandler(this);
}


Volume::~Volume()
{
	Unmounted();
		// need for error case in InitPackages()

	delete fState;
}


status_t
Volume::Init(const node_ref& rootDirectoryRef, node_ref& _packageRootRef)
{
	fState = new(std::nothrow) State;
	if (fState == NULL || !fState->Init())
		RETURN_ERROR(B_NO_MEMORY);

	fRootDirectoryRef = rootDirectoryRef;

	// open the root directory
	BDirectory directory;
	status_t error = directory.SetTo(&fRootDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::Init(): failed to open root directory: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	// get the directory path
	BEntry entry;
	error = directory.GetEntry(&entry);

	BPath path;
	if (error == B_OK)
		error = entry.GetPath(&path);

	if (error != B_OK) {
		ERROR("Volume::Init(): failed to get root directory path: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	fPath = path.Path();
	if (fPath.IsEmpty())
		RETURN_ERROR(B_NO_MEMORY);

	// get a volume info from the FS
	int fd = directory.Dup();
	if (fd < 0) {
		ERROR("Volume::Init(): failed to get root directory FD: %s\n",
			strerror(fd));
		RETURN_ERROR(fd);
	}
	FileDescriptorCloser fdCloser(fd);

	PackageFSVolumeInfo info;
	if (ioctl(fd, PACKAGE_FS_OPERATION_GET_VOLUME_INFO, &info, sizeof(info))
			!= 0) {
		ERROR("Volume::Init(): failed to get volume info: %s\n",
			strerror(errno));
		RETURN_ERROR(errno);
	}

	fMountType = info.mountType;
	fPackagesDirectoryRef.device = info.packagesDirectoryInfos[0].deviceID;
	fPackagesDirectoryRef.node = info.packagesDirectoryInfos[0].nodeID;
// TODO: Adjust for old state support!

	_packageRootRef.device = info.rootDeviceID;
	_packageRootRef.node = info.rootDirectoryID;

	return B_OK;
}


status_t
Volume::InitPackages(Listener* listener)
{
	// node-monitor the volume's packages directory
	status_t error = watch_node(&fPackagesDirectoryRef, B_WATCH_DIRECTORY,
		BMessenger(this));
	if (error == B_OK) {
		fListener = listener;
	} else {
		ERROR("Volume::InitPackages(): failed to start watching the packages "
			"directory of the volume at \"%s\": %s\n",
			fPath.String(), strerror(error));
		// Not good, but not fatal. Only the manual package operations in the
		// packages directory won't work correctly.
	}

	// read the packages directory and get the active packages
	int fd = OpenRootDirectory();
	if (fd < 0) {
		ERROR("Volume::InitPackages(): failed to open root directory: %s\n",
			strerror(fd));
		RETURN_ERROR(fd);
	}
	FileDescriptorCloser fdCloser(fd);

	error = _ReadPackagesDirectory();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = _GetActivePackages(fd);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the admin directory, if it doesn't exist yet
	BDirectory packagesDirectory;
	if (packagesDirectory.SetTo(&fPackagesDirectoryRef) == B_OK) {
		if (!BEntry(&packagesDirectory, kAdminDirectoryName).Exists())
			packagesDirectory.CreateDirectory(kAdminDirectoryName, NULL);
	}

	return B_OK;
}


status_t
Volume::AddPackagesToRepository(BSolverRepository& repository, bool activeOnly)
{
	for (PackageFileNameHashTable::Iterator it = fState->ByFileNameIterator();
			it.HasNext();) {
		Package* package = it.Next();
		if (activeOnly && !package->IsActive())
			continue;

		status_t error = repository.AddPackage(package->Info());
		if (error != B_OK) {
			ERROR("Volume::AddPackagesToRepository(): failed to add package %s "
				"to repository: %s\n", package->FileName().String(),
				strerror(error));
			return error;
		}
	}

	return B_OK;
}


void
Volume::InitialVerify(Volume* nextVolume, Volume* nextNextVolume)
{
INFORM("Volume::InitialVerify(%p, %p)\n", nextVolume, nextNextVolume);
	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK) {
		ERROR("Volume::InitialVerify(): failed to create solver: %s\n",
			strerror(error));
		return;
	}
	ObjectDeleter<BSolver> solverDeleter(solver);

	// add a repository with all active packages
	BSolverRepository repository;
	error = _AddRepository(solver, repository, true, true);
	if (error != B_OK) {
		ERROR("Volume::InitialVerify(): failed to add repository: %s\n",
			strerror(error));
		return;
	}

	// add a repository for the next volume
	BSolverRepository nextRepository;
	if (nextVolume != NULL) {
		nextRepository.SetPriority(1);
		error = nextVolume->_AddRepository(solver, nextRepository, true, false);
		if (error != B_OK) {
			ERROR("Volume::InitialVerify(): failed to add repository: %s\n",
				strerror(error));
			return;
		}
	}

	// add a repository for the next next volume
	BSolverRepository nextNextRepository;
	if (nextNextVolume != NULL) {
		nextNextRepository.SetPriority(2);
		error = nextNextVolume->_AddRepository(solver, nextNextRepository, true,
			false);
		if (error != B_OK) {
			ERROR("Volume::InitialVerify(): failed to add repository: %s\n",
				strerror(error));
			return;
		}
	}

	// verify
	error = solver->VerifyInstallation();
	if (error != B_OK) {
		ERROR("Volume::InitialVerify(): failed to verify: %s\n",
			strerror(error));
		return;
	}

	if (!solver->HasProblems()) {
		INFORM("Volume::InitialVerify(): volume at \"%s\" is consistent\n",
			Path().String());
		return;
	}

	// print the problems
// TODO: Notify the user ...
	INFORM("Volume::InitialVerify(): volume at \"%s\" has problems:\n",
		Path().String());

	int32 problemCount = solver->CountProblems();
	for (int32 i = 0; i < problemCount; i++) {
		BSolverProblem* problem = solver->ProblemAt(i);
		INFORM("  %" B_PRId32 ": %s\n", i + 1, problem->ToString().String());
		int32 solutionCount = problem->CountSolutions();
		for (int32 k = 0; k < solutionCount; k++) {
			const BSolverProblemSolution* solution = problem->SolutionAt(k);
			INFORM("    solution %" B_PRId32 ":\n", k + 1);
			int32 elementCount = solution->CountElements();
			for (int32 l = 0; l < elementCount; l++) {
				const BSolverProblemSolutionElement* element
					= solution->ElementAt(l);
				INFORM("      - %s\n", element->ToString().String());
			}
		}
	}
}


void
Volume::HandleGetLocationInfoRequest(BMessage* message)
{
	AutoLocker<State> stateLocker(fState);

	// If the cached reply message is up-to-date, just send it.
	int64 changeCount;
	if (fLocationInfoReply.FindInt64("change count", &changeCount) == B_OK
		&& changeCount == fState->ChangeCount()) {
		stateLocker.Unlock();
		message->SendReply(&fLocationInfoReply, (BHandler*)NULL,
			kCommunicationTimeout);
		return;
	}

	// rebuild the reply message
	fLocationInfoReply.MakeEmpty();

	if (fLocationInfoReply.AddInt32("base directory device",
			fRootDirectoryRef.device) != B_OK
		|| fLocationInfoReply.AddInt64("base directory node",
			fRootDirectoryRef.node) != B_OK
		|| fLocationInfoReply.AddInt32("packages directory device",
			fPackagesDirectoryRef.device) != B_OK
		|| fLocationInfoReply.AddInt64("packages directory node",
			fPackagesDirectoryRef.node) != B_OK) {
		return;
	}

	for (PackageFileNameHashTable::Iterator it = fState->ByFileNameIterator();
			it.HasNext();) {
		Package* package = it.Next();
		const char* fieldName = package->IsActive()
			? "active packages" : "inactive packages";
		BMessage packageArchive;
		if (package->Info().Archive(&packageArchive) != B_OK
			|| fLocationInfoReply.AddMessage(fieldName, &packageArchive)
				!= B_OK) {
			return;
		}
	}

	if (fLocationInfoReply.AddInt64("change count", fState->ChangeCount())
			!= B_OK) {
		return;
	}

	stateLocker.Unlock();

	message->SendReply(&fLocationInfoReply, (BHandler*)NULL,
		kCommunicationTimeout);
}


void
Volume::HandleCommitTransactionRequest(BMessage* message)
{
	// Prepare the reply in so far that we can at least set the error code
	// without risk of failure.
	BMessage reply(B_MESSAGE_COMMIT_TRANSACTION_REPLY);
	if (reply.AddInt32("error", B_ERROR) != B_OK)
		return;

	// perform the request
	PackageSet dummy;
	CommitTransactionHandler handler(this, dummy, dummy);
	int32 error;
	try {
		handler.HandleRequest(message, &reply);
		error = B_DAEMON_OK;
	} catch (Exception& exception) {
		error = exception.Error();

		if (!exception.ErrorMessage().IsEmpty())
			reply.AddString("error message", exception.ErrorMessage());
		if (!exception.PackageName().IsEmpty())
			reply.AddString("error package", exception.PackageName());
	} catch (std::bad_alloc& exception) {
		error = B_NO_MEMORY;
	}

	// revert on error
	if (error != B_DAEMON_OK)
		handler.Revert();

	// send the reply
	reply.ReplaceInt32("error", error);
	message->SendReply(&reply, (BHandler*)NULL, kCommunicationTimeout);
}


void
Volume::PackageJobPending()
{
	fState->PackageJobPending();
}


void
Volume::PackageJobFinished()
{
	fState->PackageJobFinished();
}


bool
Volume::IsPackageJobPending() const
{
	return fState->IsPackageJobPending();
}


void
Volume::Unmounted()
{
	if (fListener != NULL) {
		stop_watching(BMessenger(this));
		fListener = NULL;
	}

	if (BLooper* looper = Looper())
		looper->RemoveHandler(this);
}


void
Volume::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				break;

			switch (opcode) {
				case B_ENTRY_CREATED:
					_HandleEntryCreatedOrRemoved(message, true);
					break;
				case B_ENTRY_REMOVED:
					_HandleEntryCreatedOrRemoved(message, false);
					break;
				case B_ENTRY_MOVED:
					_HandleEntryMoved(message);
					break;
				default:
					break;
			}
			break;
		}

		case kHandleNodeMonitorEvents:
			if (fListener != NULL) {
				if (system_time() >= fNodeMonitorEventHandleTime)
					fListener->VolumeNodeMonitorEventOccurred(this);
			}
			break;

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


BPackageInstallationLocation
Volume::Location() const
{
	switch (fMountType) {
		case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
			return B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;
		case PACKAGE_FS_MOUNT_TYPE_HOME:
			return B_PACKAGE_INSTALLATION_LOCATION_HOME;
		case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
		default:
			return B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT;
	}
}


PackageFileNameHashTable::Iterator
Volume::PackagesByFileNameIterator() const
{
	return fState->ByFileNameIterator();
}


int
Volume::OpenRootDirectory() const
{
	BDirectory directory;
	status_t error = directory.SetTo(&fRootDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::OpenRootDirectory(): failed to open root directory: "
			"%s\n", strerror(error));
		RETURN_ERROR(error);
	}

	return directory.Dup();
}


void
Volume::ProcessPendingNodeMonitorEvents()
{
	// get the events
	NodeMonitorEventList events;
	{
		AutoLocker<BLocker> eventsLock(fPendingNodeMonitorEventsLock);
		events.MoveFrom(&fPendingNodeMonitorEvents);
	}

	// process them
	while (NodeMonitorEvent* event = events.RemoveHead()) {
		ObjectDeleter<NodeMonitorEvent> eventDeleter(event);
		if (event->WasCreated())
			_PackagesEntryCreated(event->EntryName());
		else
			_PackagesEntryRemoved(event->EntryName());
	}
}


bool
Volume::HasPendingPackageActivationChanges() const
{
	return !fPackagesToBeActivated.empty() || !fPackagesToBeDeactivated.empty();
}


void
Volume::ProcessPendingPackageActivationChanges()
{
	if (!HasPendingPackageActivationChanges())
		return;

	// perform the request
	CommitTransactionHandler handler(this, fPackagesToBeActivated,
		fPackagesToBeDeactivated);
	int32 error;
	try {
		handler.HandleRequest(fPackagesToBeActivated, fPackagesToBeDeactivated);
		error = B_DAEMON_OK;
	} catch (Exception& exception) {
		error = exception.Error();
		ERROR("Volume::ProcessPendingPackageActivationChanges(): package "
			"activation failed: %s\n", exception.ToString().String());
// TODO: Notify the user!
	} catch (std::bad_alloc& exception) {
		error = B_NO_MEMORY;
		ERROR("Volume::ProcessPendingPackageActivationChanges(): package "
			"activation failed: out of memory\n");
// TODO: Notify the user!
	}

	// revert on error
	if (error != B_DAEMON_OK)
		handler.Revert();

	// clear the activation/deactivation sets in any event
	fPackagesToBeActivated.clear();
	fPackagesToBeDeactivated.clear();
}


void
Volume::ClearPackageActivationChanges()
{
	fPackagesToBeActivated.clear();
	fPackagesToBeDeactivated.clear();
}


status_t
Volume::CreateTransaction(BPackageInstallationLocation location,
	BActivationTransaction& _transaction, BDirectory& _transactionDirectory)
{
	// open admin directory
	BDirectory adminDirectory;
	status_t error = _OpenPackagesSubDirectory(
		RelativePath(kAdminDirectoryName), true, adminDirectory);
	if (error != B_OK)
		return error;

	// create a transaction directory
	int uniqueId = 1;
	BString directoryName;
	for (;; uniqueId++) {
		directoryName.SetToFormat("transaction-%d", uniqueId);
		if (directoryName.IsEmpty())
			return B_NO_MEMORY;

		error = adminDirectory.CreateDirectory(directoryName,
			&_transactionDirectory);
		if (error == B_OK)
			break;
		if (error != B_FILE_EXISTS)
			return error;
	}

	// init the transaction
	error = _transaction.SetTo(location, fState->ChangeCount(), directoryName);
	if (error != B_OK) {
		BEntry entry;
		_transactionDirectory.GetEntry(&entry);
		_transactionDirectory.Unset();
		if (entry.InitCheck() == B_OK)
			entry.Remove();
		return error;
	}

	return B_OK;
}


void
Volume::CommitTransaction(const BActivationTransaction& transaction,
	const PackageSet& packagesAlreadyAdded,
	const PackageSet& packagesAlreadyRemoved,
	BDaemonClient::BCommitTransactionResult& _result)
{
	// perform the request
	CommitTransactionHandler handler(this, packagesAlreadyAdded,
		packagesAlreadyRemoved);
	int32 error;
	try {
		handler.HandleRequest(transaction, NULL);
		error = B_DAEMON_OK;
		_result.SetTo(error, BString(), BString(),
			handler.OldStateDirectoryName());
	} catch (Exception& exception) {
		error = exception.Error();
		_result.SetTo(error, exception.ErrorMessage(), exception.PackageName(),
			BString());
	} catch (std::bad_alloc& exception) {
		error = B_NO_MEMORY;
		_result.SetTo(error, BString(), BString(), BString());
	}

	// revert on error
	if (error != B_DAEMON_OK)
		handler.Revert();
}


void
Volume::_HandleEntryCreatedOrRemoved(const BMessage* message, bool created)
{
	// only moves to or from our packages directory are interesting
	int32 deviceID;
	int64 directoryID;
	const char* name;
	if (message->FindInt32("device", &deviceID) != B_OK
		|| message->FindInt64("directory", &directoryID) != B_OK
		|| message->FindString("name", &name) != B_OK
		|| node_ref(deviceID, directoryID) != fPackagesDirectoryRef) {
		return;
	}

	_QueueNodeMonitorEvent(name, created);
}


void
Volume::_HandleEntryMoved(const BMessage* message)
{
	int32 deviceID;
	int64 fromDirectoryID;
	int64 toDirectoryID;
	const char* fromName;
	const char* toName;
	if (message->FindInt32("device", &deviceID) != B_OK
		|| message->FindInt64("from directory", &fromDirectoryID) != B_OK
		|| message->FindInt64("to directory", &toDirectoryID) != B_OK
		|| message->FindString("from name", &fromName) != B_OK
		|| message->FindString("name", &toName) != B_OK
		|| deviceID != fPackagesDirectoryRef.device
		|| (fromDirectoryID != fPackagesDirectoryRef.node
			&& toDirectoryID != fPackagesDirectoryRef.node)) {
		return;
	}

	AutoLocker<BLocker> eventsLock(fPendingNodeMonitorEventsLock);
		// make sure for a move the two events cannot get split

	if (fromDirectoryID == fPackagesDirectoryRef.node)
		_QueueNodeMonitorEvent(fromName, false);
	if (toDirectoryID == fPackagesDirectoryRef.node)
		_QueueNodeMonitorEvent(toName, true);
}


void
Volume::_QueueNodeMonitorEvent(const BString& name, bool wasCreated)
{
	if (name.IsEmpty()) {
		ERROR("Volume::_QueueNodeMonitorEvent(): got empty name.\n");
		return;
	}

	// ignore entries that don't have the ".hpkg" extension
	if (!name.EndsWith(kPackageFileNameExtension))
		return;

	NodeMonitorEvent* event
		= new(std::nothrow) NodeMonitorEvent(name, wasCreated);
	if (event == NULL) {
		ERROR("Volume::_QueueNodeMonitorEvent(): out of memory.\n");
		return;
	}

	AutoLocker<BLocker> eventsLock(fPendingNodeMonitorEventsLock);
	fPendingNodeMonitorEvents.Add(event);
	eventsLock.Unlock();

	fNodeMonitorEventHandleTime
		= system_time() + kNodeMonitorEventHandlingDelay;
	BMessage message(kHandleNodeMonitorEvents);
	BMessageRunner::StartSending(this, &message, kNodeMonitorEventHandlingDelay,
		1);
}


void
Volume::_PackagesEntryCreated(const char* name)
{
INFORM("Volume::_PackagesEntryCreated(\"%s\")\n", name);
	// Ignore the event, if the package is already known.
	Package* package = fState->FindPackage(name);
	if (package != NULL) {
		if (package->EntryCreatedIgnoreLevel() > 0) {
			package->DecrementEntryCreatedIgnoreLevel();
		} else {
			WARN("node monitoring created event for already known entry "
				"\"%s\"\n", name);
		}

		return;
	}

	entry_ref entry;
	entry.device = fPackagesDirectoryRef.device;
	entry.directory = fPackagesDirectoryRef.node;
	status_t error = entry.set_name(name);
	if (error != B_OK) {
		ERROR("out of memory\n");
		return;
	}

	package = new(std::nothrow) Package;
	if (package == NULL) {
		ERROR("out of memory\n");
		return;
	}
	ObjectDeleter<Package> packageDeleter(package);

	error = package->Init(entry);
	if (error != B_OK) {
		ERROR("failed to init package for file \"%s\"\n", name);
		return;
	}

	_AddPackage(package);
	packageDeleter.Detach();

	try {
		fPackagesToBeActivated.insert(package);
	} catch (std::bad_alloc& exception) {
		ERROR("out of memory\n");
		return;
	}
}


void
Volume::_PackagesEntryRemoved(const char* name)
{
INFORM("Volume::_PackagesEntryRemoved(\"%s\")\n", name);
	Package* package = fState->FindPackage(name);
	if (package == NULL)
		return;

	// Ignore the event, if we generated it ourselves.
	if (package->EntryRemovedIgnoreLevel() > 0) {
		package->DecrementEntryRemovedIgnoreLevel();
		return;
	}

	// Remove the package from the packages-to-be-activated set, if it is in
	// there (unlikely, unless we see a create-remove-create sequence).
	PackageSet::iterator it = fPackagesToBeActivated.find(package);
	if (it != fPackagesToBeActivated.end())
		fPackagesToBeActivated.erase(it);

	// If the package isn't active, just remove it for good.
	if (!package->IsActive()) {
		_RemovePackage(package);
		delete package;
		return;
	}

	// The package must be deactivated.
	try {
		fPackagesToBeDeactivated.insert(package);
	} catch (std::bad_alloc& exception) {
		ERROR("out of memory\n");
		return;
	}
}


void
Volume::_FillInActivationChangeItem(PackageFSActivationChangeItem* item,
	PackageFSActivationChangeType type, Package* package, char*& nameBuffer)
{
	item->type = type;
	item->packageDeviceID = package->NodeRef().device;
	item->packageNodeID = package->NodeRef().node;
	item->nameLength = package->FileName().Length();
	item->parentDeviceID = fPackagesDirectoryRef.device;
	item->parentDirectoryID = fPackagesDirectoryRef.node;
	item->name = nameBuffer;
	strcpy(nameBuffer, package->FileName());
	nameBuffer += package->FileName().Length() + 1;
}


void
Volume::_AddPackage(Package* package)
{
	fState->AddPackage(package);
}

void
Volume::_RemovePackage(Package* package)
{
	fState->RemovePackage(package);
}


status_t
Volume::_ReadPackagesDirectory()
{
	BDirectory directory;
	status_t error = directory.SetTo(&fPackagesDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::_ReadPackagesDirectory(): failed to open packages "
			"directory: %s\n", strerror(error));
		RETURN_ERROR(error);
	}

	entry_ref entry;
	while (directory.GetNextRef(&entry) == B_OK) {
		if (!BString(entry.name).EndsWith(kPackageFileNameExtension))
			continue;

		Package* package = new(std::nothrow) Package;
		if (package == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		ObjectDeleter<Package> packageDeleter(package);

		status_t error = package->Init(entry);
		if (error == B_OK) {
			_AddPackage(package);
			packageDeleter.Detach();
		}
	}

	return B_OK;
}


status_t
Volume::_GetActivePackages(int fd)
{
// TODO: Adjust for old state support!
	uint32 maxPackageCount = 16 * 1024;
	PackageFSGetPackageInfosRequest* request = NULL;
	MemoryDeleter requestDeleter;
	size_t bufferSize;
	for (;;) {
		bufferSize = sizeof(PackageFSGetPackageInfosRequest)
			+ (maxPackageCount - 1) * sizeof(PackageFSPackageInfo);
		request = (PackageFSGetPackageInfosRequest*)malloc(bufferSize);
		if (request == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		requestDeleter.SetTo(request);

		if (ioctl(fd, PACKAGE_FS_OPERATION_GET_PACKAGE_INFOS, request,
				bufferSize) != 0) {
			ERROR("Volume::_GetActivePackages(): failed to get active package "
				"info from package FS: %s\n", strerror(errno));
			RETURN_ERROR(errno);
		}

		if (request->packageCount <= maxPackageCount)
			break;

		maxPackageCount = request->packageCount;
		requestDeleter.Unset();
	}

	// mark the returned packages active
	for (uint32 i = 0; i < request->packageCount; i++) {
		Package* package = fState->FindPackage(
			node_ref(request->infos[i].packageDeviceID,
				request->infos[i].packageNodeID));
		if (package == NULL) {
			WARN("active package (dev: %" B_PRIdDEV ", node: %" B_PRIdINO ") "
				"not found in package directory\n",
				request->infos[i].packageDeviceID,
				request->infos[i].packageNodeID);
// TODO: Deactivate the package right away?
			continue;
		}

		fState->SetPackageActive(package, true);
INFORM("active package: \"%s\"\n", package->FileName().String());
	}

for (PackageNodeRefHashTable::Iterator it = fState->ByNodeRefIterator();
	it.HasNext();) {
	Package* package = it.Next();
	if (!package->IsActive())
		INFORM("inactive package: \"%s\"\n", package->FileName().String());
}

// INFORM("%" B_PRIu32 " active packages:\n", request->packageCount);
// for (uint32 i = 0; i < request->packageCount; i++) {
// 	INFORM("  dev: %" B_PRIdDEV ", node: %" B_PRIdINO "\n",
// 		request->infos[i].packageDeviceID, request->infos[i].packageNodeID);
// }

	return B_OK;
}


status_t
Volume::_AddRepository(BSolver* solver, BSolverRepository& repository,
	bool activeOnly, bool installed)
{
	status_t error = repository.SetTo(Path());
	if (error != B_OK) {
		ERROR("Volume::_AddRepository(): failed to init repository: %s\n",
			strerror(error));
		return error;
	}

	repository.SetInstalled(installed);

	error = AddPackagesToRepository(repository, true);
	if (error != B_OK) {
		ERROR("Volume::_AddRepository(): failed to add packages to "
			"repository: %s\n", strerror(error));
		return error;
	}

	error = solver->AddRepository(&repository);
	if (error != B_OK) {
		ERROR("Volume::_AddRepository(): failed to add repository to solver: "
			"%s\n", strerror(error));
		return error;
	}

	return B_OK;
}


status_t
Volume::_OpenPackagesFile(const RelativePath& subDirectoryPath,
	const char* fileName, uint32 openMode, BFile& _file, BEntry* _entry)
{
	BDirectory directory;
	if (!subDirectoryPath.IsEmpty()) {
		status_t error = _OpenPackagesSubDirectory(subDirectoryPath,
			(openMode & B_CREATE_FILE) != 0, directory);
		if (error != B_OK) {
			ERROR("Volume::_OpenPackagesFile(): failed to open packages "
				"subdirectory \"%s\": %s\n",
				subDirectoryPath.ToString().String(), strerror(error));
			RETURN_ERROR(error);
		}
	} else {
		status_t error = directory.SetTo(&fPackagesDirectoryRef);
		if (error != B_OK) {
			ERROR("Volume::_OpenPackagesFile(): failed to open packages "
				"directory: %s\n", strerror(error));
			RETURN_ERROR(error);
		}
	}

	BEntry stackEntry;
	BEntry& entry = _entry != NULL ? *_entry : stackEntry;
	status_t error = entry.SetTo(&directory, fileName);
	if (error != B_OK) {
		ERROR("Volume::_OpenPackagesFile(): failed to get entry for file: %s",
			strerror(error));
		RETURN_ERROR(error);
	}

	return _file.SetTo(&entry, openMode);
}


status_t
Volume::_OpenPackagesSubDirectory(const RelativePath& path, bool create,
	BDirectory& _directory)
{
	// open the packages directory
	BDirectory directory;
	status_t error = directory.SetTo(&fPackagesDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::_OpenPackagesSubDirectory(): failed to open packages "
			"directory: %s\n", strerror(error));
		RETURN_ERROR(error);
	}

	return FSUtils::OpenSubDirectory(directory, path, create, _directory);
}


status_t
Volume::_CreateActivationFileContent(const PackageSet& toActivate,
	const PackageSet& toDeactivate, BString& _content)
{
	BString activationFileContent;
	for (PackageFileNameHashTable::Iterator it = fState->ByFileNameIterator();
			it.HasNext();) {
		Package* package = it.Next();
		if (package->IsActive()
			&& toDeactivate.find(package) == toDeactivate.end()) {
			int32 length = activationFileContent.Length();
			activationFileContent << package->FileName() << '\n';
			if (activationFileContent.Length()
					< length + package->FileName().Length() + 1) {
				return B_NO_MEMORY;
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
			return B_NO_MEMORY;
		}
	}

	_content = activationFileContent;
	return B_OK;
}


status_t
Volume::_WriteActivationFile(const RelativePath& directoryPath,
	const char* fileName, const PackageSet& toActivate,
	const PackageSet& toDeactivate,
	BEntry& _entry)
{
	// create the content
	BString activationFileContent;
	status_t error = _CreateActivationFileContent(toActivate, toDeactivate,
		activationFileContent);
	if (error != B_OK)
		return error;

	// write the file
	error = _WriteTextFile(directoryPath, fileName, activationFileContent,
		_entry);
	if (error != B_OK) {
		ERROR("Volume::_WriteActivationFile(): failed to write activation "
			"file \"%s/%s\": %s\n", directoryPath.ToString().String(), fileName,
			strerror(error));
		return error;
	}

	return B_OK;
}


status_t
Volume::_WriteTextFile(const RelativePath& directoryPath, const char* fileName,
	const BString& content, BEntry& _entry)
{
	BFile file;
	status_t error = _OpenPackagesFile(directoryPath,
		fileName, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE, file, &_entry);
	if (error != B_OK) {
		ERROR("Volume::_WriteTextFile(): failed to create file \"%s/%s\": %s\n",
			directoryPath.ToString().String(), fileName, strerror(error));
		return error;
	}

	ssize_t bytesWritten = file.Write(content.String(),
		content.Length());
	if (bytesWritten < 0) {
		ERROR("Volume::_WriteTextFile(): failed to write file \"%s/%s\": %s\n",
			directoryPath.ToString().String(), fileName,
			strerror(bytesWritten));
		return bytesWritten;
	}

	return B_OK;
}


void
Volume::_ChangePackageActivation(const PackageSet& packagesToActivate,
	const PackageSet& packagesToDeactivate)
{
INFORM("Volume::_ChangePackageActivation(): activating %zu, deactivating %zu packages\n",
packagesToActivate.size(), packagesToDeactivate.size());

	// write the temporary package activation file
	BEntry activationFileEntry;
	status_t error = _WriteActivationFile(RelativePath(kAdminDirectoryName),
		kTemporaryActivationFileName, packagesToActivate, packagesToDeactivate,
		activationFileEntry);
	if (error != B_OK)
		throw Exception(error, "failed to write activation file");

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
		throw Exception(B_NO_MEMORY);
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
	int fd = OpenRootDirectory();
	if (fd < 0)
		throw Exception(fd, "failed to open root directory");
	FileDescriptorCloser fdCloser(fd);

	if (ioctl(fd, PACKAGE_FS_OPERATION_CHANGE_ACTIVATION, request, requestSize)
			!= 0) {
// TODO: We need more error information and error handling!
		throw Exception(errno, "ioctl() to de-/activate packages failed");
	}

	// rename the temporary activation file to the final file
	error = activationFileEntry.Rename(kActivationFileName, true);
	if (error != B_OK) {
		throw Exception(error,
			"failed to rename temporary activation file to final file");
// TODO: We should probably try to reverse the activation changes, though that
// will fail, if this method has been called in response to node monitoring
// events. Alternatively moving the package activation file could be made part
// of the ioctl(), since packagefs should be able to undo package changes until
// the very end, unless running out of memory. In the end the situation would be
// bad anyway, though, since the activation file may refer to removed packages
// and things would be in an inconsistent state after rebooting.
	}

	// Update our state, i.e. remove deactivated packages and mark activated
	// packages accordingly.
	fState->ActivationChanged(packagesToActivate, packagesToDeactivate);
}
