// ServerConnectionProvider.cpp

#include "AutoLocker.h"
#include "ExtendedServerInfo.h"
#include "ServerConnection.h"
#include "ServerConnectionProvider.h"

// constructor
ServerConnectionProvider::ServerConnectionProvider(VolumeManager* volumeManager,
	ExtendedServerInfo* serverInfo,
	vnode_id connectionBrokenTarget)
	: Referencable(true),
	  fLock("server connection provider"),
	  fVolumeManager(volumeManager),
	  fServerInfo(serverInfo),
	  fServerConnection(NULL),
	  fConnectionBrokenTarget(connectionBrokenTarget)
{
	if (fServerInfo)
		fServerInfo->AddReference();
}

// destructor
ServerConnectionProvider::~ServerConnectionProvider()
{
	AutoLocker<Locker> _(fLock);
	if (fServerConnection) {
		fServerConnection->Close();
		fServerConnection->RemoveReference();
	}

	if (fServerInfo)
		fServerInfo->RemoveReference();
}

// Init
status_t
ServerConnectionProvider::Init()
{
	return B_OK;
}

// GetServerConnection
status_t
ServerConnectionProvider::GetServerConnection(
	ServerConnection** serverConnection)
{
	AutoLocker<Locker> _(fLock);

	// if there is no server connection yet, create one
	if (!fServerConnection) {
		fServerConnection = new(nothrow) ServerConnection(fVolumeManager,
			fServerInfo);
		if (!fServerConnection)
			return B_NO_MEMORY;
		status_t error = fServerConnection->Init(fConnectionBrokenTarget);
		if (error != B_OK)
			return error;
	}

	if (!fServerConnection->IsConnected())
		return B_ERROR;

	fServerConnection->AddReference();
	*serverConnection = fServerConnection;
	return B_OK;
}

// GetExistingServerConnection
ServerConnection*
ServerConnectionProvider::GetExistingServerConnection()
{
	AutoLocker<Locker> _(fLock);

	// if there is no server connection yet, create one
	if (!fServerConnection || !fServerConnection->IsConnected())
		return NULL;

	fServerConnection->AddReference();
	return fServerConnection;
}

// CloseServerConnection
void
ServerConnectionProvider::CloseServerConnection()
{
	AutoLocker<Locker> _(fLock);
	if (fServerConnection)
		fServerConnection->Close();
}

