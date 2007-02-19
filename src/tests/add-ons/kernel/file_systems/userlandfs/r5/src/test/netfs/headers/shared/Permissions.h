// Permissions.h

#ifndef NET_FS_PERMISSIONS_H
#define NET_FS_PERMISSIONS_H

#include <SupportDefs.h>

enum {
	// file permissions
	READ_PERMISSION					= 0x01,
	WRITE_PERMISSION				= 0x02,

	// directory permissions
	READ_DIR_PERMISSION				= 0x04,
	WRITE_DIR_PERMISSION			= 0x08,
	RESOLVE_DIR_ENTRY_PERMISSION	= 0x10,

	// share permissions
	MOUNT_SHARE_PERMISSION			= 0x20,
	QUERY_SHARE_PERMISSION			= 0x40,

	// all permissions
	ALL_PERMISSIONS					= 0x7f,
};

class Permissions {
public:
	inline						Permissions();
	inline						Permissions(uint32 permissions);
	inline						Permissions(const Permissions& other);

	inline	uint32				GetPermissions() const;

	inline	Permissions&		AddPermissions(uint32 permissions);
	inline	Permissions&		AddPermissions(Permissions permissions);

	inline	bool				Implies(uint32 permissions) const;

	inline	bool				ImpliesReadPermission() const;
	inline	bool				ImpliesWritePermission() const;
	inline	bool				ImpliesReadDirPermission() const;
	inline	bool				ImpliesWriteDirPermission() const;
	inline	bool				ImpliesResolveDirEntryPermission() const;
	inline	bool				ImpliesMountSharePermission() const;
	inline	bool				ImpliesQuerySharePermission() const;

	inline	Permissions&		operator=(const Permissions& other);
	inline	bool				operator==(const Permissions& other) const;
	inline	bool				operator!=(const Permissions& other) const;

private:
			uint32				fPermissions;
};

// inline implementation

// constructor
inline
Permissions::Permissions()
	: fPermissions(0)
{
}

// constructor
inline
Permissions::Permissions(uint32 permissions)
	: fPermissions(permissions)
{
}

// copy constructor
inline
Permissions::Permissions(const Permissions& other)
	: fPermissions(other.fPermissions)
{
}

// GetPermissions
inline
uint32
Permissions::GetPermissions() const
{
	return fPermissions;
}

// AddPermissions
inline
Permissions&
Permissions::AddPermissions(uint32 permissions)
{
	fPermissions |= permissions;
	return *this;
}

// AddPermissions
inline
Permissions&
Permissions::AddPermissions(Permissions permissions)
{
	fPermissions |= permissions.fPermissions;
	return *this;
}

// Implies
inline
bool
Permissions::Implies(uint32 permissions) const
{
	return ((fPermissions & permissions) == permissions);
}

// ImpliesReadPermission
inline
bool
Permissions::ImpliesReadPermission() const
{
	return Implies(READ_PERMISSION);
}

// ImpliesWritePermission
inline
bool
Permissions::ImpliesWritePermission() const
{
	return Implies(WRITE_PERMISSION);
}

// ImpliesReadDirPermission
inline
bool
Permissions::ImpliesReadDirPermission() const
{
	return Implies(READ_DIR_PERMISSION);
}

// ImpliesWriteDirPermission
inline
bool
Permissions::ImpliesWriteDirPermission() const
{
	return Implies(WRITE_DIR_PERMISSION);
}

// ImpliesResolveDirEntryPermission
inline
bool
Permissions::ImpliesResolveDirEntryPermission() const
{
	return Implies(RESOLVE_DIR_ENTRY_PERMISSION);
}

// ImpliesMountSharePermission
inline
bool
Permissions::ImpliesMountSharePermission() const
{
	return Implies(MOUNT_SHARE_PERMISSION);
}

// ImpliesQuerySharePermission
inline
bool
Permissions::ImpliesQuerySharePermission() const
{
	return Implies(QUERY_SHARE_PERMISSION);
}

// =
inline
Permissions&
Permissions::operator=(const Permissions& other)
{
	fPermissions = other.fPermissions;
	return *this;
}

// ==
inline
bool
Permissions::operator==(const Permissions& other) const
{
	return (fPermissions == other.fPermissions);
}

// !=
inline
bool
Permissions::operator!=(const Permissions& other) const
{
	return (fPermissions != other.fPermissions);
}

#endif	// NET_FS_PERMISSIONS_H
