// NetFSServer.cpp

#include <errno.h>
#include <netdb.h>
#include <new>
#include <stdio.h>
#include <string.h>

#ifdef HAIKU_TARGET_PLATFORM_BEOS
#	include <socket.h>
#else
#	include <unistd.h>
#	include <netinet/in.h>
#	include <sys/socket.h>
#endif

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Node.h>
#include <Path.h>
#include <util/DoublyLinkedList.h>

#include "Connection.h"
#include "ConnectionListener.h"
#include "DebugSupport.h"
#include "DriverSettings.h"
#include "FDManager.h"
#include "InsecureChannel.h"
#include "NetFSDefs.h"
#include "NetFSServer.h"
#include "NetFSServerRosterDefs.h"
#include "RequestChannel.h"
#include "Requests.h"
#include "SecurityContext.h"
#include "StatisticsManager.h"
#include "TaskManager.h"
#include "Utils.h"
#include "VolumeManager.h"

static const char* kSettingsDirName				= "netfs";
static const char* kSettingsFileName			= "netfs_server";
static const char* kFallbackSettingsFileName	= "netfs_server_fallback";

// usage
static const char* kUsage =
"Usage: netfs_server <options>\n"
"options:\n"
"  --dont-broadcast  - don't use broadcasting to announce the server's\n"
"                      availability to clients\n"
"  -h, --help        - print this text\n"
;

// ConnectionInitializer
class NetFSServer::ConnectionInitializer {
public:
	ConnectionInitializer(NetFSServer* server,
		ConnectionListener* connectionListener, Connection* connection)
		: fServer(server),
		  fConnectionListener(connectionListener),
		  fConnection(connection),
		  fThread(-1)
	{
	}

	~ConnectionInitializer()
	{
		delete fConnection;
	}

	status_t Run()
	{
		fThread = spawn_thread(&_ThreadEntry, "connection initializer",
			B_NORMAL_PRIORITY, this);
		if (fThread < 0)
			return fThread;
		resume_thread(fThread);
		return B_OK;
	}

private:
	static int32 _ThreadEntry(void* data)
	{
		return ((ConnectionInitializer*)data)->_Thread();
	}

	int32 _Thread()
	{
		// finish connection initialization
		User* user = NULL;
		status_t error = fConnectionListener->FinishInitialization(
			fConnection, fServer->GetSecurityContext(), &user);
		// create a client connection
		ClientConnection* clientConnection = NULL;
		if (error == B_OK) {
			clientConnection = new(std::nothrow) ClientConnection(fConnection,
				fServer->GetSecurityContext(), user, fServer);
			if (!clientConnection)
				error = B_NO_MEMORY;
		}
		if (error == B_OK) {
			fConnection = NULL;	// connection belongs to client connection now
			error = clientConnection->Init();
		}
		// add the client connection to the server
		if (error == B_OK)
			error = fServer->_AddClientConnection(clientConnection);
		// cleanup on error
		if (error != B_OK)
			delete clientConnection;
		delete this;
		return 0;
	}

private:
	NetFSServer*		fServer;
	ConnectionListener*	fConnectionListener;
	Connection*			fConnection;
	thread_id			fThread;
};


// ServerInfoSender
class NetFSServer::ServerInfoSender : public Task {
public:
	ServerInfoSender(int socket, const ServerInfo& serverInfo)
		: Task("server info sender"),
		  fChannel(new(std::nothrow) InsecureChannel(socket)),
		  fServerInfo(serverInfo)
	{
		if (!fChannel)
			closesocket(socket);
	}

	~ServerInfoSender()
	{
		delete fChannel;
	}

	status_t Init()
	{
		if (!fChannel)
			return B_NO_MEMORY;

		return B_OK;
	}

	virtual void Stop()
	{
		if (fChannel)
			fChannel->Close();
	}

	virtual status_t Execute()
	{
		if (!fChannel) {
			SetDone(true);
			return B_NO_INIT;
		}

		RequestChannel requestChannel(fChannel);

		// create the server info request
		ServerInfoRequest request;
		request.serverInfo = fServerInfo;

		// send the request
		status_t error = requestChannel.SendRequest(&request);
		if (error != B_OK) {
			ERROR("ServerInfoSender: ERROR: Failed to send request: %s\n",
				strerror(error));
		}

		SetDone(true);
		return B_OK;
	}

private:
	Channel*		fChannel;
	ServerInfo		fServerInfo;
};


// NetFSServer

