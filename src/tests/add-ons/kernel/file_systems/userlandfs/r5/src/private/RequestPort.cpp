// RequestPort.cpp

#include <new>

#include "AutoDeleter.h"
#include "Debug.h"
#include "Request.h"
#include "RequestHandler.h"
#include "RequestPort.h"

// TODO: Limit the stacking of requests?

// AllocatorNode
struct RequestPort::AllocatorNode {
	AllocatorNode(Port* port) : allocator(port), previous(NULL) {}

	RequestAllocator	allocator;
	AllocatorNode*		previous;
};


// constructor
RequestPort::RequestPort(int32 size)
	: fPort(size),
	  fCurrentAllocatorNode(NULL)
{
}

// constructor
RequestPort::RequestPort(const Port::Info* info)
	: fPort(info),
	  fCurrentAllocatorNode(NULL)
{
}

// destructor
RequestPort::~RequestPort()
{
	while (fCurrentAllocatorNode)
		_PopAllocator();
}

// Close
void
RequestPort::Close()
{
	fPort.Close();
}

// InitCheck
status_t
RequestPort::InitCheck() const
{
	return fPort.InitCheck();
}

// GetPort
Port*
RequestPort::GetPort()
{
	return &fPort;
}

// GetPortInfo
const Port::Info*
RequestPort::GetPortInfo() const
{
	return fPort.GetInfo();
}

// SendRequest
status_t
RequestPort::SendRequest(RequestAllocator* allocator)
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		RETURN_ERROR(InitCheck());
	if (!allocator || allocator->GetRequest() != fPort.GetBuffer()
		|| allocator->GetRequestSize() < (int32)sizeof(Request)
		|| allocator->GetRequestSize() > fPort.GetCapacity()) {
		RETURN_ERROR(B_BAD_VALUE);
	}
	allocator->FinishDeferredInit();
//PRINT(("RequestPort::SendRequest(%lu)\n", allocator->GetRequest()->GetType()));
#if USER && !KERNEL_EMU
	if (!is_userland_request(allocator->GetRequest()->GetType())) {
		ERROR(("RequestPort::SendRequest(%lu): request is not a userland "
			"request\n", allocator->GetRequest()->GetType()));
		debugger("Request is not a userland request.");
	}
#else
	if (!is_kernel_request(allocator->GetRequest()->GetType())) {
		ERROR(("RequestPort::SendRequest(%lu): request is not a userland "
			"request\n", allocator->GetRequest()->GetType()));
		debugger("Request is not a userland request.");
	}
#endif
	RETURN_ERROR(fPort.Send(allocator->GetRequestSize()));
}

// SendRequest
status_t
RequestPort::SendRequest(RequestAllocator* allocator,
	RequestHandler* handler, Request** reply, bigtime_t timeout)
{
	status_t error = SendRequest(allocator);
	if (error != B_OK)
		return error;
	return HandleRequests(handler, reply, timeout);
}

// ReceiveRequest
//
// The caller is responsible for calling ReleaseRequest() with the request.
status_t
RequestPort::ReceiveRequest(Request** request, bigtime_t timeout)
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		RETURN_ERROR(InitCheck());
	if (!request)
		RETURN_ERROR(B_BAD_VALUE);
	// allocate a request allocator
	AllocatorNode* node = new(nothrow) AllocatorNode(&fPort);
	if (!node)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<AllocatorNode> deleter(node);
	// receive the message
	status_t error = fPort.Receive(timeout);
	if (error != B_OK) {
		if (error != B_TIMED_OUT && error != B_WOULD_BLOCK)
			RETURN_ERROR(error);
		return error;
	}
	// allocate the request
	error = node->allocator.ReadRequest();
	if (error != B_OK)
		RETURN_ERROR(error);
	// everything went fine: push the allocator
	*request = node->allocator.GetRequest();
	node->previous = fCurrentAllocatorNode;
	fCurrentAllocatorNode = node;
	deleter.Detach();
//PRINT(("RequestPort::RequestReceived(%lu)\n", (*request)->GetType()));
	return B_OK;
}

// HandleRequests
//
// If request is not NULL, the caller is responsible for calling
// ReleaseRequest() with the request. If it is NULL, the request will already
// be gone, when the method returns.
status_t
RequestPort::HandleRequests(RequestHandler* handler, Request** request,
	bigtime_t timeout)
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		RETURN_ERROR(InitCheck());
	if (!handler)
		RETURN_ERROR(B_BAD_VALUE);
	handler->SetPort(this);
	Request* currentRequest = NULL;
	do {
		if (currentRequest)
			ReleaseRequest(currentRequest);
		status_t error = ReceiveRequest(&currentRequest, timeout);
		if (error != B_OK)
			return error;
		// handle the request
		error = handler->HandleRequest(currentRequest);
		if (error != B_OK) {
			ReleaseRequest(currentRequest);
			RETURN_ERROR(error);
		}
	} while (!handler->IsDone());
	if (request)
		*request = currentRequest;
	else
		ReleaseRequest(currentRequest);
	return B_OK;
}

// ReleaseRequest
void
RequestPort::ReleaseRequest(Request* request)
{
	if (request && fCurrentAllocatorNode
		&& request == fCurrentAllocatorNode->allocator.GetRequest()) {
		_PopAllocator();
	}
}

// _PopAllocator
void
RequestPort::_PopAllocator()
{
	if (fCurrentAllocatorNode) {
		AllocatorNode* node = fCurrentAllocatorNode->previous;
		delete fCurrentAllocatorNode;
		fCurrentAllocatorNode = node;
	}
}

