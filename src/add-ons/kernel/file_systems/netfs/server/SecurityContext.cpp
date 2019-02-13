// SecurityContext.cpp

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Entry.h>
#include <HashMap.h>
#include <Message.h>
#include <Path.h>

#include "Compatibility.h"
#include "Node.h"
#include "SecurityContext.h"
#include "UserSecurityContext.h"

typedef AutoLocker<SecurityContext> ContextLocker;

// get_node_ref_for_path
static
status_t
get_node_ref_for_path(const char* path, node_ref* ref)
{
	if (!path || !ref)
		return B_BAD_VALUE;
	struct stat st;
	if (lstat(path, &st) < 0)
		return errno;
	ref->device = st.st_dev;
	ref->node = st.st_ino;
	return B_OK;
}

// #pragma mark -
// #pragma mark ----- User -----

// constructor
User::User()
	: BReferenceable(),
	  BArchivable(),
	  fName(),
	  fPassword()
{
}

// constructor
User::User(BMessage* archive)
	: BReferenceable(),
	  BArchivable(archive),
	  fName(),
	  fPassword()
{
	Unarchive(archive);
}

// destructor
User::~User()
{
}

// Archive
status_t
User::Archive(BMessage* archive, bool deep) const
{
	if (!archive)
		return B_BAD_VALUE;
	// name
	status_t error = B_OK;
	if (error == B_OK && fName.GetLength() > 0)
		error = archive->AddString("name", fName.GetString());
	// password
	if (error == B_OK && fPassword.GetLength() > 0)
		error = archive->AddString("password", fPassword.GetString());
	return error;
}

// Instantiate
BArchivable*
User::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "User"))
		return NULL;
	return new(std::nothrow) User(archive);
}

// Init
status_t
User::Init(const char* name, const char* password)
{
	if (!name)
		return B_BAD_VALUE;
	if (!fName.SetTo(name))
		return B_NO_MEMORY;
	if (password && !fPassword.SetTo(password))
		return B_NO_MEMORY;
	return B_OK;
}

// InitCheck
status_t
User::InitCheck() const
{
	if (fName.GetLength() == 0)
		return B_NO_INIT;
	return B_OK;
}

// Unarchive
status_t
User::Unarchive(const BMessage* archive)
{
	// name
	const char* name;
	if (archive->FindString("name", &name) != B_OK)
		return B_BAD_DATA;
	fName.SetTo(name);
	// password
	const char* password;
	if (archive->FindString("password", &password) == B_OK)
		fPassword.SetTo(password);
	else
		fPassword.Unset();
	return B_OK;
}

// GetName
const char*
User::GetName() const
{
	return fName.GetString();
}

// GetPassword
const char*
User::GetPassword() const
{
	return fPassword.GetString();
}


// #pragma mark -
// #pragma mark ----- Share -----

// constructor
Share::Share()
	: BReferenceable(),
	  BArchivable(),
	  fName(),
	  fNodeRef(),
	  fPath()
{
}

// constructor
Share::Share(BMessage* archive)
	: BReferenceable(),
	  BArchivable(archive),
	  fName(),
	  fNodeRef(),
	  fPath()
{
	Unarchive(archive);
}

// destructor
Share::~Share()
{
}

// Archive
status_t
Share::Archive(BMessage* archive, bool deep) const
{
	if (!archive)
		return B_BAD_VALUE;
	// name
	status_t error = B_OK;
	if (error == B_OK && fName.GetLength() > 0)
		error = archive->AddString("name", fName.GetString());
	// path
	if (error == B_OK && fPath.GetLength() > 0)
		error = archive->AddString("path", fPath.GetString());
	return error;
}

// Instantiate
BArchivable*
Share::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "Share"))
		return NULL;
	return new(std::nothrow) Share(archive);
}

// Init
status_t
Share::Init(const char* name, const node_ref& ref, const char* path)
{
	// check params
	if (!name)
		return B_BAD_VALUE;
	// if a path is not given, retrieve it
	BPath localPath;
	if (!path) {
		entry_ref entryRef(ref.device, ref.node, ".");
		status_t error = localPath.SetTo(&entryRef);
		if (error != B_OK)
			return error;
		path = localPath.Path();
	}
	// set the attributes
	if (!fName.SetTo(name))
		return B_NO_MEMORY;
	if (!fPath.SetTo(path))
		return B_NO_MEMORY;
	fNodeRef = ref;
	return B_OK;
}