// constructor
NetFSServer::NetFSServer(bool useBroadcasting)
	:
	BApplication(kNetFSServerSignature),
	fSecurityContext(NULL),
	fConnectionListenerFactory(),
	fConnectionListener(NULL),
	fLock("netfs server"),
	fClientConnections(),
	fVolumeManager(NULL),
	fClosedConnections(),
	fClosedConnectionsSemaphore(-1),
	fConnectionListenerThread(-1),
	fConnectionDeleter(-1),
	fBroadcaster(-1),
	fBroadcastingSocket(-1),
	fBroadcasterSemaphore(-1),
	fServerInfoConnectionListener(-1),
	fServerInfoConnectionListenerSocket(-1),
	fServerInfoUpdated(0),
	fUseBroadcasting(useBroadcasting),
	fTerminating(false)
{
}

// destructor
NetFSServer::~NetFSServer()
{
	fTerminating = true;
	// stop the connection listener
	if (fConnectionListener)
		fConnectionListener->StopListening();
	if (fConnectionListenerThread >= 0) {
		int32 result;
		wait_for_thread(fConnectionListenerThread, &result);
	}
	delete fConnectionListener;

	// delete the broadcaster semaphore
	if (fBroadcasterSemaphore >= 0)
		delete_sem(fBroadcasterSemaphore);

	// terminate the broadcaster
	if (fBroadcaster >= 0) {
		safe_closesocket(fBroadcastingSocket);

		// interrupt the thread in case it is currently snoozing
		suspend_thread(fBroadcaster);
		int32 result;
		wait_for_thread(fBroadcaster, &result);
	}

	// terminate the server info connection listener
	_ExitServerInfoConnectionListener();

	// terminate the connection deleter
	if (fClosedConnectionsSemaphore >= 0)
		delete_sem(fClosedConnectionsSemaphore);
	if (fConnectionDeleter >= 0) {
		int32 result;
		wait_for_thread(fConnectionDeleter, &result);
	}

	// blow away all remaining connections
	AutoLocker<Locker> _(fLock);
	// open connections
	for (int32 i = 0;
		 ClientConnection* connection
		 	= (ClientConnection*)fClientConnections.ItemAt(i);
		 i++) {
		connection->Close();
		delete connection;
	}

	// closed connections
	for (int32 i = 0;
		 ClientConnection* connection
		 	= (ClientConnection*)fClosedConnections.ItemAt(i);
		 i++) {
		delete connection;
	}
	VolumeManager::DeleteDefault();
	FDManager::DeleteDefault();
	delete fSecurityContext;
}

// Init
status_t
NetFSServer::Init()
{
	// init the settings
	status_t error = _InitSettings();
	if (error != B_OK)
		return error;

	// create the FD manager
	error = FDManager::CreateDefault();
	if (error != B_OK)
		return error;

	// create the volume manager
	error = VolumeManager::CreateDefault();
	if (error != B_OK)
		return error;
	fVolumeManager = VolumeManager::GetDefault();

	// create a connection listener
//	error = fConnectionListenerFactory.CreateConnectionListener(
//		"port", NULL, &fConnectionListener);
	error = fConnectionListenerFactory.CreateConnectionListener(
		"insecure", NULL, &fConnectionListener);
	if (error != B_OK)
		return error;

	// spawn the connection listener thread
	fConnectionListenerThread = spawn_thread(&_ConnectionListenerEntry,
		"connection listener", B_NORMAL_PRIORITY, this);
	if (fConnectionListenerThread < 0)
		return fConnectionListenerThread;

	// create the closed connections semaphore
	fClosedConnectionsSemaphore = create_sem(0, "closed connections");
	if (fClosedConnectionsSemaphore < 0)
		return fClosedConnectionsSemaphore;

	// spawn the connection deleter
	fConnectionDeleter = spawn_thread(&_ConnectionDeleterEntry,
		"connection deleter", B_NORMAL_PRIORITY, this);
	if (fConnectionDeleter < 0)
		return fConnectionDeleter;

	// init the server info connection listener
	error = _InitServerInfoConnectionListener();
	if (error != B_OK)
		return error;

	// create the broadcaster semaphore
	fBroadcasterSemaphore = create_sem(0, "broadcaster snooze");

	// spawn the broadcaster
	if (fUseBroadcasting) {
		fBroadcaster = spawn_thread(&_BroadcasterEntry, "broadcaster",
			B_NORMAL_PRIORITY, this);
		if (fBroadcaster < 0) {
			WARN("NetFSServer::Init(): Failed to spawn broadcaster thread "
				"(%s). Continuing anyway.\n", strerror(fBroadcaster));
		}
	}
	return B_OK;
}

// Run
thread_id
NetFSServer::Run()
{
	// start the connection listener
	resume_thread(fConnectionListenerThread);

	// start the connection deleter
	resume_thread(fConnectionDeleter);

	// start the server info connection listener
	resume_thread(fServerInfoConnectionListener);

	// start the broadcaster
	resume_thread(fBroadcaster);

	return BApplication::Run();
}

