/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <pwd.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <OS.h>

#include <errno_private.h>
#include <libroot_private.h>
#include <RegistrarDefs.h>
#include <user_group.h>

#include <util/KMessage.h>


using BPrivate::UserGroupLocker;
using BPrivate::relocate_pointer;


static KMessage sPasswdDBReply;
static passwd** sPasswdEntries = NULL;
static size_t sPasswdEntryCount = 0;
static size_t sIterationIndex = 0;

static struct passwd sPasswdBuffer;
static char sPasswdStringBuffer[MAX_PASSWD_BUFFER_SIZE];


static status_t
query_passwd_entry(const char* name, uid_t _uid, struct passwd *passwd,
	char *buffer, size_t bufferSize, struct passwd **_result)
{
	*_result = NULL;

	KMessage message(BPrivate::B_REG_GET_USER);
	if (name)
		message.AddString("name", name);
	else
		message.AddInt32("uid", _uid);

	KMessage reply;
	status_t error = BPrivate::send_authentication_request_to_registrar(message,
		reply);
	if (error != B_OK)
		return error;

	int32 uid;
	int32 gid;
	const char* password;
	const char* home;
	const char* shell;
	const char* realName;

	if ((error = reply.FindInt32("uid", &uid)) != B_OK
		|| (error = reply.FindInt32("gid", &gid)) != B_OK
		|| (error = reply.FindString("name", &name)) != B_OK
		|| (error = reply.FindString("password", &password)) != B_OK
		|| (error = reply.FindString("home", &home)) != B_OK
		|| (error = reply.FindString("shell", &shell)) != B_OK
		|| (error = reply.FindString("real name", &realName)) != B_OK) {
		return error;
	}

	error = BPrivate::copy_passwd_to_buffer(name, password, uid, gid, home,
		shell, realName, passwd, buffer, bufferSize);
	if (error == B_OK)
		*_result = passwd;

	return error;
}


static status_t
init_passwd_db()
{
	if (sPasswdEntries != NULL)
		return B_OK;

	// ask the registrar
	KMessage message(BPrivate::B_REG_GET_PASSWD_DB);
	status_t error = BPrivate::send_authentication_request_to_registrar(message,
		sPasswdDBReply);
	if (error != B_OK)
		return error;

	// unpack the reply
	int32 count;
	passwd** entries;
	int32 numBytes;
	if ((error = sPasswdDBReply.FindInt32("count", &count)) != B_OK
		|| (error = sPasswdDBReply.FindData("entries", B_RAW_TYPE,
				(const void**)&entries, &numBytes)) != B_OK) {
		return error;
	}

	// relocate the entries
	addr_t baseAddress = (addr_t)entries;
	for (int32 i = 0; i < count; i++) {
		passwd* entry = relocate_pointer(baseAddress, entries[i]);
		relocate_pointer(baseAddress, entry->pw_name);
		relocate_pointer(baseAddress, entry->pw_passwd);
		relocate_pointer(baseAddress, entry->pw_dir);
		relocate_pointer(baseAddress, entry->pw_shell);
		relocate_pointer(baseAddress, entry->pw_gecos);
	}

	sPasswdEntries = entries;
	sPasswdEntryCount = count;

	return B_OK;
}


// #pragma mark -


struct passwd*
getpwent(void)
{
	struct passwd* result = NULL;
	int status = getpwent_r(&sPasswdBuffer, sPasswdStringBuffer,
		sizeof(sPasswdStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getpwent_r(struct passwd* passwd, char* buffer, size_t bufferSize,
	struct passwd** _result)
{
	UserGroupLocker _;

	int status = B_NO_MEMORY;

	*_result = NULL;

	if ((status = init_passwd_db()) == B_OK) {
		if (sIterationIndex >= sPasswdEntryCount)
			return ENOENT;

		status = BPrivate::copy_passwd_to_buffer(
			sPasswdEntries[sIterationIndex], passwd, buffer, bufferSize);

		if (status == B_OK) {
			sIterationIndex++;
			*_result = passwd;
		}
	}

	return status;
}


void
setpwent(void)
{
	UserGroupLocker _;

	sIterationIndex = 0;
}


void
endpwent(void)
{
	UserGroupLocker locker;

	sPasswdDBReply.Unset();
	sPasswdEntries = NULL;
	sPasswdEntryCount = 0;
	sIterationIndex = 0;
}


struct passwd *
getpwnam(const char *name)
{
	struct passwd* result = NULL;
	int status = getpwnam_r(name, &sPasswdBuffer, sPasswdStringBuffer,
		sizeof(sPasswdStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getpwnam_r(const char *name, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **_result)
{
	return query_passwd_entry(name, 0, passwd, buffer, bufferSize, _result);
}


struct passwd *
getpwuid(uid_t uid)
{
	struct passwd* result = NULL;
	int status = getpwuid_r(uid, &sPasswdBuffer, sPasswdStringBuffer,
		sizeof(sPasswdStringBuffer), &result);
	if (status != 0)
		__set_errno(status);
	return result;
}


int
getpwuid_r(uid_t uid, struct passwd *passwd, char *buffer,
	size_t bufferSize, struct passwd **_result)
{
	return query_passwd_entry(NULL, uid, passwd, buffer, bufferSize, _result);
}
