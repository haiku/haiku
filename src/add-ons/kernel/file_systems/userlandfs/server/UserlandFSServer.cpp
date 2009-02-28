// UserlandFSServer.cpp

#include "UserlandFSServer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Clipboard.h>
#include <FindDirectory.h>
#include <fs_interface.h>
#include <Locker.h>
#include <Path.h>

#include <image_private.h>

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "Compatibility.h"
#include "Debug.h"
#include "DispatcherDefs.h"
#include "FileSystem.h"
#include "FSInfo.h"
#include "RequestThread.h"
#include "ServerDefs.h"


static const int32 kRequestThreadCount = 10;


// constructor
UserlandFSServer::UserlandFSServer(const char* signature)
	: BApplication(signature),
	  fAddOnImage(-1),
	  fFileSystem(NULL),
	  fNotificationRequestPort(NULL),
	  fRequestThreads(NULL)
{
}

// destructor
UserlandFSServer::~UserlandFSServer()
{
	if (fRequestThreads) {
		for (int32 i = 0; i < kRequestThreadCount; i++)
			fRequestThreads[i].PrepareTermination();
		for (int32 i = 0; i < kRequestThreadCount; i++)
			fRequestThreads[i].Terminate();
		delete[] fRequestThreads;
	}
	delete fNotificationRequestPort;
	delete fFileSystem;
	if (fAddOnImage >= 0)
		unload_add_on(fAddOnImage);
}

// Init
status_t
UserlandFSServer::Init(const char* fileSystem)
{
	// get the add-on path
	BPath addOnPath;
	status_t error = find_directory(B_USER_ADDONS_DIRECTORY, &addOnPath);
	if (error != B_OK)
		RETURN_ERROR(error);
	error = addOnPath.Append("userlandfs");
	if (error != B_OK)
		RETURN_ERROR(error);
	error = addOnPath.Append(fileSystem);
	if (error != B_OK)
		RETURN_ERROR(error);

	// load the add-on
	fAddOnImage = load_add_on(addOnPath.Path());
	if (fAddOnImage < 0)
		RETURN_ERROR(fAddOnImage);

	// Get the FS creation function -- the add-on links against one of our
	// libraries exporting that function, so we search recursively.
	union {
		void* address;
		status_t (*function)(const char*, image_id, FileSystem**);
	} createFSFunction;
	error = get_image_symbol_etc(fAddOnImage, "userlandfs_create_file_system",
		B_SYMBOL_TYPE_TEXT, true, NULL, &createFSFunction.address);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the FileSystem interface
	error = createFSFunction.function(fileSystem, fAddOnImage, &fFileSystem);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the notification request port
	fNotificationRequestPort = new(nothrow) RequestPort(kRequestPortSize);
	if (!fNotificationRequestPort)
		RETURN_ERROR(B_NO_MEMORY);
	error = fNotificationRequestPort->InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// now create the request threads
	fRequestThreads = new(nothrow) RequestThread[kRequestThreadCount];
	if (!fRequestThreads)
		RETURN_ERROR(B_NO_MEMORY);
	for (int32 i = 0; i < kRequestThreadCount; i++) {
		error = fRequestThreads[i].Init(fFileSystem);
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	// run the threads
	for (int32 i = 0; i < kRequestThreadCount; i++)
		fRequestThreads[i].Run();

	// enter the debugger here, if desired
	if (gServerSettings.ShallEnterDebugger())
		debugger("File system ready to use.");

	// finally register with the dispatcher
	error = _RegisterWithDispatcher(fileSystem);
	RETURN_ERROR(error);
}

// GetNotificationRequestPort
RequestPort*
UserlandFSServer::GetNotificationRequestPort()
{
	if (UserlandFSServer* server = dynamic_cast<UserlandFSServer*>(be_app))
		return server->fNotificationRequestPort;
	return NULL;
}

// GetFileSystem
FileSystem*
UserlandFSServer::GetFileSystem()
{
	if (UserlandFSServer* server = dynamic_cast<UserlandFSServer*>(be_app))
		return server->fFileSystem;
	return NULL;
}

// _RegisterWithDispatcher
status_t
UserlandFSServer::_RegisterWithDispatcher(const char* fsName)
{
	// get the dispatcher messenger from the clipboard
	BMessenger messenger;
	BClipboard clipboard(kUserlandFSDispatcherClipboardName);
	if (AutoLocker<BClipboard> locker = clipboard) {
		status_t error = B_OK;
		if (BMessage* data = clipboard.Data()) {
			error = data->FindMessenger("messenger", &messenger);
			if (error != B_OK) {
				ERROR(("No dispatcher messenger in clipboard.\n"));
				return error;
			}
			if (!messenger.IsValid()) {
				ERROR(("Found dispatcher messenger not valid.\n"));
				return B_ERROR;
			}
		} else {
			ERROR(("Failed to get clipboard data container\n"));
			return B_ERROR;
		}
	} else {
		ERROR(("Failed to lock the clipboard.\n"));
		return B_ERROR;
	}

	// get the port infos
	Port::Info infos[kRequestThreadCount + 1];
	infos[0] = *fNotificationRequestPort->GetPortInfo();
	for (int32 i = 0; i < kRequestThreadCount; i++)
		infos[i + 1] = *fRequestThreads[i].GetPortInfo();

	// FS capabilities
	FSCapabilities capabilities;
	fFileSystem->GetCapabilities(capabilities);

	// init an FS info
	FSInfo info;
	status_t error = info.SetTo(fsName, infos, kRequestThreadCount + 1,
		capabilities, fFileSystem->GetClientFSType());

	// prepare the message
	BMessage message(UFS_REGISTER_FS);
	if (error == B_OK)
		error = message.AddInt32("team", Team());
	if (error == B_OK)
		error = info.Archive(&message);

	// send the message
	BMessage reply;
	error = messenger.SendMessage(&message, &reply);
	if (error == B_OK && reply.what != UFS_REGISTER_FS_ACK) {
		ERROR(("FS registration failed.\n"));
		error = B_ERROR;
	}
	return error;
}