// Init
status_t
Share::Init(const char* name, const char* path)
{
	if (!name || !path)
		return B_BAD_VALUE;
	node_ref nodeRef;
	if (get_node_ref_for_path(path, &nodeRef) != B_OK) {
		nodeRef.device = -1;
		nodeRef.node = -1;
	}
	return Init(name, nodeRef, path);
}

// InitCheck
status_t
Share::InitCheck() const
{
	if (fName.GetLength() == 0 || fPath.GetLength() == 0)
		return B_NO_INIT;
	return B_OK;
}

// Unarchive
status_t
Share::Unarchive(const BMessage* archive)
{
	// name
	const char* name = NULL;
	if (archive->FindString("name", &name) != B_OK)
		return B_BAD_DATA;
	// path
	const char* path = NULL;
	if (archive->FindString("path", &path) != B_OK)
		return B_BAD_DATA;
	return Init(name, path);
}

// GetName
const char*
Share::GetName() const
{
	return fName.GetString();
}

// DoesExist
bool
Share::DoesExist() const
{
	return (fNodeRef.device >= 0);
}

// GetNodeRef
const node_ref&
Share::GetNodeRef() const
{
	return fNodeRef;
}

// GetVolumeID
dev_t
Share::GetVolumeID() const
{
	return fNodeRef.device;
}

// GetNodeID
ino_t
Share::GetNodeID() const
{
	return fNodeRef.node;
}

// GetPath
const char*
Share::GetPath() const
{
	return fPath.GetString();
}


// #pragma mark -
// #pragma mark ----- SecurityContext -----

// UserMap
struct SecurityContext::UserMap : HashMap<HashString, User*> {
};

// ShareMap
struct SecurityContext::ShareMap : HashMap<HashString, Share*> {
};

// UserPath
struct SecurityContext::UserPath {
	UserPath() {}

	UserPath(const char* path, User* user)
		: path(path),
		  user(user)
	{
	}

	UserPath(const UserPath& other)
		: path(other.path),
		  user(other.user)
	{
	}

	uint32 GetHashCode() const
	{
#ifdef B_HAIKU_64_BIT
		uint64 v = (uint64)user;
		return (path.GetHashCode() * 31) + ((uint32)(v >> 32) ^ (uint32)v);
#else
		return path.GetHashCode() * 31 + (uint32)user;
#endif
	}

	UserPath& operator=(const UserPath& other)
	{
		path = other.path;
		user = other.user;
		return *this;
	}

	bool operator==(const UserPath& other) const
	{
		return (path == other.path && user == other.user);
	}

	bool operator!=(const UserPath& other) const
	{
		return !(*this == other);
	}

	HashString	path;
	User*		user;
};

// PermissionMap
struct SecurityContext::PermissionMap
	: HashMap<SecurityContext::UserPath, Permissions> {
};

// NodePathMap
struct SecurityContext::NodePathMap : HashMap<NodeRef, HashString> {
};

// PathNodeMap
struct SecurityContext::PathNodeMap : HashMap<HashString, NodeRef> {
};

// constructor
SecurityContext::SecurityContext()
	: BArchivable(),
	  BLocker("security context"),
	  fUsers(new(std::nothrow) UserMap),
	  fShares(new(std::nothrow) ShareMap),
	  fPermissions(new(std::nothrow) PermissionMap),
	  fNode2Path(new(std::nothrow) NodePathMap),
	  fPath2Node(new(std::nothrow) PathNodeMap)
{
}

