/*
 * Copyright 2013-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 */


#include "Volume.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <SymLink.h>

#include <package/CommitTransactionResult.h>
#include <package/PackageRoster.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverRepository.h>
#include <package/solver/SolverResult.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <NotOwningEntryRef.h>
#include <package/DaemonDefs.h>
#include <RosterPrivate.h>

#include "CommitTransactionHandler.h"
#include "Constants.h"
#include "DebugSupport.h"
#include "Exception.h"
#include "PackageFileManager.h"
#include "Root.h"
#include "VolumeState.h"


using namespace BPackageKit::BPrivate;


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


// #pragma mark - PackagesDirectory


struct Volume::PackagesDirectory {
public:
	PackagesDirectory()
		:
		fNodeRef(),
		fName()
	{
	}

	void Init(const node_ref& nodeRef, bool isPackagesDir)
	{
		fNodeRef = nodeRef;

		if (isPackagesDir)
			return;

		BDirectory directory;
		BEntry entry;
		if (directory.SetTo(&fNodeRef) == B_OK
			&& directory.GetEntry(&entry) == B_OK) {
			fName = entry.Name();
		}

		if (fName.IsEmpty())
			fName = "unknown state";
	}

	const node_ref& NodeRef() const
	{
		return fNodeRef;
	}

	const BString& Name() const
	{
		return fName;
	}

private:
	node_ref	fNodeRef;
	BString		fName;
};


// #pragma mark - Volume


Volume::Volume(BLooper* looper)
	:
	BHandler(),
	fPath(),
	fMountType(PACKAGE_FS_MOUNT_TYPE_CUSTOM),
	fRootDirectoryRef(),
	fPackagesDirectories(NULL),
	fPackagesDirectoryCount(0),
	fRoot(NULL),
	fListener(NULL),
	fPackageFileManager(NULL),
	fLatestState(NULL),
	fActiveState(NULL),
	fChangeCount(0),
	fLock("volume"),
	fPendingNodeMonitorEventsLock("pending node monitor events"),
	fPendingNodeMonitorEvents(),
	fNodeMonitorEventHandleTime(0),
	fPackagesToBeActivated(),
	fPackagesToBeDeactivated(),
	fLocationInfoReply(B_MESSAGE_GET_INSTALLATION_LOCATION_INFO_REPLY),
	fPendingPackageJobCount(0)
{
	looper->AddHandler(this);
}


Volume::~Volume()
{
	Unmounted();
		// needed for error case in InitPackages()

	_SetLatestState(NULL, true);

	delete[] fPackagesDirectories;
	delete fPackageFileManager;
}


