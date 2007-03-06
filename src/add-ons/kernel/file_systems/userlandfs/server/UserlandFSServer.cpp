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

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "beos_fs_cache.h"
#include "beos_fs_interface.h"
#include "BeOSKernelFileSystem.h"
#include "Compatibility.h"
#include "Debug.h"
#include "DispatcherDefs.h"
#include "FileSystem.h"
#include "FSInfo.h"
#include "HaikuKernelFileSystem.h"
#include "RequestThread.h"
#include "ServerDefs.h"

static const int32 kRequestThreadCount = 10;

static const int32 kMaxBlockCacheBlocks = 16384;

// constructor
UserlandFSServer::UserlandFSServer(const char* signature)
	: BApplication(signature),
	  fAddOnImage(-1),
	  fFileSystem(NULL),
	  fNotificationRequestPort(NULL),
	  fRequestThreads(NULL),
	  fBlockCacheInitialized(false)
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
	if (fBlockCacheInitialized)
		beos_shutdown_block_cache();
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

	// create the FileSystem interface
	// BeOS kernel interface
	if (_CreateBeOSKernelInterface(fileSystem, fAddOnImage, &fFileSystem)
			== B_OK) {
		// BeOS interface
	} else if (_CreateHaikuKernelInterface(fileSystem, fAddOnImage,
			&fFileSystem) == B_OK) {
		// Haiku interface
	} else {
		ERROR(("Add-on doesn't has a supported interface.\n"));
	}

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
		capabilities);

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

// _CreateBeOSKernelInterface
status_t
UserlandFSServer::_CreateBeOSKernelInterface(const char* fsName, image_id image,
	FileSystem** _fileSystem)
{
	// get the symbols "fs_entry" and "api_version"
	beos_vnode_ops* fsOps;
	status_t error = get_image_symbol(image, "fs_entry", B_SYMBOL_TYPE_DATA,
		(void**)&fsOps);
	if (error != B_OK)
		RETURN_ERROR(error);
	int32* apiVersion;
	error = get_image_symbol(image, "api_version", B_SYMBOL_TYPE_DATA,
		(void**)&apiVersion);
	if (error != B_OK)
		RETURN_ERROR(error);

	// check api version
	if (*apiVersion != BEOS_FS_API_VERSION)
		RETURN_ERROR(B_ERROR);

	// create the file system
	BeOSKernelFileSystem* fileSystem = new(nothrow) BeOSKernelFileSystem(fsOps);
	if (!fileSystem)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<BeOSKernelFileSystem> fsDeleter(fileSystem);

	// init the block cache
	error = beos_init_block_cache(kMaxBlockCacheBlocks, 0);
	if (error != B_OK)
		RETURN_ERROR(error);
	fBlockCacheInitialized = true;

	// everything went fine
	fsDeleter.Detach();
	*_fileSystem = fileSystem;
	return B_OK;
}

// _CreateHaikuKernelInterface
status_t
UserlandFSServer::_CreateHaikuKernelInterface(const char* fsName,
	image_id image, FileSystem** _fileSystem)
{
	// get the modules
	module_info** modules;
	status_t error = get_image_symbol(image, "modules", B_SYMBOL_TYPE_DATA,
		(void**)&modules);
	if (error != B_OK)
		RETURN_ERROR(error);

	// module name must match "file_systems/<name>/v1"
	char moduleName[B_PATH_NAME_LENGTH];
	snprintf(moduleName, sizeof(moduleName), "file_systems/%s/v1", fsName);

	// find the module
	file_system_module_info* module = NULL;
	for (int32 i = 0; modules[i]->name; i++) {
		if (strcmp(modules[i]->name, moduleName) == 0) {
			module = (file_system_module_info*)modules[i];
			break;
		}
	}
	if (!module)
		RETURN_ERROR(B_ERROR);

	// create the file system
	HaikuKernelFileSystem* fileSystem
		= new(nothrow) HaikuKernelFileSystem(module);
	if (!fileSystem)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<HaikuKernelFileSystem> fsDeleter(fileSystem);

	// init the FS
	error = fileSystem->Init();
	if (error != B_OK)
		return error;

	// everything went fine
	fsDeleter.Detach();
	*_fileSystem = fileSystem;
	return B_OK;
}
