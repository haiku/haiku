// AuthenticationServer.cpp

#include "AuthenticationServer.h"

#include <new>

#include <HashMap.h>
#include <HashString.h>
#include <util/KMessage.h>

#include "AuthenticationPanel.h"
#include "AuthenticationServerDefs.h"
#include "DebugSupport.h"
#include "TaskManager.h"


// Authentication
class AuthenticationServer::Authentication {
public:
	Authentication()
		: fUser(),
		  fPassword()
	{
	}

	Authentication(const char* user, const char* password)
		: fUser(user),
		  fPassword(password)
	{
	}

	status_t SetTo(const char* user, const char* password)
	{
		if (fUser.SetTo(user) && fPassword.SetTo(password))
			return B_OK;
		return B_NO_MEMORY;
	}

	bool IsValid() const
	{
		return (fUser.GetLength() > 0);
	}

	const char* GetUser() const
	{
		return fUser.GetString();
	}

	const char* GetPassword() const
	{
		return fPassword.GetString();
	}

private:
	HashString	fUser;
	HashString	fPassword;
};

// ServerKey
class AuthenticationServer::ServerKey {
public:
	ServerKey()
		: fContext(),
		  fServer()
	{
	}

	ServerKey(const char* context, const char* server)
		: fContext(context),
		  fServer(server)
	{
	}

	ServerKey(const ServerKey& other)
		: fContext(other.fContext),
		  fServer(other.fServer)
	{
	}

	uint32 GetHashCode() const
	{
		return fContext.GetHashCode() * 17 + fServer.GetHashCode();
	}

	ServerKey& operator=(const ServerKey& other)
	{
		fContext = other.fContext;
		fServer = other.fServer;
		return *this;
	}

	bool operator==(const ServerKey& other) const
	{
		return (fContext == other.fContext && fServer == other.fServer);
	}

	bool operator!=(const ServerKey& other) const
	{
		return !(*this == other);
	}

private:
	HashString	fContext;
	HashString	fServer;
};

// ServerEntry
class AuthenticationServer::ServerEntry {
public:
	ServerEntry()
		: fDefaultAuthentication(),
		  fUseDefaultAuthentication(false)
	{
	}

	~ServerEntry()
	{
		// delete the authentications
		for (AuthenticationMap::Iterator it = fAuthentications.GetIterator();
			 it.HasNext();) {
			delete it.Next().value;
		}
	}

	void SetUseDefaultAuthentication(bool useDefaultAuthentication)
	{
		fUseDefaultAuthentication = useDefaultAuthentication;
	}

	bool UseDefaultAuthentication() const
	{
		return fUseDefaultAuthentication;
	}

	status_t SetDefaultAuthentication(const char* user, const char* password)
	{
		return fDefaultAuthentication.SetTo(user, password);
	}

	const Authentication& GetDefaultAuthentication() const
	{
		return fDefaultAuthentication;
	}

	status_t SetAuthentication(const char* share, const char* user,
		const char* password)
	{
		// check, if an entry already exists for the share -- if it does,
		// just set it
		Authentication* authentication = fAuthentications.Get(share);
		if (authentication)
			return authentication->SetTo(user, password);
		// the entry does not exist yet: create and add a new one
		authentication = new(std::nothrow) Authentication;
		if (!authentication)
			return B_NO_MEMORY;
		status_t error = authentication->SetTo(user, password);
		if (error == B_OK)
			error = fAuthentications.Put(share, authentication);
		if (error != B_OK)
			delete authentication;
		return error;
	}

	Authentication* GetAuthentication(const char* share) const
	{
		return fAuthentications.Get(share);
	}

private:
	typedef HashMap<HashString, Authentication*> AuthenticationMap;

	Authentication		fDefaultAuthentication;
	bool				fUseDefaultAuthentication;
	AuthenticationMap	fAuthentications;
};

// ServerEntryMap
struct AuthenticationServer::ServerEntryMap
	: HashMap<ServerKey, ServerEntry*> {
};

