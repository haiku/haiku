// UserlandFSDispatcher.cpp

#include <new>

#include <Application.h>
#include <Clipboard.h>
#include <Locker.h>
#include <Message.h>
#include <Roster.h>

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "Compatibility.h"
#include "Debug.h"
#include "DispatcherDefs.h"
#include "FileSystem.h"
#include "FSInfo.h"
#include "RequestAllocator.h"
#include "RequestPort.h"
#include "Requests.h"
#include "ServerDefs.h"
#include "String.h"
#include "UserlandFSDispatcher.h"

// constructor
UserlandFSDispatcher::UserlandFSDispatcher(const char* signature)
	: BApplication(signature),
	  fTerminating(false),
	  fRequestProcessor(-1),
	  fConnectionPort(-1),
	  fConnectionReplyPort(-1),
	  fRequestLock(),
	  fRequestPort(NULL)
{
}

// destructor
UserlandFSDispatcher::~UserlandFSDispatcher()
{
	fTerminating = true;
	// stop roster watching
	be_roster->StopWatching(this);
	// close/delete the ports
	fRequestLock.Lock();
	if (fRequestPort)
		fRequestPort->Close();
	fRequestLock.Unlock();
	if (fConnectionPort >= 0)
		delete_port(fConnectionPort);
	if (fConnectionReplyPort >= 0)
		delete_port(fConnectionReplyPort);
	// wait for the request processor
 	if (fRequestProcessor >= 0) {
		int32 result;
		wait_for_thread(fRequestProcessor, &result);
	}
}

// Init
status_t
UserlandFSDispatcher::Init()
{
	// ensure that we are the only dispatcher
	BClipboard clipboard(kUserlandFSDispatcherClipboardName);
	if (!clipboard.Lock()) {
		ERROR(("Failed to lock the clipboard.\n"));
		return B_ERROR;
	}
	status_t error = B_OK;
	if (BMessage* data = clipboard.Data()) {
		// check the old value in the clipboard
		BMessenger messenger;
		if (data->FindMessenger("messenger", &messenger) == B_OK) {
			if (messenger.IsValid()) {
				PRINT(("There's already a dispatcher running.\n"));
				error = B_ERROR;
			}
		}
		// clear the clipboard
		if (error == B_OK) {
			clipboard.Clear();
			data = clipboard.Data();
			if (!data)
				error = B_ERROR;
		}
		// add our messenger
		if (error == B_OK) {
			SET_ERROR(error, data->AddMessenger("messenger", be_app_messenger));
			if (error == B_OK)
				SET_ERROR(error, clipboard.Commit());
			// work-around for BeOS R5: The very first commit to a clipboard
			// (i.e. the one that creates the clipboard) seems to be ignored.
			if (error == B_OK)
				SET_ERROR(error, clipboard.Commit());
			if (error != B_OK)
				ERROR(("Failed to set clipboard messenger.\n"));
		}
	} else {
		ERROR(("Failed to get clipboard data container\n"));
		error = B_ERROR;
	}
	clipboard.Unlock();
	if (error != B_OK)
		return error;
	// create the connection port and connection reply port
	fConnectionPort = create_port(1, kUserlandFSDispatcherPortName);
	if (fConnectionPort < 0)
		return fConnectionPort;
	fConnectionReplyPort = create_port(1, kUserlandFSDispatcherReplyPortName);
	if (fConnectionReplyPort < 0)
		return fConnectionReplyPort;
	// start watching for terminated applications
	error = be_roster->StartWatching(this, B_REQUEST_QUIT);
	if (error != B_OK)
		return error ;
	// spawn request processor thread
	fRequestProcessor = spawn_thread(_RequestProcessorEntry,
		"main request processor", B_NORMAL_PRIORITY, this);
	if (fRequestProcessor < 0)
		return fRequestProcessor;
	resume_thread(fRequestProcessor);
	return B_OK;
}