// constructor
SecurityContext::SecurityContext(BMessage* archive)
	: BArchivable(archive),
	  fUsers(new(std::nothrow) UserMap),
	  fShares(new(std::nothrow) ShareMap),
	  fPermissions(new(std::nothrow) PermissionMap),
	  fNode2Path(new(std::nothrow) NodePathMap),
	  fPath2Node(new(std::nothrow) PathNodeMap)
{
	if (InitCheck() != B_OK)
		return;
	status_t error = B_OK;

	// users
	BMessage userArchive;
	for (int32 i = 0;
		 archive->FindMessage("users", i, &userArchive) == B_OK;
		 i++) {
		User tmpUser;
		error = tmpUser.Unarchive(&userArchive);
		if (error != B_OK)
			return;
		error = AddUser(tmpUser.GetName(), tmpUser.GetPassword());
		if (error != B_OK)
			return;
	}

	// shares
	BMessage shareArchive;
	for (int32 i = 0;
		 archive->FindMessage("shares", i, &shareArchive) == B_OK;
		 i++) {
		Share tmpShare;
		error = tmpShare.Unarchive(&shareArchive);
		if (error != B_OK)
			return;
		error = AddShare(tmpShare.GetName(), tmpShare.GetPath());
		if (error != B_OK)
			return;
	}

	// permissions
	BMessage permissionsArchive;
	if (archive->FindMessage("permissions", &permissionsArchive) != B_OK)
		return;
	#ifdef HAIKU_TARGET_PLATFORM_DANO
		const char* userName;
	#else
		char* userName;
	#endif
	type_code type;
	for (int32 userIndex = 0;
		 permissionsArchive.GetInfo(B_MESSAGE_TYPE, userIndex, &userName, &type)
		 	== B_OK;
		 userIndex++) {
		User* user = FindUser(userName);
		if (!user)
			return;
		BReference<User> userReference(user, true);
		error = permissionsArchive.FindMessage(userName, &userArchive);
		if (error != B_OK)
			return;

		// got a user: iterate through its permissions
		#ifdef HAIKU_TARGET_PLATFORM_DANO
			const char* path;
		#else
			char* path;
		#endif
		for (int32 i = 0;
			 userArchive.GetInfo(B_INT32_TYPE, i, &path, &type) == B_OK;
			 i++) {
			uint32 permissions;
			error = userArchive.FindInt32(path, (int32*)&permissions);
			if (error == B_OK)
				error = SetNodePermissions(path, user, permissions);
		}
	}
}

// destructor
SecurityContext::~SecurityContext()
{
	// remove all user references
	for (UserMap::Iterator it = fUsers->GetIterator(); it.HasNext();) {
		User* user = it.Next().value;
		user->ReleaseReference();
	}

	// remove all share references
	for (ShareMap::Iterator it = fShares->GetIterator(); it.HasNext();) {
		Share* share = it.Next().value;
		share->ReleaseReference();
	}

	delete fUsers;
	delete fShares;
	delete fPermissions;
	delete fNode2Path;
	delete fPath2Node;
}

// Archive
status_t
SecurityContext::Archive(BMessage* archive, bool deep) const
{
	if (!archive)
		return B_BAD_VALUE;
	status_t error = B_OK;
	ContextLocker _(const_cast<SecurityContext*>(this));

	// users
	int32 userCount = fUsers->Size();
	for (UserMap::Iterator it = fUsers->GetIterator(); it.HasNext();) {
		User* user = it.Next().value;
		BMessage userArchive;
		error = user->Archive(&userArchive, deep);
		if (error != B_OK)
			return error;
		error = archive->AddMessage("users", &userArchive);
		if (error != B_OK)
			return error;
	}

	// shares
	for (ShareMap::Iterator it = fShares->GetIterator(); it.HasNext();) {
		Share* share = it.Next().value;
		BMessage shareArchive;
		error = share->Archive(&shareArchive, deep);
		if (error != B_OK)
			return error;
		error = archive->AddMessage("shares", &shareArchive);
		if (error != B_OK)
			return error;
	}

	// permissions
	// we slice them per user
	BMessage* tmpUserArchives = new(std::nothrow) BMessage[userCount];
	if (!tmpUserArchives)
		return B_NO_MEMORY;
	ArrayDeleter<BMessage> deleter(tmpUserArchives);
	HashMap<HashKeyPointer<User*>, BMessage*> userArchives;
	int32 i = 0;
	for (UserMap::Iterator it = fUsers->GetIterator(); it.HasNext();) {
		User* user = it.Next().value;
		error = userArchives.Put(user, tmpUserArchives + i);
		if (error != B_OK)
			return error;
		i++;
	}

	// fill the per user archives
	for (PermissionMap::Iterator it = fPermissions->GetIterator();
		 it.HasNext();) {
		PermissionMap::Entry entry = it.Next();
		BMessage* userArchive = userArchives.Get(entry.key.user);
		error = userArchive->AddInt32(entry.key.path.GetString(),
			entry.value.GetPermissions());
		if (error != B_OK)
			return error;
	}

	// put the user permissions together
	BMessage permissionsArchive;
	for (UserMap::Iterator it = fUsers->GetIterator(); it.HasNext();) {
		User* user = it.Next().value;
		error = permissionsArchive.AddMessage(user->GetName(),
			userArchives.Get(user));
		if (error != B_OK)
			return error;
	}
	error = archive->AddMessage("permissions", &permissionsArchive);
	if (error != B_OK)
		return error;
	return B_OK;
}

