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

#include "RPCReply.h"


using namespace RPC;


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
RequestManager::AddRequest(Request* req)
{
	MutexLocker _(fLock);
	if (fQueueTail != NULL)
		fQueueTail->fNext = req;
	else 
		fQueueHead = req;
	fQueueTail = req;
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


Server::Server(Connection* conn, ServerAddress* addr)
	:
	fConnection(conn),
	fAddress(addr),
	fPrivateData(NULL),
	fXID(rand() << 1)
{
	_StartListening();
}


Server::~Server()
{
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
Server::SendCall(Call* call, Reply** reply)
{
	Request* req;
	status_t result = SendCallAsync(call, reply, &req);
	if (result != B_OK)
		return result;

	result = WaitCall(req);
	if (result != B_OK) {
		CancelCall(req);
		delete req;
		return result;
	}

	delete req;
	return B_OK;
}


status_t
Server::SendCallAsync(Call* call, Reply** reply, Request** request)
{
	if (fThreadError != B_OK)
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
	return ResendCallAsync(call, req);
}


status_t
Server::ResendCallAsync(Call* call, Request* req)
{
	if (fThreadError != B_OK) {
		fRequests.FindRequest(req->fXID);
		delete req;
		return fThreadError;
	}

	XDR::WriteStream& stream = call->Stream();
	status_t result = fConnection->Send(stream.Buffer(), stream.Size());
	if (result != B_OK) {
		fRequests.FindRequest(req->fXID);
		delete req;
		return result;
	}

	return B_OK;
}


status_t
Server::WakeCall(Request* request)
{
	Request* req = fRequests.FindRequest(request->fXID);
	if (req == NULL)
		return B_OK;

	request->fError = B_FILE_ERROR;
	*request->fReply = NULL;
	request->fDone = true;
	request->fEvent.NotifyAll();

	return B_OK;
}


status_t
Server::Repair()
{
	fThreadCancel = true;

	status_t result = fConnection->Reconnect();
	if (result != B_OK)
		return result;

	wait_for_thread(fThread, &result);
	return _StartListening();
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
	void* buffer;

	while (!fThreadCancel) {
		result = fConnection->Receive(&buffer, &size);
		if (result == B_NO_MEMORY)
			continue;
		else if (result != B_OK) {
			fThreadError = result;
			return result;
		}
		
		Reply* reply = new(std::nothrow) Reply(buffer, size);
		if (reply == NULL) {
			free(buffer);
			continue;
		}

		Request* req = fRequests.FindRequest(reply->GetXID());
		if (req != NULL) {
			*req->fReply = reply;
			req->fDone = true;
			req->fEvent.NotifyAll();
		} else
			delete reply;
	}

	return B_OK;
}


status_t
Server::_ListenerThreadStart(void* ptr)
{
	Server* server = reinterpret_cast<Server*>(ptr);
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
ServerManager::Acquire(Server** pserv, const ServerAddress& id,
	ProgramData* (*createPriv)(Server*))
{
	status_t result;

	MutexLocker locker(fLock);
	ServerNode* node = _Find(id);
	if (node != NULL) {
		node->fRefCount++;
		*pserv = node->fServer;

		return B_OK;
	}

	node = new(std::nothrow) ServerNode;
	if (node == NULL)
		return B_NO_MEMORY;

	node->fID = id;

	Connection* conn;
	result = Connection::Connect(&conn, id);
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
	node->fServer->SetPrivateData(createPriv(node->fServer));

	node->fRefCount = 1;
	node->fLeft = node->fRight = NULL;

	ServerNode* nd = _Insert(node);
	if (nd != node) {
		nd->fRefCount++;

		delete node->fServer;
		delete node;
		*pserv = nd->fServer;
		return B_OK;
	}

	*pserv = node->fServer;
	return B_OK;
}


void
ServerManager::Release(Server* serv)
{
	MutexLocker _(fLock);
	ServerNode* node = _Find(serv->ID());
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
ServerManager::_Find(const ServerAddress& id)
{
	ServerNode* node = fRoot;
	while (node != NULL) {
		if (node->fID == id)
			return node;
		if (node->fID < id)
			node = node->fRight;
		else
			node = node->fLeft;
	}

	return node;
}


void
ServerManager::_Delete(ServerNode* node)
{
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