// MessageReceived
void
UserlandFSDispatcher::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case UFS_REGISTER_FS:
		{
			// get the team
			team_id team;
			status_t error = message->FindInt32("team", &team);
			if (error != B_OK)
				PRINT(("UFS_REGISTER_FS failed: no team\n"));
			// get the FS info
			FSInfo* info = NULL;
			if (error == B_OK) {
				info = new(nothrow) FSInfo;
				if (info) {
					error = info->SetTo(message);
				} else {
					error = B_NO_MEMORY;
					PRINT(("UFS_REGISTER_FS failed: failed to allocate "
						"FSInfo\n"));
				}
			}
			ObjectDeleter<FSInfo> infoDeleter(info);
			// find the FileSystem
			FileSystem* fileSystem = NULL;
			if (error == B_OK) {
				AutoLocker<FileSystemMap> _(fFileSystems);
				fileSystem = _GetFileSystemNoInit(team);
				if (fileSystem) {
					fileSystem->CompleteInit(info);
					infoDeleter.Detach();
				} else {
					PRINT(("UFS_REGISTER_FS: no FileSystem found for "
						"team %ld, trying to register anyway\n", team));
					// try to find by name
					fileSystem = fFileSystems.Get(info->GetName());
					if (fileSystem) {
						// there's already an FS with that name registered
						PRINT(("UFS_REGISTER_FS failed: FileSystem with "
							"name %s does already exist.\n", info->GetName()));
						fileSystem = NULL;
						error = B_ERROR;
					} else {
						// the FS is not known yet: create one
						fileSystem = new FileSystem(team, info, &error);
						if (fileSystem) {
							infoDeleter.Detach();
						} else {
							error = B_NO_MEMORY;
							PRINT(("UFS_REGISTER_FS failed: failed to allocate "
								"FileSystem\n"));
						}
						// add it
						if (error == B_OK) {
							error = fFileSystems.Put(info->GetName(),
								fileSystem);
							if (error != B_OK) {
								PRINT(("UFS_REGISTER_FS failed: failed to "
									"add FileSystem\n"));
								delete fileSystem;
							}
						}
					}
				}
			}
			// send the reply
			if (error == B_OK)
				message->SendReply(UFS_REGISTER_FS_ACK);
			else
				message->SendReply(UFS_REGISTER_FS_DENIED);
			if (fileSystem)
				_PutFileSystem(fileSystem);
			break;
		}
		case B_SOME_APP_QUIT:
		{
			// get the team
			team_id team;
			status_t error = message->FindInt32("be:team", &team);
			if (error != B_OK)
				return;
			// find the FileSystem
			FileSystem* fileSystem = _GetFileSystemNoInit(team);
			if (!fileSystem)
				return;
			// abort the initialization
			fileSystem->AbortInit();
			_PutFileSystem(fileSystem);
		}
		default:
			BApplication::MessageReceived(message);
	}
}

// _GetFileSystem
status_t
UserlandFSDispatcher::_GetFileSystem(const char* name, FileSystem** _fileSystem)
{
	if (!name || !_fileSystem)
		RETURN_ERROR(B_BAD_VALUE);
	// get the file system
	FileSystem* fileSystem;
	{
		AutoLocker<FileSystemMap> _(fFileSystems);
		fileSystem = fFileSystems.Get(name);
		if (fileSystem) {
			fileSystem->AddReference();
		} else {
			// doesn't exists yet: create
			status_t error;
			fileSystem = new(nothrow) FileSystem(name, &error);
			if (!fileSystem)
				RETURN_ERROR(B_NO_MEMORY);
			if (error == B_OK)
				error = fFileSystems.Put(fileSystem->GetName(), fileSystem);
			if (error != B_OK) {
				delete fileSystem;
				RETURN_ERROR(error);
			}
		}
	}
	// prepare access
	status_t error = fileSystem->Access();
	if (error != B_OK) {
		_PutFileSystem(fileSystem);
		RETURN_ERROR(error);
	}
	*_fileSystem = fileSystem;
	return B_OK;
}

// _GetFileSystemNoInit
FileSystem*
UserlandFSDispatcher::_GetFileSystemNoInit(team_id team)
{
	AutoLocker<FileSystemMap> _(fFileSystems);
	for (FileSystemMap::Iterator it = fFileSystems.GetIterator();
		 it.HasNext();) {
		FileSystem* fileSystem = it.Next().value;
		if (fileSystem->GetTeam() == team) {
			// found it
			if (fileSystem)
				fileSystem->AddReference();
			return fileSystem;
		}
	}
	return NULL;
}

// _PutFileSystem
status_t
UserlandFSDispatcher::_PutFileSystem(FileSystem* fileSystem)
{
	if (!fileSystem)
		RETURN_ERROR(B_BAD_VALUE);
	AutoLocker<FileSystemMap> _(fFileSystems);
	if (fFileSystems.Get(fileSystem->GetName()) != fileSystem)
		RETURN_ERROR(B_BAD_VALUE);
	if (fileSystem->RemoveReference() && fileSystem->InitCheck() != B_OK) {
PRINT(("removing FileSystem `%s'\n", fileSystem->GetName()));
		fFileSystems.Remove(fileSystem->GetName());
		delete fileSystem;
	}
	return B_OK;
}

