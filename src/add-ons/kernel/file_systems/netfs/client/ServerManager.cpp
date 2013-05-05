// ServerManager.cpp

#include "ServerManager.h"

#include <errno.h>
#include <unistd.h>

#ifdef HAIKU_TARGET_PLATFORM_BEOS
#	include <socket.h>
#else
#	include <netinet/in.h>
#	include <sys/socket.h>
#endif

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <ByteOrder.h>
#include <HashMap.h>

#include "Compatibility.h"
#include "DebugSupport.h"
#include "ExtendedServerInfo.h"
#include "InsecureChannel.h"
#include "NetAddress.h"
#include "NetFSDefs.h"
#include "RequestChannel.h"
#include "Requests.h"
#include "TaskManager.h"
#include "Utils.h"

// server info states
enum {
	STATE_ADDING,
	STATE_REMOVING,
	STATE_UPDATING,
	STATE_READY,
	STATE_OBSOLETE
};


// ServerInfoMap
struct ServerManager::ServerInfoMap : HashMap<NetAddress, ExtendedServerInfo*> {
};

// ServerInfoTask
class ServerManager::ServerInfoTask : public Task {
public:
	ServerInfoTask(ServerManager* manager, ExtendedServerInfo* oldServerInfo,
		ExtendedServerInfo* serverInfo)
		: Task("server info task"),
		  fServerManager(manager),
		  fOldServerInfo(oldServerInfo),
		  fServerInfo(serverInfo),
		  fFD(-1),
		  fSuccess(false)
	{
		if (fServerInfo)
			fServerInfo->AcquireReference();
	}

	virtual ~ServerInfoTask()
	{
		Stop();
		if (!fSuccess) {
			if (fOldServerInfo)
				fServerManager->_UpdatingServerFailed(fServerInfo);
			else
				fServerManager->_AddingServerFailed(fServerInfo);
		}
		if (fServerInfo)
			fServerInfo->ReleaseReference();
	}

	status_t Init()
	{
		// create a socket
		fFD = socket(AF_INET, SOCK_STREAM, 0);
		if (fFD < 0) {
			ERROR("ServerManager::ServerInfoTask: ERROR: Failed to create "
				"socket: %s\n", strerror(errno));
			return errno;
		}
		return B_OK;
	}

	virtual status_t Execute()
	{
		// connect to the server info port
		sockaddr_in addr = fServerInfo->GetAddress().GetAddress();
		addr.sin_port = htons(kDefaultServerInfoPort);
		if (connect(fFD, (sockaddr*)&addr, sizeof(addr)) < 0) {
			ERROR("ServerManager::ServerInfoTask: ERROR: Failed to connect "
				"to server info port: %s\n", strerror(errno));
			return errno;
		}

		// create a channel
		InsecureChannel channel(fFD);

		// receive a request
		RequestChannel requestChannel(&channel);
		Request* _request;
		status_t error = requestChannel.ReceiveRequest(&_request);
		if (error != B_OK) {
			ERROR("ServerManager::ServerInfoTask: ERROR: Failed to receive "
				"server info request: %s\n", strerror(errno));
			return error;
		}
		ObjectDeleter<Request> requestDeleter(_request);
		ServerInfoRequest* request = dynamic_cast<ServerInfoRequest*>(_request);
		if (!request) {
			ERROR("ServerManager::ServerInfoTask: ERROR: Received request "
				"is not a server info request.\n");
			return B_BAD_DATA;
		}

		// get the info
		error = fServerInfo->SetTo(&request->serverInfo);
		if (error != B_OK)
			return error;

		// notify the manager
		if (fOldServerInfo)
			fServerManager->_ServerUpdated(fServerInfo);
		else
			fServerManager->_ServerAdded(fServerInfo);

		fSuccess = true;
		return B_OK;
	}

	virtual void Stop()
	{
		safe_closesocket(fFD);
	}

private:
	ServerManager*		fServerManager;
	ExtendedServerInfo*	fOldServerInfo;
	ExtendedServerInfo*	fServerInfo;
	vint32				fFD;
	bool				fUpdate;
	bool				fSuccess;
};


// #pragma mark -

// constructor
ServerManager::ServerManager(Listener* listener)
	: fLock("server manager"),
	  fBroadcastListener(-1),
	  fBroadcastListenerSocket(-1),
	  fListener(listener),
	  fTerminating(false)
{
}

// destructor
ServerManager::~ServerManager()
{
	Uninit();
}

