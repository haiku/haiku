/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCServer.h"

#include <stdlib.h>

#include <util/AutoLock.h>
#include <util/Random.h>

#include "RPCCallbackServer.h"
#include "RPCReply.h"


using namespace RPC;


Request::Request()
{
	mutex_init(&fEventLock, "nfs4 Request");
}


Request::~Request()
{
	mutex_lock(&fEventLock);
	mutex_destroy(&fEventLock);
}


RequestManager::RequestManager()
	:
	fQueueHead(NULL),
	fQueueTail(NULL)
{
	mutex_init(&fLock, NULL);
}


RequestManager::~RequestManager()
{
	mutex_destroy(&fLock);
}


void
RequestManager::AddRequest(Request* request)
{
	ASSERT(request != NULL);

	MutexLocker _(fLock);
	if (fQueueTail != NULL)
		fQueueTail->fNext = request;
	else 
		fQueueHead = request;
	fQueueTail = request;
	request->fNext = NULL;
}


Request*
RequestManager::FindRequest(uint32 xid)
{
	MutexLocker _(fLock);
	Request* req = fQueueHead;
	Request* prev = NULL;
	while (req != NULL) {
		if (req->fXID == xid) {
			if (prev != NULL)
				prev->fNext = req->fNext;
			if (fQueueTail == req)
				fQueueTail = prev;
			if (fQueueHead == req)
				fQueueHead = req->fNext;

			return req;
		}

		prev = req;
		req = req->fNext;
	}

	return NULL;
}


Server::Server(Connection* connection, PeerAddress* address)
	:
	fConnection(connection),
	fAddress(address),
	fPrivateData(NULL),
	fCallback(NULL),
	fRepairCount(0),
	fXID(get_random<uint32>())
{
	ASSERT(connection != NULL);
	ASSERT(address != NULL);

	mutex_init(&fCallbackLock, NULL);
	mutex_init(&fRepairLock, NULL);

	_StartListening();
}


Server::~Server()
{
	if (fCallback != NULL)
		fCallback->CBServer()->UnregisterCallback(fCallback);
	delete fCallback;
	mutex_destroy(&fCallbackLock);
	mutex_destroy(&fRepairLock);

	delete fPrivateData;

	fThreadCancel = true;
	fConnection->Disconnect();

	status_t result;
	wait_for_thread(fThread, &result);

	delete fConnection;
}


status_t
Server::_StartListening()
{
	fThreadCancel = false;
	fThreadError = B_OK;
	fThread = spawn_kernel_thread(&Server::_ListenerThreadStart,
		"NFSv4 Listener", B_NORMAL_PRIORITY, this);
	if (fThread < B_OK)
		return fThread;

	status_t result = resume_thread(fThread);
	if (result != B_OK) {
		kill_thread(fThread);
		return result;
	}

	return B_OK;
}


status_t
Server::SendCallAsync(Call* call, Reply** reply, Request** request)
{
	ASSERT(call != NULL);
	ASSERT(reply != NULL);
	ASSERT(request != NULL);

	if (fThreadError != B_OK && Repair() != B_OK)
		return fThreadError;

	Request* req = new(std::nothrow) Request;
	if (req == NULL)
		return B_NO_MEMORY;

	uint32 xid = _GetXID();
	call->SetXID(xid);
	req->fXID = xid;
	req->fReply = reply;
	req->fEvent.Init(&req->fEvent, NULL);
	req->fDone = false;
	req->fError = B_OK;
	req->fNext = NULL;

	fRequests.AddRequest(req);

	*request = req;
	status_t error = ResendCallAsync(call, req);
	if (error != B_OK)
		delete req;
	return error;
}


status_t
Server::ResendCallAsync(Call* call, Request* request)
{
	ASSERT(call != NULL);
	ASSERT(request != NULL);

	if (fThreadError != B_OK && Repair() != B_OK) {
		fRequests.FindRequest(request->fXID);
		return fThreadError;
	}

	XDR::WriteStream& stream = call->Stream();
	status_t result = fConnection->Send(stream.Buffer(), stream.Size());
	if (result != B_OK) {
		fRequests.FindRequest(request->fXID);
		return result;
	}

	return B_OK;
}


status_t
Server::WakeCall(Request* request)
{
	ASSERT(request != NULL);

	Request* req = fRequests.FindRequest(request->fXID);
	if (req == NULL)
		return B_OK;

	request->fError = B_IO_ERROR;
	*request->fReply = NULL;
	request->fDone = true;
	mutex_lock(&request->fEventLock);
		// don't let the sending thread free request->fEvent until NotifyAll returns
	request->fEvent.NotifyAll();
	mutex_unlock(&request->fEventLock);

	return B_OK;
}


status_t
Server::Repair()
{
	uint32 thisRepair = fRepairCount;

	MutexLocker _(fRepairLock);
	if (fRepairCount != thisRepair)
		return B_OK;

	fThreadCancel = true;

	status_t result = fConnection->Reconnect();
	if (result != B_OK)
		return result;

	wait_for_thread(fThread, &result);
	result = _StartListening();

	if (result == B_OK)
		fRepairCount++;

	return result;
}


Callback*
Server::GetCallback()
{
	MutexLocker _(fCallbackLock);

	if (fCallback == NULL) {
		fCallback = new(std::nothrow) Callback(this);
		if (fCallback == NULL)
			return NULL;

		CallbackServer* server = CallbackServer::Get(this);
		if (server == NULL) {
			delete fCallback;
			return NULL;
		}

		if (server->RegisterCallback(fCallback) != B_OK) {
			delete fCallback;
			return NULL;
		}
	}

	return fCallback;
}


