// RequestConnection.cpp

#include <new>

#include <OS.h>

#include "Channel.h"
#include "Connection.h"
#include "RequestChannel.h"
#include "RequestConnection.h"
#include "RequestHandler.h"

// DownStreamThread
class RequestConnection::DownStreamThread {
public:
	DownStreamThread()
		: fThread(-1),
		  fConnection(NULL),
		  fChannel(NULL),
		  fHandler(NULL),
		  fTerminating(false)
	{
	}

	~DownStreamThread()
	{
		Terminate();
		delete fChannel;
	}

	status_t Init(RequestConnection* connection, Channel* channel,
		RequestHandler* handler)
	{
		if (!connection || !channel || !handler)
			return B_BAD_VALUE;
		fConnection = connection;
		fHandler = handler;
		fChannel = new(std::nothrow) RequestChannel(channel);
		if (!fChannel)
			return B_NO_MEMORY;
		fThread = spawn_thread(_LoopEntry, "down stream thread",
			B_NORMAL_PRIORITY, this);
		if (fThread < 0)
			return fThread;
		return B_OK;
	}

	void Run()
	{
		resume_thread(fThread);
	}

	void Terminate()
	{
		fTerminating = true;
		if (fThread > 0 && find_thread(NULL) != fThread) {
			int32 result;
			wait_for_thread(fThread, &result);
		}
	}

	RequestChannel* GetRequestChannel()
	{
		return fChannel;
	}

private:
	static int32 _LoopEntry(void* data)
	{
		return ((DownStreamThread*)data)->_Loop();
	}

	int32 _Loop()
	{
		while (!fTerminating) {
			Request* request;
			status_t error = fChannel->ReceiveRequest(&request);
			if (error == B_OK) {
				error = fHandler->HandleRequest(request, fChannel);
				delete request;
			}
			if (error != B_OK)
				fTerminating = fConnection->DownStreamChannelError(this, error);
		}
		return 0;
	}

private:
	thread_id			fThread;
	RequestConnection*	fConnection;
	RequestChannel*		fChannel;
	RequestHandler*		fHandler;
	volatile bool		fTerminating;
};


// RequestConnection

// constructor
RequestConnection::RequestConnection(Connection* connection,
	RequestHandler* requestHandler, bool ownsRequestHandler)
	: fConnection(connection),
	  fRequestHandler(requestHandler),
	  fOwnsRequestHandler(ownsRequestHandler),
	  fThreads(NULL),
	  fThreadCount(0),
	  fTerminationCount(0)
{
}

// destructor
RequestConnection::~RequestConnection()
{
	Close();
	delete[] fThreads;
	delete fConnection;
	if (fOwnsRequestHandler)
		delete fRequestHandler;
}

// Init
status_t
RequestConnection::Init()
{
	// check parameters
	if (!fConnection || !fRequestHandler)
		return B_BAD_VALUE;
	if (fConnection->CountDownStreamChannels() < 1)
		return B_ERROR;
	// create a thread per down-stream channel
	fThreadCount = fConnection->CountDownStreamChannels();
	fThreads = new(std::nothrow) DownStreamThread[fThreadCount];
	if (!fThreads)
		return B_NO_MEMORY;
	// initialize the threads
	for (int32 i = 0; i < fThreadCount; i++) {
		status_t error = fThreads[i].Init(this,
			fConnection->DownStreamChannelAt(i), fRequestHandler);
		if (error != B_OK)
			return error;
	}
	// run the threads
	for (int32 i = 0; i < fThreadCount; i++)
		fThreads[i].Run();
	return B_OK;
}

// Close
void
RequestConnection::Close()
{
	atomic_add(&fTerminationCount, 1);
	if (fConnection)
		fConnection->Close();
	if (fThreads) {
		for (int32 i = 0; i < fThreadCount; i++)
			fThreads[i].Terminate();
	}
}

// SendRequest
status_t
RequestConnection::SendRequest(Request* request, Request** reply)
{
	return _SendRequest(request, reply, NULL);
}

// SendRequest
status_t
RequestConnection::SendRequest(Request* request, RequestHandler* replyHandler)
{
	if (!replyHandler)
		return B_BAD_VALUE;
	return _SendRequest(request, NULL, replyHandler);
}

// DownStreamChannelError
bool
RequestConnection::DownStreamChannelError(DownStreamThread* thread,
	status_t error)
{
	if (atomic_add(&fTerminationCount, 1) == 0 && fRequestHandler) {
		ConnectionBrokenRequest request;
		request.error = error;
		fRequestHandler->HandleRequest(&request, thread->GetRequestChannel());
	}
	return true;
}

// _SendRequest
status_t
RequestConnection::_SendRequest(Request* request, Request** _reply,
	RequestHandler* replyHandler)
{
	// check parameters
	if (!request)
		return B_BAD_VALUE;
	// get a channel
	Channel* channel = NULL;
	status_t error = fConnection->GetUpStreamChannel(&channel);
	if (error != B_OK)
		return error;
	// send the request
	RequestChannel requestChannel(channel);
	error = requestChannel.SendRequest(request);
	// receive the reply
	Request* reply = NULL;
	if (error == B_OK && (_reply || replyHandler)) {
		error = requestChannel.ReceiveRequest(&reply);
		// handle the reply
		if (error == B_OK) {
			if (replyHandler)
				error = replyHandler->HandleRequest(reply, &requestChannel);
			if (error == B_OK && _reply)
				*_reply = reply;
			else
				delete reply;
		}
	}
	// cleanup
	if (fConnection->PutUpStreamChannel(channel) != B_OK) {
		// Ugh! A serious error. Probably insufficient memory.
		delete channel;
	}
	return error;
}