// MessageReceived
void
NetFSServer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case NETFS_REQUEST_GET_MESSENGER:
		{
			// for the time being we process all requests here
			BMessage reply;
			reply.AddMessenger("messenger", be_app_messenger);
			_SendReply(message, &reply);
			break;
		}

		case NETFS_REQUEST_ADD_USER:
		{
			// get user name and password
			const char* user;
			const char* password;
			if (message->FindString("user", &user) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}
			if (message->FindString("password", &password) != B_OK)
				password = NULL;

			// add the user
			status_t error = fSecurityContext->AddUser(user, password);
			_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_REMOVE_USER:
		{
			// get user name
			const char* userName;
			if (message->FindString("user", &userName) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			// remove the user
			User* user;
			status_t error = fSecurityContext->RemoveUser(userName, &user);
			if (error == B_OK) {
				// propagate the information to the client connections
				AutoLocker<Locker> _(fLock);
				for (int32 i = 0;
					 ClientConnection* connection
					 	= (ClientConnection*)fClientConnections.ItemAt(i);
					 i++) {
					connection->UserRemoved(user);
				}

				user->ReleaseReference();
			}

			_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_GET_USERS:
		{
			// get the users
			BMessage reply;
			BMessage users;
			status_t error = fSecurityContext->GetUsers(&users);
			if (error == B_OK)
				error = reply.AddMessage("users", &users);

			if (error == B_OK)
				_SendReply(message, &reply);
			else
				_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_GET_USER_STATISTICS:
		{
			// get user name
			const char* userName;
			if (message->FindString("user", &userName) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			// get the user
			User* user = fSecurityContext->FindUser(userName);
			if (!user) {
				_SendReply(message, B_ENTRY_NOT_FOUND);
				break;
			}
			BReference<User> userReference(user, true);

			// get the statistics
			BMessage statistics;
			status_t error = StatisticsManager::GetDefault()
				->GetUserStatistics(user, &statistics);

			// prepare the reply
			BMessage reply;
			if (error == B_OK)
				error = reply.AddMessage("statistics", &statistics);

			// send the reply
			if (error == B_OK)
				_SendReply(message, &reply);
			else
				_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_ADD_SHARE:
		{
			// get share name and path
			const char* share;
			const char* path;
			if (message->FindString("share", &share) != B_OK
				|| message->FindString("path", &path) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			// add the share
			status_t error = fSecurityContext->AddShare(share, path);

			if (error == B_OK)
				_ServerInfoUpdated();

			_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_REMOVE_SHARE:
		{
			// get share name
			const char* shareName;
			if (message->FindString("share", &shareName) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			// remove the share
			Share* share;
			status_t error = fSecurityContext->RemoveShare(shareName, &share);
			if (error == B_OK) {
				// propagate the information to the client connections
				AutoLocker<Locker> _(fLock);
				for (int32 i = 0;
					 ClientConnection* connection
					 	= (ClientConnection*)fClientConnections.ItemAt(i);
					 i++) {
					connection->ShareRemoved(share);
				}

				share->ReleaseReference();
			}

			if (error == B_OK)
				_ServerInfoUpdated();

			_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_GET_SHARES:
		{
			// get the shares
			BMessage reply;
			BMessage shares;
			status_t error = fSecurityContext->GetShares(&shares);
			if (error == B_OK)
				error = reply.AddMessage("shares", &shares);

			if (error == B_OK)
				_SendReply(message, &reply);
			else
				_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_GET_SHARE_USERS:
		{
			// get share name
			const char* shareName;
			if (message->FindString("share", &shareName) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			AutoLocker<Locker> securityContextLocker(fSecurityContext);

			// get the share
			Share* share = fSecurityContext->FindShare(shareName);
			if (!share) {
				_SendReply(message, B_ENTRY_NOT_FOUND);
				break;
			}
			BReference<Share> shareReference(share, true);

			// get all users
			BMessage allUsers;
			status_t error = fSecurityContext->GetUsers(&allUsers);
			if (error != B_OK) {
				_SendReply(message, error);
				break;
			}

			// filter the users with mount permission
			BMessage users;
			const char* userName;
			for (int32 i = 0;
				 allUsers.FindString("users", i, &userName) == B_OK;
				 i++) {
				if (User* user = fSecurityContext->FindUser(userName)) {
					// get the user's permissions
					Permissions permissions = fSecurityContext
						->GetNodePermissions(share->GetPath(), user);
					user->ReleaseReference();

					// add the user, if they have the permission to mount the
					// share
					if (permissions.ImpliesMountSharePermission()) {
						error = users.AddString("users", userName);
						if (error != B_OK) {
							_SendReply(message, error);
							break;
						}
					}
				}
			}

			securityContextLocker.Unlock();

			// prepare the reply
			BMessage reply;
			if (error == B_OK)
				error = reply.AddMessage("users", &users);

			// send the reply
			if (error == B_OK)
				_SendReply(message, &reply);
			else
				_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_GET_SHARE_STATISTICS:
		{
			// get share name
			const char* shareName;
			if (message->FindString("share", &shareName) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			// get the share
			Share* share = fSecurityContext->FindShare(shareName);
			if (!share) {
				_SendReply(message, B_ENTRY_NOT_FOUND);
				break;
			}
			BReference<Share> shareReference(share, true);

			// get the statistics
			BMessage statistics;
			status_t error = StatisticsManager::GetDefault()
				->GetShareStatistics(share, &statistics);

			// prepare the reply
			BMessage reply;
			if (error == B_OK)
				error = reply.AddMessage("statistics", &statistics);

			// send the reply
			if (error == B_OK)
				_SendReply(message, &reply);
			else
				_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_SET_USER_PERMISSIONS:
		{
			// get share and user name, and the permissions
			const char* shareName;
			const char* userName;
			uint32 permissions;
			if (message->FindString("share", &shareName) != B_OK
				|| message->FindString("user", &userName) != B_OK
				|| message->FindInt32("permissions", (int32*)&permissions)
					!= B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			// get the share and the user
			Share* share = fSecurityContext->FindShare(shareName);
			User* user = fSecurityContext->FindUser(userName);
			BReference<Share> shareReference(share);
			BReference<User> userReference(user);
			if (!share || !user) {
				_SendReply(message, B_ENTRY_NOT_FOUND);
				break;
			}

			// set the permissions
			status_t error = B_OK;
			if (permissions == 0) {
				fSecurityContext->ClearNodePermissions(share->GetPath(), user);
			} else {
				error = fSecurityContext->SetNodePermissions(share->GetPath(),
					user, permissions);
			}

			if (error == B_OK) {
				// propagate the information to the client connections
				AutoLocker<Locker> _(fLock);
				for (int32 i = 0;
					 ClientConnection* connection
					 	= (ClientConnection*)fClientConnections.ItemAt(i);
					 i++) {
					connection->UserPermissionsChanged(share, user,
						permissions);
				}
			}

			_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_GET_USER_PERMISSIONS:
		{
			// get share and user name
			const char* shareName;
			const char* userName;
			if (message->FindString("share", &shareName) != B_OK
				|| message->FindString("user", &userName) != B_OK) {
				_SendReply(message, B_BAD_VALUE);
				break;
			}

			// get the share and the user
			Share* share = fSecurityContext->FindShare(shareName);
			User* user = fSecurityContext->FindUser(userName);
			BReference<Share> shareReference(share);
			BReference<User> userReference(user);
			if (!share || !user) {
				_SendReply(message, B_ENTRY_NOT_FOUND);
				break;
			}

			// get the permissions
			Permissions permissions = fSecurityContext->GetNodePermissions(
				share->GetPath(), user);

			// prepare the reply
			BMessage reply;
			status_t error = reply.AddInt32("permissions",
				(int32)permissions.GetPermissions());

			// send it
			if (error == B_OK)
				_SendReply(message, &reply);
			else
				_SendReply(message, error);
			break;
		}

		case NETFS_REQUEST_SAVE_SETTINGS:
		{
			status_t error = _SaveSettings();

			// send a reply
			_SendReply(message, error);
			break;
		}
	}
}

// GetVolumeManager
VolumeManager*
NetFSServer::GetVolumeManager() const
{
	return fVolumeManager;
}

// GetSecurityContext
SecurityContext*
NetFSServer::GetSecurityContext() const
{
	return fSecurityContext;
}

// _AddClientConnection
status_t
NetFSServer::_AddClientConnection(ClientConnection* clientConnection)
{
	if (!clientConnection)
		return B_BAD_VALUE;
	AutoLocker<Locker> locker(fLock);
	if (!fClientConnections.AddItem(clientConnection))
		return B_NO_MEMORY;
	return B_OK;
}

// ClientConnectionClosed
void
NetFSServer::ClientConnectionClosed(ClientConnection* connection, bool broken)
{
	PRINT("NetFSServer::ClientConnectionClosed(%d)\n", broken);
	if (!connection)
		return;
	AutoLocker<Locker> locker(fLock);
	if (!fClientConnections.RemoveItem(connection))
		return;
	if (!fClosedConnections.AddItem(connection)) {
		// out of memory: Try to delete the connection right now.,
		// There's a certain chance that we'll access free()d memory in the
		// process, but things are apparently bad enough, anyway.
		locker.Unlock();
		delete connection;
		return;
	}
	release_sem(fClosedConnectionsSemaphore);
}

// _LoadSecurityContext
status_t
NetFSServer::_LoadSecurityContext(SecurityContext** _securityContext)
{
	// create a security context
	SecurityContext* securityContext = new(std::nothrow) SecurityContext;
	if (!securityContext)
		return B_NO_MEMORY;
	status_t error = securityContext->InitCheck();
	if (error != B_OK) {
		delete securityContext;
		return error;
	}
	ObjectDeleter<SecurityContext> securityContextDeleter(securityContext);

	// load the fallback settings, if present
	BPath path;
	DriverSettings settings;

	if (_GetSettingsDirPath(&path, false) == B_OK
			&& path.Append(kFallbackSettingsFileName) == B_OK
			&& settings.Load(path.Path()) == B_OK) {
		// load users
		DriverParameter parameter;
		for (DriverParameterIterator it = settings.GetParameterIterator("user");
			 it.GetNext(&parameter);) {
			const char* userName = parameter.ValueAt(0);
			const char* password = parameter.GetParameterValue("password");
			if (!userName) {
				WARN("Skipping nameless user settings entry.\n");
				continue;
			}
//			PRINT(("user: %s, password: %s\n", parameter.ValueAt(0),
//				parameter.GetParameterValue("password")));
			error = securityContext->AddUser(userName, password);
			if (error != B_OK)
				ERROR("ERROR: Failed to add user `%s'\n", userName);
		}

		// load shares
		for (DriverParameterIterator it = settings.GetParameterIterator("share");
			 it.GetNext(&parameter);) {
			const char* shareName = parameter.ValueAt(0);
			const char* path = parameter.GetParameterValue("path");
			if (!shareName || !path) {
				WARN("settings: Skipping invalid share settings entry (no name"
					" or no path).\n");
				continue;
			}
//			PRINT(("share: %s, path: %s\n", parameter.ValueAt(0),
//				parameter.GetParameterValue("path")));
			Share* share;
			error = securityContext->AddShare(shareName, path, &share);
			if (error != B_OK) {
				ERROR("ERROR: Failed to add share `%s'\n", shareName);
				continue;
			}
			BReference<Share> shareReference(share, true);
			DriverParameter userParameter;
			// iterate through the share users
			for (DriverParameterIterator userIt
					= parameter.GetParameterIterator("user");
				 userIt.GetNext(&userParameter);) {
				const char* userName = userParameter.ValueAt(0);
//				PRINT(("  user: %s\n", userName));
				User* user = securityContext->FindUser(userName);
				if (!user) {
					ERROR("ERROR: Undefined user `%s'.\n", userName);
					continue;
				}
				BReference<User> userReference(user, true);
				DriverParameter permissionsParameter;
				if (!userParameter.FindParameter("permissions",
						&permissionsParameter)) {
					continue;
				}
				Permissions permissions;
				for (int32 i = 0; i < permissionsParameter.CountValues(); i++) {
					const char* permission = permissionsParameter.ValueAt(i);
//					PRINT(("    permission: %s\n", permission));
					if (strcmp(permission, "mount") == 0) {
						permissions.AddPermissions(MOUNT_SHARE_PERMISSION);
					} else if (strcmp(permission, "query") == 0) {
						permissions.AddPermissions(QUERY_SHARE_PERMISSION);
					} else if (strcmp(permission, "read") == 0) {
						permissions.AddPermissions(READ_PERMISSION
							| READ_DIR_PERMISSION | RESOLVE_DIR_ENTRY_PERMISSION);
					} else if (strcmp(permission, "write") == 0) {
						permissions.AddPermissions(WRITE_PERMISSION
							| WRITE_DIR_PERMISSION);
					} else if (strcmp(permission, "all") == 0) {
						permissions.AddPermissions(ALL_PERMISSIONS);
					}
				}
				error = securityContext->SetNodePermissions(share->GetPath(), user,
					permissions);
				if (error != B_OK) {
					ERROR("ERROR: Failed to set permissions for share `%s'\n",
						share->GetName());
				}
			}
		}
	}

	securityContextDeleter.Detach();
	*_securityContext = securityContext;
	return B_OK;
}

// _InitSettings
status_t
NetFSServer::_InitSettings()
{
	status_t error = _LoadSettings();
	if (error != B_OK) {
		WARN("NetFSServer::_InitSettings(): WARNING: Failed to load settings "
			"file: %s - falling back to driver settings.\n", strerror(error));

		// fall back to the driver settings file
		error = _LoadSecurityContext(&fSecurityContext);
		if (error != B_OK) {
			WARN("NetFSServer::_InitSettings(): WARNING: Failed to load "
				"settings from driver settings: %s\n", strerror(error));

			// use defaults
			// create a security context
			fSecurityContext = new(std::nothrow) SecurityContext;
			if (!fSecurityContext)
				return B_NO_MEMORY;
			error = fSecurityContext->InitCheck();
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}

// _LoadSettings
status_t
NetFSServer::_LoadSettings()
{
	// get the settings file path
	BPath filePath;
	status_t error = _GetSettingsFilePath(&filePath, false);
	if (error != B_OK)
		RETURN_ERROR(error);

	// if existing load the settings
	BEntry bEntry;
	if (FDManager::SetEntry(&bEntry, filePath.Path()) != B_OK
		|| !bEntry.Exists()) {
		return B_ENTRY_NOT_FOUND;
	}

	// open the settings file
	BFile file;
	error = FDManager::SetFile(&file, filePath.Path(), B_READ_ONLY);
	if (error != B_OK)
		RETURN_ERROR(error);

	// read the settings
	BMessage settings;
	error = settings.Unflatten(&file);
	if (error != B_OK)
		RETURN_ERROR(error);

	// get the security context archive
	BMessage securityContextArchive;
	error = settings.FindMessage("security context",
		&securityContextArchive);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create a security context
	SecurityContext* securityContext
		= new(std::nothrow) SecurityContext(&securityContextArchive);
	if (!securityContext)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<SecurityContext> securityContextDeleter(securityContext);
	error = securityContext->InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	// set it
	delete fSecurityContext;
	fSecurityContext = securityContext;
	securityContextDeleter.Detach();

	return B_OK;
}

// _SaveSettings
status_t
NetFSServer::_SaveSettings()
{
	AutoLocker<Locker> locker(fSecurityContext);

	// create the settings archive
	BMessage settings;

	// archive the security context
	BMessage securityContextArchive;
	status_t error = fSecurityContext->Archive(&securityContextArchive, true);
	if (error != B_OK)
		RETURN_ERROR(error);

	// add it to the settings archive
	error = settings.AddMessage("security context", &securityContextArchive);
	if (error != B_OK)
		RETURN_ERROR(error);

	// open the settings file
	BPath filePath;
	error = _GetSettingsFilePath(&filePath, true);
	if (error != B_OK)
		RETURN_ERROR(error);
	BFile file;
	error = FDManager::SetFile(&file, filePath.Path(),
		B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (error != B_OK)
		RETURN_ERROR(error);

	// write to the settings file
	error = settings.Flatten(&file);
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}

// _GetSettingsDirPath
status_t
NetFSServer::_GetSettingsDirPath(BPath* path, bool create)
{
	// get the user settings directory
	BPath settingsDir;
	status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, &settingsDir,
		create);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create our subdir, if not existing and desired
	error = path->SetTo(settingsDir.Path(), kSettingsDirName);
	if (error != B_OK)
		RETURN_ERROR(error);
	BEntry bEntry;
	if (create
		&& (FDManager::SetEntry(&bEntry, settingsDir.Path()) != B_OK
			|| !bEntry.Exists())) {
		error = create_directory(path->Path(), S_IRWXU | S_IRWXG | S_IRWXO);
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	return B_OK;
}

// _GetSettingsFilePath
status_t
NetFSServer::_GetSettingsFilePath(BPath* path, bool createDir)
{
	// get settings dir
	BPath dirPath;
	status_t error = _GetSettingsDirPath(&dirPath, createDir);
	if (error != B_OK)
		return error;

	// construct the file path
	return path->SetTo(dirPath.Path(), kSettingsFileName);
}

// _InitServerInfoConnectionListener
status_t
NetFSServer::_InitServerInfoConnectionListener()
{
	// spawn the listener thread
	fServerInfoConnectionListener = spawn_thread(
		&_ServerInfoConnectionListenerEntry,
		"server info connection listener", B_NORMAL_PRIORITY, this);
	if (fServerInfoConnectionListener < 0)
		return fServerInfoConnectionListener;
	// create a listener socket
	fServerInfoConnectionListenerSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fServerInfoConnectionListenerSocket < 0)
		return errno;
	// bind it to the port
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(kDefaultServerInfoPort);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fServerInfoConnectionListenerSocket, (sockaddr*)&addr,
		sizeof(addr)) < 0) {
		return errno;
	}
	// start listening
	if (listen(fServerInfoConnectionListenerSocket, 5) < 0)
		return errno;
	return B_OK;
}

// _ExitServerInfoConnectionListener
void
NetFSServer::_ExitServerInfoConnectionListener()
{
	// close the socket
	safe_closesocket(fServerInfoConnectionListenerSocket);
	// wait for the listener
	if (fServerInfoConnectionListener >= 0) {
		int32 result;
		wait_for_thread(fServerInfoConnectionListener, &result);
	}
}

// _ConnectionListenerEntry
int32
NetFSServer::_ConnectionListenerEntry(void* data)
{
	return ((NetFSServer*)data)->_ConnectionListener();
}

// _ConnectionListener
int32
NetFSServer::_ConnectionListener()
{
	// start listening for connections
	Connection* connection = NULL;
	status_t error = B_OK;
	do {
		error = fConnectionListener->Listen(&connection);
		if (error == B_OK) {
			ConnectionInitializer* initializer
				= new(std::nothrow) ConnectionInitializer(this, fConnectionListener,
					connection);
			if (initializer) {
				if (initializer->Run() != B_OK) {
					ERROR("Failed to run connection initializer.\n")
					delete initializer;
				}
			} else {
				ERROR("Failed to create connection initializer.\n")
				delete connection;
			}

		}
	} while (error == B_OK && !fTerminating);

	return 0;
}

// _ConnectionDeleterEntry
int32
NetFSServer::_ConnectionDeleterEntry(void* data)
{
	return ((NetFSServer*)data)->_ConnectionDeleter();
}

// _ConnectionDeleter
int32
NetFSServer::_ConnectionDeleter()
{
	while (!fTerminating) {
		status_t error = acquire_sem(fClosedConnectionsSemaphore);
		ClientConnection* connection = NULL;
		if (error == B_OK) {
			AutoLocker<Locker> _(fLock);
			connection = (ClientConnection*)fClosedConnections.RemoveItem((int32)0);
		}
		if (connection)
			delete connection;
	}
	return 0;
}

// _BroadcasterEntry
int32
NetFSServer::_BroadcasterEntry(void* data)
{
	return ((NetFSServer*)data)->_Broadcaster();
}

// _Broadcaster
int32
NetFSServer::_Broadcaster()
{
	// create the socket
	fBroadcastingSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fBroadcastingSocket < 0) {
		WARN("NetFSServer::_Broadcaster(): WARN: Failed to init broadcasting: "
			"%s.\n", strerror(errno));
		return errno;
	}

	// set the socket broadcast option
	#ifndef HAIKU_TARGET_PLATFORM_BEOS
		int soBroadcastValue = 1;
		if (setsockopt(fBroadcastingSocket, SOL_SOCKET, SO_BROADCAST,
			&soBroadcastValue, sizeof(soBroadcastValue)) < 0) {
			WARN("NetFSServer::_Broadcaster(): WARN: Failed to set "
				"SO_BROADCAST on socket: %s.\n", strerror(errno));
		}
	#endif

	// prepare the broadcast message
	BroadcastMessage message;
	message.magic = B_HOST_TO_BENDIAN_INT32(BROADCAST_MESSAGE_MAGIC);
	message.protocolVersion = B_HOST_TO_BENDIAN_INT32(NETFS_PROTOCOL_VERSION);

	bool update = false;
	while (!fTerminating) {
		// set tick/update
		uint32 messageCode = (update ? BROADCAST_MESSAGE_SERVER_UPDATE
			: BROADCAST_MESSAGE_SERVER_TICK);
		message.message = B_HOST_TO_BENDIAN_INT32(messageCode);

		// send broadcasting message
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(kDefaultBroadcastPort);
		addr.sin_addr.s_addr = INADDR_BROADCAST;
		int addrSize = sizeof(addr);
		ssize_t bytesSent = sendto(fBroadcastingSocket, &message,
			sizeof(message), 0, (sockaddr*)&addr, addrSize);
		if (bytesSent < 0) {
			WARN("NetFSServer::_Broadcaster(): WARN: sending failed: %s.\n",
				strerror(errno));
			return errno;
		}

		// snooze a bit
		// we snooze a minimal interval to avoid shooting updates like a
		// machine gun
		snooze(kMinBroadcastingInterval);
		bigtime_t remainingTime = kBroadcastingInterval
			- kMinBroadcastingInterval;

		// snooze the rest blocking on our semaphore (if it exists)
		if (fBroadcasterSemaphore >= 0) {
			status_t snoozeError = acquire_sem_etc(fBroadcasterSemaphore, 1,
				B_RELATIVE_TIMEOUT, remainingTime);

			// set the semaphore count back to zero
			while (snoozeError == B_OK) {
				snoozeError = acquire_sem_etc(fBroadcasterSemaphore, 1,
					B_RELATIVE_TIMEOUT, 0);
			}
		} else
			snooze(remainingTime);

		update = atomic_and(&fServerInfoUpdated, 0);
	}

	// close the socket
	safe_closesocket(fBroadcastingSocket);
	return B_OK;
}

// _ServerInfoConnectionListenerEntry
int32
NetFSServer::_ServerInfoConnectionListenerEntry(void* data)
{
	return ((NetFSServer*)data)->_ServerInfoConnectionListener();
}

// _ServerInfoConnectionListener
int32
NetFSServer::_ServerInfoConnectionListener()
{
	if (fServerInfoConnectionListenerSocket < 0)
		return B_BAD_VALUE;

	TaskManager taskManager;

	// accept a incoming connection
	while (!fTerminating) {
		int fd = -1;
		do {
			taskManager.RemoveDoneTasks();

			fd = accept(fServerInfoConnectionListenerSocket, NULL, 0);
			if (fd < 0) {
				status_t error = errno;
				if (error != B_INTERRUPTED)
					return error;
				if (fTerminating)
					return B_OK;
			}
		} while (fd < 0);

		// get a fresh server info
		ServerInfo info;
		status_t error = _GetServerInfo(info);
		if (error != B_OK) {
			closesocket(fd);
			return error;
		}

		// create a server info sender thread
		ServerInfoSender* sender = new(std::nothrow) ServerInfoSender(fd, info);
		if (sender == NULL) {
			closesocket(fd);
			delete sender;
			return B_NO_MEMORY;
		}
		if ((error = sender->Init()) != B_OK) {
			closesocket(fd);
			delete sender;
			return error;
		}
		taskManager.RunTask(sender);
	}

	return B_OK;
}

// _GetServerInfo
status_t
NetFSServer::_GetServerInfo(ServerInfo& serverInfo)
{
	// set the server name and the connection method
	char hostName[1024];
	if (gethostname(hostName, sizeof(hostName)) < 0) {
		ERROR("NetFSServer::_GetServerInfo(): ERROR: Failed to get host "
			"name.");
		return B_ERROR;
	}
	status_t error = serverInfo.SetServerName(hostName);
// TODO: Set the actually used connection method!
	if (error == B_OK)
		error = serverInfo.SetConnectionMethod("insecure");
	if (error != B_OK)
		return error;

	// get the shares from the security context
	BMessage shares;
	error = fSecurityContext->GetShares(&shares);
	if (error != B_OK)
		return error;

	// add the shares
	const char* shareName;
	for (int32 i = 0; shares.FindString("shares", i, &shareName) == B_OK; i++) {
		error = serverInfo.AddShare(shareName);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}

// _ServerInfoUpdated
void
NetFSServer::_ServerInfoUpdated()
{
	atomic_or(&fServerInfoUpdated, 1);
	release_sem(fBroadcasterSemaphore);
}

// _SendReply
void
NetFSServer::_SendReply(BMessage* message, BMessage* reply, status_t error)
{
	// no reply is specified, if no data have to be sent
	BMessage stackReply;
	if (!reply)
		reply = &stackReply;

	reply->AddInt32("error", error);
	message->SendReply(reply, (BHandler*)NULL, 0LL);
}

// _SendReply
void
NetFSServer::_SendReply(BMessage* message, status_t error)
{
	_SendReply(message, NULL, error);
}


// #pragma mark -

// print_usage
static
void
print_usage(bool error)
{
	fprintf((error ? stderr : stdout), kUsage);
}

// main
int
main(int argc, char** argv)
{
	#ifdef DEBUG_OBJECT_TRACKING
		ObjectTracker::InitDefault();
	#endif

	// parse the arguments
	bool broadcast = true;
	for (int argi = 1; argi < argc; argi++) {
		const char* arg = argv[argi];
		if (strcmp(arg, "--dont-broadcast") == 0) {
			broadcast = false;
		} else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
			print_usage(false);
			return 0;
		} else {
			print_usage(true);
			return 1;
		}
	}

	// create the statistics manager
	status_t error = StatisticsManager::CreateDefault();
	if (error != B_OK) {
		fprintf(stderr, "Failed to create statistics manager: %s\n",
			strerror(error));
		return 1;
	}

	// init and run the server
	{
		NetFSServer server(broadcast);
		error = server.Init();
		if (error != B_OK) {
			fprintf(stderr, "Failed to initialize server: %s\n",
				strerror(error));
			return 1;
		}
		server.Run();
	}

	// delete the statistics manager
	StatisticsManager::DeleteDefault();

	#ifdef DEBUG_OBJECT_TRACKING
		ObjectTracker::ExitDefault();
	#endif

	return 0;
}