// UserDialogTask
class AuthenticationServer::UserDialogTask : public Task {
public:
	UserDialogTask(AuthenticationServer* authenticationServer,
		const char* context, const char* server, const char* share,
		bool badPassword, port_id replyPort,
		int32 replyToken)
		: Task("user dialog task"),
		  fAuthenticationServer(authenticationServer),
		  fContext(context),
		  fServer(server),
		  fShare(share),
		  fBadPassword(badPassword),
		  fReplyPort(replyPort),
		  fReplyToken(replyToken),
		  fPanel(NULL)
	{
	}

	virtual status_t Execute()
	{
		// open the panel
		char user[B_OS_NAME_LENGTH];
		char password[B_OS_NAME_LENGTH];
		bool keep = true;
		fPanel = new(std::nothrow) AuthenticationPanel();
		status_t error = (fPanel ? B_OK : B_NO_MEMORY);
		bool cancelled = false;
		HashString defaultUser;
		HashString defaultPassword;
		fAuthenticationServer->_GetAuthentication(fContext.GetString(),
			fServer.GetString(), NULL, &defaultUser, &defaultPassword);
		if (error == B_OK) {
			cancelled = fPanel->GetAuthentication(fServer.GetString(),
				fShare.GetString(), defaultUser.GetString(),
				defaultPassword.GetString(), false, fBadPassword, user,
				password, &keep);
		}
		fPanel = NULL;
		// send the reply
		if (error != B_OK) {
			fAuthenticationServer->_SendRequestReply(fReplyPort, fReplyToken,
				error, true, NULL, NULL);
		} else if (cancelled) {
			fAuthenticationServer->_SendRequestReply(fReplyPort, fReplyToken,
				B_OK, true, NULL, NULL);
		} else {
			fAuthenticationServer->_AddAuthentication(fContext.GetString(),
				fServer.GetString(), fShare.GetString(), user, password,
				keep);
			fAuthenticationServer->_SendRequestReply(fReplyPort, fReplyToken,
				B_OK, false, user, password);
		}
		return error;
	}

	virtual void Stop()
	{
		if (fPanel)
			fPanel->Cancel();
	}

private:
	AuthenticationServer*	fAuthenticationServer;
	HashString				fContext;
	HashString				fServer;
	HashString				fShare;
	bool					fBadPassword;
	port_id					fReplyPort;
	int32					fReplyToken;
	AuthenticationPanel*	fPanel;
};


// constructor
AuthenticationServer::AuthenticationServer()
	:
	BApplication("application/x-vnd.haiku-authentication_server"),
	fLock(),
	fRequestPort(-1),
	fRequestThread(-1),
	fServerEntries(NULL),
	fTerminating(false)
{
}

// destructor
AuthenticationServer::~AuthenticationServer()
{
	fTerminating = true;
	// terminate the request thread
	if (fRequestPort >= 0)
		delete_port(fRequestPort);
	if (fRequestThread >= 0) {
		int32 result;
		wait_for_thread(fRequestPort, &result);
	}
	// delete the server entries
	for (ServerEntryMap::Iterator it = fServerEntries->GetIterator();
		 it.HasNext();) {
		delete it.Next().value;
	}
}

// Init
status_t
AuthenticationServer::Init()
{
	// create the server entry map
	fServerEntries = new(std::nothrow) ServerEntryMap;
	if (!fServerEntries)
		return B_NO_MEMORY;
	status_t error = fServerEntries->InitCheck();
	if (error != B_OK)
		return error;
	// create the request port
	fRequestPort = create_port(10, kAuthenticationServerPortName);
	if (fRequestPort < 0)
		return fRequestPort;
	// spawn the request thread
	fRequestThread = spawn_thread(&_RequestThreadEntry, "request thread",
		B_NORMAL_PRIORITY, this);
	if (fRequestThread < 0)
		return fRequestThread;
	resume_thread(fRequestThread);
	return B_OK;
}

// _RequestThreadEntry
int32
AuthenticationServer::_RequestThreadEntry(void* data)
{
	return ((AuthenticationServer*)data)->_RequestThread();
}

