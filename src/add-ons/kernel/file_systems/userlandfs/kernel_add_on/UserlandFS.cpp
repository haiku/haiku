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

UserlandFS* UserlandFS::sUserlandFS = NULL;

// constructor
UserlandFS::UserlandFS()
	: fPort(NULL),
	  fFileSystems(NULL),
	  fDebuggerCommandsAdded(false)
{
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

// InitUserlandFS
status_t
UserlandFS::InitUserlandFS(UserlandFS** _userlandFS)
{
	// create the singleton instance
	sUserlandFS = new(nothrow) UserlandFS;
	if (!sUserlandFS)
		return B_NO_MEMORY;

	// init the thing
	status_t error = sUserlandFS->_Init();
	if (error != B_OK) {
		delete sUserlandFS;
		sUserlandFS = NULL;
		return error;
	}

	*_userlandFS = sUserlandFS;

	return B_OK;
}

// UninitUserlandFS
void
UserlandFS::UninitUserlandFS()
{
	delete sUserlandFS;
	sUserlandFS = NULL;
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
	// check parameters
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

// _Init
status_t
UserlandFS::_Init()
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