// Instantiate
BArchivable*
SecurityContext::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "SecurityContext"))
		return NULL;
	return new(std::nothrow) SecurityContext(archive);
}


// InitCheck
status_t
SecurityContext::InitCheck() const
{
	if (!fUsers || !fShares || !fPermissions || !fNode2Path || !fPath2Node)
		return B_NO_MEMORY;

	if (fUsers->InitCheck() != B_OK)
		return fUsers->InitCheck();

	if (fShares->InitCheck() != B_OK)
		return fShares->InitCheck();

	if (fPermissions->InitCheck() != B_OK)
		return fPermissions->InitCheck();

	if (fNode2Path->InitCheck() != B_OK)
		return fNode2Path->InitCheck();

	if (fPath2Node->InitCheck() != B_OK)
		return fPath2Node->InitCheck();

	return B_OK;
}

// AddUser
//
// The caller gets a reference, if _user is not NULL.
status_t
SecurityContext::AddUser(const char* name, const char* password, User** _user)
{
	if (!name)
		return B_BAD_VALUE;

	// check, if the user does already exist
	ContextLocker _(this);
	if (fUsers->Get(name))
		return B_BAD_VALUE;

	// create a the user
	User* user = new(std::nothrow) User;
	if (!user)
		return B_NO_MEMORY;
	BReference<User> userReference(user, true);
	status_t error = user->Init(name, password);
	if (error != B_OK)
		return error;

	// add the user
	error = fUsers->Put(name, user);
	if (error != B_OK)
		return error;

	userReference.Detach();
	if (_user) {
		*_user = user;
		user->AcquireReference();
	}
	return B_OK;
}

// RemoveUser
//
// The caller gets a reference, if _user is not NULL.
status_t
SecurityContext::RemoveUser(const char* name, User** _user)
{
	if (!name)
		return B_BAD_VALUE;

	ContextLocker _(this);

	// get the user
	User* user = FindUser(name);
	if (!user)
		return B_ENTRY_NOT_FOUND;
	BReference<User> userReference(user, true);

	// remove it
	status_t error = RemoveUser(user);
	if (error == B_OK && _user) {
		*_user = user;
		user->AcquireReference();
	}

	return error;
}

// RemoveUser
status_t
SecurityContext::RemoveUser(User* user)
{
	if (!user)
		return B_BAD_VALUE;

	ContextLocker _(this);

	// find and remove it
	if (fUsers->Get(user->GetName()) != user)
		return B_BAD_VALUE;
	fUsers->Remove(user->GetName());

	// remove all permission entries for this user
	for (PermissionMap::Iterator it = fPermissions->GetIterator();
		 it.HasNext();) {
		PermissionMap::Entry entry = it.Next();
		if (entry.key.user == user)
			fPermissions->Remove(it);
	}

	// surrender our user reference
	user->ReleaseReference();

	return B_OK;
}

// FindUser
//
// The caller gets a reference.
User*
SecurityContext::FindUser(const char* name)
{
	if (!name)
		return NULL;

	ContextLocker _(this);
	User* user = fUsers->Get(name);
	if (user)
		user->AcquireReference();
	return user;
}

// AuthenticateUser
//
// The caller gets a reference.
status_t
SecurityContext::AuthenticateUser(const char* name, const char* password,
	User** _user)
{
	if (!_user)
		return B_BAD_VALUE;

	// find user
	ContextLocker _(this);
	User* user = FindUser(name);
	if (!user)
		return B_PERMISSION_DENIED;
	BReference<User> userReference(user, true);

	// check password
	if (user->GetPassword()) {
		if (!password || strcmp(user->GetPassword(), password) != 0)
			return B_PERMISSION_DENIED;
	} else if (password)
		return B_PERMISSION_DENIED;

	*_user = user;
	userReference.Detach();
	return B_OK;
}

// CountUsers
int32
SecurityContext::CountUsers()
{
	ContextLocker _(this);
	return fUsers->Size();
}

// GetUsers
status_t
SecurityContext::GetUsers(BMessage* users)
{
	if (!users)
		return B_BAD_VALUE;

	ContextLocker _(this);

	// iterate through all users and add their names to the message
	for (UserMap::Iterator it = fUsers->GetIterator(); it.HasNext();) {
		User* user = it.Next().value;
		status_t error = users->AddString("users", user->GetName());
		if (error != B_OK)
			return error;
	}

	return B_OK;
}

