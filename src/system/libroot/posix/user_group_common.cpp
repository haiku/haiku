/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <user_group.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <errno_private.h>
#include <libroot_private.h>
#include <locks.h>
#include <RegistrarDefs.h>

#include <util/KMessage.h>


using BPrivate::Tokenizer;


static const char* const kUserGroupLockName = "user group";

const char* BPrivate::kPasswdFile = "/etc/passwd";
const char* BPrivate::kGroupFile = "/etc/group";
const char* BPrivate::kShadowPwdFile = "/etc/shadow";

static mutex sUserGroupLock = MUTEX_INITIALIZER(kUserGroupLockName);
static port_id sRegistrarPort = -1;


status_t
BPrivate::user_group_lock()
{
	return mutex_lock(&sUserGroupLock);
}


status_t
BPrivate::user_group_unlock()
{
	mutex_unlock(&sUserGroupLock);
	return B_OK;
}


port_id
BPrivate::get_registrar_authentication_port()
{
	if (sRegistrarPort < 0)
		sRegistrarPort = find_port(REGISTRAR_AUTHENTICATION_PORT_NAME);

	return sRegistrarPort;
}


status_t
BPrivate::send_authentication_request_to_registrar(KMessage& request,
	KMessage& reply)
{
	status_t error = request.SendTo(get_registrar_authentication_port(), 0,
		&reply);
	if (error != B_OK)
		return error;

	return (status_t)reply.What();
}


class BPrivate::Tokenizer {
public:
	Tokenizer(char* string)
		: fString(string)
	{
	}

	char* NextToken(char separator)
	{
		if (fString == NULL)
			return NULL;

		char* token = fString;
		fString = strchr(fString, separator);
		if (fString != NULL) {
			*fString = '\0';
			fString++;
		}

		return token;
	}

	char* NextTrimmedToken(char separator)
	{
		char* token = NextToken(separator);
		if (token == NULL)
			return NULL;

		// skip spaces at the beginning
		while (*token != '\0' && isspace(*token))
			token++;

		// cut off spaces at the end
		char* end = token + strlen(token);
		while (end != token && isspace(end[-1]))
			end--;
		*end = '\0';

		return token;
	}

private:
	char*		fString;
};


static char*
buffer_dup_string(const char* string, char*& buffer, size_t& bufferLen)
{
	if (string == NULL)
		return NULL;

	size_t size = strlen(string) + 1;
	if (size > bufferLen)
		return NULL;

	strcpy(buffer, string);
	char* result = buffer;
	buffer += size;
	bufferLen -= size;

	return result;
}


static void*
buffer_allocate(size_t size, size_t align, char*& buffer, size_t& bufferSize)
{
	// align padding
	addr_t pad = align - (((addr_t)buffer - 1)  & (align - 1)) - 1;
	if (pad + size > bufferSize)
		return NULL;

	char* result = buffer + pad;
	buffer = result + size;
	bufferSize -= pad + size;

	return result;
}


// #pragma mark - passwd support


status_t
BPrivate::copy_passwd_to_buffer(const char* name, const char* password,
	uid_t uid, gid_t gid, const char* home, const char* shell,
	const char* realName, passwd* entry, char* buffer, size_t bufferSize)
{
	entry->pw_uid = uid;
	entry->pw_gid = gid;

	entry->pw_name = buffer_dup_string(name, buffer, bufferSize);
	entry->pw_passwd = buffer_dup_string(password, buffer, bufferSize);
	entry->pw_dir = buffer_dup_string(home, buffer, bufferSize);
	entry->pw_shell = buffer_dup_string(shell, buffer, bufferSize);
	entry->pw_gecos = buffer_dup_string(realName, buffer, bufferSize);

	if (entry->pw_name && entry->pw_passwd && entry->pw_dir
			&& entry->pw_shell && entry->pw_gecos) {
		return 0;
	}

	return ERANGE;
}


status_t
BPrivate::copy_passwd_to_buffer(const passwd* from, passwd* entry, char* buffer,
	size_t bufferSize)
{
	return copy_passwd_to_buffer(from->pw_name, from->pw_passwd, from->pw_uid,
		from->pw_gid, from->pw_dir, from->pw_shell, from->pw_gecos, entry,
		buffer, bufferSize);
}


status_t
BPrivate::parse_passwd_line(char* line, char*& name, char*& password,
	uid_t& uid, gid_t& gid, char*& home, char*& shell, char*& realName)
{
	Tokenizer tokenizer(line);

	name = tokenizer.NextTrimmedToken(':');
	password = tokenizer.NextTrimmedToken(':');
	char* userID = tokenizer.NextTrimmedToken(':');
	char* groupID = tokenizer.NextTrimmedToken(':');
	realName = tokenizer.NextTrimmedToken(':');
	home = tokenizer.NextTrimmedToken(':');
	shell = tokenizer.NextTrimmedToken(':');

	// skip if invalid
	size_t nameLen;
	if (shell == NULL || (nameLen = strlen(name)) == 0
			|| !isdigit(*userID) || !isdigit(*groupID)
		|| nameLen >= MAX_PASSWD_NAME_LEN
		|| strlen(password) >= MAX_PASSWD_PASSWORD_LEN
		|| strlen(realName) >= MAX_PASSWD_REAL_NAME_LEN
		|| strlen(home) >= MAX_PASSWD_HOME_DIR_LEN
		|| strlen(shell) >= MAX_PASSWD_SHELL_LEN) {
		return B_BAD_VALUE;
	}

	uid = atoi(userID);
	gid = atoi(groupID);

	return B_OK;
}


// #pragma mark - group support


