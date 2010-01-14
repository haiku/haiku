// SecurityContext.h

#ifndef NET_FS_SECURITY_CONTEXT_H
#define NET_FS_SECURITY_CONTEXT_H

#include <Archivable.h>
#include <HashString.h>
#include <Locker.h>
#include <Node.h>
#include <Referenceable.h>

#include "Permissions.h"
#include "Vector.h"

class UserSecurityContext;

// User
class User : public BReferenceable, public BArchivable {
public:
								User();
								User(BMessage* archive);
								~User();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);


			status_t			Init(const char* name, const char* password);
			status_t			InitCheck() const;

			status_t			Unarchive(const BMessage* archive);

			const char*			GetName() const;
			const char*			GetPassword() const;

private:
			HashString			fName;
			HashString			fPassword;
};

// Share
class Share : public BReferenceable, public BArchivable {
public:
								Share();
								Share(BMessage* archive);
								~Share();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

			status_t			Init(const char* name, const node_ref& ref,
									const char* path = NULL);
			status_t			Init(const char* name, const char* path);
			status_t			InitCheck() const;

			status_t			Unarchive(const BMessage* archive);

			const char*			GetName() const;
			bool				DoesExist() const;
			const node_ref&		GetNodeRef() const;
			dev_t				GetVolumeID() const;
			ino_t				GetNodeID() const;
			const char*			GetPath() const;

private:
			HashString			fName;
			node_ref			fNodeRef;
			HashString			fPath;
};

// SecurityContext
class SecurityContext : public BArchivable, public BLocker {
public:
								SecurityContext();
								SecurityContext(BMessage* archive);
								~SecurityContext();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

			status_t			InitCheck() const;

			status_t			AddUser(const char* name, const char* password,
									User** user = NULL);
			status_t			RemoveUser(const char* name,
									User** user = NULL);
			status_t			RemoveUser(User* user);
			User*				FindUser(const char* name);
			status_t			AuthenticateUser(const char* name,
									const char* password, User** user);
			int32				CountUsers();
			status_t			GetUsers(BMessage* users);

			status_t			AddShare(const char* name, const node_ref& ref,
									Share** share = NULL);
			status_t			AddShare(const char* name, const char* path,
									Share** share = NULL);
			status_t			RemoveShare(const char* name,
									Share** share = NULL);
			status_t			RemoveShare(Share* share);
			Share*				FindShare(const char* name);
			int32				CountShares();
			status_t			GetShares(BMessage* shares);

			status_t			SetNodePermissions(const node_ref& ref,
									User* user, Permissions permissions);
			status_t			SetNodePermissions(const char* path,
									User* user, Permissions permissions);
			void				ClearNodePermissions(const node_ref& ref,
									User* user = NULL);
			void				ClearNodePermissions(const char* path,
									User* user = NULL);
			Permissions			GetNodePermissions(const node_ref& ref,
									User* user);
			Permissions			GetNodePermissions(const char* path,
									User* user);

			status_t			GetUserSecurityContext(User* user,
									UserSecurityContext* userContext);

private:
			status_t			_AddNodePath(const char* path,
									node_ref* ref = NULL);
			status_t			_AddNodePath(const node_ref& ref,
									HashString* path = NULL);
			status_t			_EnterNodePath(const char* path,
									const node_ref& ref);
			bool				_GetNodeForPath(const char* path,
									node_ref* ref);

private:
			struct UserMap;
			struct ShareMap;
			struct UserPath;
			struct PermissionMap;
			struct NodePathMap;
			struct PathNodeMap;

			UserMap*			fUsers;
			ShareMap*			fShares;
			PermissionMap*		fPermissions;
			NodePathMap*		fNode2Path;
			PathNodeMap*		fPath2Node;
};

#endif	// NET_FS_SECURITY_CONTEXT_H
