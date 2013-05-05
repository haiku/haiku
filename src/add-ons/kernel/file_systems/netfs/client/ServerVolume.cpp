// ServerVolume.cpp

#include "ServerVolume.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "Compatibility.h"
#include "DebugSupport.h"
#include "ExtendedServerInfo.h"
#include "QueryManager.h"
#include "SendReceiveRequest.h"
#include "ServerConnection.h"
#include "ServerConnectionProvider.h"
#include "ServerQueryIterator.h"
#include "ShareVolume.h"
#include "VolumeEvent.h"
#include "VolumeManager.h"
#include "VolumeSupport.h"

// constructor
ServerVolume::ServerVolume(VolumeManager* volumeManager,
	ExtendedServerInfo* serverInfo)
	: VirtualVolume(volumeManager),
	  fServerInfo(serverInfo),
	  fConnectionProvider(NULL)
{
	fServerInfo->AcquireReference();
}

// destructor
ServerVolume::~ServerVolume()
{
	if (fConnectionProvider)
		fConnectionProvider->ReleaseReference();
	if (fServerInfo)
		fServerInfo->ReleaseReference();
}

// GetServerAddress
NetAddress
ServerVolume::GetServerAddress()
{
	AutoLocker<Locker> _(fLock);
	return fServerInfo->GetAddress();
}

// SetServerInfo
void
ServerVolume::SetServerInfo(ExtendedServerInfo* serverInfo)
{
	if (!serverInfo)
		return;

	// set the new info
	fLock.Lock();
	fServerInfo->ReleaseReference();
	fServerInfo = serverInfo;
	fServerInfo->AcquireReference();
	BReference<ExtendedServerInfo> newReference(fServerInfo);

	// remove shares, that are no longer there

	// init a directory iterator
	VirtualDirIterator iterator;
	iterator.SetDirectory(fRootNode, true);

	// iterate through the directory
	const char* name;
	Node* node;
	while (iterator.GetCurrentEntry(&name, &node)) {
		iterator.NextEntry();
		// TODO: Searching by name is currently O(n).
		bool remove = (!serverInfo->GetShareInfo(name));
		fLock.Unlock();

		if (remove) {
			PRINT("  removing share: %s\n", name);
			if (Volume* volume = GetChildVolume(name)) {
				volume->SetUnmounting(true);
				volume->PutVolume();
			}
		}

		fLock.Lock();
	}

	// uninit the directory iterator
	iterator.SetDirectory(NULL);
	fLock.Unlock();

	// add new shares
	int32 count = serverInfo->CountShares();
	for (int32 i = 0; i < count; i++) {
		ExtendedShareInfo* shareInfo = serverInfo->ShareInfoAt(i);
		const char* shareName = shareInfo->GetShareName();

		Volume* volume = GetChildVolume(shareName);
		if (volume) {
			volume->PutVolume();
		} else {
			PRINT("  adding share: %s\n",
				shareInfo->GetShareName());
			status_t error = _AddShare(shareInfo);
			if (error != B_OK) {
				ERROR("ServerVolume::SetServerInfo(): ERROR: Failed to add "
					"share `%s': %s\n", shareName, strerror(error));
			}
		}
	}
}

// Init
status_t
ServerVolume::Init(const char* name)
{
	status_t error = VirtualVolume::Init(name);
	if (error != B_OK)
		return error;

	// create the server connection provider
	fConnectionProvider = new ServerConnectionProvider(fVolumeManager,
		fServerInfo, GetRootID());
	if (!fConnectionProvider) {
		Uninit();
		return B_NO_MEMORY;
	}
	error = fConnectionProvider->Init();
	if (error != B_OK) {
		Uninit();
		return error;
	}

	// add share volumes
	int32 count = fServerInfo->CountShares();
	for (int32 i = 0; i < count; i++) {
		ExtendedShareInfo* shareInfo = fServerInfo->ShareInfoAt(i);

		error = _AddShare(shareInfo);
		if (error != B_OK) {
			ERROR("ServerVolume::Init(): ERROR: Failed to add share `%s': "
				"%s\n", shareInfo->GetShareName(), strerror(error));
		}
	}

	return B_OK;
}

// Uninit
void
ServerVolume::Uninit()
{
	if (fConnectionProvider)
		fConnectionProvider->CloseServerConnection();

	VirtualVolume::Uninit();
}

// PrepareToUnmount
void
ServerVolume::PrepareToUnmount()
{
	VirtualVolume::PrepareToUnmount();
}

// HandleEvent
void
ServerVolume::HandleEvent(VolumeEvent* event)
{
	if (event->GetType() == CONNECTION_BROKEN_EVENT) {
		// tell all share volumes that they have been disconnected

		// init a directory iterator
		fLock.Lock();
		VirtualDirIterator iterator;
		iterator.SetDirectory(fRootNode, true);

		// iterate through the directory
		const char* name;
		Node* node;
		while (iterator.GetCurrentEntry(&name, &node)) {
			iterator.NextEntry();
			Volume* volume = fVolumeManager->GetVolume(node->GetID());
			fLock.Unlock();
			if (ShareVolume* shareVolume = dynamic_cast<ShareVolume*>(volume))
				shareVolume->ConnectionClosed();
			if (volume)
				volume->PutVolume();
			fLock.Lock();
		}

		// uninit the directory iterator
		iterator.SetDirectory(NULL);

		// mark ourselves unmounting
		SetUnmounting(true);
		fLock.Unlock();
	}
}


// #pragma mark -
// #pragma mark ----- FS -----

// Unmount
status_t
ServerVolume::Unmount()
{
	return B_OK;
}


// #pragma mark -
// #pragma mark ----- queries -----