// Init
status_t
ServerManager::Init()
{
	// create the server info map
	fServerInfos = new(std::nothrow) ServerInfoMap();
	if (!fServerInfos)
		RETURN_ERROR(B_NO_MEMORY);
	status_t error = fServerInfos->InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// init the broadcast listener
	error = _InitBroadcastListener();
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}

// Uninit
void
ServerManager::Uninit()
{
	// stop the broadcast listener
	fTerminating = true;
	_TerminateBroadcastListener();

	// remove all server infos
	AutoLocker<Locker> _(fLock);
	for (ServerInfoMap::Iterator it = fServerInfos->GetIterator();
		 it.HasNext();) {
		ExtendedServerInfo* serverInfo = it.Next().value;
		serverInfo->ReleaseReference();
	}
	fServerInfos->Clear();
}

// Run
void
ServerManager::Run()
{
	// start the broadcast listener
	resume_thread(fBroadcastListener);
}

// GetServerInfo
ExtendedServerInfo*
ServerManager::GetServerInfo(const NetAddress& address)
{
	AutoLocker<Locker> _(fLock);
	ExtendedServerInfo* serverInfo = fServerInfos->Get(address);
	if (!serverInfo
		|| (serverInfo->GetState() != STATE_READY
			&& serverInfo->GetState() != STATE_UPDATING)) {
		return NULL;
	}
	serverInfo->AcquireReference();
	return serverInfo;
}

// AddServer
status_t
ServerManager::AddServer(const NetAddress& address)
{
	// check, if the server is already known
	AutoLocker<Locker> locker(fLock);
	ExtendedServerInfo* oldInfo = fServerInfos->Get(address);
	if (oldInfo)
		return B_OK;

	// create a new server info and add it
	ExtendedServerInfo* serverInfo
		= new(std::nothrow) ExtendedServerInfo(address);
	if (!serverInfo)
		return B_NO_MEMORY;
	serverInfo->SetState(STATE_ADDING);
	BReference<ExtendedServerInfo> serverInfoReference(serverInfo, true);
	status_t error = fServerInfos->Put(address, serverInfo);
	if (error != B_OK)
		return error;
	serverInfo->AcquireReference();

	// create and execute the task -- it will do what is necessary
	ServerInfoTask task(this, NULL, serverInfo);
	error = task.Init();
	if (error != B_OK)
		return error;

	locker.Unlock();
	return task.Execute();
}

// RemoveServer
void
ServerManager::RemoveServer(const NetAddress& address)
{
	// check, if the server is known at all
	AutoLocker<Locker> locker(fLock);
	ExtendedServerInfo* serverInfo = fServerInfos->Get(address);
	if (!serverInfo)
		return;

	// If its current state is not STATE_READY, then an info thread is currently
	// trying to add/update it. We mark the info STATE_REMOVING, which will
	// remove the info as soon as possible.
	if (serverInfo->GetState() == STATE_READY) {
		BReference<ExtendedServerInfo> _(serverInfo);
		_RemoveServer(serverInfo);
		locker.Unlock();
		fListener->ServerRemoved(serverInfo);
	} else
		serverInfo->SetState(STATE_REMOVING);
}

// _BroadcastListenerEntry
int32
ServerManager::_BroadcastListenerEntry(void* data)
{
	return ((ServerManager*)data)->_BroadcastListener();
}

