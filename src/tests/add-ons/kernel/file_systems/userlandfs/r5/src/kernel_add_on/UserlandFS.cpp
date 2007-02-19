// UserlandFS.cpp

#include <KernelExport.h>

#include "AutoLocker.h"
#include "Compatibility.h"
#include "Debug.h"
#include "DispatcherDefs.h"
#include "FileSystem.h"
#include "KernelDebug.h"
#include "RequestPort.h"
#include "Requests.h"
#include "UserlandFS.h"

typedef AutoLocker<UserlandFS::FileSystemMap> FileSystemLocker;

UserlandFS* volatile UserlandFS::sUserlandFS = NULL;
spinlock UserlandFS::sUserlandFSLock = 0;
vint32 UserlandFS::sMountedFileSystems = 0;		

// constructor
UserlandFS::UserlandFS()
	: LazyInitializable(),
	  fPort(NULL),
	  fFileSystems(NULL),
	  fDebuggerCommandsAdded(false)
{
	// beware what you do here: the caller holds a spin lock
}

// destructor
UserlandFS::~UserlandFS()
{
PRINT(("UserlandFS::~UserlandFS()\n"))
	if (fPort) {
		// send a disconnect request
		RequestAllocator allocator(fPort->GetPort());
		UFSDisconnectRequest* request;
		if (AllocateRequest(allocator, &request) == B_OK) {
			if (fPort->SendRequest(&allocator) != B_OK)
				PRINT(("  failed to send disconnect request\n"));
		} else
			PRINT(("  failed to allocate disconnect request\n"));
		delete fPort;
	} else
		PRINT(("  no port\n"));

	delete fFileSystems;
	if (fDebuggerCommandsAdded)
		KernelDebug::RemoveDebuggerCommands();
}

// RegisterUserlandFS
status_t
UserlandFS::RegisterUserlandFS(UserlandFS** _userlandFS)
{
	// first check, if there's already an instance
	bool create = false;

	cpu_status cpuStatus = disable_interrupts();
	acquire_spinlock(&sUserlandFSLock);

	if (sUserlandFS)
		sMountedFileSystems++;
	else
		create = true;

	release_spinlock(&sUserlandFSLock);
	restore_interrupts(cpuStatus);

	// if there's not, create a new
	status_t error = B_OK;
	if (create) {
		// first create an instance
		// Note, that we can't even construct a LazyInitializable with a
		// spinlock being held, since it allocates a semaphore, which may
		// allocate memory, which will acquire a semaphore.
		UserlandFS* userlandFS = new(nothrow) UserlandFS;
		if (userlandFS) {
			// now set the instance unless someone else beat us to it
			bool deleteInstance = false;

			cpu_status cpuStatus = disable_interrupts();
			acquire_spinlock(&sUserlandFSLock);

			sMountedFileSystems++;
			if (sUserlandFS)
				deleteInstance = true;
			else
				sUserlandFS = userlandFS;

			release_spinlock(&sUserlandFSLock);
			restore_interrupts(cpuStatus);

			// delete the new instance, if there was one already
			if (deleteInstance)
				delete userlandFS;
		} else
			error = B_NO_MEMORY;
	}
	if (error != B_OK)
		return error;

	// init the thing, if necessary
	error = sUserlandFS->Access();
	if (error == B_OK)
		*_userlandFS = sUserlandFS;
	else
		UnregisterUserlandFS();
	return error;
}

// UnregisterUserlandFS
void
UserlandFS::UnregisterUserlandFS()
{
	cpu_status cpuStatus = disable_interrupts();
	acquire_spinlock(&sUserlandFSLock);

	--sMountedFileSystems;
	UserlandFS* userlandFS = NULL;
	if (sMountedFileSystems == 0 && sUserlandFS) {
		userlandFS = sUserlandFS;
		sUserlandFS = NULL;
	}

	release_spinlock(&sUserlandFSLock);
	restore_interrupts(cpuStatus);

	// delete, if the last FS has been unmounted
	if (userlandFS) {
		userlandFS->~UserlandFS();
		delete[] (uint8*)userlandFS;
	}
}

