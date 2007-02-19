// UserSecurityContext.h

#ifndef NET_FS_USER_SECURITY_CONTEXT_H
#define NET_FS_USER_SECURITY_CONTEXT_H

#include "Permissions.h"

#include <Node.h>

class User;

class UserSecurityContext {
public:
								UserSecurityContext();
								~UserSecurityContext();

			status_t			Init(User* user);

			User*				GetUser() const;

			status_t			AddNode(dev_t volumeID, ino_t nodeID,
									Permissions permissions);

			Permissions			GetNodePermissions(dev_t volumeID,
									ino_t nodeID) const;
			Permissions			GetNodePermissions(const node_ref& ref) const;
			Permissions			GetNodePermissions(dev_t volumeID, ino_t nodeID,
									Permissions parentPermissions) const;
			Permissions			GetNodePermissions(const node_ref& ref,
									Permissions parentPermissions) const;

private:
			struct PermissionMap;

			User*				fUser;
			PermissionMap*		fPermissions;
};

#endif	// NET_FS_USER_SECURITY_CONTEXT_H