// _BroadcastListener
int32
ServerManager::_BroadcastListener()
{
	TaskManager taskManager;
	while (!fTerminating) {
		taskManager.RemoveDoneTasks();

		// receive
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(kDefaultBroadcastPort);
		addr.sin_addr.s_addr = INADDR_ANY;
		socklen_t addrSize = sizeof(addr);
		BroadcastMessage message;
//PRINT(("ServerManager::_BroadcastListener(): recvfrom()...\n"));
		ssize_t bytesRead = recvfrom(fBroadcastListenerSocket, &message,
			sizeof(message), 0, (sockaddr*)&addr, &addrSize);
		if (bytesRead < 0) {
			PRINT("ServerManager::_BroadcastListener(): recvfrom() "
				"failed: %s\n", strerror(errno));
			continue;
		}

		// check message size, magic, and protocol version
		if (bytesRead != sizeof(BroadcastMessage)) {
			PRINT("ServerManager::_BroadcastListener(): received "
				"%ld bytes, but it should be %lu\n", bytesRead,
				sizeof(BroadcastMessage));
			continue;
		}
		if (message.magic != B_HOST_TO_BENDIAN_INT32(BROADCAST_MESSAGE_MAGIC)) {
			PRINT("ServerManager::_BroadcastListener(): message has"
				" bad magic.\n");
			continue;
		}
		if (message.protocolVersion
			!= (int32)B_HOST_TO_BENDIAN_INT32(NETFS_PROTOCOL_VERSION)) {
			PRINT("ServerManager::_BroadcastListener(): protocol "
				"version does not match: %lu vs. %d.\n",
				B_BENDIAN_TO_HOST_INT32(
					message.protocolVersion),
				NETFS_PROTOCOL_VERSION);
			continue;
		}

		// check, if the server is local
		NetAddress netAddress(addr);
		#ifndef ADD_SERVER_LOCALHOST
			if (netAddress.IsLocal())
				continue;
		#endif	// ADD_SERVER_LOCALHOST

		AutoLocker<Locker> locker(fLock);
		ExtendedServerInfo* oldServerInfo = fServerInfos->Get(netAddress);

		// examine the message
		switch (B_BENDIAN_TO_HOST_INT32(message.message)) {
			case BROADCAST_MESSAGE_SERVER_TICK:
//				PRINT(("ServerManager::_BroadcastListener(): "
//					"BROADCAST_MESSAGE_SERVER_TICK.\n"));
				if (oldServerInfo)
					continue;
				break;
			case BROADCAST_MESSAGE_SERVER_UPDATE:
//				PRINT(("ServerManager::_BroadcastListener(): "
//					"BROADCAST_MESSAGE_SERVER_UPDATE.\n"));
				break;
			case BROADCAST_MESSAGE_CLIENT_HELLO:
//				PRINT(("ServerManager::_BroadcastListener(): "
//					"BROADCAST_MESSAGE_CLIENT_HELLO. Ignoring.\n"));
				continue;
				break;
		}

		if (oldServerInfo && oldServerInfo->GetState() != STATE_READY)
			continue;

		// create a new server info and add it
		ExtendedServerInfo* serverInfo
			= new(std::nothrow) ExtendedServerInfo(netAddress);
		if (!serverInfo)
			return B_NO_MEMORY;
		serverInfo->SetState(STATE_ADDING);
		BReference<ExtendedServerInfo> serverInfoReference(serverInfo, true);
		if (oldServerInfo) {
			oldServerInfo->SetState(STATE_UPDATING);
		} else {
			status_t error = fServerInfos->Put(netAddress, serverInfo);
			if (error != B_OK)
				continue;
			serverInfo->AcquireReference();
		}

		// create a task to add/update the server info
		ServerInfoTask* task = new(std::nothrow) ServerInfoTask(this, oldServerInfo,
			serverInfo);
		if (!task) {
			if (oldServerInfo) {
				oldServerInfo->SetState(STATE_READY);
			} else {
				fServerInfos->Remove(serverInfo->GetAddress());
				serverInfo->ReleaseReference();
			}
			continue;
		}
		// now the task has all info and will call the respective cleanup
		// method when being deleted
		if (task->Init() != B_OK) {
			delete task;
			continue;
		}
		status_t error = taskManager.RunTask(task);
		if (error != B_OK) {
			ERROR("ServerManager::_BroadcastListener(): Failed to start server "
				"info task: %s\n", strerror(error));
			continue;
		}
	}
	return B_OK;
}

// _InitBroadcastListener
status_t
ServerManager::_InitBroadcastListener()
{
	// create a socket
	fBroadcastListenerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fBroadcastListenerSocket < 0)
		return errno;
	// bind it to the port
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(kDefaultBroadcastPort);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fBroadcastListenerSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
		ERROR("ServerManager::_InitBroadcastListener(): ERROR: bind()ing the "
			"broadcasting socket failed: %s\n", strerror(errno));
		safe_closesocket(fBroadcastListenerSocket);
		return errno;
	}
	// spawn the thread
	#if USER
		fBroadcastListener = spawn_thread(&_BroadcastListenerEntry,
			"broadcast listener", B_NORMAL_PRIORITY, this);
	#else
		fBroadcastListener = spawn_kernel_thread(&_BroadcastListenerEntry,
			"broadcast listener", B_NORMAL_PRIORITY, this);
	#endif
	if (fBroadcastListener < 0)
		return fBroadcastListener;
	return B_OK;
}

// _TerminateBroadcastListener
void
ServerManager::_TerminateBroadcastListener()
{
	safe_closesocket(fBroadcastListenerSocket);
	if (fBroadcastListener >= 0) {
		int32 result;
		wait_for_thread(fBroadcastListener, &result);
	}
}