status_t
BPrivate::copy_group_to_buffer(const char* name, const char* password,
	gid_t gid, const char* const* members, int memberCount, group* entry,
	char* buffer, size_t bufferSize)
{
	entry->gr_gid = gid;

	// allocate member array (do that first for alignment reasons)
	entry->gr_mem = (char**)buffer_allocate(sizeof(char*) * (memberCount + 1),
		sizeof(char*), buffer, bufferSize);
	if (entry->gr_mem == NULL)
		return ERANGE;

	// copy name and password
	entry->gr_name = buffer_dup_string(name, buffer, bufferSize);
	entry->gr_passwd = buffer_dup_string(password, buffer, bufferSize);
	if (entry->gr_name == NULL || entry->gr_passwd == NULL)
		return ERANGE;

	// copy member array
	for (int i = 0; i < memberCount; i++) {
		entry->gr_mem[i] = buffer_dup_string(members[i], buffer, bufferSize);
		if (entry->gr_mem[i] == NULL)
			return ERANGE;
	}
	entry->gr_mem[memberCount] = NULL;

	return 0;
}


status_t
BPrivate::copy_group_to_buffer(const group* from, group* entry, char* buffer,
	size_t bufferSize)
{
	int memberCount = 0;
	while (from->gr_mem[memberCount] != NULL)
		memberCount++;

	return copy_group_to_buffer(from->gr_name, from->gr_passwd,
		from->gr_gid, from->gr_mem, memberCount, entry, buffer, bufferSize);
}


status_t
BPrivate::parse_group_line(char* line, char*& name, char*& password, gid_t& gid,
	char** members, int& memberCount)
{
	Tokenizer tokenizer(line);

	name = tokenizer.NextTrimmedToken(':');
	password = tokenizer.NextTrimmedToken(':');
	char* groupID = tokenizer.NextTrimmedToken(':');

	// skip if invalid
	size_t nameLen;
	if (groupID == NULL || (nameLen = strlen(name)) == 0 || !isdigit(*groupID)
		|| nameLen >= MAX_GROUP_NAME_LEN
		|| strlen(password) >= MAX_GROUP_PASSWORD_LEN) {
		return B_BAD_VALUE;
	}

	gid = atol(groupID);

	memberCount = 0;

	while (char* groupUser = tokenizer.NextTrimmedToken(',')) {
		// ignore invalid members
		if (*groupUser == '\0' || strlen(groupUser) >= MAX_PASSWD_NAME_LEN)
			continue;

		members[memberCount++] = groupUser;

		// ignore excess members
		if (memberCount == MAX_GROUP_MEMBER_COUNT)
			break;
	}

	return B_OK;
}


// #pragma mark - shadow password support


status_t
BPrivate::copy_shadow_pwd_to_buffer(const char* name, const char* password,
	int lastChanged, int min, int max, int warn, int inactive, int expiration,
	int flags, spwd* entry, char* buffer, size_t bufferSize)
{
	entry->sp_lstchg = lastChanged;
	entry->sp_min = min;
	entry->sp_max = max;
	entry->sp_warn = warn;
	entry->sp_inact = inactive;
	entry->sp_expire = expiration;
	entry->sp_flag = flags;

	entry->sp_namp = buffer_dup_string(name, buffer, bufferSize);
	entry->sp_pwdp = buffer_dup_string(password, buffer, bufferSize);

	if (entry->sp_namp && entry->sp_pwdp)
		return 0;

	return ERANGE;
}


status_t
BPrivate::copy_shadow_pwd_to_buffer(const spwd* from, spwd* entry,
	char* buffer, size_t bufferSize)
{
	return copy_shadow_pwd_to_buffer(from->sp_namp, from->sp_pwdp,
		from->sp_lstchg, from->sp_min, from->sp_max, from->sp_warn,
		from->sp_inact, from->sp_expire, from->sp_flag, entry, buffer,
		bufferSize);
}


status_t
BPrivate::parse_shadow_pwd_line(char* line, char*& name, char*& password,
	int& lastChanged, int& min, int& max, int& warn, int& inactive,
	int& expiration, int& flags)
{
	Tokenizer tokenizer(line);

	name = tokenizer.NextTrimmedToken(':');
	password = tokenizer.NextTrimmedToken(':');
	char* lastChangedString = tokenizer.NextTrimmedToken(':');
	char* minString = tokenizer.NextTrimmedToken(':');
	char* maxString = tokenizer.NextTrimmedToken(':');
	char* warnString = tokenizer.NextTrimmedToken(':');
	char* inactiveString = tokenizer.NextTrimmedToken(':');
	char* expirationString = tokenizer.NextTrimmedToken(':');
	char* flagsString = tokenizer.NextTrimmedToken(':');

	// skip if invalid
	size_t nameLen;
	if (flagsString == NULL || (nameLen = strlen(name)) == 0
		|| nameLen >= MAX_SHADOW_PWD_NAME_LEN
		|| strlen(password) >= MAX_SHADOW_PWD_PASSWORD_LEN) {
		return B_BAD_VALUE;
	}

	lastChanged = atoi(lastChangedString);
	min = minString[0] != '\0' ? atoi(minString) : -1;
	max = maxString[0] != '\0' ? atoi(maxString) : -1;
	warn = warnString[0] != '\0' ? atoi(warnString) : -1;
	inactive = inactiveString[0] != '\0' ? atoi(inactiveString) : -1;
	expiration = expirationString[0] != '\0' ? atoi(expirationString) : -1;
	flags = atoi(flagsString);

	return B_OK;
}


// #pragma mark -


void
__init_pwd_backend(void)
{
}


void
__reinit_pwd_backend_after_fork(void)
{
	mutex_init(&sUserGroupLock, kUserGroupLockName);
}