// _WaitForConnection
bool
UserlandFSDispatcher::_WaitForConnection()
{
	while (!fTerminating) {
		int32 code;
		char buffer;
		size_t bytesRead = read_port(fConnectionPort, &code, &buffer, 0);
		if (bytesRead >= 0 && code == UFS_DISPATCHER_CONNECT) {
			const Port::Info* info = fRequestPort->GetPortInfo();
			size_t bytesWritten = write_port(fConnectionReplyPort,
				UFS_DISPATCHER_CONNECT_ACK, info, sizeof(Port::Info));
			if (bytesWritten >= 0)
				return true;
		}
	}
	return false;
}

// _ProcessRequests
status_t
UserlandFSDispatcher::_ProcessRequests()
{
	while (!fTerminating) {
		Request* request;
		status_t error = fRequestPort->ReceiveRequest(&request);
		if (error != B_OK)
			RETURN_ERROR(error);
		RequestReleaser _(fRequestPort, request);
		// check the request type
		if (request->GetType() == UFS_DISCONNECT_REQUEST)
			return B_OK;
		if (request->GetType() != FS_CONNECT_REQUEST)
			RETURN_ERROR(B_BAD_VALUE);
		PRINT(("UserlandFSDispatcher::_ProcessRequests(): received FS connect "
			"request\n"));
		// it's an FS connect request
		FSConnectRequest* connectRequest = (FSConnectRequest*)request;
		// get the FS name
		int32 len = connectRequest->fsName.GetSize();
		status_t result = B_OK;
		if (len <= 0)
			result = B_BAD_DATA;
		String fsName;
		if (result == B_OK)
			fsName.SetTo((const char*)connectRequest->fsName.GetData(), len);
		if (result == B_OK && fsName.GetLength() == 0)
			result = B_BAD_DATA;
		// prepare the reply
		RequestAllocator allocator(fRequestPort->GetPort());
		FSConnectReply* reply;
		error = AllocateRequest(allocator, &reply);
		if (error != B_OK)
			RETURN_ERROR(error);
		FileSystem* fileSystem = NULL;
		if (result == B_OK)
			result = _GetFileSystem(fsName.GetString(), &fileSystem);
		if (result == B_OK) {
			const FSInfo* info = fileSystem->GetInfo();
			result = allocator.AllocateData(reply->portInfos,
				info->GetInfos(), info->GetSize(), sizeof(Port::Info));
			if (result == B_OK)
				reply->portInfoCount = info->CountInfos();
			_PutFileSystem(fileSystem);
		}
		reply->error = result;
		// send it
		error = fRequestPort->SendRequest(&allocator);
		if (error != B_OK)
			RETURN_ERROR(error);
	}
	return B_OK;
}

// _RequestProcessorEntry
int32
UserlandFSDispatcher::_RequestProcessorEntry(void* data)
{
	return ((UserlandFSDispatcher*)data)->_RequestProcessor();
}

// _RequestProcessor
int32
UserlandFSDispatcher::_RequestProcessor()
{
PRINT(("UserlandFSDispatcher::_RequestProcessor()\n"));
	while (!fTerminating) {
		// allocate a request port
		status_t error = B_OK;
		{
			fRequestLock.Lock();
			fRequestPort = new(nothrow) RequestPort(kRequestPortSize);
			if (fRequestPort)
				error = fRequestPort->InitCheck();
			else
				error = B_NO_MEMORY;
			if (error != B_OK) {
				delete fRequestPort;
				fRequestPort = NULL;
			}
			fRequestLock.Unlock();
		}
		if (error != B_OK) {
			be_app->PostMessage(B_QUIT_REQUESTED);
PRINT(("  failed to allocate request port: %s\n", strerror(error)));
			return error;
		}
		// wait for a connection and process the requests
		if (_WaitForConnection()) {
			PRINT(("UserlandFSDispatcher::_RequestProcessor(): connected\n"));
			_ProcessRequests();
			PRINT(("UserlandFSDispatcher::_RequestProcessor(): "
				"disconnected\n"));
		}
		// delete the request port
		fRequestLock.Lock();
		delete fRequestPort;
		fRequestPort = NULL;
		fRequestLock.Unlock();
	}
PRINT(("UserlandFSDispatcher::_RequestProcessor() done\n"));
	return B_OK;
}