// OpenQuery
status_t
ServerVolume::OpenQuery(const char* queryString, uint32 flags, port_id port,
	int32 token, QueryIterator** _iterator)
{
// TODO: Do nothing when there are no (mounted) shares.
	// get connection
	ServerConnection* serverConnection
		= fConnectionProvider->GetExistingServerConnection();
	if (!serverConnection)
		return ERROR_NOT_CONNECTED;
	RequestConnection* connection = serverConnection->GetRequestConnection();

	// create a query iterator and add it to the query manager
	ServerQueryIterator* iterator = new(std::nothrow) ServerQueryIterator(this);
	if (!iterator)
		return B_NO_MEMORY;
	QueryManager* queryManager = fVolumeManager->GetQueryManager();
	status_t error = queryManager->AddIterator(iterator);
	if (error != B_OK) {
		delete iterator;
		return error;
	}
	QueryIteratorPutter iteratorPutter(queryManager, iterator);

	// prepare the request
	OpenQueryRequest request;
	request.queryString.SetTo(queryString);
	request.flags = flags;
	request.port = port;
	request.token = token;

	// send the request
	OpenQueryReply* reply;
	error = SendRequest(connection, &request, &reply);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> replyDeleter(reply);
	if (reply->error != B_OK)
		RETURN_ERROR(reply->error);

	// set the result
	iterator->SetRemoteCookie(reply->cookie);
	*_iterator = iterator;
	iteratorPutter.Detach();
	return B_OK;
}

// FreeQueryIterator
void
ServerVolume::FreeQueryIterator(QueryIterator* _iterator)
{
	ServerQueryIterator* iterator
		= dynamic_cast<ServerQueryIterator*>(_iterator);

	int32 cookie = iterator->GetRemoteCookie();
	if (cookie >= 0) {
		// prepare the close request
		CloseRequest request;
		request.volumeID = -1;
		request.cookie = cookie;

		// send the request
		ServerConnection* serverConnection
			= fConnectionProvider->GetExistingServerConnection();
		if (serverConnection && serverConnection->IsConnected()) {
			CloseReply* reply;
			status_t error = SendRequest(
				serverConnection->GetRequestConnection(), &request, &reply);
			if (error == B_OK)
				delete reply;
		}
	}

	delete iterator;
}

// ReadQuery
status_t
ServerVolume::ReadQuery(QueryIterator* _iterator, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	// get connection
	ServerConnection* serverConnection
		= fConnectionProvider->GetExistingServerConnection();
	if (!serverConnection)
		return ERROR_NOT_CONNECTED;
	RequestConnection* connection = serverConnection->GetRequestConnection();

	ServerQueryIterator* iterator
		= dynamic_cast<ServerQueryIterator*>(_iterator);

	*countRead = 0;

	for (;;) {
		// if the iterator hasn't cached any more share volume IDs, we need to
		// ask the server for the next entry
		if (!iterator->HasNextShareVolumeID()) {
			// prepare the request
			ReadQueryRequest request;
			request.cookie = iterator->GetRemoteCookie();
			request.count = 1;

			// send the request
			ReadQueryReply* reply;
			status_t error = SendRequest(connection, &request, &reply);
			if (error != B_OK)
				RETURN_ERROR(error);
			ObjectDeleter<Request> replyDeleter(reply);
			if (reply->error != B_OK)
				RETURN_ERROR(reply->error);

			// check, if anything has been read at all
			if (reply->count == 0) {
				*countRead = 0;
				return B_OK;
			}

			// update the iterator
			error = iterator->SetEntry(reply->clientVolumeIDs.GetElements(),
				reply->clientVolumeIDs.CountElements(), reply->dirInfo,
				reply->entryInfo);
			if (error != B_OK)
				return error;
		}

		// get the next concerned share volume and delegate the rest of the work
		int32 volumeID = iterator->NextShareVolumeID();
		ShareVolume* shareVolume = _GetShareVolume(volumeID);
		if (!shareVolume)
			continue;
		VolumePutter volumePutter(shareVolume);

		return shareVolume->GetQueryEntry(iterator->GetEntryInfo(),
			iterator->GetDirectoryInfo(), buffer, bufferSize, countRead);
	}
}


// #pragma mark -
// #pragma mark ----- private -----

// _AddShare
status_t
ServerVolume::_AddShare(ExtendedShareInfo* shareInfo)
{
	// create the share volume
	ShareVolume* shareVolume = new(std::nothrow) ShareVolume(fVolumeManager,
		fConnectionProvider, fServerInfo, shareInfo);
	if (!shareVolume)
		return B_NO_MEMORY;
	status_t error = shareVolume->Init(shareInfo->GetShareName());
	if (error != B_OK) {
		delete shareVolume;
		return error;
	}

	// add the volume to the volume manager
	error = fVolumeManager->AddVolume(shareVolume);
	if (error != B_OK) {
		delete shareVolume;
		return error;
	}
	VolumePutter volumePutter(shareVolume);

	// add the volume to us
	error = AddChildVolume(shareVolume);
	if (error != B_OK) {
		shareVolume->SetUnmounting(true);
		return error;
	}

	return B_OK;
}

// _GetShareVolume
ShareVolume*
ServerVolume::_GetShareVolume(int32 volumeID)
{
	AutoLocker<Locker> locker(fLock);
	VirtualDirIterator dirIterator;
	dirIterator.SetDirectory(fRootNode, true);

	// iterate through the directory
	const char* name;
	Node* node;
	while (dirIterator.GetCurrentEntry(&name, &node)) {
		Volume* volume = fVolumeManager->GetVolume(node->GetID());
		ShareVolume* shareVolume = dynamic_cast<ShareVolume*>(volume);
		if (shareVolume && shareVolume->GetID() == volumeID)
			return shareVolume;

		volume->PutVolume();
		dirIterator.NextEntry();
	}

	return NULL;
}

