// FileSystemInitializer.cpp

#include "FileSystemInitializer.h"

#include <new>

#include "FileSystem.h"
#include "RequestAllocator.h"
#include "RequestPort.h"
#include "Requests.h"
#include "SingleReplyRequestHandler.h"

using std::nothrow;

// constructor
FileSystemInitializer::FileSystemInitializer(const char* name,
	RequestPort* initPort)
	: fName(name),
	  fInitPort(initPort),
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
	// prepare the request
	RequestAllocator allocator(fInitPort->GetPort());
	FSConnectRequest* request;
	status_t error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		RETURN_ERROR(error);
	error = allocator.AllocateString(request->fsName, fName);
	if (error != B_OK)
		RETURN_ERROR(error);

	// send the request
	SingleReplyRequestHandler handler(FS_CONNECT_REPLY);
	FSConnectReply* reply;
	error = fInitPort->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	RequestReleaser requestReleaser(fInitPort, reply);

	// process the reply
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	// get the port infos
	int32 count = reply->portInfoCount;
	if (count < 2)
		RETURN_ERROR(B_BAD_DATA);
	if (reply->portInfos.GetSize() != count * (int32)sizeof(Port::Info))
		RETURN_ERROR(B_BAD_DATA);
	Port::Info* infos = (Port::Info*)reply->portInfos.GetData();

	// create and init the FileSystem
	fFileSystem = new(nothrow) FileSystem();
	if (!fFileSystem)
		return B_NO_MEMORY;

	error = fFileSystem->Init(fName, infos, count, reply->capabilities);
	if (error != B_OK)
		return B_ERROR;

	return B_OK;
}
