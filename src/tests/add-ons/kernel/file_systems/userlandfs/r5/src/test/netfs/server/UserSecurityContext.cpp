// UserSecurityContext.cpp

#include <new>

#include "SecurityContext.h"
#include "UserSecurityContext.h"
#include "HashMap.h"
#include "Node.h"

// PermissionMap
struct UserSecurityContext::PermissionMap : HashMap<NodeRef, Permissions> {
};

// constructor
UserSecurityContext::UserSecurityContext()
	: fUser(NULL),
	  fPermissions(NULL)
{
}

// destructor
UserSecurityContext::~UserSecurityContext()
{
	if (fUser)
		fUser->RemoveReference();

	delete fPermissions;
}

// Init
status_t
UserSecurityContext::Init(User* user)
{
	if (fUser)
		fUser->RemoveReference();

	fUser = user;
	if (fUser)
		fUser->AddReference();

	delete fPermissions;
	fPermissions = new(nothrow) PermissionMap;

	if (!fPermissions)
		return B_NO_MEMORY;
	return B_OK;
}

// GetUser
User*
UserSecurityContext::GetUser() const
{
	return fUser;
}

// AddNode
status_t
UserSecurityContext::AddNode(dev_t volumeID, ino_t nodeID,
	Permissions permissions)
{
	return fPermissions->Put(NodeRef(volumeID, nodeID), permissions);
}

// GetNodePermissions
Permissions
UserSecurityContext::GetNodePermissions(dev_t volumeID, ino_t nodeID) const
{
	return fPermissions->Get(NodeRef(volumeID, nodeID));
}

// GetNodePermissions
Permissions
UserSecurityContext::GetNodePermissions(const node_ref& ref) const
{
	return fPermissions->Get(NodeRef(ref));
}

// GetNodePermissions
Permissions
UserSecurityContext::GetNodePermissions(dev_t volumeID, ino_t nodeID,
	Permissions parentPermissions) const
{
	if (fPermissions->ContainsKey(NodeRef(volumeID, nodeID)))
		return fPermissions->Get(NodeRef(volumeID, nodeID));
	return parentPermissions;
}

// GetNodePermissions
Permissions
UserSecurityContext::GetNodePermissions(const node_ref& ref,
	Permissions parentPermissions) const
{
	if (fPermissions->ContainsKey(NodeRef(ref)))
		return fPermissions->Get(NodeRef(ref));
	return parentPermissions;
}