// GetUserlandFS
UserlandFS*
UserlandFS::GetUserlandFS()
{
	return sUserlandFS;
}

// RegisterFileSystem
status_t
UserlandFS::RegisterFileSystem(const char* name, FileSystem** _fileSystem)
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		return InitCheck();
	if (!name || !_fileSystem)
		return B_BAD_VALUE;
	// check, if we do already know this file system, and create it, if not
	FileSystem* fileSystem;
	{
		FileSystemLocker _(fFileSystems);
		fileSystem = fFileSystems->Get(name);
		if (fileSystem) {
			fileSystem->AddReference();
		} else {
			status_t error;
			fileSystem = new(nothrow) FileSystem(name, fPort, &error);
			if (!fileSystem)
				return B_NO_MEMORY;
			if (error == B_OK)
				error = fFileSystems->Put(name, fileSystem);
			if (error != B_OK) {
				delete fileSystem;
				return error;
			}
		}
	}
	// prepare the file system
	status_t error = fileSystem->Access();
	if (error != B_OK) {
		UnregisterFileSystem(fileSystem);
		return error;
	}
	*_fileSystem = fileSystem;
	return error;
}

// UnregisterFileSystem
status_t
UserlandFS::UnregisterFileSystem(FileSystem* fileSystem)
{
	if (!fileSystem)
		return B_BAD_VALUE;
	// find the FS and decrement its reference counter
	bool deleteFS = false;
	{
		FileSystemLocker _(fFileSystems);
		fileSystem = fFileSystems->Get(fileSystem->GetName());
		if (!fileSystem)
			return B_BAD_VALUE;
		deleteFS = fileSystem->RemoveReference();
		if (deleteFS)
			fFileSystems->Remove(fileSystem->GetName());
	}
	// delete the FS, if the last reference has been removed
	if (deleteFS)
		delete fileSystem;
	return B_OK;
}

// CountFileSystems
int32
UserlandFS::CountFileSystems() const
{
	return fFileSystems->Size();
}

// FirstTimeInit
status_t
UserlandFS::FirstTimeInit()
{
	// add debugger commands
	KernelDebug::AddDebuggerCommands();
	fDebuggerCommandsAdded = true;
	// create file system map
	fFileSystems = new(nothrow) FileSystemMap;
	if (!fFileSystems)
		RETURN_ERROR(B_NO_MEMORY);
	status_t error = fFileSystems->InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);
	// find the dispatcher ports
	port_id port = find_port(kUserlandFSDispatcherPortName);
	if (port < 0)
		RETURN_ERROR(B_ERROR);
	port_id replyPort = find_port(kUserlandFSDispatcherReplyPortName);
	if (replyPort < 0)
		RETURN_ERROR(B_ERROR);
	// create a reply port
	// send a connection request
	error = write_port(port, UFS_DISPATCHER_CONNECT, NULL, 0);
	if (error != B_OK)
		RETURN_ERROR(error);
	// receive the reply
	int32 replyCode;
	Port::Info portInfo;
	ssize_t bytesRead = read_port(replyPort, &replyCode, &portInfo,
		sizeof(Port::Info));
	if (bytesRead < 0)
		RETURN_ERROR(bytesRead);
	if (replyCode != UFS_DISPATCHER_CONNECT_ACK)
		RETURN_ERROR(B_BAD_DATA);
	if (bytesRead != sizeof(Port::Info))
		RETURN_ERROR(B_BAD_DATA);
	// create a request port
	fPort = new(nothrow) RequestPort(&portInfo);
	if (!fPort)
		RETURN_ERROR(B_NO_MEMORY);
	if ((error = fPort->InitCheck()) != B_OK)
		RETURN_ERROR(error);
	RETURN_ERROR(error);
}

