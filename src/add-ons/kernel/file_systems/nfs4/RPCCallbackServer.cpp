/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RPCCallbackServer.h"

#include "NFS4Defs.h"
#include "RPCCallback.h"
#include "RPCCallbackReply.h"
#include "RPCCallbackRequest.h"


using namespace RPC;


CallbackServer* gRPCCallbackServer		= NULL;


CallbackServer::CallbackServer()
	:
	fConnectionList(NULL),
	fListener(NULL),
	fThreadRunning(false),
	fCallbackArray(NULL),
	fArraySize(0),
	fFreeSlot(-1)
{
	mutex_init(&fConnectionLock, NULL);
	mutex_init(&fThreadLock, NULL);
	rw_lock_init(&fArrayLock, NULL);
}


CallbackServer::~CallbackServer()
{
	StopServer();

	free(fCallbackArray);
	rw_lock_destroy(&fArrayLock);
	mutex_destroy(&fThreadLock);
	mutex_destroy(&fConnectionLock);
}


status_t
CallbackServer::RegisterCallback(Callback* callback)
{
	status_t result = StartServer();
	if (result != B_OK)
		return result;

	WriteLocker _(fArrayLock);
	if (fFreeSlot == -1) {
		uint32 newSize = max_c(fArraySize * 2, 4);
		uint32 size = newSize * sizeof(CallbackSlot);
		CallbackSlot* array	= reinterpret_cast<CallbackSlot*>(malloc(size));
		if (array == NULL)
			return B_NO_MEMORY;

		if (fCallbackArray != NULL)
			memcpy(array, fCallbackArray, fArraySize * sizeof(CallbackSlot));

		for (uint32 i = fArraySize; i < newSize; i++)
			array[i].fNext = i + 1;

		array[fArraySize * 2 - 1].fNext = -1;

		fCallbackArray = array;
		fFreeSlot = fArraySize;
		fArraySize = newSize;
	}

	int32 id = fFreeSlot;
	fFreeSlot = fCallbackArray[id].fNext;

	fCallbackArray[id].fCallback = callback;
	callback->SetID(id);

	return B_OK;
}


status_t
CallbackServer::UnregisterCallback(Callback* callback)
{
	int32 id = callback->ID();

	WriteLocker _(fArrayLock);
	fCallbackArray[id].fNext = fFreeSlot;
	fFreeSlot = id;

	return B_OK;
}


status_t
CallbackServer::StartServer()
{
	MutexLocker _(fThreadLock);
	if (fThreadRunning)
		return B_OK;

	status_t result = ConnectionListener::Listen(&fListener);
	if (result != B_OK)
		return result;

	fThread = spawn_kernel_thread(&CallbackServer::ListenerThreadLauncher,
		"NFSv4 Callback Listener", B_NORMAL_PRIORITY, this);
	if (fThread < B_OK)
		return fThread;

	fThreadRunning = true;

	result = resume_thread(fThread);
	if (result != B_OK) {
		kill_thread(fThread);
		fThreadRunning = false;
		return result;
	}

	return B_OK;
}


status_t
CallbackServer::StopServer()
{
	MutexLocker _(&fThreadLock);
	if (!fThreadRunning)
		return B_OK;

	fListener->Disconnect();
	status_t result;
	wait_for_thread(fThread, &result);

	MutexLocker locker(fConnectionLock);
	while (fConnectionList != NULL) {
		ConnectionEntry* entry = fConnectionList;
		fConnectionList = entry->fNext;
		entry->fConnection->Disconnect();
		delete entry->fConnection;
		delete entry;
	}

	delete fListener;

	fThreadRunning = false;
	return B_OK;
}


status_t
CallbackServer::NewConnection(Connection* connection)
{
	ConnectionEntry* entry = new ConnectionEntry;
	entry->fConnection = connection;
	entry->fPrev = NULL;

	MutexLocker locker(fConnectionLock);
	entry->fNext = fConnectionList;
	fConnectionList = entry;
	locker.Unlock();

	void** arguments = reinterpret_cast<void**>(malloc(sizeof(void*) * 2));
	if (arguments == NULL)
		return B_NO_MEMORY;

	arguments[0] = this;
	arguments[1] = entry;

	thread_id thread;
	thread = spawn_kernel_thread(&CallbackServer::ConnectionThreadLauncher,
		"NFSv4 Callback Connection", B_NORMAL_PRIORITY, arguments);
	if (thread < B_OK) {
		free(arguments);
		return thread;
	}

	status_t result = resume_thread(thread);
	if (result != B_OK) {
		kill_thread(thread);
		free(arguments);
		return result;
	}

	return B_OK;
}


status_t
CallbackServer::ReleaseConnection(ConnectionEntry* entry)
{
	MutexLocker _(fConnectionLock);
	if (entry->fNext != NULL)
		entry->fNext->fPrev = entry->fPrev;
	if (entry->fPrev != NULL)
		entry->fPrev->fNext = entry->fNext;
	else
		fConnectionList = entry->fNext;

	delete entry->fConnection;
	delete entry;
	return B_OK;
}


status_t
CallbackServer::ConnectionThreadLauncher(void* object)
{
	void** objects = reinterpret_cast<void**>(object);
	CallbackServer* server = reinterpret_cast<CallbackServer*>(objects[0]);
	ConnectionEntry* entry = reinterpret_cast<ConnectionEntry*>(objects[1]);
	free(objects);

	return server->ConnectionThread(entry);
}


status_t
CallbackServer::ConnectionThread(ConnectionEntry* entry)
{
	Connection* connection = entry->fConnection;
	CallbackReply* reply;

	while (fThreadRunning) {
		uint32 size;
		void* buffer;
		status_t result = connection->Receive(&buffer, &size);
		if (result != B_OK) {
			ReleaseConnection(entry);
			return result;
		}

		CallbackRequest* request = new CallbackRequest(buffer, size);
		if (request == NULL || request->Error() != B_OK) {
			free(buffer);
			continue;
		} else if (request != NULL) {
			reply = CallbackReply::Create(request->XID(), request->RPCError());
			if (reply != NULL) {
				connection->Send(reply->Stream().Buffer(),
					reply->Stream().Size());
				delete reply;
			}
			free(buffer);
			continue;
		}


		switch (request->Procedure()) {
			case CallbackProcCompound:
				GetCallback(request->ID())->EnqueueRequest(request, connection);
				break;

			case CallbackProcNull:
				reply = CallbackReply::Create(request->XID());
				if (reply != NULL) {
					connection->Send(reply->Stream().Buffer(),
						reply->Stream().Size());
					delete reply;
				}

			default:
				free(buffer);
		}
	}

	return B_OK;
}


status_t
CallbackServer::ListenerThreadLauncher(void* object)
{
	CallbackServer* server = reinterpret_cast<CallbackServer*>(object);
	return server->ListenerThread();
}


status_t
CallbackServer::ListenerThread()
{
	while (fThreadRunning) {
		Connection* connection;

		status_t result = fListener->AcceptConnection(&connection);
		if (result != B_OK) {
			fThreadRunning = false;
			return result;
		}
		result = NewConnection(connection);
		if (result != B_OK)
			delete connection;
	}

	return B_OK;
}