// _ServerAdded
void
ServerManager::_ServerAdded(ExtendedServerInfo* serverInfo)
{
	AutoLocker<Locker> locker(fLock);
	if (fServerInfos->Get(serverInfo->GetAddress()) == serverInfo) {
		// check whether someone told us to remove the server in the meantime
		if (serverInfo->GetState() == STATE_REMOVING) {
			_RemoveServer(serverInfo);
			if (fListener) {
				locker.Unlock();
				fListener->ServerRemoved(serverInfo);
			}
			return;
		}

		// no, everything is fine: go on...
		serverInfo->SetState(STATE_READY);
		if (fListener) {
			locker.Unlock();
			fListener->ServerAdded(serverInfo);
		}
	} else {
		WARN("ServerManager::_ServerAdded(%p): WARNING: Unexpected server "
			"info.\n", serverInfo);
	}
}

// _ServerUpdated
void
ServerManager::_ServerUpdated(ExtendedServerInfo* serverInfo)
{
	AutoLocker<Locker> locker(fLock);
	ExtendedServerInfo* oldInfo = fServerInfos->Get(serverInfo->GetAddress());
	if (serverInfo != oldInfo) {
		// check whether someone told us to remove the server in the meantime
		if (oldInfo->GetState() == STATE_REMOVING) {
			oldInfo->AcquireReference();
			_RemoveServer(oldInfo);
			if (fListener) {
				locker.Unlock();
				fListener->ServerRemoved(oldInfo);
			}
			oldInfo->ReleaseReference();
			return;
		}

		// no, everything is fine: go on...
		fServerInfos->Put(serverInfo->GetAddress(), serverInfo);
		serverInfo->AcquireReference();
		serverInfo->SetState(STATE_READY);
		oldInfo->SetState(STATE_OBSOLETE);
		if (fListener) {
			locker.Unlock();
			fListener->ServerUpdated(oldInfo, serverInfo);
		}
		oldInfo->ReleaseReference();
	} else {
		WARN("ServerManager::_ServerUpdated(%p): WARNING: Unexpected server "
			"info.\n", serverInfo);
	}
}

// _AddingServerFailed
void
ServerManager::_AddingServerFailed(ExtendedServerInfo* serverInfo)
{
	AutoLocker<Locker> locker(fLock);
	if (fServerInfos->Get(serverInfo->GetAddress()) == serverInfo) {
		bool removing = (serverInfo->GetState() == STATE_REMOVING);
		fServerInfos->Remove(serverInfo->GetAddress());
		serverInfo->ReleaseReference();
		serverInfo->SetState(STATE_OBSOLETE);

		// notify the listener, if someone told us in the meantime to remove
		// the server
		if (removing) {
			locker.Unlock();
			fListener->ServerRemoved(serverInfo);
		}
	} else {
		WARN("ServerManager::_AddingServerFailed(%p): WARNING: Unexpected "
			"server info.\n", serverInfo);
	}
}

// _UpdatingServerFailed
void
ServerManager::_UpdatingServerFailed(ExtendedServerInfo* serverInfo)
{
	AutoLocker<Locker> locker(fLock);
	ExtendedServerInfo* oldInfo = fServerInfos->Get(serverInfo->GetAddress());
	if (serverInfo != oldInfo) {
		// check whether someone told us to remove the server in the meantime
		if (oldInfo->GetState() == STATE_REMOVING) {
			oldInfo->AcquireReference();
			_RemoveServer(oldInfo);
			if (fListener) {
				locker.Unlock();
				fListener->ServerRemoved(oldInfo);
			}
			oldInfo->ReleaseReference();
			serverInfo->SetState(STATE_OBSOLETE);
			return;
		}

		// no, everything is fine: go on...
		serverInfo->SetState(STATE_OBSOLETE);
		oldInfo->SetState(STATE_READY);
	} else {
		WARN("ServerManager::_UpdatingServerFailed(%p): WARNING: Unexpected "
			"server info.\n", serverInfo);
	}
}

// _RemoveServer
//
// fLock must be held.
void
ServerManager::_RemoveServer(ExtendedServerInfo* serverInfo)
{
	if (!serverInfo)
		return;

	ExtendedServerInfo* oldInfo = fServerInfos->Get(serverInfo->GetAddress());
	if (oldInfo) {
		fServerInfos->Remove(oldInfo->GetAddress());
		oldInfo->SetState(STATE_OBSOLETE);
		oldInfo->ReleaseReference();
	}
}


// #pragma mark -

// destructor
ServerManager::Listener::~Listener()
{
}

