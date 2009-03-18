/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

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
#include "FileSystem.h"
#include "RequestThread.h"
#include "ServerDefs.h"
#include "UserlandFSDefs.h"


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
UserlandFSServer::Init(const char* fileSystem, port_id port)
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
	fNotificationRequestPort = new(std::nothrow) RequestPort(kRequestPortSize);
	if (!fNotificationRequestPort)
		RETURN_ERROR(B_NO_MEMORY);
	error = fNotificationRequestPort->InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// now create the request threads
	fRequestThreads = new(std::nothrow) RequestThread[kRequestThreadCount];
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

	// finally announce our existence
	error = _Announce(fileSystem, port);
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


// _Announce
status_t
UserlandFSServer::_Announce(const char* fsName, port_id port)
{
	// if not given, create a port
	if (port < 0) {
		char portName[B_OS_NAME_LENGTH];
		snprintf(portName, sizeof(portName), "_userlandfs_%s", fsName);

		port = create_port(1, portName);
		if (port < 0)
			RETURN_ERROR(port);
	}

	// allocate stack space for the FS initialization info
	const size_t bufferSize = sizeof(fs_init_info)
		+ sizeof(Port::Info) * (kRequestThreadCount + 1);
	char buffer[bufferSize];
	fs_init_info* info = (fs_init_info*)buffer;

	// get the port infos
	info->portInfoCount = kRequestThreadCount + 1;
	info->portInfos[0] = *fNotificationRequestPort->GetPortInfo();
	for (int32 i = 0; i < kRequestThreadCount; i++)
		info->portInfos[i + 1] = *fRequestThreads[i].GetPortInfo();

	// FS capabilities
	fFileSystem->GetCapabilities(info->capabilities);
	info->clientFSType = fFileSystem->GetClientFSType();

	// send the info to our port
	RETURN_ERROR(write_port(port, 0, buffer, bufferSize));
}
