/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_USER_GROUP_COMMON_H
#define _LIBROOT_USER_GROUP_COMMON_H

#include <grp.h>
#include <pwd.h>
#include <shadow.h>

#include <OS.h>


#define MAX_PASSWD_NAME_LEN			(32)
#define MAX_PASSWD_PASSWORD_LEN		(32)
#define MAX_PASSWD_REAL_NAME_LEN	(128)
#define MAX_PASSWD_HOME_DIR_LEN		(B_PATH_NAME_LENGTH)
#define MAX_PASSWD_SHELL_LEN		(B_PATH_NAME_LENGTH)

#define MAX_PASSWD_BUFFER_SIZE	(	\
	MAX_PASSWD_NAME_LEN				\
	+ MAX_PASSWD_PASSWORD_LEN		\
	+ MAX_PASSWD_REAL_NAME_LEN		\
	+ MAX_PASSWD_HOME_DIR_LEN		\
	+ MAX_PASSWD_SHELL_LEN)

#define	MAX_GROUP_NAME_LEN			(32)
#define	MAX_GROUP_PASSWORD_LEN		(32)
#define	MAX_GROUP_MEMBER_COUNT		(32)

#define MAX_GROUP_BUFFER_SIZE	(	\
	MAX_GROUP_NAME_LEN				\
	+ MAX_GROUP_PASSWORD_LEN		\
	+ ((MAX_GROUP_MEMBER_COUNT + 1) * sizeof(char*)))
	// MAX_GROUP_NAME_LEN and MAX_GROUP_PASSWORD_LEN are char* aligned


#define	MAX_SHADOW_PWD_NAME_LEN			(32)
#define	MAX_SHADOW_PWD_PASSWORD_LEN		(128)

#define MAX_SHADOW_PWD_BUFFER_SIZE	(	\
	MAX_SHADOW_PWD_NAME_LEN				\
	+ MAX_SHADOW_PWD_PASSWORD_LEN)


#ifdef __cplusplus

#include <AutoLocker.h>


namespace BPrivate {

class KMessage;
class Tokenizer;

extern const char* kPasswdFile;
extern const char* kGroupFile;
extern const char* kShadowPwdFile;


// locking

status_t user_group_lock();
status_t user_group_unlock();


class UserGroupLocking {
public:
    inline bool Lock(int*)
    {
        return user_group_lock() == B_OK;
    }

    inline void Unlock(int*)
    {
        user_group_unlock();
    }
};


class UserGroupLocker : public AutoLocker<int, UserGroupLocking> {
public:
	UserGroupLocker()
		: AutoLocker<int, UserGroupLocking>((int*)1)
	{
	}
};


port_id		get_registrar_authentication_port();
status_t	send_authentication_request_to_registrar(KMessage& request,
				KMessage& reply);


template<typename Type>
static inline Type*
relocate_pointer(addr_t baseAddress, Type*& address)
{
	return address = (Type*)(baseAddress + (addr_t)address);
}


// passwd

status_t copy_passwd_to_buffer(const char* name, const char* password, uid_t uid,
	gid_t gid, const char* home, const char* shell, const char* realName,
	passwd* entry, char* buffer, size_t bufferSize);
status_t copy_passwd_to_buffer(const passwd* from, passwd* entry, char* buffer,
	size_t bufferSize);

status_t parse_passwd_line(char* line, char*& name, char*& password, uid_t& uid,
	gid_t& gid, char*& home, char*& shell, char*& realName);


// group

status_t copy_group_to_buffer(const char* name, const char* password, gid_t gid,
	const char* const* members, int memberCount, group* entry, char* buffer,
	size_t bufferSize);
status_t copy_group_to_buffer(const group* from, group* entry, char* buffer,
	size_t bufferSize);

status_t parse_group_line(char* line, char*& name, char*& password, gid_t& gid,
	char** members, int& memberCount);


// shadow password

status_t copy_shadow_pwd_to_buffer(const char* name, const char* password,
	int lastChanged, int min, int max, int warn, int inactive, int expiration,
	int flags, spwd* entry, char* buffer, size_t bufferSize);
status_t copy_shadow_pwd_to_buffer(const spwd* from, spwd* entry,
	char* buffer, size_t bufferSize);

status_t parse_shadow_pwd_line(char* line, char*& name, char*& password,
	int& lastChanged, int& min, int& max, int& warn, int& inactive,
	int& expiration, int& flags);


}	// namespace BPrivate


#endif	// __cplusplus

#endif	// _LIBROOT_USER_GROUP_COMMON_H