// AddShare
//
// The caller gets a reference, if _share is not NULL.
status_t
SecurityContext::AddShare(const char* name, const node_ref& ref, Share** _share)
{
	if (!name)
		return B_BAD_VALUE;

	// check, if the share does already exist
	ContextLocker _(this);
	if (fShares->Get(name))
		return B_BAD_VALUE;

	// create a the share
	Share* share = new(std::nothrow) Share;
	if (!share)
		return B_NO_MEMORY;
	BReference<Share> shareReference(share, true);
	status_t error = share->Init(name, ref);
	if (error != B_OK)
		return error;

	// add the share
	error = fShares->Put(name, share);
	if (error != B_OK)
		return error;

	shareReference.Detach();
	if (_share) {
		*_share = share;
		share->AcquireReference();
	}
	return B_OK;
}

// AddShare
//
// The caller gets a reference, if _share is not NULL.
status_t
SecurityContext::AddShare(const char* name, const char* path, Share** _share)
{
	if (!name)
		return B_BAD_VALUE;

	// check, if the share does already exist
	ContextLocker _(this);
	if (fShares->Get(name))
		return B_BAD_VALUE;

	// create a the share
	Share* share = new(std::nothrow) Share;
	if (!share)
		return B_NO_MEMORY;
	BReference<Share> shareReference(share, true);
	status_t error = share->Init(name, path);
	if (error != B_OK)
		return error;

	// add the share
	error = fShares->Put(name, share);
	if (error != B_OK)
		return error;

	shareReference.Detach();
	if (_share) {
		*_share = share;
		share->AcquireReference();
	}
	return B_OK;
}

// RemoveShare
//
// The caller gets a reference, if _share is not NULL.
status_t
SecurityContext::RemoveShare(const char* name, Share** _share)
{
	if (!name)
		return B_BAD_VALUE;

	ContextLocker _(this);

	// get the share
	Share* share = FindShare(name);
	if (!share)
		return B_ENTRY_NOT_FOUND;
	BReference<Share> shareReference(share, true);

	// remove it
	status_t error = RemoveShare(share);
	if (error == B_OK && _share) {
		*_share = share;
		share->AcquireReference();
	}

	return error;
}

// RemoveShare
status_t
SecurityContext::RemoveShare(Share* share)
{
	if (!share)
		return B_BAD_VALUE;

	ContextLocker _(this);

	// find and remove it
	if (fShares->Get(share->GetName()) != share)
		return B_BAD_VALUE;
	fShares->Remove(share->GetName());

	// surrender our share reference
	share->ReleaseReference();

	return B_OK;
}

// FindShare
//
// The caller gets a reference.
Share*
SecurityContext::FindShare(const char* name)
{
	if (!name)
		return NULL;

	ContextLocker _(this);
	Share* share = fShares->Get(name);
	if (share)
		share->AcquireReference();
	return share;
}

// CountShares
int32
SecurityContext::CountShares()
{
	ContextLocker _(this);
	return fShares->Size();
}

// GetShares
status_t
SecurityContext::GetShares(BMessage* shares)
{
	if (!shares)
		return B_BAD_VALUE;

	ContextLocker _(this);

	// iterate through all shares and add their names to the message
	for (ShareMap::Iterator it = fShares->GetIterator(); it.HasNext();) {
		Share* share = it.Next().value;
		// add name
		status_t error = shares->AddString("shares", share->GetName());
		if (error != B_OK)
			return error;

		// add path
		error = shares->AddString("paths", share->GetPath());
		if (error != B_OK)
			return error;
	}

	return B_OK;
}

// SetNodePermissions
status_t
SecurityContext::SetNodePermissions(const node_ref& ref, User* user,
	Permissions permissions)
{
	if (!user)
		return B_BAD_VALUE;

	ContextLocker _(this);
	// check, whether we know the user
	if (fUsers->Get(user->GetName()) != user)
		return B_BAD_VALUE;

	HashString path;
	status_t error = _AddNodePath(ref, &path);
	if (error != B_OK)
		return error;
	return fPermissions->Put(UserPath(path.GetString(), user), permissions);
}