status_t
Volume::Init(const node_ref& rootDirectoryRef, node_ref& _packageRootRef)
{
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	error = fPendingNodeMonitorEventsLock.InitCheck();
	if (error != B_OK)
		return error;

	fLatestState = new(std::nothrow) VolumeState;
	if (fLatestState == NULL || !fLatestState->Init())
		RETURN_ERROR(B_NO_MEMORY);

	fPackageFileManager = new(std::nothrow) PackageFileManager(fLock);
	if (fPackageFileManager == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	error = fPackageFileManager->Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	fRootDirectoryRef = rootDirectoryRef;

	// open the root directory
	BDirectory directory;
	error = directory.SetTo(&fRootDirectoryRef);
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

	// get the volume info from packagefs
	uint32 maxPackagesDirCount = 16;
	PackageFSVolumeInfo* info = NULL;
	MemoryDeleter infoDeleter;
	size_t bufferSize;
	for (;;) {
		bufferSize = sizeof(PackageFSVolumeInfo)
			+ (maxPackagesDirCount - 1) * sizeof(PackageFSDirectoryInfo);
		info = (PackageFSVolumeInfo*)malloc(bufferSize);
		if (info == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		infoDeleter.SetTo(info);

		if (ioctl(fd, PACKAGE_FS_OPERATION_GET_VOLUME_INFO, info,
				bufferSize) != 0) {
			ERROR("Volume::Init(): failed to get volume info: %s\n",
				strerror(errno));
			RETURN_ERROR(errno);
		}

		if (info->packagesDirectoryCount <= maxPackagesDirCount)
			break;

		maxPackagesDirCount = info->packagesDirectoryCount;
		infoDeleter.Unset();
	}

	if (info->packagesDirectoryCount < 1) {
		ERROR("Volume::Init(): got invalid volume info from packagefs\n");
		RETURN_ERROR(B_BAD_VALUE);
	}

	fMountType = info->mountType;

	fPackagesDirectories = new(std::nothrow) PackagesDirectory[
		info->packagesDirectoryCount];
	if (fPackagesDirectories == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	fPackagesDirectoryCount = info->packagesDirectoryCount;

	for (uint32 i = 0; i < info->packagesDirectoryCount; i++) {
		fPackagesDirectories[i].Init(
			node_ref(info->packagesDirectoryInfos[i].deviceID,
				info->packagesDirectoryInfos[i].nodeID),
			i == 0);
	}

	_packageRootRef.device = info->rootDeviceID;
	_packageRootRef.node = info->rootDirectoryID;

	return B_OK;
}


status_t
Volume::InitPackages(Listener* listener)
{
	// node-monitor the volume's packages directory
	status_t error = watch_node(&PackagesDirectoryRef(), B_WATCH_DIRECTORY,
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

	error = _InitLatestState();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = _GetActivePackages(fd);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the admin directory, if it doesn't exist yet
	BDirectory packagesDirectory;
	bool createdAdminDirectory = false;
	if (packagesDirectory.SetTo(&PackagesDirectoryRef()) == B_OK) {
		if (!BEntry(&packagesDirectory, kAdminDirectoryName).Exists()) {
			packagesDirectory.CreateDirectory(kAdminDirectoryName, NULL);
			createdAdminDirectory = true;
		}
	}
	BDirectory adminDirectory(&packagesDirectory, kAdminDirectoryName);
	error = adminDirectory.InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// First boot processing requested by a magic file left by the OS installer?
	BEntry flagFileEntry(&adminDirectory, kFirstBootProcessingNeededFileName);
	if (createdAdminDirectory || flagFileEntry.Exists()) {
		INFORM("Volume::InitPackages Requesting delayed first boot processing "
			"for packages dir %s.\n", BPath(&packagesDirectory).Path());
		if (flagFileEntry.Exists())
			flagFileEntry.Remove(); // Remove early on to avoid an error loop.

		// Are there any packages needing processing?  Don't want to create an
		// empty transaction directory and then never have it cleaned up when
		// the empty transaction gets rejected.
		bool anyPackages = false;
		for (PackageNodeRefHashTable::Iterator it =
				fActiveState->ByNodeRefIterator(); it.HasNext();) {
			Package* package = it.Next();
			if (package->IsActive()) {
				anyPackages = true;
				break;
			}
		}

		if (anyPackages) {
			// Create first boot processing special transaction for current
			// volume, which also creates an empty transaction directory.
			BPackageInstallationLocation location = Location();
			BDirectory transactionDirectory;
			BActivationTransaction transaction;
			error = CreateTransaction(location, transaction,
				transactionDirectory);
			if (error != B_OK)
				RETURN_ERROR(error);

			// Add all package files in currently active state to transaction.
			for (PackageNodeRefHashTable::Iterator it =
					fActiveState->ByNodeRefIterator(); it.HasNext();) {
				Package* package = it.Next();
				if (package->IsActive()) {
					if (!transaction.AddPackageToActivate(
							package->FileName().String()))
						RETURN_ERROR(B_NO_MEMORY);
				}
			}
			transaction.SetFirstBootProcessing(true);

			// Queue up the transaction as a BMessage for processing a bit
			// later, once the package daemon has finished initialising.
			BMessage commitMessage(B_MESSAGE_COMMIT_TRANSACTION);
			error = transaction.Archive(&commitMessage);
			if (error != B_OK)
				RETURN_ERROR(error);
			BLooper *myLooper = Looper() ;
			if (myLooper == NULL)
				RETURN_ERROR(B_NOT_INITIALIZED);
			error = myLooper->PostMessage(&commitMessage);
			if (error != B_OK)
				RETURN_ERROR(error);
		}
	}

	return B_OK;
}


status_t
Volume::AddPackagesToRepository(BSolverRepository& repository, bool activeOnly)
{
	for (PackageFileNameHashTable::Iterator it
			= fLatestState->ByFileNameIterator(); it.HasNext();) {
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
	AutoLocker<BLocker> locker(fLock);

	// If the cached reply message is up-to-date, just send it.
	int64 changeCount;
	if (fLocationInfoReply.FindInt64("change count", &changeCount) == B_OK
		&& changeCount == fChangeCount) {
		locker.Unlock();
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
			PackagesDeviceID()) != B_OK
		|| fLocationInfoReply.AddInt64("packages directory node",
			PackagesDirectoryID()) != B_OK) {
		return;
	}

	for (PackageFileNameHashTable::Iterator it
			= fLatestState->ByFileNameIterator(); it.HasNext();) {
		Package* package = it.Next();
		const char* fieldName = package->IsActive()
			? "latest active packages" : "latest inactive packages";
		BMessage packageArchive;
		if (package->Info().Archive(&packageArchive) != B_OK
			|| fLocationInfoReply.AddMessage(fieldName, &packageArchive)
				!= B_OK) {
			return;
		}
	}

	if (fActiveState != fLatestState) {
		if (fPackagesDirectoryCount > 1) {
			fLocationInfoReply.AddString("old state",
				fPackagesDirectories[fPackagesDirectoryCount - 1].Name());
		}

		for (PackageFileNameHashTable::Iterator it
				= fActiveState->ByFileNameIterator(); it.HasNext();) {
			Package* package = it.Next();
			if (!package->IsActive())
				continue;

			BMessage packageArchive;
			if (package->Info().Archive(&packageArchive) != B_OK
				|| fLocationInfoReply.AddMessage("currently active packages",
					&packageArchive) != B_OK) {
				return;
			}
		}
	}

	if (fLocationInfoReply.AddInt64("change count", fChangeCount) != B_OK)
		return;

	locker.Unlock();

	message->SendReply(&fLocationInfoReply, (BHandler*)NULL,
		kCommunicationTimeout);
}


void
Volume::HandleCommitTransactionRequest(BMessage* message)
{
	BCommitTransactionResult result;
	PackageSet dummy;
	_CommitTransaction(message, NULL, dummy, dummy, result);

	BMessage reply(B_MESSAGE_COMMIT_TRANSACTION_REPLY);
	status_t error = result.AddToMessage(reply);
	if (error != B_OK) {
		ERROR("Volume::HandleCommitTransactionRequest(): Failed to add "
			"transaction result to reply: %s\n", strerror(error));
		return;
	}

	message->SendReply(&reply, (BHandler*)NULL, kCommunicationTimeout);
}


void
Volume::PackageJobPending()
{
	atomic_add(&fPendingPackageJobCount, 1);
}


void
Volume::PackageJobFinished()
{
	atomic_add(&fPendingPackageJobCount, -1);
}


bool
Volume::IsPackageJobPending() const
{
	return fPendingPackageJobCount != 0;
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


const node_ref&
Volume::PackagesDirectoryRef() const
{
	return fPackagesDirectories[0].NodeRef();
}


PackageFileNameHashTable::Iterator
Volume::PackagesByFileNameIterator() const
{
	return fLatestState->ByFileNameIterator();
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
	BCommitTransactionResult result;
	_CommitTransaction(NULL, NULL, fPackagesToBeActivated,
		fPackagesToBeDeactivated, result);

	if (result.Error() != B_TRANSACTION_OK) {
		ERROR("Volume::ProcessPendingPackageActivationChanges(): package "
			"activation failed: %s\n", result.FullErrorMessage().String());
// TODO: Notify the user!
	}

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
	error = _transaction.SetTo(location, fChangeCount, directoryName);
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
	const PackageSet& packagesAlreadyRemoved, BCommitTransactionResult& _result)
{
	_CommitTransaction(NULL, &transaction, packagesAlreadyAdded,
		packagesAlreadyRemoved, _result);
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
		|| node_ref(deviceID, directoryID) != PackagesDirectoryRef()) {
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
		|| deviceID != PackagesDeviceID()
		|| (fromDirectoryID != PackagesDirectoryID()
			&& toDirectoryID != PackagesDirectoryID())) {
		return;
	}

	AutoLocker<BLocker> eventsLock(fPendingNodeMonitorEventsLock);
		// make sure for a move the two events cannot get split

	if (fromDirectoryID == PackagesDirectoryID())
		_QueueNodeMonitorEvent(fromName, false);
	if (toDirectoryID == PackagesDirectoryID())
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
	Package* package = fLatestState->FindPackage(name);
	if (package != NULL) {
		if (package->File()->EntryCreatedIgnoreLevel() > 0) {
			package->File()->DecrementEntryCreatedIgnoreLevel();
		} else {
			WARN("node monitoring created event for already known entry "
				"\"%s\"\n", name);
		}

		// Remove the package from the packages-to-be-deactivated set, if it is in
		// there (unlikely, unless we see a remove-create sequence).
		PackageSet::iterator it = fPackagesToBeDeactivated.find(package);
		if (it != fPackagesToBeDeactivated.end())
			fPackagesToBeDeactivated.erase(it);

		return;
	}

	status_t error = fPackageFileManager->CreatePackage(
		NotOwningEntryRef(PackagesDirectoryRef(), name),
		package);
	if (error != B_OK) {
		ERROR("failed to init package for file \"%s\"\n", name);
		return;
	}

	fLock.Lock();
	fLatestState->AddPackage(package);
	fChangeCount++;
	fLock.Unlock();

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
	Package* package = fLatestState->FindPackage(name);
	if (package == NULL)
		return;

	// Ignore the event, if we generated it ourselves.
	if (package->File()->EntryRemovedIgnoreLevel() > 0) {
		package->File()->DecrementEntryRemovedIgnoreLevel();
		return;
	}

	// Remove the package from the packages-to-be-activated set, if it is in
	// there (unlikely, unless we see a create-remove-create sequence).
	PackageSet::iterator it = fPackagesToBeActivated.find(package);
	if (it != fPackagesToBeActivated.end())
		fPackagesToBeActivated.erase(it);

	// If the package isn't active, just remove it for good.
	if (!package->IsActive()) {
		AutoLocker<BLocker> locker(fLock);
		fLatestState->RemovePackage(package);
		fChangeCount++;
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


status_t
Volume::_ReadPackagesDirectory()
{
	BDirectory directory;
	status_t error = directory.SetTo(&PackagesDirectoryRef());
	if (error != B_OK) {
		ERROR("Volume::_ReadPackagesDirectory(): failed to open packages "
			"directory: %s\n", strerror(error));
		RETURN_ERROR(error);
	}

	entry_ref entry;
	while (directory.GetNextRef(&entry) == B_OK) {
		if (!BString(entry.name).EndsWith(kPackageFileNameExtension))
			continue;

		Package* package;
		status_t error = fPackageFileManager->CreatePackage(entry, package);
		if (error == B_OK) {
			AutoLocker<BLocker> locker(fLock);
			fLatestState->AddPackage(package);
			fChangeCount++;
		}
	}

	return B_OK;
}


status_t
Volume::_InitLatestState()
{
	if (_InitLatestStateFromActivatedPackages() == B_OK)
		return B_OK;

	INFORM("Failed to get activated packages info from activated packages file."
		" Assuming all package files in package directory are activated.\n");

	AutoLocker<BLocker> locker(fLock);

	for (PackageFileNameHashTable::Iterator it
				= fLatestState->ByFileNameIterator();
			Package* package = it.Next();) {
		fLatestState->SetPackageActive(package, true);
		fChangeCount++;
	}

	return B_OK;
}


status_t
Volume::_InitLatestStateFromActivatedPackages()
{
	// open admin directory
	BDirectory adminDirectory;
	status_t error = _OpenPackagesSubDirectory(
		RelativePath(kAdminDirectoryName), false, adminDirectory);
	if (error != B_OK)
		RETURN_ERROR(error);

	node_ref adminNode;
	error = adminDirectory.GetNodeRef(&adminNode);
	if (error != B_OK)
		RETURN_ERROR(error);

	// try reading the activation file
	NotOwningEntryRef entryRef(adminNode, kActivationFileName);
	BFile file;
	error = file.SetTo(&entryRef, B_READ_ONLY);
	if (error != B_OK) {
		BEntry activationEntry(&entryRef);
		BPath activationPath;
		const char *activationFilePathName = "Unknown due to errors";
		if (activationEntry.InitCheck() == B_OK &&
		activationEntry.GetPath(&activationPath) == B_OK)
			activationFilePathName = activationPath.Path();
		INFORM("Failed to open packages activation file %s: %s\n",
			activationFilePathName, strerror(error));
		RETURN_ERROR(error);
	}

	// read the whole file into memory to simplify things
	off_t size;
	error = file.GetSize(&size);
	if (error != B_OK) {
		ERROR("Failed to packages activation file size: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	if (size > (off_t)kMaxActivationFileSize) {
		ERROR("The packages activation file is too big.\n");
		RETURN_ERROR(B_BAD_DATA);
	}

	char* fileContent = (char*)malloc(size + 1);
	if (fileContent == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	MemoryDeleter fileContentDeleter(fileContent);

	ssize_t bytesRead = file.Read(fileContent, size);
	if (bytesRead < 0) {
		ERROR("Failed to read packages activation file: %s\n",
			strerror(bytesRead));
		RETURN_ERROR(errno);
	}

	if (bytesRead != size) {
		ERROR("Failed to read whole packages activation file.\n");
		RETURN_ERROR(B_ERROR);
	}

	// null-terminate to simplify parsing
	fileContent[size] = '\0';

	AutoLocker<BLocker> locker(fLock);

	// parse the file and mark the respective packages active
	const char* packageName = fileContent;
	char* const fileContentEnd = fileContent + size;
	while (packageName < fileContentEnd) {
		char* packageNameEnd = strchr(packageName, '\n');
		if (packageNameEnd == NULL)
			packageNameEnd = fileContentEnd;

		// skip empty lines
		if (packageName == packageNameEnd) {
			packageName++;
			continue;
		}
		*packageNameEnd = '\0';

		if (packageNameEnd - packageName >= B_FILE_NAME_LENGTH) {
			ERROR("Invalid packages activation file content.\n");
			RETURN_ERROR(B_BAD_DATA);
		}

		Package* package = fLatestState->FindPackage(packageName);
		if (package != NULL) {
			fLatestState->SetPackageActive(package, true);
			fChangeCount++;
		} else {
			WARN("Package \"%s\" from activation file not in packages "
				"directory.\n", packageName);
		}

		packageName = packageNameEnd + 1;
	}

	return B_OK;
}


status_t
Volume::_GetActivePackages(int fd)
{
	// get the info from packagefs
	PackageFSGetPackageInfosRequest* request = NULL;
	MemoryDeleter requestDeleter;
	size_t bufferSize = 64 * 1024;
	for (;;) {
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

		if (request->bufferSize <= bufferSize)
			break;

		bufferSize = request->bufferSize;
		requestDeleter.Unset();
	}

	INFORM("latest volume state:\n");
	_DumpState(fLatestState);

	// check whether that matches the expected state
	if (_CheckActivePackagesMatchLatestState(request)) {
		INFORM("The latest volume state is also the currently active one\n");
		fActiveState = fLatestState;
		return B_OK;
	}

	// There's a mismatch. We need a new state that reflects the actual
	// activation situation.
	VolumeState* state = new(std::nothrow) VolumeState;
	if (state == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<VolumeState> stateDeleter(state);

	for (uint32 i = 0; i < request->packageCount; i++) {
		const PackageFSPackageInfo& info = request->infos[i];
		NotOwningEntryRef entryRef(info.directoryDeviceID, info.directoryNodeID,
			info.name);
		Package* package;
		status_t error = fPackageFileManager->CreatePackage(entryRef, package);
		if (error != B_OK) {
			WARN("Failed to create package (dev: %" B_PRIdDEV ", node: %"
				B_PRIdINO ", \"%s\"): %s\n", info.directoryDeviceID,
				info.directoryNodeID, info.name, strerror(error));
			continue;
		}

		state->AddPackage(package);
		state->SetPackageActive(package, true);
	}

	INFORM("currently active volume state:\n");
	_DumpState(state);

	fActiveState = stateDeleter.Detach();
	return B_OK;
}


void
Volume::_RunQueuedScripts()
{
	BDirectory adminDirectory;
	status_t error = _OpenPackagesSubDirectory(
		RelativePath(kAdminDirectoryName), false, adminDirectory);
	if (error != B_OK)
		return;

	BDirectory scriptsDirectory;
	error = scriptsDirectory.SetTo(&adminDirectory, kQueuedScriptsDirectoryName);
	if (error != B_OK)
		return;

	// enumerate all the symlinks in the queued scripts directory
	BEntry scriptEntry;
	while (scriptsDirectory.GetNextEntry(&scriptEntry, false) == B_OK) {
		BPath scriptPath;
		scriptEntry.GetPath(&scriptPath);
		error = scriptPath.InitCheck();
		if (error != B_OK) {
			INFORM("failed to get path of post-installation script \"%s\"\n",
				strerror(error));
			continue;
		}

		errno = 0;
		int result = system(scriptPath.Path());
		if (result != 0) {
			INFORM("running post-installation script \"%s\" "
				"failed: %d (errno: %s)\n", scriptPath.Leaf(), errno, strerror(errno));
		}

		// remove the symlink, now that we've run the post-installation script
		error = scriptEntry.Remove();
		if (error != B_OK) {
			INFORM("removing queued post-install script failed \"%s\"\n",
				strerror(error));
		}
	}
}


bool
Volume::_CheckActivePackagesMatchLatestState(
	PackageFSGetPackageInfosRequest* request)
{
	if (fPackagesDirectoryCount != 1) {
		INFORM("An old packages state (\"%s\") seems to be active.\n",
			fPackagesDirectories[fPackagesDirectoryCount - 1].Name().String());
		return false;
	}

	const node_ref packagesDirRef(PackagesDirectoryRef());

	// mark the returned packages active
	for (uint32 i = 0; i < request->packageCount; i++) {
		const PackageFSPackageInfo& info = request->infos[i];
		if (node_ref(info.directoryDeviceID, info.directoryNodeID)
				!= packagesDirRef) {
			WARN("active package \"%s\" (dev: %" B_PRIdDEV ", node: %" B_PRIdINO
				") not in packages directory\n", info.name,
				info.packageDeviceID, info.packageNodeID);
			return false;
		}

		Package* package = fLatestState->FindPackage(
			node_ref(info.packageDeviceID, info.packageNodeID));
		if (package == NULL || !package->IsActive()) {
			WARN("active package \"%s\" (dev: %" B_PRIdDEV ", node: %" B_PRIdINO
				") not %s\n", info.name,
				info.packageDeviceID, info.packageNodeID,
				package == NULL
					? "found in packages directory" : "supposed to be active");
			return false;
		}
	}

	// Check whether there are packages that aren't active but should be.
	uint32 count = 0;
	for (PackageNodeRefHashTable::Iterator it
			= fLatestState->ByNodeRefIterator(); it.HasNext();) {
		Package* package = it.Next();
		if (package->IsActive())
			count++;
	}

	if (count != request->packageCount) {
		INFORM("There seem to be packages in the packages directory that "
			"should be active.\n");
		return false;
	}

	return true;
}


void
Volume::_SetLatestState(VolumeState* state, bool isActive)
{
	AutoLocker<BLocker> locker(fLock);

	bool sendNotification = fRoot->IsSystemRoot();
		// Send a notification, if this is a system root volume.
	BStringList addedPackageNames;
	BStringList removedPackageNames;

	// If a notification should be sent then assemble the latest and incoming
	// set of the packages' names.  This can be used to figure out which
	// packages are added and which are removed.

	if (sendNotification) {
		_CollectPackageNamesAdded(fLatestState, state, addedPackageNames);
		_CollectPackageNamesAdded(state, fLatestState, removedPackageNames);
	}

	if (isActive) {
		if (fLatestState != fActiveState)
			delete fActiveState;
		fActiveState = state;
	}

	if (fLatestState != fActiveState)
		delete fLatestState;
	fLatestState = state;
	fChangeCount++;

	locker.Unlock();

	// Send a notification, if this is a system root volume.
	if (sendNotification) {
		BMessage message(B_PACKAGE_UPDATE);
		if (message.AddInt32("event",
				(int32)B_INSTALLATION_LOCATION_PACKAGES_CHANGED) == B_OK
			&& message.AddStrings("added package names",
				addedPackageNames) == B_OK
			&& message.AddStrings("removed package names",
				removedPackageNames) == B_OK
			&& message.AddInt32("location", (int32)Location()) == B_OK
			&& message.AddInt64("change count", fChangeCount) == B_OK) {
			BRoster::Private().SendTo(&message, NULL, false);
		}
	}
}


/*static*/ void
Volume::_CollectPackageNamesAdded(const VolumeState* oldState,
	const VolumeState* newState, BStringList& addedPackageNames)
{
	if (newState == NULL)
		return;

	for (PackageFileNameHashTable::Iterator it
			= newState->ByFileNameIterator(); it.HasNext();) {
		Package* package = it.Next();
		BString packageName = package->Info().Name();
		if (oldState == NULL)
			addedPackageNames.Add(packageName);
		else {
			Package* oldStatePackage = oldState->FindPackage(
				package->FileName());
			if (oldStatePackage == NULL)
				addedPackageNames.Add(packageName);
		}
	}
}


void
Volume::_DumpState(VolumeState* state)
{
	uint32 inactiveCount = 0;
	for (PackageNodeRefHashTable::Iterator it = state->ByNodeRefIterator();
			it.HasNext();) {
		Package* package = it.Next();
		if (package->IsActive()) {
			INFORM("active package: \"%s\"\n", package->FileName().String());
		} else
			inactiveCount++;
	}

	if (inactiveCount == 0)
		return;

	for (PackageNodeRefHashTable::Iterator it = state->ByNodeRefIterator();
			it.HasNext();) {
		Package* package = it.Next();
		if (!package->IsActive())
			INFORM("inactive package: \"%s\"\n", package->FileName().String());
	}
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
Volume::_OpenPackagesSubDirectory(const RelativePath& path, bool create,
	BDirectory& _directory)
{
	// open the packages directory
	BDirectory directory;
	status_t error = directory.SetTo(&PackagesDirectoryRef());
	if (error != B_OK) {
		ERROR("Volume::_OpenPackagesSubDirectory(): failed to open packages "
			"directory: %s\n", strerror(error));
		RETURN_ERROR(error);
	}

	return FSUtils::OpenSubDirectory(directory, path, create, _directory);
}


void
Volume::_CommitTransaction(BMessage* message,
	const BActivationTransaction* transaction,
	const PackageSet& packagesAlreadyAdded,
	const PackageSet& packagesAlreadyRemoved, BCommitTransactionResult& _result)
{
	_result.Unset();

	// perform the request
	CommitTransactionHandler handler(this, fPackageFileManager, _result);
	BTransactionError error = B_TRANSACTION_INTERNAL_ERROR;
	try {
		handler.Init(fLatestState, fLatestState == fActiveState,
			packagesAlreadyAdded, packagesAlreadyRemoved);

		if (message != NULL)
			handler.HandleRequest(message);
		else if (transaction != NULL)
			handler.HandleRequest(*transaction);
		else
			handler.HandleRequest();

		_SetLatestState(handler.DetachVolumeState(),
			handler.IsActiveVolumeState());
		error = B_TRANSACTION_OK;
	} catch (Exception& exception) {
		error = exception.Error();
		exception.SetOnResult(_result);
		if (_result.ErrorPackage().IsEmpty()
			&& handler.CurrentPackage() != NULL) {
			_result.SetErrorPackage(handler.CurrentPackage()->FileName());
		}
	} catch (std::bad_alloc& exception) {
		error = B_TRANSACTION_NO_MEMORY;
	}

	_result.SetError(error);

	// revert on error
	if (error != B_TRANSACTION_OK)
		handler.Revert();
}
