// KernelRequestHandler.cpp

#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "KernelRequestHandler.h"
#include "RequestPort.h"
#include "Requests.h"
#include "Volume.h"

// VolumePutter
class VolumePutter {
public:
	VolumePutter(Volume* volume) : fVolume(volume) {}
	~VolumePutter()
	{
		if (fVolume)
			fVolume->RemoveReference();
	}

private:
	Volume	*fVolume;
};

// constructor
KernelRequestHandler::KernelRequestHandler(Volume* volume, uint32 expectedReply)
	: RequestHandler(),
	  fFileSystem(volume->GetFileSystem()),
	  fVolume(volume),
	  fExpectedReply(expectedReply)
{
}

// constructor
KernelRequestHandler::KernelRequestHandler(FileSystem* fileSystem,
	uint32 expectedReply)
	: RequestHandler(),
	  fFileSystem(fileSystem),
	  fVolume(NULL),
	  fExpectedReply(expectedReply)
{
}

// destructor
KernelRequestHandler::~KernelRequestHandler()
{
}

// HandleRequest
status_t
KernelRequestHandler::HandleRequest(Request* request)
{
	if (request->GetType() == fExpectedReply) {
		fDone = true;
		return B_OK;
	}
	switch (request->GetType()) {
		// notifications
		case NOTIFY_LISTENER_REQUEST:
			return _HandleRequest((NotifyListenerRequest*)request);
		case NOTIFY_SELECT_EVENT_REQUEST:
			return _HandleRequest((NotifySelectEventRequest*)request);
		case SEND_NOTIFICATION_REQUEST:
			return _HandleRequest((SendNotificationRequest*)request);
		// vnodes
		case GET_VNODE_REQUEST:
			return _HandleRequest((GetVNodeRequest*)request);
		case PUT_VNODE_REQUEST:
			return _HandleRequest((PutVNodeRequest*)request);
		case NEW_VNODE_REQUEST:
			return _HandleRequest((NewVNodeRequest*)request);
		case REMOVE_VNODE_REQUEST:
			return _HandleRequest((RemoveVNodeRequest*)request);
		case UNREMOVE_VNODE_REQUEST:
			return _HandleRequest((UnremoveVNodeRequest*)request);
		case IS_VNODE_REMOVED_REQUEST:
			return _HandleRequest((IsVNodeRemovedRequest*)request);
	}
PRINT(("KernelRequestHandler::HandleRequest(): unexpected request: %lu\n",
request->GetType()));
	return B_BAD_DATA;
}

// #pragma mark -
// #pragma mark ----- notifications -----

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(NotifyListenerRequest* request)
{
	// check and executed the request
	status_t result = B_OK;
	if (fVolume && request->nsid != fVolume->GetID())
		result = B_BAD_VALUE;
	// check the name
	char* name = (char*)request->name.GetData();
	int32 nameLen = request->name.GetSize();
	if (name && (nameLen <= 0 || strnlen(name, nameLen) < 1))
		name = NULL;
	else if (name)
		name[nameLen - 1] = '\0';
	if (!name) {
		switch (request->operation) {
			case B_ENTRY_CREATED:
			case B_ENTRY_MOVED:
			case B_ATTR_CHANGED:
				ERROR(("notify_listener(): NULL name for opcode: %ld\n",
					request->operation));
				result = B_BAD_VALUE; 
				break;
			case B_ENTRY_REMOVED:
			case B_STAT_CHANGED:
				break;
		}
	}
	// execute the request
	if (result == B_OK) {
		PRINT(("notify_listener(%ld, %ld, %Ld, %Ld, %Ld, `%s')\n",
			request->operation, request->nsid, request->vnida, request->vnidb,
			request->vnidc, name));
		result = notify_listener(request->operation, request->nsid,
			request->vnida, request->vnidb, request->vnidc, name);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	NotifyListenerReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(NotifySelectEventRequest* request)
{
	// check and executed the request
	status_t result = B_OK;
	if (fFileSystem->KnowsSelectSyncEntry(request->sync)) {
		PRINT(("notify_select_event(%p, %lu)\n", request->sync, request->ref));
		notify_select_event(request->sync, request->ref);
	} else
		result = B_BAD_VALUE;
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	NotifySelectEventReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(SendNotificationRequest* request)
{
	// check and executed the request
	status_t result = B_OK;
	if (fVolume && request->nsida != fVolume->GetID()
		&& request->nsidb != fVolume->GetID()) {
		result = B_BAD_VALUE;
	}
	// check the name
	char* name = (char*)request->name.GetData();
	int32 nameLen = request->name.GetSize();
	if (name && (nameLen <= 0 || strnlen(name, nameLen) < 1))
		name = NULL;
	else if (name)
		name[nameLen - 1] = '\0';
	if (!name) {
		switch (request->operation) {
			case B_ENTRY_CREATED:
			case B_ENTRY_MOVED:
				ERROR(("send_notification(): NULL name for opcode: %ld\n",
					request->operation));
				result = B_BAD_VALUE; 
				break;
			case B_ENTRY_REMOVED:
				break;
		}
	}
	// execute the request
	if (result == B_OK) {
		PRINT(("send_notification(%ld, %ld, %lu, %ld, %ld, %ld, %Ld, %Ld, %Ld, "
			"`%s')\n", request->port, request->token, request->what,
			request->operation, request->nsida, request->nsidb, request->vnida,
			request->vnidb, request->vnidc, name));
		result = send_notification(request->port, request->token, request->what,
			request->operation, request->nsida, request->nsidb, request->vnida,
			request->vnidb, request->vnidc, name);
	}
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	SendNotificationReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// #pragma mark -
// #pragma mark ----- vnodes -----

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(GetVNodeRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	void* node;
	if (result == B_OK)
		result = volume->GetVNode(request->vnid, &node);
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	GetVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	reply->node = node;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(PutVNodeRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	if (result == B_OK)
		result = volume->PutVNode(request->vnid);
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	PutVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(NewVNodeRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	if (result == B_OK)
		result = volume->NewVNode(request->vnid, request->node);
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	NewVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(RemoveVNodeRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	if (result == B_OK)
		result = volume->RemoveVNode(request->vnid);
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	RemoveVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(UnremoveVNodeRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	if (result == B_OK)
		result = volume->UnremoveVNode(request->vnid);
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	UnremoveVNodeReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _HandleRequest
status_t
KernelRequestHandler::_HandleRequest(IsVNodeRemovedRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	if (result == B_OK)
		result = volume->IsVNodeRemoved(request->vnid);
	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	IsVNodeRemovedReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;
	reply->error = (result < 0 ? result : B_OK);
	reply->result = result;
	// send the reply
	return fPort->SendRequest(&allocator);
}

// _GetVolume
status_t
KernelRequestHandler::_GetVolume(nspace_id id, Volume** volume)
{
	if (fVolume) {
		if (fVolume->GetID() != id) {
			*volume = NULL;
			return B_BAD_VALUE;
		}
		fVolume->AddReference();
		*volume = fVolume;
		return B_OK;
	}
	*volume = fFileSystem->GetVolume(id);
	return (*volume ? B_OK : B_BAD_VALUE);
}

