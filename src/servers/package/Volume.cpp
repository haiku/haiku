/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Volume.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
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
#include <package/DaemonDefs.h>
#include <package/PackagesDirectoryDefs.h>

#include "DebugSupport.h"


using namespace BPackageKit::BPrivate;


static const char* const kPackageFileNameExtension = ".hpkg";
static const char* const kAdminDirectoryName
	= PACKAGES_DIRECTORY_ADMIN_DIRECTORY;
static const char* const kActivationFileName
	= PACKAGES_DIRECTORY_ACTIVATION_FILE;
static const char* const kTemporaryActivationFileName
	= PACKAGES_DIRECTORY_ACTIVATION_FILE ".tmp";

static bigtime_t kCommunicationTimeout = 1000000;


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
	fPackagesByFileName(),
	fPackagesByNodeRef(),
	fPendingNodeMonitorEventsLock("pending node monitor events"),
	fPendingNodeMonitorEvents(),
	fPackagesToBeActivated(),
	fPackagesToBeDeactivated(),
	fChangeCount(0),
	fLocationInfoReply(B_MESSAGE_GET_INSTALLATION_LOCATION_INFO_REPLY)
{
	looper->AddHandler(this);
}


Volume::~Volume()
{
	Unmounted();
		// need for error case in InitPackages()

	fPackagesByFileName.Clear();

	Package* package = fPackagesByNodeRef.Clear(true);
	while (package != NULL) {
		Package* next = package->NodeRefHashTableNext();
		delete package;
		package = next;
	}
}


status_t
Volume::Init(const node_ref& rootDirectoryRef, node_ref& _packageRootRef)
{
	if (fPackagesByFileName.Init() != B_OK || fPackagesByNodeRef.Init() != B_OK)
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
	fPackagesDirectoryRef.device = info.packagesDeviceID;
	fPackagesDirectoryRef.node = info.packagesDirectoryID;

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

	return B_OK;
}


