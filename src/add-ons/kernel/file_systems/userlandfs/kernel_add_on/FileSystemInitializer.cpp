/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FileSystemInitializer.h"

#include <new>

#include <image.h>
#include <OS.h>

#include <port.h>
#include <team.h>

#include "AutoDeleter.h"

#include "FileSystem.h"
#include "RequestAllocator.h"
#include "RequestPort.h"
#include "Requests.h"
#include "SingleReplyRequestHandler.h"
#include "UserlandFSDefs.h"


// constructor
FileSystemInitializer::FileSystemInitializer(const char* name)
	:
	BReferenceable(),
	fName(name),
	fFileSystem(NULL)
{
	// Note: We don't copy the name. It's only needed in FirstTimeInit() and
	// the UserlandFS makes sure it is valid until then.
}


// destructor
FileSystemInitializer::~FileSystemInitializer()
{
	delete fFileSystem;
}


// FirstTimeInit
status_t
FileSystemInitializer::FirstTimeInit()
{
	// First check whether a server for this FS is already loaded. Look for a
	// port with the respective name.
	char portName[B_OS_NAME_LENGTH];
	snprintf(portName, sizeof(portName), "_userlandfs_%s", fName);

	port_id port = find_port(portName);
	if (port >= 0)
		return _Init(port);

	// We have to start the server ourselves. First create the port we're going
	// to use.
	port = create_port(1, portName);
	if (port < 0)
		RETURN_ERROR(port);

	// prepare the command line arguments
	char portID[16];
	snprintf(portID, sizeof(portID), "%ld", port);

	const char* args[4] = {
		"/system/servers/userlandfs_server",
		fName,
		portID,
		NULL
	};

	// start the server
	thread_id thread = load_image_etc(3, args, NULL, B_NORMAL_PRIORITY,
		B_SYSTEM_TEAM, 0);
	if (thread < 0) {
		delete_port(port);
		RETURN_ERROR(thread);
	}

	// let it own the port and start the team
	status_t error = set_port_owner(port, thread);
	if (error == B_OK)
		resume_thread(thread);

	// and do the initialization
	if (error == B_OK)
		error = _Init(port);

	if (error != B_OK) {
		kill_team(thread);
		delete_port(port);
		RETURN_ERROR(error);
	}

	return B_OK;
}


void
FileSystemInitializer::LastReferenceReleased()
{
	// don't delete
}


status_t
FileSystemInitializer::_Init(port_id port)
{
	// allocate a buffer for the FS info
	const size_t bufferSize = 1024;
	fs_init_info* info = (fs_init_info*)malloc(bufferSize);
	MemoryDeleter _(info);

	// Read the info -- we peek only, so it won't go away and we can do the
	// initialization again, in case we get completely unloaded are reloaded
	// later.
	ssize_t bytesRead = read_port_etc(port, NULL, info, bufferSize,
	    B_PEEK_PORT_MESSAGE | B_CAN_INTERRUPT, 0);
	if (bytesRead < 0)
		RETURN_ERROR(bytesRead);

	// sanity check
	if ((size_t)bytesRead < sizeof(fs_init_info)
		|| info->portInfoCount < 2
		|| (size_t)bytesRead < sizeof(fs_init_info)
			+ info->portInfoCount * sizeof(Port::Info)) {
		RETURN_ERROR(B_BAD_DATA);
	}

	// get the port's team
	port_info portInfo;
	status_t error = get_port_info(port, &portInfo);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create and init the FileSystem
	fFileSystem = new(std::nothrow) FileSystem;
	if (!fFileSystem)
		return B_NO_MEMORY;

	error = fFileSystem->Init(fName, portInfo.team, info->portInfos,
		info->portInfoCount, info->capabilities);
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}
