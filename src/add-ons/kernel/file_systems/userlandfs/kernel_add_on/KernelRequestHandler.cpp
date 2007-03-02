// KernelRequestHandler.cpp

#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "KernelRequestHandler.h"
#include "RequestPort.h"
#include "Requests.h"
#include "Volume.h"

#include <NodeMonitor.h>

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
		case NOTIFY_QUERY_REQUEST:
			return _HandleRequest((NotifyQueryRequest*)request);
		// vnodes
		case GET_VNODE_REQUEST:
			return _HandleRequest((GetVNodeRequest*)request);
		case PUT_VNODE_REQUEST:
			return _HandleRequest((PutVNodeRequest*)request);
		case NEW_VNODE_REQUEST:
			return _HandleRequest((NewVNodeRequest*)request);
		case PUBLISH_VNODE_REQUEST:
			return _HandleRequest((PublishVNodeRequest*)request);
		case REMOVE_VNODE_REQUEST:
			return _HandleRequest((RemoveVNodeRequest*)request);
		case UNREMOVE_VNODE_REQUEST:
			return _HandleRequest((UnremoveVNodeRequest*)request);
		case GET_VNODE_REMOVED_REQUEST:
			return _HandleRequest((GetVNodeRemovedRequest*)request);
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
	if (fVolume && request->device != fVolume->GetID())
		result = B_BAD_VALUE;

	// get the names
	// name
	char* name = (char*)request->name.GetData();
	int32 nameLen = request->name.GetSize();
	if (name && (nameLen <= 0))
		name = NULL;
	else if (name)
		name[nameLen - 1] = '\0';	// NULL-terminate to be safe

	// old name
	char* oldName = (char*)request->oldName.GetData();
	int32 oldNameLen = request->oldName.GetSize();
	if (oldName && (oldNameLen <= 0))
		oldName = NULL;
	else if (oldName)
		oldName[oldNameLen - 1] = '\0';	// NULL-terminate to be safe

	// check the names
	if (result == B_OK) {
		switch (request->operation) {
			case B_ENTRY_MOVED:
				if (!oldName) {
					ERROR(("NotifyListenerRequest: NULL oldName for "
						"B_ENTRY_MOVED\n"));
					result = B_BAD_VALUE; 
					break;
				}
				// fall through...
			case B_ENTRY_CREATED:
			case B_ENTRY_REMOVED:
			case B_ATTR_CHANGED:
				if (!name) {
					ERROR(("NotifyListenerRequest: NULL name for opcode: %ld\n",
						request->operation));
					result = B_BAD_VALUE; 
				}
				break;
			case B_STAT_CHANGED:
				break;
		}
	}

	// execute the request
	if (result == B_OK) {
		switch (request->operation) {
			case B_ENTRY_CREATED:
				PRINT(("notify_entry_created(%ld, %lld, \"%s\", %lld)\n",
					request->device, request->directory, name, request->node));
				result = notify_entry_created(request->device,
					request->directory, name, request->node);
				break;

			case B_ENTRY_REMOVED:
				PRINT(("notify_entry_removed(%ld, %lld, \"%s\", %lld)\n",
					request->device, request->directory, name, request->node));
				result = notify_entry_removed(request->device,
					request->directory, name, request->node);
				break;

			case B_ENTRY_MOVED:
				PRINT(("notify_entry_moved(%ld, %lld, \"%s\", %lld, \"%s\", "
					"%lld)\n", request->device, request->oldDirectory, oldName,
					request->directory, name, request->node));
				result = notify_entry_moved(request->device,
					request->oldDirectory, oldName, request->directory, name,
					request->node);
				break;

			case B_STAT_CHANGED:
				PRINT(("notify_stat_changed(%ld, %lld, 0x%lx)\n",
					request->device, request->node, request->details));
				result = notify_stat_changed(request->device, request->node,
					request->details);
				break;

			case B_ATTR_CHANGED:
				PRINT(("notify_attribute_changed(%ld, %lld, \"%s\", 0x%lx)\n",
					request->device, request->node, name,
					(int32)request->details));
				result = notify_attribute_changed(request->device,
					request->node, name, (int32)request->details);
				break;

			default:
				ERROR(("NotifyQueryRequest: unsupported operation: %ld\n",
					request->operation));
				result = B_BAD_VALUE; 
				break;
		}
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
		if (request->unspecifiedEvent) {
			// old style add-ons can't provide an event argument; we shoot
			// all events
			notify_select_event(request->sync, request->ref, B_SELECT_READ);
			notify_select_event(request->sync, request->ref, B_SELECT_WRITE);
			notify_select_event(request->sync, request->ref, B_SELECT_ERROR);
		} else {
			PRINT(("notify_select_event(%p, %lu, %d)\n", request->sync,
				request->ref, (int)request->event));
			notify_select_event(request->sync, request->ref, request->event);
		}
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
KernelRequestHandler::_HandleRequest(NotifyQueryRequest* request)
{
	// check and executed the request
	status_t result = B_OK;
	if (fVolume && request->device != fVolume->GetID())
		result = B_BAD_VALUE;

	// check the name
	char* name = (char*)request->name.GetData();
	int32 nameLen = request->name.GetSize();
	if (!name || nameLen <= 0) {
		ERROR(("NotifyQueryRequest: NULL name!\n"));
		result = B_BAD_VALUE; 
	} else
		name[nameLen - 1] = '\0';	// NULL-terminate to be safe

	// execute the request
	if (result == B_OK) {
		switch (request->operation) {
			case B_ENTRY_CREATED:
				PRINT(("notify_query_entry_created(%ld, %ld, %ld, %lld,"
					" \"%s\", %lld)\n", request->port, request->token,
					request->device, request->directory, name, request->node));
				result = notify_query_entry_created(request->port,
					request->token, request->device, request->directory, name,
					request->node);
				break;

			case B_ENTRY_REMOVED:
				PRINT(("notify_query_entry_removed(%ld, %ld, %ld, %lld,"
					" \"%s\", %lld)\n", request->port, request->token,
					request->device, request->directory, name, request->node));
				result = notify_query_entry_removed(request->port,
					request->token, request->device, request->directory, name,
					request->node);
				break;

			case B_ENTRY_MOVED:
			default:
				ERROR(("NotifyQueryRequest: unsupported operation: %ld\n",
					request->operation));
				result = B_BAD_VALUE; 
				break;
		}
	}

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	NotifyQueryReply* reply;
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
KernelRequestHandler::_HandleRequest(PublishVNodeRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	if (result == B_OK)
		result = volume->PublishVNode(request->vnid, request->node);

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	PublishVNodeReply* reply;
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
KernelRequestHandler::_HandleRequest(GetVNodeRemovedRequest* request)
{
	// check and executed the request
	Volume* volume = NULL;
	status_t result = _GetVolume(request->nsid, &volume);
	VolumePutter _(volume);
	bool removed = false;
	if (result == B_OK)
		result = volume->GetVNodeRemoved(request->vnid, &removed);

	// prepare the reply
	RequestAllocator allocator(fPort->GetPort());
	GetVNodeRemovedReply* reply;
	status_t error = AllocateRequest(allocator, &reply);
	if (error != B_OK)
		return error;

	reply->error = result;
	reply->removed = removed;

	// send the reply
	return fPort->SendRequest(&allocator);
}

// _GetVolume
status_t
KernelRequestHandler::_GetVolume(mount_id id, Volume** volume)
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