uint32
Server::_GetXID()
{
	return static_cast<uint32>(atomic_add(&fXID, 1));
}


status_t
Server::_Listener()
{
	status_t result;
	uint32 size;
	void* buffer = NULL;

	while (!fThreadCancel) {
		result = fConnection->Receive(&buffer, &size);
		if (result == B_NO_MEMORY)
			continue;
		else if (result != B_OK) {
			fThreadError = result;
			return result;
		}

		ASSERT(buffer != NULL && size > 0);
		Reply* reply = new(std::nothrow) Reply(buffer, size);
		if (reply == NULL) {
			free(buffer);
			continue;
		}

		Request* req = fRequests.FindRequest(reply->GetXID());
		if (req != NULL) {
			*req->fReply = reply;
			req->fDone = true;
			mutex_lock(&req->fEventLock);
				// don't let the sending thread free req->fEvent until NotifyAll returns
			req->fEvent.NotifyAll();
			mutex_unlock(&req->fEventLock);
		} else
			delete reply;
	}

	return B_OK;
}


status_t
Server::_ListenerThreadStart(void* object)
{
	ASSERT(object != NULL);

	Server* server = reinterpret_cast<Server*>(object);
	return server->_Listener();
}


ServerManager::ServerManager()
	:
	fRoot(NULL)
{
	mutex_init(&fLock, NULL);
}


ServerManager::~ServerManager()
{
	mutex_destroy(&fLock);
}


status_t
ServerManager::Acquire(Server** _server, AddressResolver* resolver,
	ProgramData* (*createPrivateData)(Server*))
{
	PeerAddress address;
	status_t result;

	while ((result = resolver->GetNextAddress(&address)) == B_OK) {
		result = _Acquire(_server, address, createPrivateData);
		if (result == B_OK)
			break;
	}

	return result;
}


status_t
ServerManager::_Acquire(Server** _server, const PeerAddress& address,
	ProgramData* (*createPrivateData)(Server*))
{
	ASSERT(_server != NULL);
	ASSERT(createPrivateData != NULL);

	status_t result;

	MutexLocker locker(fLock);
	ServerNode* node = _Find(address);
	if (node != NULL) {
		node->fRefCount++;
		*_server = node->fServer;

		return B_OK;
	}

	node = new(std::nothrow) ServerNode;
	if (node == NULL)
		return B_NO_MEMORY;

	node->fID = address;

	Connection* conn;
	result = Connection::Connect(&conn, address);
	if (result != B_OK) {
		delete node;
		return result;
	}

	node->fServer = new Server(conn, &node->fID);
	if (node->fServer == NULL) {
		delete node;
		delete conn;
		return B_NO_MEMORY;
	}
	node->fServer->SetPrivateData(createPrivateData(node->fServer));

	node->fRefCount = 1;
	node->fLeft = node->fRight = NULL;

	ServerNode* nd = _Insert(node);
	if (nd != node) {
		nd->fRefCount++;

		delete node->fServer;
		delete node;
		*_server = nd->fServer;
		return B_OK;
	}

	*_server = node->fServer;
	return B_OK;
}


void
ServerManager::Release(Server* server)
{
	ASSERT(server != NULL);

	MutexLocker _(fLock);
	ServerNode* node = _Find(server->ID());
	if (node != NULL) {
		node->fRefCount--;

		if (node->fRefCount == 0) {
			_Delete(node);
			delete node->fServer;
			delete node;
		}
	}
}


ServerNode*
ServerManager::_Find(const PeerAddress& address)
{
	ServerNode* node = fRoot;
	while (node != NULL) {
		if (node->fID == address)
			return node;
		if (node->fID < address)
			node = node->fRight;
		else
			node = node->fLeft;
	}

	return node;
}


void
ServerManager::_Delete(ServerNode* node)
{
	ASSERT(node != NULL);

	bool found = false;
	ServerNode* previous = NULL;
	ServerNode* current = fRoot;
	while (current != NULL) {
		if (current->fID == node->fID) {
			found = true;
			break;
		}

		if (current->fID < node->fID) {
			previous = current;
			current = current->fRight;
		} else {
			previous = current;
			current = current->fLeft;
		}
	}

	if (!found)
		return;

	if (previous == NULL)
		fRoot = NULL;
	else if (current->fLeft == NULL && current->fRight == NULL) {
		if (previous->fID < node->fID)
			previous->fRight = NULL;
		else
			previous->fLeft = NULL;
	} else if (current->fLeft != NULL && current->fRight == NULL) {
		if (previous->fID < node->fID)
			previous->fRight = current->fLeft;
		else
			previous->fLeft = current->fLeft;
	} else if (current->fLeft == NULL && current->fRight != NULL) {
		if (previous->fID < node->fID)
			previous->fRight = current->fRight;
		else
			previous->fLeft = current->fRight;
	} else {
		ServerNode* left_prev = current;
		ServerNode*	left = current->fLeft;

		while (left->fLeft != NULL) {
			left_prev = left;
			left = left->fLeft;
		}

		if (previous->fID < node->fID)
			previous->fRight = left;
		else
			previous->fLeft = left;


		left_prev->fLeft = NULL;
	}
}


ServerNode*
ServerManager::_Insert(ServerNode* node)
{
	ASSERT(node != NULL);

	ServerNode* previous = NULL;
	ServerNode* current = fRoot;
	while (current != NULL) {
		if (current->fID == node->fID)
			return current;
		if (current->fID < node->fID) {
			previous = current;
			current = current->fRight;
		} else {
			previous = current;
			current = current->fLeft;
		}
	}

	if (previous == NULL)
		fRoot = node;
	else if (previous->fID < node->fID)
		previous->fRight = node;
	else
		previous->fLeft = node;

	return node;
}