// _RequestThread
int32
AuthenticationServer::_RequestThread()
{
	TaskManager taskManager;
	while (!fTerminating) {
		taskManager.RemoveDoneTasks();
		// read the request
		KMessage request;
		status_t error = request.ReceiveFrom(fRequestPort);
		if (error != B_OK)
			continue;
		// get the parameters
		const char* context = NULL;
		const char* server = NULL;
		const char* share = NULL;
		bool badPassword = true;
		request.FindString("context", &context);
		request.FindString("server", &server);
		request.FindString("share", &share);
		request.FindBool("badPassword", &badPassword);
		if (!context || !server || !share)
			continue;
		HashString foundUser;
		HashString foundPassword;
		if (!badPassword && _GetAuthentication(context, server, share,
			&foundUser, &foundPassword)) {
			_SendRequestReply(request.ReplyPort(), request.ReplyToken(),
				error, false, foundUser.GetString(), foundPassword.GetString());
		} else {
			// we need to ask the user: create a task that does it
			UserDialogTask* task = new(std::nothrow) UserDialogTask(this,
				context, server, share, badPassword, request.ReplyPort(),
				request.ReplyToken());
			if (!task) {
				ERROR("AuthenticationServer::_RequestThread(): ERROR: "
					"failed to allocate ");
				continue;
			}
			status_t error = taskManager.RunTask(task);
			if (error != B_OK) {
				ERROR("AuthenticationServer::_RequestThread(): Failed to "
					"start server info task: %s\n", strerror(error));
				continue;
			}
		}
	}
	return 0;
}

// _GetAuthentication
/*!
	If share is NULL, the default authentication for the server is returned.
*/
bool
AuthenticationServer::_GetAuthentication(const char* context,
	const char* server, const char* share, HashString* user,
	HashString* password)
{
	if (!context || !server || !user || !password)
		return B_BAD_VALUE;
	// get the server entry
	AutoLocker<BLocker> _(fLock);
	ServerKey key(context, server);
	ServerEntry* serverEntry = fServerEntries->Get(key);
	if (!serverEntry)
		return false;
	// get the authentication
	const Authentication* authentication = NULL;
	if (share) {
		serverEntry->GetAuthentication(share);
		if (!authentication && serverEntry->UseDefaultAuthentication())
			authentication = &serverEntry->GetDefaultAuthentication();
	} else
		authentication = &serverEntry->GetDefaultAuthentication();
	if (!authentication || !authentication->IsValid())
		return false;
	return (user->SetTo(authentication->GetUser())
		&& password->SetTo(authentication->GetPassword()));
}

// _AddAuthentication
status_t
AuthenticationServer::_AddAuthentication(const char* context,
	const char* server, const char* share, const char* user,
	const char* password, bool makeDefault)
{
	AutoLocker<BLocker> _(fLock);
	ServerKey key(context, server);
	// get the server entry
	ServerEntry* serverEntry = fServerEntries->Get(key);
	if (!serverEntry) {
		// server entry does not exist yet: create a new one
		serverEntry = new(std::nothrow) ServerEntry;
		if (!serverEntry)
			return B_NO_MEMORY;
		status_t error = fServerEntries->Put(key, serverEntry);
		if (error != B_OK) {
			delete serverEntry;
			return error;
		}
	}
	// put the authentication
	status_t error = serverEntry->SetAuthentication(share, user, password);
	if (error == B_OK) {
		if (makeDefault || !serverEntry->UseDefaultAuthentication())
			serverEntry->SetDefaultAuthentication(user, password);
		if (makeDefault)
			serverEntry->SetUseDefaultAuthentication(true);
	}
	return error;
}

// _SendRequestReply
status_t
AuthenticationServer::_SendRequestReply(port_id port, int32 token,
	status_t error, bool cancelled, const char* user, const char* password)
{
	// prepare the reply
	KMessage reply;
	reply.AddInt32("error", error);
	if (error == B_OK) {
		reply.AddBool("cancelled", cancelled);
		if (!cancelled) {
			reply.AddString("user", user);
			reply.AddString("password", password);
		}
	}
	// send the reply
	return reply.SendTo(port, token);
}


// main
int
main()
{
	AuthenticationServer app;
	status_t error = app.Init();
	if (error != B_OK)
		return 1;
	app.Run();
	return 0;
}

