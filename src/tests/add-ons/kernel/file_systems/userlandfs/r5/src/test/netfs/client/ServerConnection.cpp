// ServerConnection.cpp

#include <ByteOrder.h>
#include <SupportDefs.h>

#include "AutoDeleter.h"
#include "Connection.h"
#include "ConnectionFactory.h"
#include "Debug.h"
#include "ExtendedServerInfo.h"
#include "HashMap.h"
#include "RequestConnection.h"
#include "ServerConnection.h"
#include "ShareVolume.h"
#include "VolumeEvent.h"
#include "VolumeManager.h"

// VolumeMap
struct ServerConnection::VolumeMap : HashMap<HashKey32<int32>, ShareVolume*> {
};


// constructor
ServerConnection::ServerConnection(VolumeManager* volumeManager,
	ExtendedServerInfo* serverInfo)
	: Referencable(true),
	  RequestHandler(),
	  fLock("server connection"),
	  fVolumeManager(volumeManager),
	  fServerInfo(serverInfo),
	  fConnection(NULL),
	  fVolumes(NULL),
	  fConnectionBrokenEvent(NULL),
	  fConnected(false)
{
	if (fServerInfo)
		fServerInfo->AddReference();
}

// destructor
ServerConnection::~ServerConnection()
{
PRINT(("ServerConnection::~ServerConnection()\n"))
	Close();
	delete fConnection;
	delete fVolumes;
	if (fConnectionBrokenEvent)
		fConnectionBrokenEvent->RemoveReference();
	if (fServerInfo)
		fServerInfo->RemoveReference();
}

// Init
status_t
ServerConnection::Init(vnode_id connectionBrokenTarget)
{
	if (!fServerInfo)
		RETURN_ERROR(B_BAD_VALUE);

	// create a connection broken event
	fConnectionBrokenEvent
		= new(nothrow) ConnectionBrokenEvent(connectionBrokenTarget);
	if (!fConnectionBrokenEvent)
		return B_NO_MEMORY;

	// get the server address
	const char* connectionMethod = fServerInfo->GetConnectionMethod();
	String server;
	status_t error = fServerInfo->GetAddress().GetString(&server, false);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the volume map
	fVolumes = new(nothrow) VolumeMap;
	if (!fVolumes)
		RETURN_ERROR(B_NO_MEMORY);
	error = fVolumes->InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// establish the connection
	Connection* connection;
	ConnectionFactory factory;
	error = factory.CreateConnection(connectionMethod, server.GetString(),
		&connection);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create a request connection
	fConnection = new(nothrow) RequestConnection(connection, this);
	if (!fConnection) {
		delete connection;
		RETURN_ERROR(B_NO_MEMORY);
	}
	error = fConnection->Init();
	if (error != B_OK)
		return error;

	// send an `init connection request'

	// prepare the request
	InitConnectionRequest request;
	request.bigEndian = B_HOST_IS_BENDIAN;

	// send the request
	Request* _reply;
	error = fConnection->SendRequest(&request, &_reply);
	if (error != B_OK)
		return error;
	ObjectDeleter<Request> replyDeleter(_reply);

	// everything OK?
	InitConnectionReply* reply = dynamic_cast<InitConnectionReply*>(_reply);
	if (!reply)
		return B_BAD_DATA;
	if (reply->error != B_OK)
		return reply->error;

	fConnected = true;
	return B_OK;
}

// Close
void
ServerConnection::Close()
{
	// mark the connection closed (events won't be delivered anymore)
	{
		AutoLocker<Locker> locker(fLock);
		fConnected = false;
	}

	if (fConnection)
		fConnection->Close();
}

// IsConnected
bool
ServerConnection::IsConnected()
{
	return fConnected;
}

// GetRequestConnection
RequestConnection*
ServerConnection::GetRequestConnection() const
{
	return fConnection;
}

// AddVolume
status_t
ServerConnection::AddVolume(ShareVolume* volume)
{
	if (!volume)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);
	return fVolumes->Put(volume->GetID(), volume);
}

// RemoveVolume
void
ServerConnection::RemoveVolume(ShareVolume* volume)
{
	if (!volume)
		return;

	AutoLocker<Locker> _(fLock);
	fVolumes->Remove(volume->GetID());
}

// GetVolume
ShareVolume*
ServerConnection::GetVolume(int32 volumeID)
{
	AutoLocker<Locker> _(fLock);
	return fVolumes->Get(volumeID);
}

// VisitConnectionBrokenRequest
status_t
ServerConnection::VisitConnectionBrokenRequest(ConnectionBrokenRequest* request)
{
	AutoLocker<Locker> locker(fLock);
	if (fConnected) {
		fConnected = false;
		fVolumeManager->SendVolumeEvent(fConnectionBrokenEvent);
	}
	return B_OK;
}

// VisitNodeMonitoringRequest
status_t
ServerConnection::VisitNodeMonitoringRequest(NodeMonitoringRequest* request)
{
	AutoLocker<Locker> locker(fLock);
	if (fConnected) {
		if (ShareVolume* volume = GetVolume(request->volumeID)) {
			if (fVolumeManager->GetVolume(volume->GetRootID())) {
				locker.Unlock();
				if (request->opcode == B_DEVICE_UNMOUNTED)
					volume->SetUnmounting(true);
				else
					volume->ProcessNodeMonitoringRequest(request);
				volume->PutVolume();
			}
		}
	}
	return B_OK;
}