status_t
Volume::AddPackagesToRepository(BSolverRepository& repository, bool activeOnly)
{
	for (PackageFileNameHashTable::Iterator it
			= fPackagesByFileName.GetIterator(); it.HasNext();) {
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
	// If the cached reply message is up-to-date, just send it.
	int64 changeCount;
	if (fLocationInfoReply.FindInt64("change count", &changeCount) == B_OK
		&& changeCount == fChangeCount) {
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

	for (PackageFileNameHashTable::Iterator it
			= fPackagesByFileName.GetIterator(); it.HasNext();) {
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

	if (fLocationInfoReply.AddInt64("change count", fChangeCount) != B_OK)
		return;

	message->SendReply(&fLocationInfoReply, (BHandler*)NULL,
		kCommunicationTimeout);
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

		default:
			BHandler::MessageReceived(message);
			break;
	}
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
INFORM("Volume::ProcessPendingPackageActivationChanges(): activating %zu, deactivating %zu packages\n",
fPackagesToBeActivated.size(), fPackagesToBeDeactivated.size());

	// compute the size of the allocation we need for the activation change
	// request
	int32 itemCount
		= fPackagesToBeActivated.size() + fPackagesToBeDeactivated.size();
	size_t requestSize = sizeof(PackageFSActivationChangeRequest)
		+ itemCount * sizeof(PackageFSActivationChangeItem);

	for (PackageSet::iterator it = fPackagesToBeActivated.begin();
		 it != fPackagesToBeActivated.end(); ++it) {
		requestSize += (*it)->FileName().Length() + 1;
	}

	for (PackageSet::iterator it = fPackagesToBeDeactivated.begin();
		 it != fPackagesToBeDeactivated.end(); ++it) {
		requestSize += (*it)->FileName().Length() + 1;
	}

	// allocate and prepare the request
	PackageFSActivationChangeRequest* request
		= (PackageFSActivationChangeRequest*)malloc(requestSize);
	if (request == NULL) {
		ERROR("out of memory\n");
		return;
	}
	MemoryDeleter requestDeleter(request);

	request->itemCount = itemCount;

	PackageFSActivationChangeItem* item = &request->items[0];
	char* nameBuffer = (char*)(item + itemCount);

	for (PackageSet::iterator it = fPackagesToBeActivated.begin();
		it != fPackagesToBeActivated.end(); ++it, item++) {
		_FillInActivationChangeItem(item, PACKAGE_FS_ACTIVATE_PACKAGE, *it,
			nameBuffer);
	}

	for (PackageSet::iterator it = fPackagesToBeDeactivated.begin();
		it != fPackagesToBeDeactivated.end(); ++it, item++) {
		_FillInActivationChangeItem(item, PACKAGE_FS_DEACTIVATE_PACKAGE, *it,
			nameBuffer);
	}

	// issue the request
	int fd = OpenRootDirectory();
	if (fd < 0) {
		ERROR("Volume::ProcessPendingPackageActivationChanges(): failed to "
			"open root directory: %s", strerror(fd));
		return;
	}
	FileDescriptorCloser fdCloser(fd);

	if (ioctl(fd, PACKAGE_FS_OPERATION_CHANGE_ACTIVATION, request, requestSize)
			!= 0) {
// TODO: We need more error information and error handling!
		ERROR("Volume::ProcessPendingPackageActivationChanges(): failed to "
			"activate packages: %s\n", strerror(errno));
		return;
	}

	// Update our state, i.e. remove deactivated packages and mark activated
	// packages accordingly.
	for (PackageSet::iterator it = fPackagesToBeActivated.begin();
		it != fPackagesToBeActivated.end(); ++it) {
		(*it)->SetActive(true);
		fChangeCount++;
	}

	for (PackageSet::iterator it = fPackagesToBeDeactivated.begin();
		it != fPackagesToBeDeactivated.end(); ++it) {
		Package* package = *it;
		_RemovePackage(package);
		delete package;
	}

	fPackagesToBeActivated.clear();
	fPackagesToBeDeactivated.clear();

	// write the package activation file

	// create the content
	BString activationFileContent;
	for (PackageFileNameHashTable::Iterator it
			= fPackagesByFileName.GetIterator(); it.HasNext();) {
		Package* package = it.Next();
		if (package->IsActive())
			activationFileContent << package->FileName() << '\n';
	}

	// open and write the temporary file
	BFile activationFile;
	BEntry activationFileEntry;
	status_t error = _OpenPackagesFile(kAdminDirectoryName,
		kTemporaryActivationFileName,
		B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE, activationFile,
		&activationFileEntry);
	if (error != B_OK) {
		ERROR("Volume::ProcessPendingPackageActivationChanges(): failed to "
			"create activation file: %s\n", strerror(error));
		return;
	}

	ssize_t bytesWritten = activationFile.Write(activationFileContent.String(),
		activationFileContent.Length());
	if (bytesWritten < 0) {
		ERROR("Volume::ProcessPendingPackageActivationChanges(): failed to "
			"write activation file: %s\n", strerror(bytesWritten));
		return;
	}

	// rename the temporary file to the final file
	error = activationFileEntry.Rename(kActivationFileName, true);
	if (error != B_OK) {
		ERROR("Volume::ProcessPendingPackageActivationChanges(): failed to "
			"rename temporary activation file to final file: %s\n",
			strerror(error));
		return;
	}
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
	bool firstEvent = fPendingNodeMonitorEvents.IsEmpty();
	fPendingNodeMonitorEvents.Add(event);
	eventsLock.Unlock();

	if (firstEvent && fListener != NULL)
		fListener->VolumeNodeMonitorEventOccurred(this);
}


void
Volume::_PackagesEntryCreated(const char* name)
{
INFORM("Volume::_PackagesEntryCreated(\"%s\")\n", name);
	entry_ref entry;
	entry.device = fPackagesDirectoryRef.device;
	entry.directory = fPackagesDirectoryRef.node;
	status_t error = entry.set_name(name);
	if (error != B_OK) {
		ERROR("out of memory\n");
		return;
	}

	Package* package = new(std::nothrow) Package;
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
	Package* package = fPackagesByFileName.Lookup(name);
	if (package == NULL)
		return;

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
	fPackagesByFileName.Insert(package);
	fPackagesByNodeRef.Insert(package);
}

void
Volume::_RemovePackage(Package* package)
{
	fPackagesByFileName.Remove(package);
	fPackagesByNodeRef.Remove(package);
	fChangeCount++;
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
		Package* package = fPackagesByNodeRef.Lookup(
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

		package->SetActive(true);
INFORM("active package: \"%s\"\n", package->FileName().String());
	}

for (PackageNodeRefHashTable::Iterator it = fPackagesByNodeRef.GetIterator();
	it.HasNext();) {
	Package* package = it.Next();
	if (!package->IsActive())
		INFORM("inactive package: \"%s\"\n", package->FileName().String());
}

			PackageNodeRefHashTable fPackagesByNodeRef;
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
Volume::_OpenPackagesFile(const char* subDirectoryPath, const char* fileName,
	uint32 openMode, BFile& _file, BEntry* _entry)
{
	BDirectory directory;
	if (subDirectoryPath != NULL) {
		status_t error = _OpenPackagesSubDirectory(subDirectoryPath,
			(openMode & B_CREATE_FILE) != 0, directory);
		if (error != B_OK) {
			ERROR("Volume::_OpenPackagesFile(): failed to open packages "
				"subdirectory \"%s\": %s\n", subDirectoryPath, strerror(error));
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
Volume::_OpenPackagesSubDirectory(const char* path, bool create,
	BDirectory& _directory)
{
	// open the packages directory
	BDirectory directory;
	status_t error = directory.SetTo(&fPackagesDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::_OpenConfigSubDirectory(): failed to open packages "
			"directory: %s\n", strerror(error));
		RETURN_ERROR(error);
	}

	// If creating is not allowed, just try to open it.
	if (!create)
		RETURN_ERROR(_directory.SetTo(&directory, path));

	// get an absolute path and create the subdirectory
	BPath absolutePath;
	error = absolutePath.SetTo(&directory, path);
	if (error != B_OK) {
		ERROR("Volume::_OpenConfigSubDirectory(): failed to get absolute path "
			"for subdirectory \"%s\": %s\n", path, strerror(error));
		RETURN_ERROR(error);
	}

	error = create_directory(absolutePath.Path(),
		S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	if (error != B_OK) {
		ERROR("Volume::_OpenConfigSubDirectory(): failed to create packages "
			"subdirectory \"%s\": %s\n", path, strerror(error));
		RETURN_ERROR(error);
	}

	RETURN_ERROR(_directory.SetTo(&directory, path));
}