// SetNodePermissions
status_t
SecurityContext::SetNodePermissions(const char* path, User* user,
	Permissions permissions)
{
	if (!user || !path)
		return B_BAD_VALUE;

	ContextLocker _(this);
	// check, whether we know the user
	if (fUsers->Get(user->GetName()) != user)
		return B_BAD_VALUE;

	_AddNodePath(path);
	return fPermissions->Put(UserPath(path, user), permissions);
}

// ClearNodePermissions
void
SecurityContext::ClearNodePermissions(const node_ref& ref, User* user)
{
	ContextLocker _(this);
	HashString path;
	status_t error = _AddNodePath(ref, &path);
	if (error != B_OK)
		return;

	if (user) {
		fPermissions->Remove(UserPath(path.GetString(), user));
	} else {
		for (UserMap::Iterator it = fUsers->GetIterator(); it.HasNext();)
			fPermissions->Remove(UserPath(path.GetString(), it.Next().value));
	}
}

// ClearNodePermissions
void
SecurityContext::ClearNodePermissions(const char* path, User* user)
{
	if (!path)
		return;

	ContextLocker _(this);
	_AddNodePath(path);

	if (user) {
		fPermissions->Remove(UserPath(path, user));
	} else {
		for (UserMap::Iterator it = fUsers->GetIterator(); it.HasNext();)
			fPermissions->Remove(UserPath(path, it.Next().value));
	}
}

// GetNodePermissions
Permissions
SecurityContext::GetNodePermissions(const node_ref& ref, User* user)
{
	if (!user)
		return Permissions();

	ContextLocker _(this);
	HashString path;
	status_t error = _AddNodePath(ref, &path);
	if (error != B_OK)
		return Permissions();

	return fPermissions->Get(UserPath(path.GetString(), user));
}

// GetNodePermissions
Permissions
SecurityContext::GetNodePermissions(const char* path, User* user)
{
	if (!user || !path)
		return Permissions();

	ContextLocker _(this);
	_AddNodePath(path);

	return fPermissions->Get(UserPath(path, user));
}

// GetUserSecurityContext
status_t
SecurityContext::GetUserSecurityContext(User* user,
	UserSecurityContext* userContext)
{
	if (!userContext)
		return B_BAD_VALUE;

	status_t error = userContext->Init(user);
	if (error != B_OK)
		return error;

	// iterate through all permission entries and add the ones whose user
	// matches
	ContextLocker _(this);
	for (PermissionMap::Iterator it = fPermissions->GetIterator();
		 it.HasNext();) {
		PermissionMap::Entry entry = it.Next();
		node_ref ref;
		if (entry.key.user == user
			&& _GetNodeForPath(entry.key.path.GetString(), &ref)) {
			error = userContext->AddNode(ref.device, ref.node, entry.value);
			if (error != B_OK)
				return error;
		}
	}
	return B_OK;
}

// _AddNodePath
status_t
SecurityContext::_AddNodePath(const char* path, node_ref* _ref)
{
	if (!fPath2Node->ContainsKey(path)) {
		node_ref ref;
		status_t error = get_node_ref_for_path(path, &ref);
		if (error == B_OK)
			error = _EnterNodePath(path, ref);
		if (error != B_OK)
			return error;
	}

	if (_ref)
		*_ref = fPath2Node->Get(path);
	return B_OK;
}

// _AddNodePath
status_t
SecurityContext::_AddNodePath(const node_ref& ref, HashString* _path)
{
	if (!fNode2Path->ContainsKey(ref)) {
		BPath path;
		entry_ref entryRef(ref.device, ref.node, ".");
		status_t error = path.SetTo(&entryRef);
		if (error == B_OK)
			error = _EnterNodePath(path.Path(), ref);
		if (error != B_OK)
			return error;
	}

	if (_path)
		*_path = fNode2Path->Get(ref);
	return B_OK;
}

// _EnterNodePath
status_t
SecurityContext::_EnterNodePath(const char* path, const node_ref& ref)
{
	status_t error = fNode2Path->Put(ref, path);
	if (error == B_OK) {
		error = fPath2Node->Put(path, ref);
		if (error != B_OK)
			fNode2Path->Remove(ref);
	}
	return error;
}

// _GetNodeForPath
bool
SecurityContext::_GetNodeForPath(const char* path, node_ref* ref)
{
	if (path && fPath2Node->ContainsKey(path)) {
		if (ref)
			*ref = fPath2Node->Get(path);
		return true;
	}
	return false;
}

